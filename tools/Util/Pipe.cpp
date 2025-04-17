#include "Pipe.h"
#include "network/SockUtil.h"
#include <unistd.h>
#include <fcntl.h>

using namespace std;

namespace beton {

#define CHECK_CLOSE(fd) \
    if (fd != -1) { \
        ::close(fd); \
        fd = -1; \
    }

Pipe::Pipe() {
    reset();
}

Pipe::~Pipe() {
    close();
}
void Pipe::reset() {
    close();
    open();
}

void Pipe::close() {
    CHECK_CLOSE(_pipe_fd[0]);
    CHECK_CLOSE(_pipe_fd[1]);
}

void Pipe::open() {
    if (pipe(_pipe_fd) == -1) {
        _pipe_fd[0] = _pipe_fd[1] = -1;
        throw SockException(errno, "create pipe failed");
    }
    //设置非阻塞
    SockUtil::setNonBlock(_pipe_fd[0]);
    SockUtil::setNonBlock(_pipe_fd[1]);
    //设置close-on-exec
    SockUtil::setcloseExec(_pipe_fd[0]);
    SockUtil::setcloseExec(_pipe_fd[1]);
}

int Pipe::read(char *buf, int len) {
    int ret = 0;
    if (isValid()) {
        do {
            ret = ::read(_pipe_fd[0], buf, len);
        } while (ret == -1 && errno == EINTR);
    }
    return ret;
}

int Pipe::write(const char *buf, int len) {
    int ret = 0;
    if (isValid()) {
        do {
            ret = ::write(_pipe_fd[1], buf, len);
        } while (ret == -1 && errno == EINTR);
    }
    return ret;
}

}