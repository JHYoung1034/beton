#include "SockUtil.h"
#include <errno.h>
#include <string.h>

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

}