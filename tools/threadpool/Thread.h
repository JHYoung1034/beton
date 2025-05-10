#ifndef __THREAD_H__
#define __THREAD_H__
#include <atomic>
#include "Util/Util.h"
#include "Util/Logger.h"

namespace beton {

//统计休眠时间和运行时间，计算得到线程CPU负载
class LoaderCounter {
public:
    using Ptr = std::shared_ptr<LoaderCounter>;
    //默认最多统计64个样本，最多统计1秒时长
    LoaderCounter(uint64_t max_size = 64, uint64_t max_usec = 1 * 1000 * 1000);
    uint32_t load();
    void sleep();
    void wakeup();
private:
    struct TimeRecord {
        TimeRecord(uint64_t time_usec, bool sleep)
            : _time_usec(time_usec), _sleep(sleep) {}
        uint64_t _time_usec;
        bool _sleep;
    };

private:
    bool _sleeping = false;
    uint64_t _max_size;
    uint64_t _max_usec;
    uint64_t _last_sleep_time;
    uint64_t _last_wakeup_time;
    std::mutex _mutex;
    std::list<TimeRecord> _time_rec_list;
};

//使用C++11设计一个模板类，用于包装任意类型返回值和任意类型参数的任务函数
template<class RET, class... ARGS>
class CancelableTask;

template<class RET, class... ARGS>
class CancelableTask<RET(ARGS...)> : public noncopyable {
public:
    using Ptr = std::shared_ptr<CancelableTask>;
    using Func = std::function<RET(ARGS...)>;

    template<typename F>
    CancelableTask(F &&func) {
        _strong_func = std::make_shared<Func>(std::forward<F>(func));
        _weak_func = _strong_func;
    }

    void cancel() {
        _strong_func.reset();
        _weak_func.reset();
    }

    operator bool() const {
        return _strong_func && *_strong_func;
    }

    void operator=(std::nullptr_t) {
        cancel();
    }

    RET operator()(ARGS ...args) const {
        auto strong_task = _weak_func.lock();
        if (strong_task && *strong_task) {
            return (*strong_task)(std::forward<ARGS>(args)...);
        }
        // static decltype(RET) default_ret;
        return default_return<RET>();
    }

private:
    template<typename T>
    static typename std::enable_if<std::is_void<T>::value, void>::type default_return(){}
    template<typename T>
    static typename std::enable_if<std::is_pointer<T>::value, T>::type default_return() {return nullptr;}
    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type default_return() {return 0;}
    template<typename T>
    static typename std::enable_if<std::is_floating_point<T>::value, T>::type default_return() {return 0.0f;}
    template<typename T>
    static typename std::enable_if<std::is_same<T, std::string>::value, T>::type default_return() {return "";}

private:
    std::shared_ptr<Func> _strong_func;
    std::weak_ptr<Func> _weak_func;
};

using TaskFunc = std::function<void()>;
using TaskObject = CancelableTask<void()>;

//使用双端队列实现任务队列，支持优先级任务
template<typename T>
class TaskQueue {
public:
    using Ptr = std::shared_ptr<TaskQueue>;

    void push(T &&task, bool first = false) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            first ? _task_list.emplace_front(std::forward<T>(task))
                  : _task_list.emplace_back(std::forward<T>(task));
        }
        _sem.notify();
    }
    T pop() {
        _sem.wait();
        std::lock_guard<std::mutex> lock(_mutex);
        if (_task_list.empty()) {
            return nullptr;
        }
        auto task = std::move(_task_list.front());
        _task_list.pop_front();
        return std::move(task);
    }

private:
    semaphore _sem;
    std::mutex _mutex;
    std::list<T> _task_list;
};

//线程类基类，提供线程名称、线程ID、是否启动、是否当前线程等基本信息
class Thread : public LoaderCounter {
public:
    using Ptr = std::shared_ptr<Thread>;
    typedef enum : uint8_t {
        Normal = 0,
        High,
    }TaskPriority;

    Thread(const std::string &name);
    virtual ~Thread();

    std::string name() const { return _name; }
    std::thread::id tid() const { return _thread ? _thread->get_id() : std::thread::id(); }
    bool started() const { return _started; }
    bool is_current_thread();

    virtual void async(TaskFunc task_func, TaskPriority priority = Normal, bool may_sync = true) = 0;

    static bool setThreadAffinity(uint32_t cpu_index);
    static void setThreadName(const char *name);
    static std::string currentThreadName();

public:
    virtual void run_loop() = 0;
protected:
    std::atomic<bool> _started = {false};
    std::shared_ptr<std::thread> _thread = nullptr;
private:
    std::string _name;
    //线程退出前，保持日志可用
    Logger::Ptr _logger;
};

}
#endif  //__THREAD_H__