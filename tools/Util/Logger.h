#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "Util.h"
#include <memory>
#include <set>
#include <list>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <thread>

#include <cstring>
#include <time.h>

namespace beton {
/**
 * 1. 支持写日志到terminal，文件，web_hook
 * 2. 支持日志指定天数滚动
 * 3. 支持调用栈展开
 * 
 * 日志格式：Datetime|LogLevel|ThreadName|File|Line|Content
 * 
 * 1. 定义一个类用于参数捕获，展开
 * 2. Channel 类用于写日志
*/

typedef enum : uint8_t {
    Trace = 0, Debug, Info, Warn, Error, End
}LogLevel;

#define DEFAULT_LEVEL LogLevel::Info

const char *fileNameWithoutPath(const char *file);

class Context : public std::ostringstream {
public:
    using Ptr = std::shared_ptr<Context>;
    Context();
    Context(LogLevel level, const std::string &file, int line);
    std::string str();
    const std::string &format_str();

private:
    void format();
    std::string datetime();
public:
    struct timeval _tv;
    LogLevel _level;
    std::string _thread_name;
    std::string _file;
    int _line;
    uint32_t _repeat = 0;
private:
    bool _got_content = false;
    std::string _format_content;
};

class Channel : public noncopyable {
public:
    using Ptr = std::shared_ptr<Channel>;

    Channel(const std::string &name, LogLevel level) : _name(name), _level(level){}
    virtual ~Channel() = default;
    virtual void write(const Context::Ptr &ctx) = 0;
    const std::string name()const{ return _name; }
    void setLevel(LogLevel level) { _level = level; }
    LogLevel level() const { return _level; };
private:
    std::string _name;
    LogLevel _level;
};

//用于向终端输出日志
class LogConsole : public Channel {
public:
    using Ptr = std::shared_ptr<LogConsole>;
    LogConsole(const std::string &name = "console", LogLevel level = LogLevel::Debug, bool enable_color = true);
    ///////////////////////////
    void write(const Context::Ptr &ctx) override;

private:
    bool _enable_color;
};

//用于向文件输出日志
class LogFile : public Channel {
public:
    LogFile(const std::string &name = "file", LogLevel level = LogLevel::Info,
            uint8_t max_days = 15, const std::string &dir = exeDir() + "log/");
    ~LogFile();

    void setMaxDay(uint8_t max_day);

    void setFileMaxSize(size_t max_size_MB);

    void setFileMaxCount(size_t max_count);

    void setEnableStackTrace(bool enable, LogLevel level = LogLevel::Error);

    void write(const Context::Ptr &ctx) override;

private:
    void check(const Context::Ptr &ctx);

    void openFile(time_t time);

    void delExpiredFile();

private:
    bool _enable_stack_trace = false;
    LogLevel _stack_trace_level = LogLevel::Error;

    uint8_t _max_days;
    uint8_t _file_index = 0;
    //每个日志文件切片最大128M
    size_t _max_file_size = 128;
    //最多保持15个日志切片
    size_t _max_file_count = 15;
    time_t _last_day = 0;
    time_t _last_check_time = 0;
    std::string _path;
    std::string _current_log_file;
    std::ofstream _file;
    std::set<std::string> _log_file_map;
};

//用于向注册的WebHook输出日志
class LogWebHook : public Channel {
public:
    LogWebHook(const std::string &name = "hook", LogLevel level = LogLevel::Error, const std::string &url = "");

    void addHookUrl(const std::string &url);

    void delHookUrl(const std::string &url);

    void write(const Context::Ptr &ctx) override;

private:
    std::set<std::string> _url_map;
};

//开启一个线程用于异步执行 Logger 提交的写日志任务
class LogAsyncWriter {
public:
    using Ptr = std::shared_ptr<LogAsyncWriter>;
    using WriteContext = std::pair<Channel::Ptr, Context::Ptr>;
    explicit LogAsyncWriter();
    ~LogAsyncWriter();

    void write(const WriteContext &ctx);

private:
    void flush();
    void run_loop();

private:
    bool _started = false;
    std::mutex _mutex;
    std::list<WriteContext> _content_list;

    semaphore _sem;
    std::shared_ptr<std::thread> _thread;
};

/**
 * 包含：writer, channel_list
 * 增，删，查操作线程不安全
*/
class Logger : public noncopyable, public std::enable_shared_from_this<Logger> {
public:
    using Ptr = std::shared_ptr<Logger>;
    static Logger &Instance();
    explicit Logger();
    ~Logger();

    void add(const Channel::Ptr &chn);

    void del(const std::string &name);

    Channel::Ptr get(const std::string &name);

    void foreach(std::function<void(const Channel::Ptr &chn)> func);

    void write(const Context::Ptr &ctx);

private:
    void writeChannels(const Context::Ptr &ctx);

private:
    Context::Ptr _last_ctx = nullptr;
    LogAsyncWriter::Ptr _writer;
    std::unordered_map<std::string, Channel::Ptr> _channel_map;
};

//LogCapture用于捕获，展开日志，生成Context，write 到 Logger, 
//Logger 再生成 channel+Context 的 pair 提交到 writer 线程执行异步写日志，
//Writer 最终调用 channel 的 write() 做具体的写日志动作，比如FileChannel 写日志到文件
class LogCapture {
public:
    LogCapture(Logger &logger, LogLevel level, const char *file, int line) : _logger(logger) {
        _ctx = std::make_shared<Context>(level, fileNameWithoutPath(file), line);
    }
    ~LogCapture() {
        *this << std::endl;
    }
    //遇到结束符，立即输出日志
    LogCapture &operator<<(std::ostream &(*f)(std::ostream &)) {
        if (!_ctx) { return *this; }
        _logger.write(_ctx);
        _ctx.reset();
        return *this;
    }

    template<typename T>
    LogCapture &operator<<(T &&data) {
        if (!_ctx) { return *this; }
        (*_ctx) << std::forward<T>(data);
        return *this;
    }

private:
    Logger &_logger;
    Context::Ptr _ctx = nullptr;
};

#define LogWrite(level) ::beton::LogCapture(::beton::Logger::Instance(), level, __FILE__, __LINE__)
#define TraceL LogWrite(::beton::LogLevel::Trace)
#define DebugL LogWrite(::beton::LogLevel::Debug)
#define InfoL  LogWrite(::beton::LogLevel::Info)
#define WarnL  LogWrite(::beton::LogLevel::Warn)
#define ErrorL LogWrite(::beton::LogLevel::Error)

}
#endif  //__LOGGER_H__