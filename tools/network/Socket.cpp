#include "Socket.h"

using namespace std;

namespace beton {

SocketHelper::SocketHelper() {
    
}

string SocketHelper::get_peer_ip() {
    return "";
}

string SocketHelper::get_local_ip() {
    return "";
}

uint16_t SocketHelper::get_peer_port() {
    return 0;
}

uint16_t SocketHelper::get_local_port() {
    return 0;
}

}
