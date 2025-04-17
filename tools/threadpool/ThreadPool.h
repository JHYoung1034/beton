#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "PollerThread.h"
#include "TaskThread.h"
#include <unordered_map>
#include <atomic>

namespace beton {

class ThreadPool : public noncopyable {
public:
    static ThreadPool &Instance();
    static void initialize(uint32_t poller_thread, uint32_t task_thread, bool cpu_affinity = true);
    const PollerThread::Ptr &getPoller();
    const TaskThread::Ptr &getThread();

private:
    ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

private:
    std::atomic<uint32_t> _poller_index = {0};
    std::atomic<uint32_t> _task_index = {0};
    std::unordered_map<std::string, Thread::Ptr> _poller_thread_pool;
    std::unordered_map<std::string, Thread::Ptr> _task_thread_pool;
};

}
#endif  //__THREAD_POOL_H__