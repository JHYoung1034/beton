#include "Timer.h"
#include "Logger.h"

using namespace std;

namespace beton {

Timer::Timer(float second, std::function<uint64_t()> func, const PollerThread::Ptr &poller) {
    _poller = poller;
    if (!_poller) {
        _poller = ThreadPool::Instance().getPoller();
    }
    _tag = _poller->doDelayTask(second * 1000, func);
}

Timer::~Timer() {
    if (_tag) {
        _tag->cancel();
    }
}

}