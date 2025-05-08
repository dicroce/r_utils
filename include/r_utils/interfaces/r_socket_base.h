
#ifndef r_utils_r_socket_base_h
#define r_utils_r_socket_base_h

#include <stdlib.h>
#include <string>

namespace r_utils
{

class r_socket_base
{
public:
    virtual void connect(const std::string& host, int port) = 0;
    virtual void close() const = 0;
    virtual bool valid() const = 0;
    virtual void send(const void* buf, size_t len) = 0;
    virtual void recv(void* buf, size_t len) = 0;
    virtual int raw_send(const void* buf, size_t len) = 0;
    virtual int raw_recv(void* buf, size_t len) = 0;
};

}

#endif
