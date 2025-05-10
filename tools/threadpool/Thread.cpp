#include "Thread.h"
#include <pthread.h>

using namespace std;

namespace beton {

LoaderCounter::LoaderCounter(uint64_t max_size, uint64_t max_usec)
    : _max_size(max_size), _max_usec(max_usec) {
    _last_sleep_time = _last_wakeup_time = getCurrentMicroSecond();
}

uint32_t LoaderCounter::load() {
    //1.统计所有的睡眠时长和运行时长
    uint64_t run_time = 0, sleep_time = 0;
    for (auto rec: _time_rec_list) {
        rec._sleep ? sleep_time += rec._time_usec : run_time += rec._time_usec;
    }
    //2.计算得到总时长
    auto total_time = sleep_time + run_time;
    //3.加上上次统计至今的时长，可能是睡眠时长，可能是运行时长
    total_time += getCurrentMicroSecond() - (_sleeping ? _last_sleep_time : _last_wakeup_time);
    //4.如果超出最大时长限制，减去一些时长
    while (_time_rec_list.size() > 0 && (total_time > _max_usec || _time_rec_list.size() > _max_size)) {
        auto &rec = _time_rec_list.front();
        rec._sleep ? sleep_time -= rec._time_usec : run_time -= rec._time_usec;
        total_time -= rec._time_usec;
        _time_rec_list.pop_front();
    }
    //5.计算得到运行时长，即cpu占用率
    return total_time > 0 ? (run_time * 100 / total_time) : 0;
}

void LoaderCounter::sleep() {
    std::lock_guard<std::mutex> lock(_mutex);
    _sleeping = true;
    auto current_time = getCurrentMicroSecond();
    auto run_time = current_time - _last_wakeup_time;
    _last_sleep_time = current_time;
    _time_rec_list.emplace_back(run_time, false);
    if (_time_rec_list.size() > _max_size) {
        _time_rec_list.pop_front();
    }
}

void LoaderCounter::wakeup() {
    std::lock_guard<std::mutex> lock(_mutex);
    _sleeping = false;
    auto current_time = getCurrentMicroSecond();
    auto sleep_time = current_time - _last_sleep_time;
    _last_wakeup_time = current_time;
    _time_rec_list.emplace_back(sleep_time, true);
    if (_time_rec_list.size() > _max_size) {
        _time_rec_list.pop_front();
    }
}

///////////////////////////////////////////////////////////////////////////
Thread::Thread(const std::string &name) : _name(name) {
    _logger = Logger::Instance().shared_from_this();
}

Thread::~Thread() {
    InfoL << "Destructor: " << _name << endl;
}

bool Thread::is_current_thread() {
    return std::this_thread::get_id() == tid();
}

///////////////////////////////////////////////////////////////////////////
//static function
bool Thread::setThreadAffinity(uint32_t cpu_index) {
    auto cpus = thread::hardware_concurrency();
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET((cpu_index % cpus), &mask);
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