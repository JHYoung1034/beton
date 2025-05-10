#include "PollerThread.h"
#include "network/SockUtil.h"
#include <sys/epoll.h>
#include <unistd.h>

#define EPOLL_SIZE 1024

using namespace std;

namespace beton {

PollerThread::PollerThread(const std::string &name, uint32_t index, bool cpu_affinity)
    : Thread(name) {
    _setting_func = [=]() {
        Thread::setThreadName(name.data());
        if (cpu_affinity) {
            Thread::setThreadAffinity(index);
        }
    };
}

PollerThread::~PollerThread() {
    _started = false;
    // onPipeEvent();
    _pipe.write("1", 1);
    if (_thread && _thread->joinable()) {
        _thread->join();
    }
}

void PollerThread::async(TaskFunc func, Thread::TaskPriority priority, bool may_sync) {
    if (func && may_sync && is_current_thread()) {
        func();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_task_mutex);
        if (priority == Thread::High) {
            _task_list.push_front(std::move(func));
        } else {
            _task_list.push_back(std::move(func));
        }
    }
    _pipe.write("1", 1);
}

int PollerThread::addEvent(int fd, int events, onEvent on_event_cb) {
    if (fd < 0 || !on_event_cb) {
        WarnL << "fd < 0 or on_event_cb is nullptr";
        return -1;
    }

    int ret = 0;
    if (is_current_thread()) {
        epoll_event ev;
        //EPOLLEXCLUSIVE表示独占事件，避免多个线程同时处理同一个事件引发惊群问题
        //同时，socket的fd也要设置为非阻塞
        ev.events = events | EPOLLEXCLUSIVE;
        ev.data.fd = fd;
        ret = epoll_ctl(_poller_fd, EPOLL_CTL_ADD, fd, &ev);
        if (ret == 0) {
            _event_map[fd] = std::make_shared<onEvent>(std::move(on_event_cb));
        } else {
            WarnL << "epoll add event: " << events << " failed: " << SockException(errno);
        }
    } else {
        async([fd, events, on_event_cb, this](){
            addEvent(fd, events, on_event_cb);
        });
    }
    return ret;
}

int PollerThread::modifyEvent(int fd, int events) {
    if (fd < 0) {
        WarnL << "fd < 0";
        return -1;
    }

    int ret = 0;
    if (is_current_thread()) {
        epoll_event ev;
        ev.events = events | EPOLLEXCLUSIVE;
        ev.data.fd = fd;
        ret = epoll_ctl(_poller_fd, EPOLL_CTL_MOD, fd, &ev);
        if (ret != 0) {
            WarnL << "epoll modify event: " << events << " failed: " << SockException(errno);
        }
    } else {
        async([fd, events, this](){
            modifyEvent(fd, events);
        });
    }
    return ret;
}

int PollerThread::delEvent(int fd, int events, onDelEvent on_del_event_cb) {
    if (fd < 0 || !on_del_event_cb) {
        WarnL << "fd < 0 or on_del_event_cb is nullptr";
        return -1;
    }

    int ret = 0;
    if (is_current_thread()) {
        epoll_event ev;
        ev.events = events | EPOLLEXCLUSIVE;
        ev.data.fd = fd;
        ret = epoll_ctl(_poller_fd, EPOLL_CTL_DEL, fd, &ev);
        if (ret == 0) {
            _event_map.erase(fd);
            on_del_event_cb(true);
        } else {
            WarnL << "epoll del event: " << events << " failed: " << SockException(errno);
            on_del_event_cb(false);
        }
    } else {
        async([fd, events, on_del_event_cb, this](){
            delEvent(fd, events, on_del_event_cb);
        });
    }
    return ret;
}

uint64_t PollerThread::flushDelayTask(uint64_t now) {
    //清空到期的延时任务
    std::multimap<uint64_t, DelayTask::Ptr> delay_task_list;
    for (auto it = _delay_task_map.begin(); it != _delay_task_map.end();) {
        if (it->first <= now) {
            delay_task_list.emplace(it->first, it->second);
            it = _delay_task_map.erase(it);
        } else {
            ++it;
        }
    }
    //执行到期的延时任务
    for (auto &task : delay_task_list) {
        try {
            auto next_delay = (*(task.second))();
            if (next_delay) {
                _delay_task_map.emplace(now + next_delay, task.second);
            }
        } catch (std::exception &ex) {
            WarnL << "delay task exception: " << ex.what();
        }
        catch (...) {
            WarnL << "delay task exception";
        }
    }

    return delay_task_list.empty() ? 0 : delay_task_list.begin()->first - now;
}

uint64_t PollerThread::getMinDelayTime() {
    if (_delay_task_map.empty()) {
        return 0;
    }
    auto it = _delay_task_map.begin();
    auto now = getCurrentMilliSecond();
    if (it->first > now) {
        return it->first - now;
    }
    //如果延时任务已经到期，则直接执行
    return flushDelayTask(now);
}

void PollerThread::addPipeEvent() {
    if (_pipe.isValid()) {
        if (addEvent(_pipe.getReadFd(), EPOLLIN, [this](int event) { onPipeEvent();}) == -1) {
            throw runtime_error("add pipe event failed");
        }
    }
}

void PollerThread::onPipeEvent() {
    char buffer[1024] = {0};
    int err = 0;
    while (true) {
        if ((err = _pipe.read(buffer, sizeof(buffer))) > 0) {
            //把管道事件一次性读完
            continue;
        }
        if (err == 0 || errno != EAGAIN) {
            //异常了，重新打开管道
            delEvent(_pipe.getReadFd(), EPOLLIN, [](bool){});
            _pipe.reset();
            addPipeEvent();
        }
        break;
    }
    //处理异步任务
    std::list<TaskFunc> task_list;
    {
        std::lock_guard<std::mutex> lock(_task_mutex);
        task_list.swap(_task_list);
    }
    for (auto &task : task_list) {
        try {
            task();
        } catch (std::exception &ex) {
            ErrorL << "Exception in async task: " << ex.what();
        }
    }
}

void PollerThread::run_loop() {
    if (!_started) {
        _started = true;
        _thread = make_shared<thread>(&PollerThread::run_loop, this);
        return;
    }
    //初始化时线程环境，名字，cpu亲和性等
    _setting_func();
    //创建epoll
    _poller_fd = epoll_create1(EPOLL_CLOEXEC);
    if (_poller_fd == -1) {
        WarnL << "epoll create failed: " << SockException(errno);
        throw runtime_error("epoll create failed");
    }
    //设置epoll的IO属性,如
    SockUtil::setcloseExec(_poller_fd);

    //设置管道的读端为epoll的事件
    addPipeEvent();
    //设置epoll的事件处理函数
    uint64_t min_delay_time = 0;
    struct epoll_event ev[EPOLL_SIZE];

    while (_started) {
        min_delay_time = getMinDelayTime();
        sleep();
        int ret = epoll_wait(_poller_fd, ev, sizeof(ev) / sizeof(epoll_event), min_delay_time);
        wakeup();
        if (ret < 0) {
            WarnL << "epoll wait failed: " << SockException(errno);
            continue;
        }

        //处理事件
        for (int i = 0; i < ret; ++i) {
            int fd = ev[i].data.fd;
            struct epoll_event &event = ev[i];

            auto it = _event_map.find(ev[i].data.fd);
            if (it == _event_map.end()) {
                epoll_ctl(_poller_fd, EPOLL_CTL_DEL, fd, nullptr);
            }

            auto callbakc_func = it->second;
            try {
                (*callbakc_func)(event.events);
            } catch (std::exception &ex) {
                ErrorL << "Exception in epoll event: " << ex.what();
            } catch (...) {
                ErrorL << "Unknown exception in epoll event";
            }
        }
    }
    //关闭epoll
    if (_poller_fd != -1) {
        close(_poller_fd);
        _poller_fd = -1;
    }
    _event_map.clear();
    //清空延时任务列表
    std::multimap<uint64_t, DelayTask::Ptr> delay_task_list;
    {
        std::lock_guard<std::mutex> lock(_task_mutex);
        delay_task_list.swap(_delay_task_map);
    }
    for (auto &task : delay_task_list) {
        task.second->cancel();
    }
}

PollerThread::DelayTask::Ptr PollerThread::doDelayTask(uint32_t delay_ms, function<uint64_t()> func) {
    if (delay_ms == 0 || !func) {
        return nullptr;
    }

    auto task = std::make_shared<DelayTask>(std::move(func));
    uint64_t delay_time = getCurrentMilliSecond() + delay_ms;
    async([task, delay_time, this]() {
        _delay_task_map.emplace(delay_time, task);
        //唤醒epoll
        _pipe.write("1", 1);
    }, TaskPriority::High);

    return task;
}

}