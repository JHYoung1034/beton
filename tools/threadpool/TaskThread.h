#ifndef __TASK_THREAD_H__
#define __TASK_THREAD_H__

#include "Util/Util.h"
#include "Thread.h"
#include <memory>
#include <functional>

namespace beton {
//使用条件变量实现线程中任务循环调度，适合用于非IO绑定任务
class TaskThread : public Thread {
public:
    using Ptr = std::shared_ptr<TaskThread>;
    // using TaskFunc = std::function<
    void async();

};

}
#endif  //__TASK_THREAD_H__