#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "PollerThread.h"
#include "TaskThread.h"

namespace beton {

class ThreadPool : public noncopyable {
public:
    static ThreadPool &Instance();
    static void initialize(uint32_t poller_thread, uint32_t task_thread, bool cpu_affinity = true);

    ~ThreadPool();
    PollerThread::Ptr getPoller();
    TaskThread::Ptr getThread();

private:
    ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

private:
    std::atomic<uint32_t> _poller_index = {0};
    std::atomic<uint32_t> _task_index = {0};
    std::map<std::string, Thread::Ptr> _poller_thread_pool;
    std::map<std::string, Thread::Ptr> _task_thread_pool;

    static std::atomic<bool> _initialized;
    static bool _cpu_affinity;
    static uint32_t _poller_thread_count;
    static uint32_t _task_thread_count;
};

}
#endif  //__THREAD_POOL_H__