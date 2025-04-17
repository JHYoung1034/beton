#ifndef __SOCK_UTIL_H__
#define __SOCK_UTIL_H__

#include <memory>
#include <string>
#include <sstream>
#include <exception>
#include <map>

namespace beton {

typedef enum : uint8_t {
    Err_Success = 0,
    Err_Timeout,
    Err_Dns,
    Err_Eof,
    Err_Refuse,
    Err_Shutdown,
    Err_Other = 0xFF
}ErrorCode;

class SockException : public std::exception {
public:
    SockException(int sock_code = 0, const std::string &what = "Success", ErrorCode custom_code = Err_Success);

    void reset() { _sock_code = 0; _custom_code = Err_Success; _what.clear(); }
    int sock_code() const { return _sock_code; }
    int custom_code() const { return _custom_code; }
    const char *what() const noexcept override { return _what.data(); }
    operator bool() const { return _sock_code != 0; }

private:
    int _sock_code;
    std::string _what;
    ErrorCode _custom_code;
};

std::ostream &operator<<(std::ostream &os, const SockException &err);

class SockUtil {
public:
    static void setReuseAddr(int sockfd);
    static void setReusePort(int sockfd);
    static void setNoDelay(int sockfd, bool nodelay = true);
    static void setSendBuf(int sockfd, int size);
    static void setRecvBuf(int sockfd, int size);
    static void setSndTimeout(int sockfd, int timeout_ms);
    static void setRcvTimeout(int sockfd, int timeout_ms);
    static void setNonBlock(int sockfd, bool nonblock = true);
    static void setCloseWait(int sockfd, int timeout_sec = 0);
    static void setcloseExec(int sockfd, bool close_exec = true);
    static void setNosigpipe(int sockfd);

    static void setTcpQuickAck(int sockfd, bool quick_ack = true);

    static std::string getLocalIP(int sockfd);
    // static std::pair<std::string, std::string> getInterfaceAddr();

};

}
#endif  //__SOCK_UTIL_H__