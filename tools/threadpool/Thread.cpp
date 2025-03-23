#include "Thread.h"

#include <cstring>

using namespace std;

namespace beton {

bool Thread::setThreadAffinity(int cpu_index) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (cpu_index >= 0) {
        CPU_SET(cpu_index, &mask);
    } else {
        for (auto j = 0u; j < thread::hardware_concurrency(); ++j) {
            CPU_SET(j, &mask);
        }
    }
    if (!pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)) {
        return true;
    }
    return false;
}

static string thread_local thread_name;

static string limitString(const char *name, size_t max_size) {
    string str = name;
    if (str.size() + 1 > max_size) {
        auto erased = str.size() + 1 - max_size + 3;
        str.replace(5, erased, "...");
    }
    return str;
}

void Thread::setThreadName(const char *name) {
    if (!name) { return; }
    pthread_setname_np(pthread_self(), limitString(name, 16).data());
}

string Thread::currentThreadName() {
    string ret;
    ret.resize(32);
    auto tid = pthread_self();
    pthread_getname_np(tid, (char *) ret.data(), ret.size());
    if (ret[0]) {
        ret.resize(strlen(ret.data()));
        return ret;
    }
    return to_string((uint64_t) tid);
}

}