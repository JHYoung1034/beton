#ifndef __THREAD_H__
#define __THREAD_H__
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <pthread.h>

namespace beton {

using Task = std::function<void()>;

class LoaderCounter {
public:
    using Ptr = std::shared_ptr<LoaderCounter>;

    LoaderCounter();
    virtual ~LoaderCounter();

    /**
     * 0-100
    */
    uint32_t load();

    void sleep();

    void wakeup();

private:

};

class Thread : public LoaderCounter {
public:
    using Ptr = std::shared_ptr<Thread>;

    typedef enum : uint8_t {
        Normal = 0,
        High,
    }TaskPriority;

    Thread(const std::string &name);
    virtual ~Thread();

    std::string getThreadName() const { return _name; }

    virtual void async(Task task_func, TaskPriority priority = Normal, bool may_sync = true) = 0;

    static bool setThreadAffinity(int cpu_index);
    static void setThreadName(const char *name);
    static std::string currentThreadName();

protected:
    virtual void run_loop() = 0;

private:
    void join();

private:
    std::string _name;
    std::thread _thread;
};

}
#endif  //__TASK_H__