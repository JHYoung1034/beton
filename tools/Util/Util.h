#ifndef __UTIL_H__
#define __UTIL_H__
#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <vector>


namespace beton {

#if __cplusplus < 201703L
template<typename T>
const T& clamp(const T& min, const T& set, const T& max) {
    return set < min ? min : (set > max ? max : set);
}
#endif

#define INSTANCE_IMP(class_name, ...) \
class_name &class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_instance_ref = *s_instance; \
    return s_instance_ref; \
}

class noncopyable {
protected:
    noncopyable()=default;
    ~noncopyable()=default;
private:
    noncopyable(const noncopyable&)=delete;
    noncopyable(noncopyable&&)=delete;
    noncopyable &operator=(const noncopyable&)=delete;
    noncopyable &operator=(noncopyable&&)=delete;
};

class call_once : public noncopyable {
public:
    template<typename FUNC>
    call_once(const FUNC &constructed, std::function<void()> destructed = nullptr) {
        constructed();
        _destructed = std::move(destructed);
    }
    ~call_once() {
        if (_destructed) _destructed();
    }
private:
    std::function<void()> _destructed;
};

class semaphore {
public:
    void notify(size_t n = 1) {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        _count += n;
        (n == 1) ? _condition.notify_one() : _condition.notify_all();
    }

    void wait() {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        while (_count == 0) { _condition.wait(lock); }
        --_count;
    }
private:
    size_t _count = 0;
    std::recursive_mutex _mutex;
    std::condition_variable_any _condition;
};


//util functions
void no_locks_localtime(struct tm *tmp, time_t t);
struct tm getLocalTime(time_t sec);
std::string getTimeStr(const char *fmt, time_t time = 0);
long getGMTOff();
uint64_t getCurrentMicroSecond();
uint64_t getCurrentMilliSecond();

std::string stackBacktrace(bool demangle = false);

std::string exePath(bool isExe = true);
std::string exeDir(bool isExe = true);
std::string exeName(bool isExe = true);

std::vector<std::string> split(const std::string& s, const char *delim);
//去除前后的空格、回车符、制表符...
std::string& trim(std::string &s,const std::string &chars=" \r\n\t");
std::string trim(std::string &&s,const std::string &chars=" \r\n\t");

//替换子字符串
void replace(std::string &str, const std::string &old_str, const std::string &new_str, std::string::size_type b_pos = 0) ;
//字符串是否以xx开头
bool start_with(const std::string &str, const std::string &substr);
//字符串是否以xx结尾
bool end_with(const std::string &str, const std::string &substr);

}
#endif //__UTIL_H__