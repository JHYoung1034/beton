#include "ThreadPool.h"
#include "Util/Util.h"

using namespace std;

namespace beton {

static std::atomic<bool> _initialized;
static bool _cpu_affinity;
static uint32_t _poller_thread_count;
static uint32_t _task_thread_count;

INSTANCE_IMP(ThreadPool);

ThreadPool::ThreadPool() {
    if (!_initialized) {
        auto cpus = std::thread::hardware_concurrency();
        ThreadPool::initialize(cpus, cpus);
    }

    call_once once([this](){
        //在这里开启线程
        for (uint32_t index = 0; index < _task_thread_count; index++) {
            auto name = string("task-thread") + to_string(index);
            auto thread = make_shared<TaskThread>(name, index, true);
            thread->run_loop();
            _task_thread_pool.emplace(name, thread);
        }

        for (uint32_t index = 0; index < _poller_thread_count; index++) {
            auto name = string("poller-thread") + to_string(index);
            auto thread = make_shared<PollerThread>(name, index, true);
            thread->run_loop();
            _poller_thread_pool.emplace(name, thread);
        }
    });
}

void ThreadPool::initialize(uint32_t poller_thread, uint32_t task_thread, bool cpu_affinity) {
    if (_initialized) {
        return;
    }
    _initialized = true;
    auto cpus = std::thread::hardware_concurrency();
    _poller_thread_count = poller_thread == 0 ? cpus : clamp((uint32_t)2, poller_thread, cpus*2);
    _task_thread_count = task_thread == 0 ? cpus : clamp((uint32_t)2, task_thread, cpus*2);
    _cpu_affinity = cpu_affinity;
}

const PollerThread::Ptr &ThreadPool::getPoller() {
    auto name = string("poller-thread") + to_string(_poller_index++ % _poller_thread_count);
    auto &thread = _poller_thread_pool[name];
    if (thread == nullptr) {
        thread = make_shared<PollerThread>(name, _poller_index - 1, _cpu_affinity);
        _poller_thread_pool.emplace(name, thread);
    }
    return std::move(static_pointer_cast<PollerThread>(thread));
}

const TaskThread::Ptr &ThreadPool::getThread() {
    auto name = string("task-thread") + to_string(_task_index++ % _task_thread_count);
    auto &thread = _task_thread_pool[name];
    if (thread == nullptr) {
        thread = make_shared<TaskThread>(name, _task_index - 1, _cpu_affinity);
        _task_thread_pool.emplace(name, thread);
    }
    return std::move(static_pointer_cast<TaskThread>(thread));
}

}