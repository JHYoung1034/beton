#include "Logger.h"
#include "threadpool/Thread.h"
#include "File.h"
#include <iostream>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace beton {

const char *fileNameWithoutPath(const char *file) {
    auto pos = strrchr(file, '/');
    return pos ? pos + 1 : file;
}

//////////////////////////// 日志内容类Context ////////////////////////////
Context::Context() : Context(DEFAULT_LEVEL, __FILE__, __LINE__) {}

Context::Context(LogLevel level, const string &file, int line) {
    gettimeofday(&_tv, NULL);
    _thread_name = Thread::currentThreadName();
    _level = level;
    _file = file;
    _line = line;
}

const string &Context::str() {
    if (!_got_content) {
        format();
    }
    return _format_content;
}

//日志格式：Datetime|LogLevel|ThreadName|File|Line|Content
void Context::format() {
    _got_content = true;
    static char c_level[LogLevel::End] = {'T', 'D', 'I', 'W', 'E'};
    _format_content.clear();
    stringstream oss;
    //datetime
    oss << datetime() << "|" << c_level[_level] << "|" << _thread_name << "|" << _file << "|" << _line << "| ";
    oss << ostringstream::str();
    _format_content = oss.str();
}

string Context::datetime() {
    auto tm = getLocalTime(_tv.tv_sec);
    char buf[128];
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d.%03d",
             1900 + tm.tm_year,
             1 + tm.tm_mon,
             tm.tm_mday,
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             (int) (_tv.tv_usec / 1000));
    return buf;
}

//////////////////////////// 控制台写日志类 ////////////////////////////
#define CLEAR_COLOR "\033[0m"
static const char *LOG_CONST_TABLE[][3] = {
        {"\033[44;37m", "\033[34m", "T"},
        {"\033[42;37m", "\033[32m", "D"},
        {"\033[46;37m", "\033[36m", "I"},
        {"\033[43;37m", "\033[33m", "W"},
        {"\033[41;37m", "\033[31m", "E"}};

LogConsole::LogConsole(const string &name, LogLevel level, bool enable_color)
    : Channel(name, level), _enable_color(enable_color) {
}

void LogConsole::write(const Context::Ptr &ctx) {
    if (ctx->_level < level()) { return; }

    if (_enable_color) { cout << LOG_CONST_TABLE[ctx->_level][1]; }

    cout << ctx->str();

    if (_enable_color) { cout << CLEAR_COLOR; }

    if (ctx->_repeat > 1) { cout << "\r\n    Last message repeated " << ctx->_repeat << " times"; }

    cout << endl;
}

//////////////////////////// 文件写日志类 ////////////////////////////
//日志文件：beton-2025-03-22-01.log
LogFile::LogFile(const string &name, LogLevel level, uint8_t max_days, const string &dir)
    :Channel(name, level), _max_days(max_days), _path(dir) {
    //检查日志文件个数
    if (_path.back() != '/') {
        _path.append("/");
    }

    _log_file_map.clear();
    string exe_name = fileNameWithoutPath(exeName().data());
    File::scanDir(_path, [this, exe_name](const std::string &path, bool is_dir) -> bool {
        if (!is_dir && start_with(path, exe_name) && end_with(path, ".log")) {
            _log_file_map.emplace(path);
            return true;
        }
    }, false);

    //获取最新切片日志文件
    string log_file_prefix = getTimeStr((exe_name + "-%Y-%02m-%02d-").data());
    std::cout << "today log file prefix: " << log_file_prefix << endl;

    for (auto it = _log_file_map.begin(); it != _log_file_map.end(); ++it) {
        if (it->empty()) { continue; }

        auto file_name = fileNameWithoutPath(it->data());
        if (start_with(file_name, log_file_prefix)) {
            char buffer[128] = {};
            int tm_mday;  // day of the month - [1, 31]
            int tm_mon;   // months since January - [0, 11]
            int tm_year;  // years since 1900
            uint32_t index;
            //今天第几个文件
            int count = sscanf(file_name, "%[^-]-%d-%d-%d-%d.log", buffer, &tm_year, &tm_mon, &tm_mday, &index);
            if (count == 5) {
                _file_index = index >= _file_index ? index : _file_index;
            }
        }
    }
}

LogFile::~LogFile() {
    if (_file.is_open()) {
        _file.flush();
        _file.close();
    }
}

void LogFile::setMaxDay(uint8_t max_day) {
    max_day = clamp(1, (int)max_day, 365);
    if (_max_days != max_day) { _max_days = max_day; }
}

void LogFile::setFileMaxSize(size_t max_size_MB) {
    max_size_MB = clamp(1, (int)max_size_MB, 256);
    if (_max_file_size != max_size_MB) { _max_file_size = max_size_MB; }
}

void LogFile::setFileMaxCount(size_t max_count) {
    max_count = clamp(1, (int)max_count, 30);
    if (_max_file_count != max_count) { _max_file_count = max_count; }
}

void LogFile::write(const Context::Ptr &ctx) {
    //如果没打开文件，则打开
    if (!_file.is_open()) {
        openFile(ctx->_tv.tv_sec);
    }

    //每次写日志都要检查是否切换到第二天了，日志大小是否到达最大限值了
    check(ctx);

    //写日志文件
    if (_file.is_open()) {
        _file << ctx->str();
    }
}

static constexpr auto s_second_per_day = 24 * 60 * 60;

static uint64_t getDay(time_t second) {
    return (second + getGMTOff()) / s_second_per_day;
}

static string getLogFilePathName(const string &dir, time_t time, uint32_t file_index) {
    auto tm = getLocalTime(time);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s-%d-%02d-%02d-%02d.log",
        fileNameWithoutPath(exeName().data()),
        1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, file_index);
    return dir + buf;
}

static time_t getLogFileTime(const string &full_path) {
    string name = fileNameWithoutPath(full_path.data());
    struct tm tm{0};
    if (!strptime(name.substr(name.find("-") + 1).data(), "%Y-%02m-%02d", &tm)) {
        return 0;
    }
    //此函数会把本地时间转换成GMT时间戳
    return mktime(&tm);
}

void LogFile::check(const Context::Ptr &ctx) {
    time_t second = ctx->_tv.tv_sec;
    auto log_day = getDay(second);
    //检查时间是否切换到第二天
    if (log_day != _last_day) {
        if (_last_day != 0) {
            _file_index = 0;
        }
        _last_day = log_day;
        openFile(second);
    }

    //检查日志大小是否到达最大限值
    if (second - _last_check_time > 60) {
        if (File::fileSize(_current_log_file.data()) > _max_file_size * 1024 *1024) {
            openFile(second);
        }
        _last_check_time = second;
    }
}

void LogFile::openFile(time_t time) {
    auto log_file = getLogFilePathName(_path, time, _file_index++);
    _log_file_map.emplace(log_file);
    _current_log_file = log_file;

    _file.close();

    //不存在文件夹，先创建文件夹
    if (0 != access(_path.data(), F_OK)) {
        File::create_path(_path.data(), S_IRWXO | S_IRWXG | S_IRWXU);
    }

    if (_file.is_open()) {
        _file.close();
        _file.open(log_file, ios_base::out | ios_base::app);
    }

    //每次新建一个日志文件，都要检查一下是否超过文件数量了，如果有则删除
    delExpiredFile();
}

void LogFile::delExpiredFile() {
    //获取今天是第几天
    auto today = getDay(time(nullptr));
    //遍历所有日志文件，删除超过若干天前的过期日志文件
    for (auto it = _log_file_map.begin(); it != _log_file_map.end();) {
        if (it->empty()) {
            continue;
        }
        auto day = getDay(getLogFileTime(it->data()));
        if (today < day + _max_days) {
            //这个日志文件距今尚未超过一定天数,后面的文件更新，所以停止遍历
            break;
        }
        //这个文件距今超过了一定天数，则删除文件
        File::delete_file(it->data());
        //删除这条记录
        it = _log_file_map.erase(it);
    }

    //按文件个数清理，限制最大文件切片个数
    while (_log_file_map.size() > _max_file_count) {
        auto it = _log_file_map.begin();
        if (it != _log_file_map.end() && !it->empty() && *it == _current_log_file) {
            //当前文件，停止删除
            break;
        }
        //删除文件
        File::delete_file(it->data());
        //删除这条记录
        _log_file_map.erase(it);
    }
}

//////////////////////////// webhook写日志类 ////////////////////////////
LogWebHook::LogWebHook(const std::string &name, LogLevel level, const std::string &url)
    :Channel(name, level) {
    if (!url.empty()) {
        _url_map.emplace();
    }
}

void LogWebHook::addHookUrl(const std::string &url) {

}

void LogWebHook::delHookUrl(const std::string &url) {

}

void LogWebHook::write(const Context::Ptr &ctx) {

}

//////////////////////////// 异步写日志线程类 ////////////////////////////
LogAsyncWriter::LogAsyncWriter() {
    _started = true;
    _thread = make_shared<std::thread>(&LogAsyncWriter::run_loop, this);
}

LogAsyncWriter::~LogAsyncWriter() {
    _started = false;
    _sem.notify();
    _thread->join();
    flush();
}

void LogAsyncWriter::write(const WriteContext &ctx) {
    {
        std::lock_guard<mutex> lock(_mutex);
        _content_list.push_back(ctx);
    }
    _sem.notify();
}

void LogAsyncWriter::flush() {
    decltype(_content_list) tmp;
    {
        lock_guard<mutex> lock(_mutex);
        tmp.swap(_content_list);
    }
    for (auto ctx : tmp) {
        ctx.first->write(ctx.second);
    }
}

void LogAsyncWriter::run_loop() {
    Thread::setThreadName("async-log");
    while (_started) {
        _sem.wait();
        flush();
    }
}

//////////////////////////// 日志控制类 ////////////////////////////
void Logger::add(const Channel::Ptr &chn) {
    if (!chn) { return; }

    _channel_map.emplace(chn->name(), chn);
}

void Logger::del(const std::string &name) {
    if (name.empty()) { return; }
    _channel_map.erase(name);
}

Channel::Ptr Logger::get(const std::string &name) {
    if (name.empty() || _channel_map.find(name) == _channel_map.end()) {
        return nullptr;
    }
    return _channel_map[name];
}

void Logger::foreach(std::function<void(const Channel::Ptr &chn)> func) {
    for (auto it : _channel_map) {
        if (it.second) {
            func(it.second);
        }
    }
}

void Logger::write(const Context::Ptr &ctx) {
    if (_channel_map.empty()) {
        return;
    }

    for (auto chn : _channel_map) {
        if (chn.second) {
            chn.second->write(ctx);
        }
    }

    _last_ctx = ctx;
}

}