#ifndef __POLER_THREAD_H__
#define __POLER_THREAD_H__
#include <map>
#include "Thread.h"
#include "Util/Pipe.h"

namespace beton {

class PollerThread : public Thread {
public:
    using Ptr = std::shared_ptr<PollerThread>;
    using onEvent = std::function<void(int event)>;
    using onDelEvent = std::function<void(bool success)>;
    using DelayTask = CancelableTask<uint64_t()>;

    PollerThread(const std::string &name, uint32_t index, bool cpu_affinity);

    ~PollerThread();
    ///////////////////////////////////////////////
    void async(TaskFunc func, Thread::TaskPriority priority = Thread::Normal, bool may_sync = true) override;
    ///////////////////////////////////////////////
    int addEvent(int fd, int events, onEvent on_event_cb);

    int modifyEvent(int fd, int events);

    int delEvent(int fd, int events, onDelEvent on_del_event_cb);

    DelayTask::Ptr doDelayTask(uint32_t delay_ms, std::function<uint64_t()> func);

    void run_loop() override;

private:
    uint64_t getMinDelayTime();

    uint64_t flushDelayTask(uint64_t now);

    void addPipeEvent();

    void onPipeEvent();

private:
    //现成初始化
    std::function<void()> _setting_func;
    //异步任务,使用管道唤醒epoll执行异步任务
    Pipe _pipe;
    std::mutex _task_mutex;
    std::list<TaskFunc> _task_list;
    //epoll 事件处理
    int _poller_fd;
    std::unordered_map<int, std::shared_ptr<onEvent> > _event_map;
    //延时任务处理,有可能多个任务延时时间相同，所以用multimap,允许存在相同的延时时间，并对所有延时任务排序
    std::multimap<uint64_t, DelayTask::Ptr> _delay_task_map;
};

}
#endif  //__POLLER_THREAD_H__