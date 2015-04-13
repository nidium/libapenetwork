/*
    APE Network Library
    Copyright (C) 2010-2013 Anthony Catel <paraboul@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __APE_SOCKET_H
#define __APE_SOCKET_H

#include "common.h"
#include "ape_buffer.h"
#include "ape_pool.h"

#ifdef _WIN32

//#include <winsock2.h>
//#pragma comment(lib, "ws2_32.lib")

#if 0
#define ECONNRESET WSAECONNRESET
#define EINPROGRESS WSAEINPROGRESS
#define EALREADY WSAEALREADY
#define ECONNABORTED WSAECONNABORTED
#endif
#define ioctl ioctlsocket
#define hstrerror(x) ""
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>
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
  char*         iov_base;
};

#endif

/* TODO: TCP_NOPUSH  */

#ifdef TCP_CORK
    #define PACK_TCP(fd) \
    do { \
        int __state = 1; \
        setsockopt(fd, IPPROTO_TCP, TCP_CORK, &__state, sizeof(__state)); \
    } while(0)

    #define FLUSH_TCP(fd) \
    do { \
        int __state = 0; \
        setsockopt(fd, IPPROTO_TCP, TCP_CORK, &__state, sizeof(__state)); \
    } while(0)
#else
    #define PACK_TCP(fd)
    #define FLUSH_TCP(fd)
#endif


enum ape_socket_flags {
    APE_SOCKET_WOULD_BLOCK  = (1 << 0),
    APE_SOCKET_CORK         = (1 << 1)
#ifdef _MSC_VER
};
#else
} __attribute__ ((__packed__));
#endif

enum ape_socket_proto {
    APE_SOCKET_PT_TCP,
    APE_SOCKET_PT_UDP,
    APE_SOCKET_PT_SSL
#ifdef _MSC_VER
};
#else
} __attribute__ ((__packed__));
#endif

enum ape_socket_type {
    APE_SOCKET_TP_UNKNOWN,
    APE_SOCKET_TP_SERVER,
    APE_SOCKET_TP_CLIENT
#ifdef _MSC_VER
};
#else
} __attribute__ ((__packed__));
#endif

enum ape_socket_state {
    APE_SOCKET_ST_ONLINE,
    APE_SOCKET_ST_PROGRESS,
    APE_SOCKET_ST_PENDING,
    APE_SOCKET_ST_OFFLINE,
    APE_SOCKET_ST_SHUTDOWN
#ifdef _MSC_VER
};
#else
} __attribute__ ((__packed__));
#endif

typedef enum _ape_socket_data_autorelease {
    APE_DATA_STATIC,
    APE_DATA_GLOBAL_STATIC,
    APE_DATA_AUTORELEASE,
    APE_DATA_OWN,
    APE_DATA_COPY
} ape_socket_data_autorelease;

typedef struct _ape_socket ape_socket;


typedef struct {
    void (*on_read)         (ape_socket *, ape_global *, void *arg);
    void (*on_disconnect)   (ape_socket *, ape_global *, void *arg);
    void (*on_connect)      (ape_socket *, ape_socket *, ape_global *, void *arg);
    void (*on_connected)    (ape_socket *, ape_global *, void *arg);
    void (*on_message)      (ape_socket *, ape_global *, const unsigned char *packet,
        size_t len, struct sockaddr_in *addr, void *arg);
    void (*on_drain)        (ape_socket *, ape_global *, void *arg);
    void *arg;
} ape_socket_callbacks;


/* Jobs pool */
/* (1 << 0) is reserved */
#define APE_SOCKET_JOB_WRITEV   (1 << 1)
#define APE_SOCKET_JOB_SENDFILE (1 << 2)
#define APE_SOCKET_JOB_SHUTDOWN (1 << 3)
#define APE_SOCKET_JOB_ACTIVE   (1 << 4)
#define APE_SOCKET_JOB_IOV      (1 << 5)

typedef struct _ape_socket_jobs_t {
    union {
        void *data;
        int fd;
        buffer *buf;
    } ptr; /* public */
    struct _ape_pool *next;
    uint32_t flags;
    off_t offset;
} ape_socket_jobs_t;


struct _ape_socket {
    ape_event_descriptor s;
    buffer data_in;
    
    ape_pool_list_t jobs;

    struct sockaddr_in sockaddr;
    ape_socket_callbacks callbacks;

    void *ctx;  /* public pointer */
    void *_ctx; /* internal public pointer */
    ape_global *ape;
    ape_socket *parent; /* server socket in case of client */

    struct _ape_dns_cb_argv *dns_state;

#ifdef _MSC_VER
#pragma pack(push, 1)
    struct {
#else
    struct __attribute__ ((__packed__)) {
#endif
    	enum ape_socket_flags flags;
    	enum ape_socket_proto proto;
    	enum ape_socket_type type;
    	enum ape_socket_state state;
    } states;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _HAVE_SSL_SUPPORT
    struct {
        struct _ape_ssl *ssl;
        uint8_t issecure;
    } SSL;
#endif
    uint16_t    remote_port;
    uint16_t    local_port;
};

#define APE_SOCKET_FD(socket) socket->s.fd

#define APE_SOCKET_PACKET_FREE (1 << 1)

typedef struct _ape_socket_packet {
    /* inherit from ape_pool_t (same first sizeof(ape_pool_t) bytes memory-print) */
    ape_pool_t pool;
    size_t len;
    size_t offset;
    ape_socket_data_autorelease data_type;
} ape_socket_packet_t;

#ifdef __cplusplus
extern "C" {
#endif

ape_socket *APE_socket_new(enum ape_socket_proto pt, int from, ape_global *ape);

int APE_socket_listen(ape_socket *socket, uint16_t port,
        const char *local_ip, int defer_accept, int reuse_port);
int APE_socket_connect(ape_socket *socket, uint16_t port,
        const char *remote_ip_host, uint16_t localport);
int APE_socket_write(ape_socket *socket, void *data,
    size_t len, ape_socket_data_autorelease data_type);
int APE_socket_writev(ape_socket *socket, const struct iovec *iov, int iovcnt);
void APE_socket_shutdown(ape_socket *socket);
void APE_socket_shutdown_now(ape_socket *socket);
int APE_sendfile(ape_socket *socket, const char *file);
char *APE_socket_ipv4(ape_socket *socket);
int APE_socket_is_online(ape_socket *socket);

int ape_socket_destroy(ape_socket *socket);
int ape_socket_do_jobs(ape_socket *socket);
int ape_socket_accept(ape_socket *socket);
int ape_socket_read(ape_socket *socket);
int ape_socket_read_udp(ape_socket *socket);
int ape_socket_connected(void *arf);
int ape_socket_write_udp(ape_socket *from, const char *data,
    size_t len, const char *ip, uint16_t port);

#ifdef __cplusplus
}
#endif

/*int ape_socket_write_file(ape_socket *socket, const char *file,
        ape_global *ape);
*/
#endif

// vim: ts=4 sts=4 sw=4 et

