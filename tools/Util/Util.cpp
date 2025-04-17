#include "Util.h"
#include <ctime>
#include <execinfo.h>
#include <cstdlib>
#include <cxxabi.h>
#include <strstream>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <cstring>
#include <chrono>

using namespace std;

namespace beton {

static int _daylight_active;
static long _current_timezone;

int get_daylight_active() {
    return _daylight_active;
}

static int is_leap_year(time_t year) {
    if (year % 4)
        return 0; /* A year not divisible by 4 is not leap. */
    else if (year % 100)
        return 1; /* If div by 4 and not 100 is surely leap. */
    else if (year % 400)
        return 0; /* If div by 100 *and* not by 400 is not leap. */
    else
        return 1; /* If div by 100 and 400 is leap. */
}

void no_locks_localtime(struct tm *tmp, time_t t) {
    const time_t secs_min = 60;
    const time_t secs_hour = 3600;
    const time_t secs_day = 3600 * 24;

    t -= _current_timezone; /* Adjust for timezone. */
    t += 3600 * get_daylight_active(); /* Adjust for daylight time. */
    time_t days = t / secs_day; /* Days passed since epoch. */
    time_t seconds = t % secs_day; /* Remaining seconds. */

    tmp->tm_isdst = get_daylight_active();
    tmp->tm_hour = seconds / secs_hour;
    tmp->tm_min = (seconds % secs_hour) / secs_min;
    tmp->tm_sec = (seconds % secs_hour) % secs_min;
#ifndef _WIN32
    tmp->tm_gmtoff = -_current_timezone;
#endif
    /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
     * where sunday = 0, so to calculate the day of the week we have to add 4
     * and take the modulo by 7. */
    tmp->tm_wday = (days + 4) % 7;

    /* Calculate the current year. */
    tmp->tm_year = 1970;
    while (1) {
        /* Leap years have one day more. */
        time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
        if (days_this_year > days)
            break;
        days -= days_this_year;
        tmp->tm_year++;
    }
    tmp->tm_yday = days; /* Number of day of the current year. */

    /* We need to calculate in which month and day of the month we are. To do
     * so we need to skip days according to how many days there are in each
     * month, and adjust for the leap year that has one more day in February. */
    int mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    mdays[1] += is_leap_year(tmp->tm_year);

    tmp->tm_mon = 0;
    while (days >= mdays[tmp->tm_mon]) {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++;
    }

    tmp->tm_mday = days + 1; /* Add 1 since our 'days' is zero-based. */
    tmp->tm_year -= 1900; /* Surprisingly tm_year is year-1900. */
}

void local_time_init() {
    /* Obtain timezone and daylight info. */
    tzset(); /* Now 'timezome' global is populated. */
#if defined(__linux__) || defined(__sun)
    _current_timezone  = timezone;
#elif defined(_WIN32)
	time_t time_utc;
	struct tm tm_local;

	// Get the UTC time
	time(&time_utc);

	// Get the local time
	// Use localtime_r for threads safe for linux
	//localtime_r(&time_utc, &tm_local);
	localtime_s(&tm_local, &time_utc);

	time_t time_local;
	struct tm tm_gmt;

	// Change tm to time_t
	time_local = mktime(&tm_local);

	// Change it to GMT tm
	//gmtime_r(&time_utc, &tm_gmt);//linux
	gmtime_s(&tm_gmt, &time_utc);

	int time_zone = tm_local.tm_hour - tm_gmt.tm_hour;
	if (time_zone < -12) {
		time_zone += 24;
	}
	else if (time_zone > 12) {
		time_zone -= 24;
	}

    _current_timezone = time_zone;
#else
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    _current_timezone = tz.tz_minuteswest * 60L;
#endif
    time_t t = time(NULL);
    struct tm *aux = localtime(&t);
    _daylight_active = aux->tm_isdst;
}

struct tm getLocalTime(time_t sec) {
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &sec);
#else
    no_locks_localtime(&tm, sec);
#endif //_WIN32
    return tm;
}

string getTimeStr(const char *fmt, time_t time) {
    if (!time) {
        time = ::time(nullptr);
    }
    auto tm = getLocalTime(time);
    size_t size = strlen(fmt) + 64;
    string ret;
    ret.resize(size);
    size = std::strftime(&ret[0], size, fmt, &tm);
    if (size > 0) {
        ret.resize(size);
    }
    else{
        ret = fmt;
    }
    return ret;
}

static long s_gmtoff = 0; //时间差
static call_once s_token([]() {
    local_time_init();
    s_gmtoff = getLocalTime(time(nullptr)).tm_gmtoff;
});

long getGMTOff() {
    return s_gmtoff;
}

uint64_t getCurrentMicroSecond() {
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t getCurrentMilliSecond() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static string demangleFunction(const char* mangled) {
    int status;
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    if (status == 0) {
        string result(demangled);
        free(demangled);
        return result;
    }
    return mangled; // 失败时返回原名称
}

string stackBacktrace(bool demangle) {
    void* buffer[2048];
    int size = backtrace(buffer, 2048);
    char** symbols = backtrace_symbols(buffer, size);

    strstream oss;
    for (int i = size - 1; i >= 1; --i) {
        if (demangle) {
            string stack = symbols[i];
            auto start = stack.find('(') + 1;
            auto end = stack.find("+");
            string func_name = stack.substr(start, end - start);
            oss << demangleFunction(func_name.data()) << (i > 1 ? "->" : "");
        } else {
            oss << symbols[i] << "\n";
        }
    }

    free(symbols);
    return oss.str();
}

string exePath(bool isExe) {
    char buffer[PATH_MAX * 2 + 1] = {0};
    int n = -1;
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));

    string filePath;
    if (n <= 0) {
        filePath = "./";
    } else {
        filePath = buffer;
    }

    return filePath;
}

string exeDir(bool isExe) {
    auto path = exePath(isExe);
    return path.substr(0, path.rfind('/') + 1);
}

string exeName(bool isExe) {
    auto path = exePath(isExe);
    return path.substr(path.rfind('/') + 1);
}

vector<string> split(const string &s, const char *delim) {
    vector<string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != string::npos) {
        if (index - last > 0) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + strlen(delim);
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0) {
        ret.push_back(s.substr(last));
    }
    return ret;
}

#define TRIM(s, chars) \
do{ \
    string map(0xFF, '\0'); \
    for (auto &ch : chars) { \
        map[(unsigned char &)ch] = '\1'; \
    } \
    while( s.size() && map.at((unsigned char &)s.back())) s.pop_back(); \
    while( s.size() && map.at((unsigned char &)s.front())) s.erase(0,1); \
}while(0);

//去除前后的空格、回车符、制表符
std::string &trim(std::string &s, const string &chars) {
    TRIM(s, chars);
    return s;
}

std::string trim(std::string &&s, const string &chars) {
    TRIM(s, chars);
    return std::move(s);
}

void replace(string &str, const string &old_str, const string &new_str,std::string::size_type b_pos) {
    if (old_str.empty() || old_str == new_str) {
        return;
    }
    auto pos = str.find(old_str,b_pos);
    if (pos == string::npos) {
        return;
    }
    str.replace(pos, old_str.size(), new_str);
    replace(str, old_str, new_str,pos + new_str.length());
}

bool start_with(const string &str, const string &substr) {
    return str.find(substr) == 0;
}

bool end_with(const string &str, const string &substr) {
    auto pos = str.rfind(substr);
    return pos != string::npos && pos == str.size() - substr.size();
}

}