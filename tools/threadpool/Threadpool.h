#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "PollerThread.h"
#include "TaskThread.h"
#include <unordered_map>
#include <atomic>

namespace beton {

class ThreadPool : public noncopyable {
public:
    static void initialize(uint32_t poller_thread, uint32_t task_thread, bool cpu_affinity = true);
    static PollerThread::Ptr &getPoller();
    static TaskThread::Ptr &getThread();

private:
    ThreadPool() = delete;
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

private:
    static std::atomic<bool> _initialized;
    static bool _cpu_affinity;
    static uint32_t _poller_thread_count;
    static uint32_t _task_thread_count;
    static std::unordered_map<std::string, Thread::Ptr> _poller_thread_pool;
    static std::unordered_map<std::string, Thread::Ptr> _task_thread_pool;
};

}
#endif  //__THREAD_POOL_H__