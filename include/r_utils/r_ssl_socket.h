
#ifndef r_utils_r_ssl_socket_h
#define r_utils_r_ssl_socket_h

extern "C" {
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
//#include <mbedtls/ssl_internal.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
}

#include "r_utils/interfaces/r_stream_io.h"
#include "r_utils/interfaces/r_socket_io.h"
#include "r_utils/interfaces/r_pollable.h"
#include "r_utils/r_socket_address.h"
#include "r_utils/r_socket.h"
#include <string>
#include <mutex>

namespace r_utils {

class r_ssl_socket : public r_stream_io, public r_pollable, public r_socket_io
{
public:
    r_ssl_socket(bool enable_auth = false);
    ~r_ssl_socket();

    void connect(const std::string& host, int port);
    void close();

    virtual int raw_send(const void* buf, size_t len) override;
    virtual int raw_recv(void* buf, size_t len) override;

    virtual void send(const void* buf, size_t len) override;
    virtual void recv(void* buf, size_t len) override;

    virtual bool valid() const override;

    virtual bool wait_till_recv_wont_block(uint64_t& millis) const override;
    virtual bool wait_till_send_wont_block(uint64_t& millis) const override;

    inline std::string get_peer_ip() const { return _sok.get_peer_ip(); }
    inline std::string get_local_ip() const { return _sok.get_local_ip(); }

private:
    r_raw_socket _sok;

    mbedtls_ssl_context _ssl;
    mbedtls_ssl_config _conf;
    mbedtls_ctr_drbg_context _ctr_drbg;
    mbedtls_entropy_context _entropy;
    mbedtls_x509_crt _ca_cert;

    std::string _host;
    mutable bool _valid;
};

} // namespace r_utils

#endif
