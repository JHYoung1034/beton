#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "Util/Util.h"
#include "SockUtil.h"
#include <functional>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

namespace beton {

class SockInfo {
public:
    typedef enum : uint8_t {
        Sock_Invalid = 0,
        Sock_Tcp = 1,
        Sock_Udp,
        Sock_TcpServer,
        Sock_UdpServer
    }SockType;

    virtual std::string get_peer_ip() = 0;
    virtual std::string get_local_ip() = 0;
    virtual uint16_t get_peer_port() = 0;
    virtual uint16_t get_local_port() = 0;
};

class SockFd {
public:
    SockFd(int fd) : _sockfd(fd) {}
    ~SockFd() {
        if (_sockfd != -1) {
            ::shutdown(_sockfd, SHUT_RDWR);
            ::close(_sockfd);
        }
    }
    void setFdNum(int fd) { _sockfd = fd; }
    int getFdNum() const { return _sockfd; }
private:
    int _sockfd = -1;
};

/**
 * 一个socket应该包含 fd 的IO管理(创建，关闭，接收，发送)，以及网络建立管理(bind, connect, listen, accept)
 * socket 类属于事件发起的源头，应该绑定到一个线程上，使其仅在唯一线程处理事件，避免线程竞争
*/

class Socket : public SockInfo, public noncopyable, public std::enable_shared_from_this<Socket> {
public:
    using Ptr = std::shared_ptr<Socket>;
    using onConnectRes = std::function<void(const SockException &ex)>;
    using onError = std::function<void(const SockException &ex)>;
    using onAcceptBefore = std::function<void(int sockfd)>;
    using onAccept = std::function<void(const Socket::Ptr &sock)>;
    using onRecv = std::function<void(const std::string &data)>;

    Socket(SockInfo::SockType type);
    ~Socket();

    //------------------SockInfo----------------//
    std::string get_peer_ip() override;
    std::string get_local_ip() override;
    uint16_t get_peer_port() override;
    uint16_t get_local_port() override;

    //--------------网络接口管理------------------//
    /**
     * @param host: 目标地址，IP或者域名
     * @param func: 结果回调
     * @param port: 目标端口，如果host是域名，可不填
     * @param timeout_sec: 连接超时，默认5s
     * @param bind_addr: 网卡/IP, 把socket绑定到本地的网卡/IP上
     * @param bind_port: 绑定到某个端口上，无需内核自动选择
    */
    void connect(const std::string &host, const onConnectRes &func,
            uint16_t port = 0, uint8_t timeout_sec = 5,
            const std::string &bind_addr = "", uint16_t bind_port = 0);

    /**
     * @param port: 监听端口
     * @param bind_addr: 指定监听的网卡/IP, 空IP即监听所有网卡/IP
     * @param backlog: 最大排队队列，包含了 [未就绪] 和 [已就绪] 连接, 默认1024
    */
    void listen(uint16_t port, const std::string &bind_addr = "::", int backlog = 1024);

    //---------------设置一些事件回调-----------------------//
    void setAcceptBeforceCB(const onAcceptBefore &cb);

    void setAcceptCB(const onAccept &cb);

    void setRecvCB(const onRecv &cb);

    void setErrorCB(const onError &cb);

private:
    

};


class SocketHelper : public SockInfo, public std::enable_shared_from_this<SocketHelper> {
public:
    SocketHelper();

    //---------------sockInfo------------------//
    std::string get_peer_ip() override;
    std::string get_local_ip() override;
    uint16_t get_peer_port() override;
    uint16_t get_local_port() override;


private:

};

}
#endif  //__SOCKET_H__