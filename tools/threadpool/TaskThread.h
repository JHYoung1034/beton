#ifndef __TASK_THREAD_H__
#define __TASK_THREAD_H__

#include "Util/Util.h"
#include "Thread.h"

namespace beton {
//使用条件变量实现线程中任务循环调度，适合用于非IO绑定任务
class TaskThread : public Thread {
public:
    using Ptr = std::shared_ptr<TaskThread>;
    
    TaskThread(const std::string &name, uint32_t index, bool cpu_affinity);
    ~TaskThread();
    void async(TaskFunc task_func, Thread::TaskPriority priority = Thread::Normal, bool may_sync = true) override;
    void run_loop() override;

private:
    TaskQueue<TaskFunc>::Ptr _task_queue;
    std::function<void()> _setting_func;
};

}
#endif  //__TASK_THREAD_H__