#ifndef __POLER_THREAD_H__
#define __POLER_THREAD_H__

#include <memory>
#include <functional>
#include "Thread.h"

namespace beton {

class PollerThread : public Thread {
public:
    using Ptr = std::shared_ptr<PollerThread>;

    PollerThread();

};

}
#endif  //__POLLER_THREAD_H__