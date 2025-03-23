#include "Threadpool.h"
#include "Util/Util.h"

using namespace std;

namespace beton {

atomic<bool> ThreadPool::_initialized = {false};

void ThreadPool::initialize(uint32_t poller_thread, uint32_t task_thread, bool cpu_affinity) {
    if (_initialized) {
        return;
    }
    _initialized = true;
    auto cpus = std::thread::hardware_concurrency();
    _poller_thread_count = poller_thread == 0 ? cpus : clamp((uint32_t)2, poller_thread, cpus*2);
    _task_thread_count = task_thread == 0 ? cpus : clamp((uint32_t)2, task_thread, cpus*2);
    _cpu_affinity = cpu_affinity;

    static call_once once([](){

    });
}

PollerThread::Ptr &ThreadPool::getPoller() {

}

TaskThread::Ptr &ThreadPool::getThread() {

}


}