#include "ThreadPool.h"

using namespace std;

namespace beton {

std::atomic<bool> ThreadPool::_initialized = {false};
bool ThreadPool::_cpu_affinity = true;
uint32_t ThreadPool::_poller_thread_count = 0;
uint32_t ThreadPool::_task_thread_count = 0;

INSTANCE_IMP(ThreadPool);

ThreadPool::ThreadPool() {
    if (!_initialized) {
        auto cpus = std::thread::hardware_concurrency();
        ThreadPool::initialize(cpus, cpus);
    }

    call_once once([this](){
        //在这里开启线程
        for (uint32_t index = 0; index < _task_thread_count; index++) {
            auto name = string("task") + to_string(index);
            auto thread = static_pointer_cast<Thread>(make_shared<TaskThread>(name, index, true));
            thread->run_loop();
            _task_thread_pool.emplace(name, thread);
        }

        for (uint32_t index = 0; index < _poller_thread_count; index++) {
            auto name = string("poller") + to_string(index);
            auto thread = static_pointer_cast<Thread>(make_shared<PollerThread>(name, index, true));
            thread->run_loop();
            _poller_thread_pool.emplace(name, thread);
        }
    });
}

ThreadPool::~ThreadPool() {
    _initialized = false;
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

PollerThread::Ptr ThreadPool::getPoller() {
    auto name = string("poller") + to_string(_poller_index++ % _poller_thread_count);
    auto &thread = _poller_thread_pool[name];
    if (thread == nullptr) {
        thread = make_shared<PollerThread>(name, _poller_index - 1, _cpu_affinity);
        _poller_thread_pool.emplace(name, thread);
    }
    PollerThread::Ptr poller = static_pointer_cast<PollerThread>(thread);
    return poller;
}

TaskThread::Ptr ThreadPool::getThread() {
    auto name = string("task") + to_string(_task_index++ % _task_thread_count);
    auto &thread = _task_thread_pool[name];
    if (thread == nullptr) {
        thread = make_shared<TaskThread>(name, _task_index - 1, _cpu_affinity);
        _task_thread_pool.emplace(name, thread);
    }
    TaskThread::Ptr task = static_pointer_cast<TaskThread>(thread);
    return task;
}

}