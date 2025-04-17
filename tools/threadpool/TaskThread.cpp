#include "TaskThread.h"

using namespace std;

namespace beton {

TaskThread::TaskThread(const string &name, uint32_t index, bool cpu_affinity)
    : Thread(name) {
    _setting_func = [=](){
        Thread::setThreadName(name.data());
        if (cpu_affinity) {
            Thread::setThreadAffinity(index);
        }
    };
    _task_queue = make_shared<TaskQueue<TaskFunc>>();
}

void TaskThread::async(TaskFunc func, Thread::TaskPriority priority, bool may_sync) {
    if (func && may_sync && is_current_thread()) {
        func();
        return;
    }
    _task_queue->push(std::move(func), priority==High);
}

void TaskThread::run_loop() {
    if (!_started) {
        _started = true;
        _thread = make_shared<thread>(&TaskThread::run_loop, this);
        return;
    }

    _setting_func();

    while (_started) {
        sleep();
        auto task = _task_queue->pop();
        wakeup();
        if (task) { task(); }
    }
}

}