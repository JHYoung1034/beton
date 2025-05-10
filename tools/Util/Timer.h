#ifndef __TIMER_H__
#define __TIMER_H__
#include "threadpool/ThreadPool.h"

namespace beton {

class Timer {
public:
    Timer(float second, std::function<uint64_t()> func, const PollerThread::Ptr &poller = nullptr);
    ~Timer();

private:
    PollerThread::Ptr _poller;
    PollerThread::DelayTask::Ptr _tag;
};

}
#endif //__TIMER_H__