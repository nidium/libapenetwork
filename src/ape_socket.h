/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_SOCKET_H
#define __APE_SOCKET_H

#include "ape_common.h"

#include "ape_buffer.h"
#include "ape_pool.h"
#include "ape_lz4.h"
#include "ape_dns.h"

#ifdef _WIN32
  //#include <winsock2.h>
  //#pragma comment(lib, "ws2_32.lib")
  #define ioctl ioctlsocket
  #define hstrerror(x) ""
#else
  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <sys/un.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>

  #include <netdb.h>
#endif

#define APE_SOCKET_BACKLOG 511

/* get a ape_socket pointer from event returns */
#define APE_EVENT_SOCKET_PTR(attach) ((ape_socket *)attach)


#ifdef _HAVE_SSL_SUPPORT
#define APE_SOCKET_ISSECURE(socket) socket->SSL.issecure
#else
#define APE_SOCKET_ISSECURE(socket) 0
#endif

#ifdef __WIN32
struct iovec
{
    unsigned long iov_len;
    char *iov_base;
};

#endif

/* TODO: TCP_NOPUSH  */

#ifdef TCP_CORK
#define PACK_TCP(fd)                                                      \
    do {                                                                  \
        int __state = 1;                                                  \
        setsockopt(fd, IPPROTO_TCP, TCP_CORK, &__state, sizeof(__state)); \
    } while (0)

#define FLUSH_TCP(fd)                                                     \
    do {                                                                  \
        int __state = 0;                                                  \
        setsockopt(fd, IPPROTO_TCP, TCP_CORK, &__state, sizeof(__state)); \
    } while (0)
#else
#define PACK_TCP(fd) ((void)fd);
#define FLUSH_TCP(fd) ((void)fd);
#endif


enum ape_socket_flags
{
    APE_SOCKET_WOULD_BLOCK = (1 << 0),
    APE_SOCKET_CORK        = (1 << 1)
};

enum ape_socket_proto
{
    APE_SOCKET_PT_TCP,
    APE_SOCKET_PT_UDP,
    APE_SOCKET_PT_SSL,
    APE_SOCKET_PT_UNIX
};

enum ape_socket_type
{
    APE_SOCKET_TP_UNKNOWN,
    APE_SOCKET_TP_SERVER,
    APE_SOCKET_TP_CLIENT
};

enum ape_socket_state
{
    APE_SOCKET_ST_ONLINE,
    APE_SOCKET_ST_PROGRESS,
    APE_SOCKET_ST_PENDING,
    APE_SOCKET_ST_OFFLINE,
    APE_SOCKET_ST_SHUTDOWN
};

typedef enum _ape_socket_data_autorelease {
    APE_DATA_STATIC,
    APE_DATA_GLOBAL_STATIC,
    APE_DATA_AUTORELEASE,
    APE_DATA_OWN,
    APE_DATA_COPY
} ape_socket_data_autorelease;

typedef struct _ape_socket ape_socket;

typedef struct
{
    void (*on_read)(
        ape_socket *, const uint8_t *data, size_t len, ape_global *, void *arg);
    void (*on_disconnect)(ape_socket *, ape_global *, void *arg);
    void (*on_connect)(ape_socket *, ape_socket *, ape_global *, void *arg);
    void (*on_connected)(ape_socket *, ape_global *, void *arg);
    void (*on_message)(ape_socket *,
                       ape_global *,
                       const unsigned char *packet,
                       size_t len,
                       struct sockaddr_in *addr,
                       void *arg);
    void (*on_drain)(ape_socket *, ape_global *, void *arg);
    void *arg;
} ape_socket_callbacks;

#define APE_LZ4_COMPRESS_TX (1 << 1)
#define APE_LZ4_COMPRESS_RX (1 << 2)

/* Jobs pool */
/* (1 << 0) is reserved */
#define APE_SOCKET_JOB_WRITEV (1 << 1)
#define APE_SOCKET_JOB_SENDFILE (1 << 2)
#define APE_SOCKET_JOB_SHUTDOWN (1 << 3)
#define APE_SOCKET_JOB_ACTIVE (1 << 4)
#define APE_SOCKET_JOB_IOV (1 << 5)

typedef struct _ape_socket_jobs_t
{
    ape_pool_t pool;
    off_t offset;
} ape_socket_jobs_t;


struct _ape_socket
{
    ape_event_descriptor s;
    buffer data_in;

    ape_pool_list_t jobs;

    struct sockaddr_in sockaddr;
    ape_socket_callbacks callbacks;

    void *ctx;  /* public pointer */
    void *_ctx; /* internal public pointer */
    ape_global *ape;
    ape_socket *parent; /* server socket in case of client */

    ape_timer_t *delay_timer;

    struct _ape_dns_cb_argv *dns_state;

    struct
    {
        struct
        {
            APE_LZ4_stream_t *ctx;
            char *cmp_buffer;
            char *dict_buffer;
        } tx;

        struct
        {
            APE_LZ4_streamDecode_t *ctx;
            struct
            {
                char *data;
                int pos;
            } dict_buffer;

            struct
            {
                char *data;
                int used;
                int size;
            } buffer;

            uint32_t decompress_position;
            uint32_t current_block_size;
        } rx;
    } lz4;

    struct
    {
        uint8_t flags;
        uint8_t proto;
        uint8_t type;
        uint8_t state;
    } states;

#ifdef _HAVE_SSL_SUPPORT
    struct
    {
        struct _ape_ssl *ssl;
        int need_write;
        uint8_t issecure;
    } SSL;
#endif
    uint16_t remote_port;
    uint16_t local_port;
    size_t max_buffer_memory_mb;
    size_t current_buffer_memory_bytes;

};

#define APE_SOCKET_FD(socket) socket->s.fd
#define APE_SOCKET_IS_LZ4(socket, onwhat) (socket->lz4.onwhat.ctx)

#define APE_SOCKET_PACKET_FREE (1 << 1)

typedef struct _ape_socket_packet
{
    /* inherit from ape_pool_t (same first sizeof(ape_pool_t) bytes
     * memory-print) */
    ape_pool_t pool;
    size_t len;
    size_t offset;
    ape_socket_data_autorelease data_type;
} ape_socket_packet_t;

#ifdef __cplusplus
extern "C" {
#endif

ape_socket *APE_socket_new(uint8_t pt, int from, ape_global *ape);

void APE_socket_enable_lz4(ape_socket *socket, int rxtx);
void APE_socket_setBufferMaxSize(ape_socket *socket, size_t MB);

void APE_socket_shutdown_delay(ape_socket *socket, int ms);
void APE_socket_shutdown(ape_socket *socket);
void APE_socket_shutdown_now(ape_socket *socket);
void APE_socket_remove_callbacks(ape_socket *socket);

int APE_socket_setTimeout(ape_socket *socket, sockopt_t secs);
int APE_socket_listen(ape_socket *socket,
                      uint16_t port,
                      const char *local_ip,
                      int defer_accept,
                      int reuse_port);
int APE_socket_connect(ape_socket *socket,
                       uint16_t port,
                       const char *remote_ip_host,
                       uint16_t localport);
int APE_socket_write(ape_socket *socket,
                     void *data,
                     size_t len,
                     ape_socket_data_autorelease data_type);
int APE_socket_writev(ape_socket *socket, const struct iovec *iov, int iovcnt);
int APE_sendfile(ape_socket *socket, const char *file);
int APE_socket_is_online(ape_socket *socket);

char *APE_socket_ipv4(ape_socket *socket);

int ape_socket_destroy(ape_socket *socket);
int ape_socket_do_jobs(ape_socket *socket);
int ape_socket_accept(ape_socket *socket);
int ape_socket_read(ape_socket *socket);
int ape_socket_read_udp(ape_socket *socket);
int ape_socket_connected(void *arf);
int ape_socket_write_udp(ape_socket *from,
                         const char *data,
                         size_t len,
                         const char *ip,
                         uint16_t port);

#ifdef __cplusplus
}
#endif

/*int ape_socket_write_file(ape_socket *socket, const char *file,
        ape_global *ape);
*/
#endif

