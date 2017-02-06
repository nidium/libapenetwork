/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_socket.h"
#include "ape_dns.h"
#include "ape_timers_next.h"
#include "ape_ssl.h"
#include <stdint.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/uio.h>
#include <openssl/err.h>
#else
#include <io.h>
#include <malloc.h>
#endif
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <ares.h>
#else
#include "ares.h"
#endif

#ifdef __linux__
#include <sys/sendfile.h>
#endif
#include <limits.h>
#include <string.h>

#define APE_LZ4_BLOCK_SIZE (1024 * 8)
#define APE_LZ4_BLOCK_COMP_SIZE APE_LZ4_COMPRESSBOUND(APE_LZ4_BLOCK_SIZE)
#define APE_LZ4_BUFFER_SIZE (1024 * 16)

#define APE_LZ4_DICT_BUFFER_SIZE (1024 * 64)

static int ape_socket_free(void *arg);
static int ape_shutdown(ape_socket *socket, int rw);
static int ape_socket_destroy_async(ape_socket *socket);

#ifdef _MSC_VER
long int writev(int fd, const struct iovec *vector, int count)
{
    DWORD sent;
    int ret = WSASend(fd, (LPWSABUF)vector, count, &sent, 0, 0, 0);

    return sent;
}
#endif

/*
Use only one syscall (ioctl) if FIONBIO is defined
It behaves the same for socket file descriptor to use
either ioctl(...FIONBIO...) or fcntl(...O_NONBLOCK...)
*/
#ifdef FIONBIO
static __inline int setnonblocking(int fd)
{
    int ret = 1;

    return ioctl(fd, FIONBIO, &ret);
}
#else
#define setnonblocking(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

int _nco = 0, _ndec = 0;

static ape_socket_jobs_t *ape_socket_job_get_slot(ape_socket *socket, int type);
static ape_pool_list_t *ape_socket_new_packet_queue(size_t n);
static int ape_socket_queue_data(ape_socket *socket, unsigned char *data,
                                 size_t len, size_t offset,
                                 ape_socket_data_autorelease data_type);
static void ape_init_job_list(ape_pool_list_t *list, size_t n);

__inline static void
ape_socket_release_data(unsigned char *data,
                        ape_socket_data_autorelease data_type)
{
    switch (data_type) {
        case APE_DATA_AUTORELEASE:
        case APE_DATA_COPY:
            free(data);
            break;
        case APE_DATA_STATIC:
        case APE_DATA_GLOBAL_STATIC:
        case APE_DATA_OWN:
        default:
            break;
    }
}

void APE_socket_enable_lz4(ape_socket *socket, int rxtx)
{
    if ((rxtx & APE_LZ4_COMPRESS_TX) && !socket->lz4.tx.ctx) {
        socket->lz4.tx.ctx         = APE_LZ4_createStream();
        socket->lz4.tx.cmp_buffer  = malloc(APE_LZ4_BUFFER_SIZE);
        socket->lz4.tx.dict_buffer = malloc(APE_LZ4_DICT_BUFFER_SIZE);
    }

    if ((rxtx & APE_LZ4_COMPRESS_RX) && !socket->lz4.rx.ctx) {
        socket->lz4.rx.ctx              = APE_LZ4_createStreamDecode();
        socket->lz4.rx.dict_buffer.data = malloc(APE_LZ4_DICT_BUFFER_SIZE);
        socket->lz4.rx.dict_buffer.pos  = 0;

        socket->lz4.rx.buffer.size = APE_LZ4_BLOCK_COMP_SIZE + sizeof(int);
        socket->lz4.rx.buffer.data = malloc(socket->lz4.rx.buffer.size);
        socket->lz4.rx.buffer.used = 0;

        socket->lz4.rx.decompress_position = 0;
        socket->lz4.rx.current_block_size  = 0;
    }
}

static void ape_socket_free_lz4(ape_socket *socket)
{

    if (socket->lz4.tx.ctx) {
        APE_LZ4_freeStream(socket->lz4.tx.ctx);
        free(socket->lz4.tx.cmp_buffer);
        free(socket->lz4.tx.dict_buffer);
    }

    if (socket->lz4.rx.ctx) {
        APE_LZ4_freeStreamDecode(socket->lz4.rx.ctx);
        free(socket->lz4.rx.dict_buffer.data);
        free(socket->lz4.rx.buffer.data);
    }
}

ape_socket *APE_socket_new(uint8_t pt, int from, ape_global *ape)
{
    int sock  = from,
        proto = (pt == APE_SOCKET_PT_UDP ? SOCK_DGRAM : SOCK_STREAM);
    ape_socket *ret;

    _nco++;

    if ((sock == 0
         && (sock
             = socket((pt == APE_SOCKET_PT_UNIX ? AF_UNIX
                                                : AF_INET) /* TODO AF_INET6 */,
                      proto, 0))
                == -1)
        || setnonblocking(sock) == -1) {

        APE_ERROR("libapenetwork", "[Socket] Cant create socket(%d) : %s\n", SOCKERRNO,
               strerror(SOCKERRNO));
        return NULL;
    }

    ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));
    ret->ape = ape;

    ret->s.fd   = sock;
    ret->s.type = APE_EVENT_SOCKET;

    ret->states.flags = 0;
    ret->states.proto = pt;
    ret->states.type  = APE_SOCKET_TP_UNKNOWN;
    ret->states.state = APE_SOCKET_ST_PENDING;

    ret->delay_timer = NULL;

#ifdef _HAVE_SSL_SUPPORT
    ret->SSL.issecure   = (pt == APE_SOCKET_PT_SSL);
    ret->SSL.need_write = 0;
#endif

    buffer_init(&ret->data_in);
    ape_init_job_list(&ret->jobs, 2);

    return ret;
}

void APE_socket_setBufferMaxSize(ape_socket *socket, size_t MB)
{
    socket->max_buffer_memory_mb = MB;
}

int APE_socket_setTimeout(ape_socket *socket, sockopt_t secs)
{
    if (socket->states.proto == APE_SOCKET_PT_UDP) {
        return 0;
    }

#ifdef TCP_KEEPALIVE /* BSD, Darwin */
#define KEEPALIVE_OPT TCP_KEEPALIVE
#elif defined(TCP_KEEPIDLE) /* Linux */
#define KEEPALIVE_OPT TCP_KEEPIDLE
#else
#error "TCP KeepAlive not supported"
#endif

#ifndef SO_KEEPALIVE
#error "TCP KeepAlive not supported"
#endif
    sockopt_t enable = 1;

    if (setsockopt(socket->s.fd, SOL_SOCKET, SO_KEEPALIVE, &enable,
                   sizeof(enable))
        == -1) {
        APE_ERROR("libapenetwork",
                "[Socket] Failed to set socket timeout (SO_KEEPALIVE) : error %d %s\n",
                errno, strerror(errno));
        return 0;
    }

    if (setsockopt(socket->s.fd, IPPROTO_TCP, KEEPALIVE_OPT, &secs,
                   sizeof(secs))
        == -1) {
        APE_ERROR("libapenetwork",
                "[Socket] Failed to set socket timeout (TCP_KEEPALIVE) : error %d %s\n",
                errno, strerror(errno));
        return 0;
    }
#ifdef TCP_KEEPINTVL
    sockopt_t keepintvl = 5;
    if (setsockopt(socket->s.fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl,
                   sizeof(keepintvl))
        == -1) {
        APE_ERROR("libapenetwork",
                "[Socket] Failed to set socket timeout (TCP_KEEPINTVL) : error %d %s\n",
                errno, strerror(errno));
        return 0;
    }
#endif
#ifdef TCP_KEEPCNT
    sockopt_t kepcnt = 3;
    if (setsockopt(socket->s.fd, IPPROTO_TCP, TCP_KEEPINTVL, &kepcnt,
                   sizeof(kepcnt))
        == -1) {
        APE_ERROR("libapenetwork",
                "[Socket] Failed to set socket timeout (TCP_KEEPCNT) : error %d %s\n",
                errno, strerror(errno));
        return 0;
    }
#endif

#ifdef TCP_USER_TIMEOUT
    size_t mstimeout = secs * 1000ULL;

    if (setsockopt(socket->s.fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &mstimeout,
                   sizeof(mstimeout))
        == -1) {
        APE_ERROR("libapenetwork",
            "[Socket] Failed to set socket timeout (TCP_USER_TIMEOUT) : error %d %s\n",
            errno, strerror(errno));
        return 0;
    }
#endif

    return 1;
}

int APE_socket_listen(ape_socket *socket, uint16_t port, const char *local_ip,
                      int defer_accept, int reuse_port)
{
    struct sockaddr_in addr;
    sockopt_t reuse_addr = 1;

#ifdef TCP_DEFER_ACCEPT
    int timeout = 2;
#endif
    if (port == 0) {
        return -1;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(local_ip);
    memset(&(addr.sin_zero), '\0', 8);

    setsockopt(socket->s.fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
               sizeof(int));
#ifdef SO_REUSEPORT
    if (reuse_port
        && setsockopt(socket->s.fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port,
                      sizeof(int))
               != 0) {

        APE_WARN("libapenetwork", "[Socket] setsockopt (SO_REUSEPORT) (%s:%u) warning : %s\n",
               local_ip, port, strerror(errno));
    }
#else
    if (reuse_port) {
        APE_WARN("libapenetwork",
            "[Socket] SO_REUSEPORT is requested but compiled on an unsuported "
            "target\n");
    }
#endif

    if (bind(socket->s.fd, (struct sockaddr *)&addr, sizeof(struct sockaddr))
            == -1
        ||
        /* only listen for STREAM socket */
        ((socket->states.proto == APE_SOCKET_PT_TCP
          || socket->states.proto == APE_SOCKET_PT_SSL)
         && listen(socket->s.fd, APE_SOCKET_BACKLOG) == -1)) {

        sclose(socket->s.fd);
        APE_ERROR("libapenetwork", "[Socket] bind(%s:%u) error : %s\n", local_ip, port,
               strerror(SOCKERRNO));
        return -1;
    }
    if (defer_accept) {
#ifdef TCP_DEFER_ACCEPT
        setsockopt(socket->s.fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout,
                   sizeof(int));
#else
#ifdef SO_ACCEPTFILTER
        {
            accept_filter_arg afa = { "dataready", "" };
            setsockopt(socket->s.fd, SOL_SOCKET, SO_ACCEPTFILTER, &afa,
                       sizeof(afa));
        }
#endif
#endif
    }
    socket->states.type  = APE_SOCKET_TP_SERVER;
    socket->states.state = APE_SOCKET_ST_ONLINE;
    socket->local_port   = port;

    events_add((ape_event_descriptor *)socket, EVENT_READ | EVENT_WRITE,
               socket->ape);

    return 0;
}

static int ape_socket_connect_ready_to_connect(const char *remote_ip, void *arg,
                                               int status)
{
    ape_socket *socket = arg;
    struct sockaddr_in addr;
    struct sockaddr *punaddr;

    socket->dns_state = NULL;

#ifdef _HAS_ARES_SUPPORT
    if (status != ARES_SUCCESS) {
        ape_socket_destroy(socket);
        return -1;
    }
#endif

    if (socket->states.proto == APE_SOCKET_PT_UNIX) {
#ifdef _WIN32
        APE_ERROR("libapenetwork", "[Socket] Unix socket are not supported on Windows\n");
        return -1;
#else
        struct sockaddr_un unixaddr;

        unixaddr.sun_family = AF_UNIX;
        memset(unixaddr.sun_path, 0, sizeof(unixaddr.sun_path));
        memcpy(unixaddr.sun_path, remote_ip,
               ape_min(strlen(remote_ip), sizeof(unixaddr.sun_path) - 1));

        punaddr = (struct sockaddr *)&unixaddr;
#endif
    } else {

        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(socket->remote_port);
        addr.sin_addr.s_addr = inet_addr(remote_ip);
        memset(&(addr.sin_zero), '\0', 8);

        punaddr = (struct sockaddr *)&addr;
    }
    int ntry = 0;
retry_connect:
    if (socket->local_port != 0) {
        struct sockaddr_in addr_loc;
        addr_loc.sin_family      = AF_INET;
        addr_loc.sin_port        = htons(socket->local_port);
        addr_loc.sin_addr.s_addr = inet_addr("0.0.0.0");
        memset(&(addr_loc.sin_zero), '\0', 8);

        if (bind(socket->s.fd, (struct sockaddr *)&addr_loc, sizeof(addr_loc))
            == -1) {
            APE_ERROR("libapenetwork", "[Socket] bind() error(%d) on %d : %s\n", SOCKERRNO,
                   socket->s.fd, strerror(SOCKERRNO));
            return -1;
        }
    }

    if (connect(socket->s.fd, punaddr, sizeof(struct sockaddr)) == -1
        && (SOCKERRNO != EWOULDBLOCK && SOCKERRNO != EINPROGRESS)) {
        APE_ERROR("libapenetwork", "[Socket] connect() error(%d) on %d : %s (retry : %d)\n",
               SOCKERRNO, socket->s.fd, strerror(SOCKERRNO), ntry);

        switch (SOCKERRNO) {
            case EADDRNOTAVAIL:
                ntry++;
                if (ntry < 10) {
                    goto retry_connect;
                }
            default:
                break;
        }

        ape_socket_destroy(socket);
        return -1;
    }
    socket->states.type = APE_SOCKET_TP_CLIENT;

    /* UDP is "always connected" */
    socket->states.state
        = (socket->states.proto == APE_SOCKET_PT_UDP ? APE_SOCKET_ST_ONLINE
                                                     : APE_SOCKET_ST_PROGRESS);

    events_add((ape_event_descriptor *)socket, EVENT_READ | EVENT_WRITE,
               socket->ape);

    if (socket->states.proto == APE_SOCKET_PT_UDP) {
        ape_global *ape = socket->ape;
        timer_dispatch_async(ape_socket_connected, socket);
    }

    return 0;
}

int APE_socket_connect(ape_socket *socket, uint16_t port,
                       const char *remote_ip_host, uint16_t localport)
{
    if (!socket) {
        return -1;
    }
    if (port == 0) {
        ape_socket_destroy(socket);
        return -1;
    }

    socket->remote_port = port;
    socket->local_port  = localport;

    if (socket->states.proto == APE_SOCKET_PT_UNIX) {
        socket->dns_state = NULL;
        return ape_socket_connect_ready_to_connect(remote_ip_host, socket, 0);
    }

#ifdef _HAS_ARES_SUPPORT
    socket->dns_state
        = ape_gethostbyname(remote_ip_host, ape_socket_connect_ready_to_connect,
                            socket, socket->ape);

#else
    return ape_socket_connect_ready_to_connect(remote_ip_host, socket, 0);
#endif
    return 0;
}

/*
    Queue a shutdown
*/
void APE_socket_shutdown(ape_socket *socket)
{
    if (!socket) {
        return;
    }

    if (socket->states.state == APE_SOCKET_ST_SHUTDOWN) {
        return;
    }
    if (socket->states.state == APE_SOCKET_ST_PROGRESS
        || socket->states.state == APE_SOCKET_ST_PENDING) {

        ape_socket_destroy(socket);

        return;
    }

    if (socket->states.state != APE_SOCKET_ST_ONLINE) {
        return;
    }
    if ((socket->states.flags & APE_SOCKET_WOULD_BLOCK)
        || (socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE)) {
        ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SHUTDOWN);
        // APE_DEBUG("libapenetwork", "[Socket] Shutdown pushed to queue\n");
        return;
    }

    ape_shutdown(socket, SHUT_RDWR);
}

int ape_socket_shutdown_delay_cb(void *socket)
{
    APE_socket_shutdown((ape_socket *)socket);

    ((ape_socket *)socket)->delay_timer = NULL;

    return 0;
}

void APE_socket_shutdown_delay(ape_socket *socket, int ms)
{
    if (!socket || socket->delay_timer) {
        return;
    }

    socket->delay_timer =
        APE_timer_create(socket->ape, ms, ape_socket_shutdown_delay_cb, socket);
}


/*
    Shutdown the socket without pushing the action to the job list
*/
void APE_socket_shutdown_now(ape_socket *socket)
{
    if (!socket) {
        return;
    }
    if (socket->states.state == APE_SOCKET_ST_SHUTDOWN) {
        return;
    }
    if (socket->states.state == APE_SOCKET_ST_PROGRESS
        || socket->states.state == APE_SOCKET_ST_PENDING) {

        ape_socket_destroy(socket);
        return;
    }
    if (socket->states.state != APE_SOCKET_ST_ONLINE) {
        return;
    }

    ape_shutdown(socket, SHUT_RDWR);
}

void APE_socket_remove_callbacks(ape_socket *socket)
{
    socket->callbacks.on_connect    = NULL;
    socket->callbacks.on_connected  = NULL;
    socket->callbacks.on_disconnect = NULL;
    socket->callbacks.on_drain      = NULL;
    socket->callbacks.on_message    = NULL;
    socket->callbacks.arg           = NULL;
}

static int ape_socket_close(ape_socket *socket)
{
    ape_global *ape;

    if (socket == NULL || socket->states.state == APE_SOCKET_ST_OFFLINE)
        return 0;

    ape = socket->ape;
    ape_dns_invalidate(socket->dns_state);
    socket->states.state = APE_SOCKET_ST_OFFLINE;

    if (socket->callbacks.on_disconnect != NULL) {
        socket->callbacks.on_disconnect(socket, ape, socket->callbacks.arg);
    }

    sclose(APE_SOCKET_FD(socket));

    events_del(APE_SOCKET_FD(socket), ape);

    return 1;
}

static void ape_socket_packet_pool_cleaner(ape_pool_t *pool, void *ctx)
{
    ape_socket_packet_t *packet = (ape_socket_packet_t *)pool;
    ape_socket *socket          = (ape_socket *)ctx;

    if (packet->pool.ptr.data != NULL) {
        socket->ape->total_memory_buffered -= packet->len - packet->offset;
        ape_socket_release_data(packet->pool.ptr.data, packet->data_type);
    }
}

static void ape_socket_job_pool_cleaner(ape_pool_t *pool, void *ctx)
{
    if (pool->flags & APE_SOCKET_JOB_ACTIVE) {
        switch (pool->flags & ~(APE_POOL_ALL_FLAGS | APE_SOCKET_JOB_ACTIVE)) {
            case APE_SOCKET_JOB_WRITEV: {
                ape_pool_list_t *plist = (ape_pool_list_t *)pool->ptr.data;
                ape_destroy_pool_list_with_cleaner(
                    plist, ape_socket_packet_pool_cleaner, ctx);
            } break;
            case APE_SOCKET_JOB_SENDFILE:
                close(pool->ptr.fd);
                break;
            default:
                break;
        }
    }
}

static int ape_socket_free(void *arg)
{
    ape_socket *socket = arg;
    _ndec++;
#ifdef _HAVE_SSL_SUPPORT
    if (socket->SSL.issecure) {
        ape_ssl_destroy(socket->SSL.ssl);
    }
#endif
    buffer_delete(&socket->data_in);
    ape_destroy_pool_with_cleaner(socket->jobs.head,
                                  ape_socket_job_pool_cleaner, socket);

    ape_socket_free_lz4(socket);

    if (socket->delay_timer) {
        APE_timer_destroy(socket->ape, socket->delay_timer);
    }

    free(socket);

    return 0;
}

static int _ape_socket_destroy(void *arg)
{
    ape_socket *socket = arg;

    if (ape_socket_close(socket)) {
        ape_socket_free(socket);
    }

    return 0;
}

static int ape_socket_destroy_async(ape_socket *socket)
{
    ape_global *ape;

    if (socket == NULL || socket->states.state == APE_SOCKET_ST_OFFLINE)
        return -1;

    ape = socket->ape;

    timer_dispatch_async(_ape_socket_destroy, socket);

    return 0;
}

int ape_socket_destroy(ape_socket *socket)
{
    ape_global *ape;

    if (!ape_socket_close(socket)) {
        return -1;
    }

    ape = socket->ape;
    timer_dispatch_async(ape_socket_free, socket);

    return 0;
}

#ifndef __WIN32
int APE_sendfile(ape_socket *socket, const char *file)
{
    int fd;
    ape_socket_jobs_t *job;
    off_t offset_file = 0, nwrite = 0;

    if (socket->states.state != APE_SOCKET_ST_ONLINE) {
        return 0;
    }

    if ((fd = open(file, O_RDONLY)) == -1) {
        APE_ERROR("libapenetwork", "[Socket] Failed to open %s - %s\n", file, strerror(errno));
        return 0;
    }

    if ((socket->states.flags & APE_SOCKET_WOULD_BLOCK)
        || (socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE)) {

        socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
        job              = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SENDFILE);
        job->pool.ptr.fd = fd;

        return 1;
    }
#if (defined(__APPLE__) || defined(__FREEBSD__))
    nwrite = 4096;
    while (sendfile(fd, socket->s.fd, offset_file, &nwrite, NULL, 0) == 0
           && nwrite != 0) {
        offset_file += nwrite;
        nwrite = 4096;
    }

    if (nwrite != 0) {
        nwrite = -1;
    }
#else
    do {
        nwrite = sendfile(socket->s.fd, fd, NULL, 4096);
    } while (nwrite > 0);
#endif
    if (nwrite == -1 && errno == EAGAIN) {
        socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
        job              = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SENDFILE);
        job->pool.ptr.fd = fd;
        /* BSD/OSX require offset */
        job->offset = offset_file;
    } else {
        // APE_DEBUG("libapenetwork", "[Socket] File sent...\n");
        close(fd);
    }

    return 1;
}

int APE_socket_writev(ape_socket *socket, const struct iovec *iov, int iovcnt)
{
    ssize_t wroteBytes;

    if (!socket || socket->states.state != APE_SOCKET_ST_ONLINE
        || iovcnt == 0) {
        return -1;
    }

#ifdef _HAVE_SSL_SUPPORT
    if (APE_SOCKET_ISSECURE(socket)) {
        /*TODO: SSL, NOT IMPLEMENTED */
        return -1;
    }
#endif
    if ((socket->states.flags & APE_SOCKET_WOULD_BLOCK)
        || (socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE)) {
        return 1;
    }
    wroteBytes = writev(socket->s.fd, iov, iovcnt);
    /* TODO: Verify if the write was finished */
    if (wroteBytes == -1) {
        APE_ERROR("libapenetwork", "[Socket] IO error (%d) : %s\n", APE_SOCKET_FD(socket), strerror(errno));
        APE_socket_shutdown_now(socket);
        return -1;
    }

    return 0;
}
#endif

int APE_socket_write(ape_socket *socket, void *data, size_t len,
                     ape_socket_data_autorelease data_type)
{
    size_t t_bytes = 0, r_bytes = len;
    ssize_t n    = 0;
    int io_error = 0, rerrno = 0;

    if (!socket || socket->states.state != APE_SOCKET_ST_ONLINE || len == 0) {

        ape_socket_release_data(
            data, (data_type == APE_DATA_COPY ? APE_DATA_OWN : data_type));
        return -1;
    }

    if ((socket->states.flags & APE_SOCKET_WOULD_BLOCK)
        || (socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE)) {
        ape_socket_queue_data(socket, data, len, 0, data_type);
        return len;
    }
#ifdef _HAVE_SSL_SUPPORT
    if (APE_SOCKET_ISSECURE(socket)) {
        int w;
        // APE_DEBUG("libapenetwork", "[Socket] Want write on a secure connection\n");

        ERR_clear_error();

        if ((w = ape_ssl_write(socket->SSL.ssl, data, len)) == -1) {
            unsigned long err = SSL_get_error(socket->SSL.ssl->con, w);
            switch (err) {
                case SSL_ERROR_ZERO_RETURN:
                    break;
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_READ:
                    /*
                        Flag that indicates that we need to recall
                        the same ape_ssl_write() in the next socket read event.
                        We will call ape_socket_do_jobs() to retry.
                    */
                    socket->SSL.need_write = 1;

                    /*  In the case where OpenSSL didn't manage to write
                        the whole buffer we need to recall ape_ssl_write()
                        with the same buffer content and len.

                        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER is set in order
                        to give a chance to a APE_DATA_STATIC
                        to be wired with zero-copy.

                        TODO: check SSL_MODE_ENABLE_PARTIAL_WRITE for
                        more granulated buffer.
                    */
                    socket->states.flags |= APE_SOCKET_WOULD_BLOCK;

                    ape_socket_queue_data(socket, data, len, 0, data_type);
                    return len;
                default:
                    io_error = 1;
                    break;
            }
        }

    } else {
        if (APE_SOCKET_IS_LZ4(socket, tx)) {

            int number_of_blocks
                = len / APE_LZ4_BLOCK_SIZE + (len % APE_LZ4_BLOCK_SIZE ? 1 : 0);

            /* Maximum size of a compressed buffer */
            int block_size = APE_LZ4_BLOCK_COMP_SIZE;
            /*
                Add 4 bytes to save the original size of each block
            */
            int dst_len = (block_size + sizeof(int)) * number_of_blocks;
            /*
                Check if we can use our pre-allocated buffer
            */
            char *dst_cmp = (dst_len > APE_LZ4_BUFFER_SIZE)
                                ? (char *)malloc(dst_len)
                                : socket->lz4.tx.cmp_buffer;

            int dst_pos = 0;
            for (int cur_block = 0; cur_block < number_of_blocks; cur_block++) {

                int cmp_len = APE_LZ4_compress_fast_continue(
                    socket->lz4.tx.ctx,
                    /* src */ (char *)data + (cur_block * APE_LZ4_BLOCK_SIZE),
                    /* dst */ dst_cmp + dst_pos + sizeof(int),
                    /* src_size */ ape_min(
                        APE_LZ4_BLOCK_SIZE,
                        len - (APE_LZ4_BLOCK_SIZE * cur_block)),
                    /* dst_size */ block_size, 1);

                /* Copy the compressed size, right before the compressed data */
                memcpy(dst_cmp + dst_pos, &cmp_len, sizeof(int));

                dst_pos += cmp_len + sizeof(int);

                if (cmp_len <= 0) {
                    APE_ERROR("libapenetwork", "[Socket] LZ4 compression error %d\n", cmp_len);
                    return -1;
                }
            }

            /*
                We can't keep track of our buffer.
                Save it. (max 64KB)
            */
            APE_LZ4_saveDict(socket->lz4.tx.ctx, socket->lz4.tx.dict_buffer,
                             APE_LZ4_DICT_BUFFER_SIZE);

            /*
                Release the original data since we're using the compressed one,
                and that the dict was saved to LZ4.
            */
            ape_socket_release_data(
                data, (data_type == APE_DATA_COPY ? APE_DATA_OWN : data_type));

            data = dst_cmp;
            len = r_bytes = dst_pos;

            data_type = (dst_cmp == socket->lz4.tx.cmp_buffer) ? APE_DATA_OWN
                                                               : APE_DATA_COPY;
        }
#endif
        while (t_bytes < len) {

            if ((n = swrite(socket->s.fd, data + t_bytes, r_bytes)) < 0) {
                if (SOCKERRNO == EAGAIN && r_bytes != 0) {
                    socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                    ape_socket_queue_data(socket, data, len, t_bytes,
                                          data_type);
                    return r_bytes;
                } else {
                    io_error = 1;
                    rerrno   = SOCKERRNO;
                    break;
                }
            }

            t_bytes += n;
            r_bytes -= ape_min(n, 0);
        }
#ifdef _HAVE_SSL_SUPPORT
    }
#endif

    ape_socket_release_data(
        data, (data_type == APE_DATA_COPY && !APE_SOCKET_IS_LZ4(socket, tx)
                   ? APE_DATA_OWN
                   : data_type));

    if (io_error) {
        APE_ERROR("libapenetwork", "[Socket] IO error (%d) : %s\n", APE_SOCKET_FD(socket), strerror(rerrno));
        APE_socket_shutdown_now(socket);
        return -1;
    }

    return 0;
}

char *APE_socket_ipv4(ape_socket *socket)
{
    if (!socket) {
        return NULL;
    }

    /*
        /!\ this is not reentrant
    */
    char *ip = inet_ntoa(socket->sockaddr.sin_addr);

    return ip;
}

int APE_socket_is_online(ape_socket *socket)
{
    return (socket && socket->states.state == APE_SOCKET_ST_ONLINE);
}

int ape_socket_do_jobs(ape_socket *socket)
{
    int njobsdone = 0;

#if defined(IOV_MAX)
    const size_t max_chunks = IOV_MAX;
#elif defined(MAX_IOVEC)
    const size_t max_chunks = MAX_IOVEC;
#elif defined(UIO_MAXIOV)
    const size_t max_chunks = UIO_MAXIOV;
#elif(defined(__FreeBSD__) && __FreeBSD_version < 500000) \
    || defined(__DragonFly__) || defined(__APPLE__)
    const size_t max_chunks = 1024;
#elif defined(_SC_IOV_MAX)
    const size_t max_chunks = sysconf(_SC_IOV_MAX);
#else
    const unsigned int max_chunks = 1024;
#endif
    ape_socket_jobs_t *job;
#ifndef __WIN32
    struct iovec chunks[max_chunks];
#else
    struct iovec *chunks    = _alloca(sizeof(struct iovec) * max_chunks);
#endif
    job = (ape_socket_jobs_t *)socket->jobs.head;

    while (job != NULL && (job->pool.flags & APE_SOCKET_JOB_ACTIVE)) {
        switch (job->pool.flags
                & ~(APE_POOL_ALL_FLAGS | APE_SOCKET_JOB_ACTIVE)) {
            case APE_SOCKET_JOB_WRITEV: {
                unsigned i;
                ssize_t n;
                ape_pool_list_t *plist = (ape_pool_list_t *)job->pool.ptr.data;
                ape_socket_packet_t *packet
                    = (ape_socket_packet_t *)plist->head;
#ifdef _HAVE_SSL_SUPPORT
                if (APE_SOCKET_ISSECURE(socket)) {
                    ERR_clear_error();

                    while (packet != NULL && packet->pool.ptr.data != NULL) {

                        if ((n = ape_ssl_write(socket->SSL.ssl,
                                               packet->pool.ptr.data,
                                               packet->len))
                            == -1) {

                            unsigned long err
                                = SSL_get_error(socket->SSL.ssl->con, n);
                            switch (err) {
                                case SSL_ERROR_ZERO_RETURN:
                                    break;
                                case SSL_ERROR_WANT_WRITE:
                                case SSL_ERROR_WANT_READ:
                                    socket->SSL.need_write = 1;
                                    socket->states.flags
                                        |= APE_SOCKET_WOULD_BLOCK;
                                    return 0;
                                default:
                                    APE_socket_shutdown_now(socket);
                                    return 0;
                            }
                        }
                        ape_socket_release_data(packet->pool.ptr.data,
                                                packet->data_type);
                        packet->pool.ptr.data = NULL;
                        packet = (ape_socket_packet_t *)ape_pool_head_to_queue(
                            plist);
                    }
                } else {
#endif
                chunk:
                    for (i = 0; packet != NULL && i < max_chunks; i++) {
                        if (packet->pool.ptr.data == NULL) {
                            break;
                        }

                        chunks[i].iov_base
                            = (char *)packet->pool.ptr.data + packet->offset;
                        chunks[i].iov_len = packet->len - packet->offset;

                        packet = (ape_socket_packet_t *)packet->pool.next;
                    }

                rewrite:
                    if ((n = writev(socket->s.fd, chunks, i)) == -1) {
                        if (errno == EAGAIN) {
                            socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                            return 0;
                        } else if (errno == EINTR) {
                            goto rewrite;
                        } else {
                            APE_ERROR("libapenetwork", "[Socket] writev() IO error: disconnect\n");
                            APE_socket_shutdown_now(socket);
                            return -1;
                        }
                    }
                    socket->ape->total_memory_buffered -= n;
                    socket->current_buffer_memory_bytes -= n;

                    packet = (ape_socket_packet_t *)plist->head;

                    while (packet != NULL && packet->pool.ptr.data != NULL) {
                        n -= packet->len - packet->offset;

                        /* packet not finished */
                        if (n < 0) {
                            packet->offset = packet->len + n;
                            break;
                        }
                        ape_socket_release_data(packet->pool.ptr.data,
                                                packet->data_type);
                        packet->pool.ptr.data = NULL;

                        packet = (ape_socket_packet_t *)ape_pool_head_to_queue(
                            plist);

                        if (n == 0) {
                            break;
                        }
                    }

                    /* job not finished */
                    if (packet->pool.ptr.data != NULL) {
                        goto chunk;
                    }
#ifdef _HAVE_SSL_SUPPORT
                }
#endif
                break;
            }
#ifndef __WIN32
            case APE_SOCKET_JOB_SENDFILE: {
                off_t nwrite = 4096;
#if (defined(__APPLE__) || defined(__FREEBSD__))
                while (sendfile(job->pool.ptr.fd, socket->s.fd, job->offset,
                                &nwrite, NULL, 0)
                           == 0
                       && nwrite != 0) {
                    job->offset += nwrite;
                    nwrite = 4096;
                }
                if (nwrite != 0) {
                    nwrite = -1;
                }
#else
                do {
                    nwrite
                        = sendfile(socket->s.fd, job->pool.ptr.fd, NULL, 4096);
                } while (nwrite > 0);
#endif
                /* Job not finished */
                if (nwrite == -1 && errno == EAGAIN) {
                    socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                    return 0;
                }
/* Job finished */
#ifndef __WIN32
                close(job->pool.ptr.fd);
#else
                _close(job->pool.ptr.fd);
#endif
                job->offset        = 0;
                job->pool.ptr.data = NULL;

                break;
            }
#endif

            case APE_SOCKET_JOB_SHUTDOWN:

                ape_shutdown(socket, SHUT_RDWR);

                return -1;
            default:
                break;
        }

        job->pool.flags &= ~APE_SOCKET_JOB_ACTIVE;
        job = (ape_socket_jobs_t *)ape_pool_head_to_current(&socket->jobs);

        njobsdone++;
    }

    return njobsdone;
}

static int ape_socket_queue_data(ape_socket *socket, unsigned char *data,
                                 size_t len, size_t offset,
                                 ape_socket_data_autorelease data_type)
{
    ape_socket_jobs_t *job;
    ape_socket_packet_t *packets;
    ape_pool_list_t *list;

    /* if the data is a local scoped data, copy it */
    data_type = (data_type == APE_DATA_STATIC ? APE_DATA_COPY : data_type);

    if (data_type == APE_DATA_COPY) {
        unsigned char *data_copy = malloc(len);
        memcpy(data_copy, data, len);
        data = data_copy;
    }

    job  = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_WRITEV);
    list = job->pool.ptr.data;

    if (list == NULL) {
        list               = ape_socket_new_packet_queue(8);
        job->pool.ptr.data = list;
    }
    packets = (ape_socket_packet_t *)list->current;

    packets->pool.ptr.data = data;
    packets->len           = len;
    packets->offset        = offset;
    packets->data_type     = data_type;

    /* Always have spare slots */
    if (packets->pool.next == NULL) {
        ape_grow_pool(list, 8);
    }

    list->current = packets->pool.next;

    socket->current_buffer_memory_bytes += len - offset;
    socket->ape->total_memory_buffered += len - offset;

    /* Check whether we're exceeding the buffer max memory */
    if (socket->max_buffer_memory_mb != 0
        && socket->current_buffer_memory_bytes
               > (socket->max_buffer_memory_mb * 1024ULL * 1024ULL)) {

        APE_ERROR("libapenetwork", "[Socket] Maximum buffer size reached for socket %d\n",
                socket->s.fd);
        APE_socket_shutdown_now(socket);
    }

    return 0;
}

#if 0
static int ape_socket_queue_buffer(ape_socket *socket, buffer *b)
{
    return ape_socket_queue_data(socket, b->data, b->used, 0);
}
#endif

int ape_socket_connected(void *arg)
{
    ape_socket *socket = (ape_socket *)arg;

#ifdef _HAVE_SSL_SUPPORT
    if (APE_SOCKET_ISSECURE(socket)) {
        socket->SSL.ssl
            = ape_ssl_init_con(socket->ape->ssl_global_ctx, socket->s.fd, 0);
    }
#endif

    if (socket->callbacks.on_connected != NULL) {
        socket->callbacks.on_connected(socket, socket->ape,
                                       socket->callbacks.arg);
    }

    return 0;
}

int ape_socket_accept(ape_socket *socket)
{
    int fd, sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in their_addr;
    ape_socket *client;

    while (1) { /* walk through backlog */
        fd = accept(socket->s.fd, (struct sockaddr *)&their_addr,
                    (unsigned int *)&sin_size);

        if (fd == -1) {
            if (errno == EINTR) continue;
            /* TODO : case ECONNABORTED */
            break;
        }

        client = APE_socket_new(socket->states.proto, fd, socket->ape);

        memcpy(&client->sockaddr, &their_addr, sizeof(struct sockaddr_in));

        /* clients inherits server callbacks */
        client->callbacks = socket->callbacks;
        client->parent    = socket;

        client->states.state = APE_SOCKET_ST_ONLINE;
        client->states.type  = APE_SOCKET_TP_CLIENT;
#ifdef _HAVE_SSL_SUPPORT
        if (APE_SOCKET_ISSECURE(socket)) {
            client->SSL.ssl
                = ape_ssl_init_con(socket->SSL.ssl, client->s.fd, 1);
        }
#endif
        events_add((ape_event_descriptor *)client, EVENT_READ | EVENT_WRITE,
                   socket->ape);

        if (socket->callbacks.on_connect != NULL) {
            socket->callbacks.on_connect(socket, client, socket->ape,
                                         socket->callbacks.arg);
        }
    }

    return 0;
}

int ape_socket_read_udp(ape_socket *server)
{
/* UDP packet can't exceed this size */
#define MAX_PACKET_SIZE_UDP 65535
    struct sockaddr_in address;
    socklen_t address_len = sizeof(struct sockaddr_in);

    char buff[MAX_PACKET_SIZE_UDP];
    int rlen = 0;

    while ((rlen = recvfrom(server->s.fd, buff, MAX_PACKET_SIZE_UDP, 0,
                            (struct sockaddr *)&address, &address_len))
           > 0) {

        if (rlen < 1) {
            return 0;
        }

        buff[rlen] = '\0';

        if (server->callbacks.on_message != NULL) {
            server->callbacks.on_message(server, server->ape,
                                         (unsigned char *)buff, rlen, &address,
                                         server->callbacks.arg);
        }
    }
#undef MAX_PACKET_SIZE_UDP

    return 1;
}

int ape_socket_write_udp(ape_socket *from, const char *data, size_t len,
                         const char *ip, uint16_t port)
{
    if (from->states.proto != APE_SOCKET_PT_UDP) {
        APE_WARN("libapenetwork",
            "[Socket] Trying to call sendto from a non UDP socket\n");
        return -1;
    }

    struct sockaddr_in addr;

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    memset(&(addr.sin_zero), '\0', 8);

    int ret = sendto(from->s.fd, data, len, 0, (struct sockaddr *)&addr,
                     sizeof(addr));

    if (ret == -1) {
        APE_WARN("libapenetwork", "[Socket] UDP I/O Error (%d): %s\n", errno,
               strerror(errno));

        if (errno == EAGAIN) {
            APE_DEBUG("libapenetwork", "[Socket] EAGAIN on UDP sendto()\n");
        }
    }

    return ret;
}

static int ape_shutdown(ape_socket *socket, int rw)
{
    if (socket->states.state != APE_SOCKET_ST_ONLINE) {
        return 1;
    }
#ifdef _HAVE_SSL_SUPPORT
    if (APE_SOCKET_ISSECURE(socket)) {
        ape_ssl_shutdown(socket->SSL.ssl);
    }
#endif

    if (socket->states.proto != APE_SOCKET_PT_UDP) {
        shutdown(socket->s.fd, rw);
    }

    socket->states.state = APE_SOCKET_ST_SHUTDOWN;

    if (socket->states.type == APE_SOCKET_TP_SERVER) {
        ape_socket_destroy(socket);
    } else {
        ape_socket_destroy_async(socket);
    }
    return 1;
}

static int ape_socket_read_lz4_stream(ape_socket *socket)
{
    const char *pData = (char *)socket->data_in.data;
    ssize_t pLen      = socket->data_in.used;
    char tmpBuf[APE_LZ4_BLOCK_SIZE];
    int leftOver            = 0;
    int to_copy_from_socket = 0;

    /*
        We have some leftover data in our data buffer
    */
    if (socket->lz4.rx.buffer.used) {
        to_copy_from_socket
            = ape_min(socket->data_in.used,
                      socket->lz4.rx.buffer.size - socket->lz4.rx.buffer.used);

        memcpy(socket->lz4.rx.buffer.data + socket->lz4.rx.buffer.used,
               socket->data_in.data, to_copy_from_socket);

        socket->lz4.rx.buffer.used += to_copy_from_socket;

        pLen  = socket->lz4.rx.buffer.used;
        pData = socket->lz4.rx.buffer.data;

        /*
            Amount of data remaining in our socket buffer
        */
        leftOver = socket->data_in.used - to_copy_from_socket;
    }

    while (pLen > 0) {

        int buffer_pos = 0;

        /* Read next block size */
        if (socket->lz4.rx.decompress_position < sizeof(int)) {
            buffer_pos = ape_min(
                pLen, sizeof(int) - socket->lz4.rx.decompress_position);

            memcpy(&socket->lz4.rx.current_block_size
                       + socket->lz4.rx.decompress_position,
                   pData, buffer_pos);

            pData += buffer_pos;
        }

        socket->lz4.rx.decompress_position += pLen;

        /* do we have enough data? */
        if (pLen > buffer_pos
            && socket->lz4.rx.current_block_size <= APE_LZ4_BLOCK_COMP_SIZE
            && pLen - buffer_pos >= socket->lz4.rx.current_block_size) {

            int rc = APE_LZ4_decompress_safe_continue(
                socket->lz4.rx.ctx,
                /* src */ pData,
                /* dst */ tmpBuf,
                /* comp size */ socket->lz4.rx.current_block_size,
                /* maxDecompressedSize */ APE_LZ4_BLOCK_SIZE);

            if (rc <= 0) {
                APE_ERROR("libapenetwork", "[Socket] LZ4 Decompression error %d\n", rc);
                return -1;
            }

            if (socket->lz4.rx.dict_buffer.pos + rc
                > APE_LZ4_DICT_BUFFER_SIZE) {
                int availsize
                    = APE_LZ4_DICT_BUFFER_SIZE - socket->lz4.rx.dict_buffer.pos;
                int needsize = rc - availsize;

                memmove(socket->lz4.rx.dict_buffer.data,
                        socket->lz4.rx.dict_buffer.data + needsize,
                        socket->lz4.rx.dict_buffer.pos - needsize);

                memcpy((socket->lz4.rx.dict_buffer.data
                        + socket->lz4.rx.dict_buffer.pos)
                           - needsize,
                       tmpBuf, rc);

                socket->lz4.rx.dict_buffer.pos = APE_LZ4_DICT_BUFFER_SIZE;
            } else {
                memcpy(socket->lz4.rx.dict_buffer.data
                           + socket->lz4.rx.dict_buffer.pos,
                       tmpBuf, rc);
                socket->lz4.rx.dict_buffer.pos += rc;
            }

            APE_LZ4_setStreamDecode(socket->lz4.rx.ctx,
                                    socket->lz4.rx.dict_buffer.data,
                                    socket->lz4.rx.dict_buffer.pos);

            socket->callbacks.on_read(socket, (const unsigned char *)tmpBuf, rc,
                                      socket->ape, socket->callbacks.arg);

            pLen -= socket->lz4.rx.current_block_size + sizeof(int);
            pData += socket->lz4.rx.current_block_size;

            if (leftOver) {
                /*
                    Reset pData into out original socket data
                    to avoid unecessary data copy
                */
                int consumed
                    = socket->lz4.rx.current_block_size
                      - (socket->lz4.rx.buffer.used - to_copy_from_socket);

                pData    = (const char *)socket->data_in.data + consumed;
                pLen     = socket->data_in.used - consumed;
                leftOver = 0;
            }

            if (socket->lz4.rx.buffer.used) {
                socket->lz4.rx.buffer.used = 0;
            }

            socket->lz4.rx.decompress_position = 0;
            socket->lz4.rx.current_block_size  = 0;

        } else if (socket->lz4.rx.current_block_size
                   > APE_LZ4_BLOCK_COMP_SIZE) {
            /* invalid block size received */
            APE_ERROR("libapenetwork", "[Socket] io_error from block size (%d)\n",
                    socket->lz4.rx.current_block_size);
            return -1;
        } else if (pLen - buffer_pos < socket->lz4.rx.current_block_size) {
            memmove(socket->lz4.rx.buffer.data, pData, pLen);
            socket->lz4.rx.buffer.used = pLen - buffer_pos;
            break;
        } /* else if not enough data */
    }


    return 0;
}

/* Consume socket buffer */
int ape_socket_read(ape_socket *socket)
{
    ssize_t nread;
    int io_error = 0;

    if (APE_SOCKET_ISSECURE(socket) && socket->SSL.need_write) {
        socket->SSL.need_write = 0;
        ape_socket_do_jobs(socket);
    }

    if (socket->states.state != APE_SOCKET_ST_ONLINE) {

        return 0;
    }
    do {
        /* TODO : avoid extra calling (avoid realloc) */
        buffer_prepare(&socket->data_in, 2048);
#ifdef _HAVE_SSL_SUPPORT
        if (APE_SOCKET_ISSECURE(socket)) {
            ERR_clear_error();

            nread = ape_ssl_read(socket->SSL.ssl,
                                 socket->data_in.data + socket->data_in.used,
                                 socket->data_in.size - socket->data_in.used);

            if (nread < 0) {
                unsigned long err = SSL_get_error(socket->SSL.ssl->con, nread);

                switch (err) {
                    case SSL_ERROR_ZERO_RETURN:
                        nread = 0;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        break;
                    case SSL_ERROR_WANT_READ:
                        break;
                    default:
                        APE_socket_shutdown_now(socket);
                        return 0;
                }
            }
            socket->data_in.used += ape_max(nread, 0);
        } else {
#endif
        socket_reread:
            nread = read(socket->s.fd,
                         socket->data_in.data + socket->data_in.used,
                         socket->data_in.size - socket->data_in.used);

            if (nread == -1) {
                switch (errno) {
                    case EINTR:
                        goto socket_reread;
                    case EAGAIN:
                        break;
                    case ETIMEDOUT:
                    /* fall through */
                    default:
                        io_error = 1;
                        break;
                }
            }

            socket->data_in.used += ape_max(nread, 0);
#ifdef _HAVE_SSL_SUPPORT
        }
#endif
    } while (nread > 0);

    if (socket->data_in.used != 0) {
        if (socket->callbacks.on_read != NULL
            && socket->states.state != APE_SOCKET_ST_SHUTDOWN) {

            if (APE_SOCKET_IS_LZ4(socket, rx)) {
                if (ape_socket_read_lz4_stream(socket) != 0) {
                    io_error = 1;
                }
            } else {
                socket->callbacks.on_read(socket, socket->data_in.data,
                                          socket->data_in.used, socket->ape,
                                          socket->callbacks.arg);
            }
        }

        socket->data_in.used = 0;
    }

    if ((nread == 0 || io_error)
        && socket->states.state != APE_SOCKET_ST_SHUTDOWN) {
        if (io_error) {
            APE_ERROR("libapenetwork", "[Socket] Socket %d disconnected because of IO error\n",
                    socket->s.fd);
        }
        ape_socket_destroy(socket);

        return -1;
    }

    return socket->data_in.used;
}


static ape_socket_jobs_t *ape_socket_job_get_slot(ape_socket *socket, int type)
{
    ape_socket_jobs_t *jobs = (ape_socket_jobs_t *)socket->jobs.current;

    /* If we request a write job we can push the data to the iov list */
    if ((type == APE_SOCKET_JOB_WRITEV
         && (jobs->pool.flags & APE_SOCKET_JOB_WRITEV))
        || !(jobs->pool.flags & APE_SOCKET_JOB_ACTIVE)) {

        jobs->pool.flags |= APE_SOCKET_JOB_ACTIVE | type;

        return jobs;
    }

    jobs = (ape_socket_jobs_t *)(jobs == (ape_socket_jobs_t *)socket->jobs.queue
                                     ? ape_grow_pool(&socket->jobs, 2)
                                     : jobs->pool.next);

    socket->jobs.current = (ape_pool_t *)jobs;
    jobs->pool.flags |= APE_SOCKET_JOB_ACTIVE | type;

    return jobs;
}

static void ape_init_job_list(ape_pool_list_t *list, size_t n)
{
    ape_init_pool_list(list, sizeof(ape_socket_jobs_t), n);
}

static ape_pool_list_t *ape_socket_new_packet_queue(size_t n)
{
    return ape_new_pool_list(sizeof(ape_socket_packet_t), n);
}

