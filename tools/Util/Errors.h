#ifndef __ERRORS_H__
#define __ERRORS_H__

#include <errno.h>
#include <cstring>
#include <string>

namespace beton {

class ErrorMsg {
public:
    static std::string getErrorMsg(int err) {
        return std::string(strerror(err));
    }

    static std::string getErrorMsg() {
        return getErrorMsg(errno);
    }
};

}
#endif  //__ERRORS_H__