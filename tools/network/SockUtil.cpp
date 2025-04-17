#include "SockUtil.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

namespace beton {

SockException::SockException(int sock_code, const std::string &des, ErrorCode custom_code)
    : _sock_code(sock_code), _what(des), _custom_code(custom_code) {
    if (_sock_code != 0) {
        _what.reserve(128);
        strerror_r(_sock_code, (char *)_what.data(), 128);
    }
}

std::ostream &operator<<(std::ostream &os, const SockException &ex) {
    os << ex.sock_code() << "|" << ex.custom_code() << "(" << ex.what() << ")";
    return os;
}


///////////////////////////////////////////////////////////////////////////////////
void SockUtil::setReuseAddr(int sockfd) {
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == -1) {
        throw SockException(errno, "set reuse addr failed");
    }
}

void SockUtil::setReusePort(int sockfd) {
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char *)&opt, sizeof(opt)) == -1) {
        throw SockException(errno, "set reuse port failed");
    }
}

void SockUtil::setNoDelay(int sockfd, bool nodelay) {
    int opt = nodelay ? 1 : 0;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt)) == -1) {
        throw SockException(errno, "set no delay failed");
    }
}

void SockUtil::setSendBuf(int sockfd, int size) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&size, sizeof(size)) == -1) {
        throw SockException(errno, "set send buffer failed");
    }
}

void SockUtil::setRecvBuf(int sockfd, int size) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(size)) == -1) {
        throw SockException(errno, "set recv buffer failed");
    }
}

void SockUtil::setSndTimeout(int sockfd, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv)) == -1) {
        throw SockException(errno, "set send timeout failed");
    }
}

void SockUtil::setRcvTimeout(int sockfd, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) == -1) {
        throw SockException(errno, "set recv timeout failed");
    }
}

void SockUtil::setNonBlock(int sockfd, bool nonblock) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        throw SockException(errno, "get socket flags failed");
    }
    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        throw SockException(errno, "set socket flags failed");
    }
}

void SockUtil::setCloseWait(int sockfd, int timeout_sec) {
    if (timeout_sec > 0) {
        struct linger so_linger;
        so_linger.l_onoff = 1;
        so_linger.l_linger = timeout_sec;
        if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char *)&so_linger, sizeof(so_linger)) == -1) {
            throw SockException(errno, "set close wait failed");
        }
    } else {
        struct linger so_linger;
        so_linger.l_onoff = 0;
        so_linger.l_linger = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char *)&so_linger, sizeof(so_linger)) == -1) {
            throw SockException(errno, "set close wait failed");
        }
    }

}

void SockUtil::setcloseExec(int sockfd, bool close_exec) {
    int flags = fcntl(sockfd, F_GETFD, 0);
    if (flags == -1) {
        throw SockException(errno, "get socket flags failed");
    }
    if (close_exec) {
        flags |= FD_CLOEXEC;
    } else {
        flags &= ~FD_CLOEXEC;
    }
    if (fcntl(sockfd, F_SETFD, flags) == -1) {
        throw SockException(errno, "set socket flags failed");
    }
}

void SockUtil::setNosigpipe(int sockfd) {
#if defined(SO_NOSIGPIPE)
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&opt, sizeof(opt)) == -1) {
        throw SockException(errno, "set nosigpipe failed");
    }
#endif
}

void SockUtil::setTcpQuickAck(int sockfd, bool quick_ack) {
    int opt = quick_ack ? 1 : 0;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, (const char *)&opt, sizeof(opt)) == -1) {
        throw SockException(errno, "set tcp quick ack failed");
    }
}

std::string SockUtil::getLocalIP(int sockfd) {
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &addr_len) == -1) {
        throw SockException(errno, "get local ip failed");
    }
    if (inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == nullptr) {
        throw SockException(errno, "inet_ntop failed");
    }
    return std::string(ip);
}

// std::pair<std::string, std::string> SockUtil::getInterfaceAddr() {

// }

}