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

#include "ape_socket.h"
#include "ape_dns.h"
#include "ape_timers_next.h"
#include "ape_ssl.h"
#include <stdint.h>
#include <stdio.h>
#ifndef __WIN32
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

static int ape_socket_free(void *arg);
static int ape_shutdown(ape_socket *socket, int rw);
static int ape_socket_destroy_async(ape_socket *socket);

/*
Use only one syscall (ioctl) if FIONBIO is defined
It behaves the same for socket file descriptor to use
either ioctl(...FIONBIO...) or fcntl(...O_NONBLOCK...)
*/
#ifdef FIONBIO
static __inline int setnonblocking(int fd)
{
    int  ret = 1;

    return ioctl(fd, FIONBIO, &ret);
}
#else
#define setnonblocking(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)
#endif

#ifdef __WIN32
#define read(fd, buf, len) recv(fd, buf, len, 0)
long int writev(int fd, const struct iovec* vector, int count)
{
    DWORD sent;
    int ret = WSASend(fd, (LPWSABUF)vector, count, &sent, 0, 0, 0);

    return sent;
}
#endif

int _nco = 0, _ndec = 0;

#if 0
static ape_socket_jobs_t *ape_socket_new_jobs_queue(size_t n);
#endif
static ape_socket_jobs_t *ape_socket_job_get_slot(ape_socket *socket, int type);
static ape_pool_list_t *ape_socket_new_packet_queue(size_t n);
static int ape_socket_queue_data(ape_socket *socket, unsigned char *data, size_t len, size_t offset, ape_socket_data_autorelease data_type);
static void ape_init_job_list(ape_pool_list_t *list, size_t n);

__inline static void ape_socket_release_data(unsigned char *data, ape_socket_data_autorelease data_type)
{
    switch (data_type) {
        case APE_DATA_AUTORELEASE:
        case APE_DATA_COPY:
            free(data);
            break;
        default:
            break;
    }
}

ape_socket *APE_socket_new(uint8_t pt, int from, ape_global *ape)
{
    int sock = from, proto = SOCK_STREAM;

    ape_socket *ret = NULL;
    
#ifdef __WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSA failed\n");
        return NULL;
    }

    /* TODO WSAClean et al */
#endif
    _nco++;
    proto = (pt == APE_SOCKET_PT_UDP ? SOCK_DGRAM : SOCK_STREAM);

    /* TODO: set IPPROTO_UDP/TCP ? */
    if ((sock == 0 &&
        (sock = socket(AF_INET /* TODO AF_INET6 */, proto, 0)) == -1) ||
        setnonblocking(sock) == -1) {

        printf("[Socket] Cant create socket(%d) : %s\n", errno, strerror(errno));
        return NULL;
    }

    ret             = malloc(sizeof(*ret));
    ret->ape        = ape;
    ret->s.fd       = sock;
    ret->s.type     = APE_SOCKET;
    ret->states.flags   = 0;
    ret->states.type    = APE_SOCKET_TP_UNKNOWN;
    ret->states.state   = APE_SOCKET_ST_PENDING;
    ret->states.proto   = pt;
    ret->ctx            = NULL;
    ret->parent         = NULL;
    ret->dns_state      = NULL;

#ifdef _HAVE_SSL_SUPPORT
    ret->SSL.issecure   = (pt == APE_SOCKET_PT_SSL);
    ret->SSL.ssl        = NULL;
#endif
    ret->callbacks.on_read          = NULL;
    ret->callbacks.on_disconnect    = NULL;
    ret->callbacks.on_connect       = NULL;
    ret->callbacks.on_connected     = NULL;
    ret->callbacks.on_message       = NULL;
    ret->callbacks.on_drain         = NULL;
    ret->callbacks.arg              = NULL;

    ret->remote_port = 0;
    ret->local_port  = 0;

    memset(&ret->sockaddr, 0, sizeof(struct sockaddr_in));

    buffer_init(&ret->data_in);

    ape_init_job_list(&ret->jobs, 2);
    
    //printf("New socket : %d\n", sock);

    return ret;
}

int APE_socket_listen(ape_socket *socket, uint16_t port,
        const char *local_ip, int defer_accept, int reuse_port)
{
    struct sockaddr_in addr;
#ifdef __WIN32
    const char reuse_addr = 1;
#else
    int reuse_addr = 1;
#endif
#ifdef TCP_DEFER_ACCEPT
    int timeout = 2;
#endif
    if (port == 0) {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(local_ip);
    memset(&(addr.sin_zero), '\0', 8);

    setsockopt(socket->s.fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));
#ifdef SO_REUSEPORT
    if (reuse_port && setsockopt(socket->s.fd, SOL_SOCKET, SO_REUSEPORT,
        &reuse_port, sizeof(int)) != 0) {

        printf("[Socket] setsockopt (SO_REUSEPORT) (%s:%u) warning : %s\n", local_ip, port, strerror(errno));
    }
#else
    if (reuse_port) {
        printf("[Socket] SO_REUSEPORT is requested but compiled on an unsuported target\n");
    }
#endif

    if (bind(socket->s.fd,
                (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1 ||
                /* only listen for STREAM socket */
                ((socket->states.proto == APE_SOCKET_PT_TCP ||
                    socket->states.proto == APE_SOCKET_PT_SSL) &&
                listen(socket->s.fd, APE_SOCKET_BACKLOG) == -1)) {
#ifdef __WIN32
        closesocket(socket->s.fd);
#else
        close(socket->s.fd);
#endif
        printf("[Socket] bind(%s:%u) error : %s\n", local_ip, port, strerror(errno));
        return -1;
    }
    if (defer_accept) {
#ifdef TCP_DEFER_ACCEPT
        setsockopt(socket->s.fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout,
                sizeof(int));
#elif SO_ACCEPTFILTER
        {
            accept_filter_arg afa = {"dataready", ""};
            setsockopt(socket->s.fd, SOL_SOCKET, SO_ACCEPTFILTER, &afa, sizeof(afa));
        }
#endif
    }
    socket->states.type = APE_SOCKET_TP_SERVER;
    socket->states.state = APE_SOCKET_ST_ONLINE;
    socket->local_port = port;

    events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, socket->ape);

    return 0;
}

static int ape_socket_connect_ready_to_connect(const char *remote_ip,
        void *arg, int status)
{
    ape_socket *socket = arg;
    struct sockaddr_in addr;
    
    socket->dns_state = NULL;

#ifdef _HAS_ARES_SUPPORT
    if (status != ARES_SUCCESS) {
        ape_socket_destroy(socket);
        return -1;
    }
#endif
    addr.sin_family = AF_INET;
    addr.sin_port = htons(socket->remote_port);
    addr.sin_addr.s_addr = inet_addr(remote_ip);
    memset(&(addr.sin_zero), '\0', 8);
#ifdef __WIN32
  #undef EINPROGRESS
  #undef EWOULDBLOCK
  #undef errno

  #define EINPROGRESS WSAEINPROGRESS
  #define EWOULDBLOCK WSAEWOULDBLOCK
  #define errno  WSAGetLastError()
#endif
    int ntry = 0;
retry_connect:
    if (socket->local_port != 0) {
        struct sockaddr_in addr_loc;
        addr_loc.sin_family = AF_INET;
        addr_loc.sin_port = htons(socket->local_port);
        addr_loc.sin_addr.s_addr = inet_addr("0.0.0.0");
        memset(&(addr_loc.sin_zero), '\0', 8);

        if (bind(socket->s.fd, (struct sockaddr *)&addr_loc, sizeof(addr_loc)) == -1) {
            printf("[Socket] bind() error(%d) on %d : %s\n", errno, socket->s.fd, strerror(errno));
            return -1;
        }
    }

    if (connect(socket->s.fd, (struct sockaddr *)&addr,
                sizeof(struct sockaddr)) == -1 &&
                (errno != EWOULDBLOCK && errno != EINPROGRESS)) {
        printf("[Socket] connect() error(%d) on %d : %s (retry : %d)\n", errno, socket->s.fd, strerror(errno), ntry);

        switch (errno) {
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
    socket->states.state = (socket->states.proto == APE_SOCKET_PT_UDP ?
        APE_SOCKET_ST_ONLINE : APE_SOCKET_ST_PROGRESS);

    events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, socket->ape);

    if (socket->states.proto == APE_SOCKET_PT_UDP) {
        ape_global *ape = socket->ape;
        timer_dispatch_async(ape_socket_connected, socket);
    }

    return 0;
}

int APE_socket_connect(ape_socket *socket, uint16_t port,
        const char *remote_ip_host, uint16_t localport)
{
    if (port == 0) {
        ape_socket_destroy(socket);
        return -1;
    }

    socket->remote_port = port;
    socket->local_port  = localport;
#ifdef _HAS_ARES_SUPPORT
    socket->dns_state = ape_gethostbyname(remote_ip_host,
            ape_socket_connect_ready_to_connect,
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
    ape_global *ape = socket->ape;
    
    if (socket->states.state == APE_SOCKET_ST_SHUTDOWN) {
        return;
    }
    if (socket->states.state == APE_SOCKET_ST_PROGRESS ||
        socket->states.state == APE_SOCKET_ST_PENDING) {

        ape_socket_destroy(socket);
    
        return;
    }
    
    if (socket->states.state != APE_SOCKET_ST_ONLINE) {
        return;
    }
    if (socket->states.flags & APE_SOCKET_WOULD_BLOCK ||
            socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE) {
        ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SHUTDOWN);
        //printf("Shutdown pushed to queue\n");
        return;
    }

    ape_shutdown(socket, 2);
}


/*
    Shutdown the socket without pushing the action to the job list
*/
void APE_socket_shutdown_now(ape_socket *socket)
{
    ape_global *ape = socket->ape;
    if (socket->states.state == APE_SOCKET_ST_SHUTDOWN) {
        return;
    }
    if (socket->states.state == APE_SOCKET_ST_PROGRESS ||
        socket->states.state == APE_SOCKET_ST_PENDING) {

        ape_socket_destroy(socket);
        return;
    }
    if (socket->states.state != APE_SOCKET_ST_ONLINE) {
        return;
    }

    ape_shutdown(socket, 2);
}

static int ape_socket_close(ape_socket *socket)
{
    if (socket == NULL || socket->states.state == APE_SOCKET_ST_OFFLINE)
        return 0;

    ape_global *ape = socket->ape;
    ape_dns_invalidate(socket->dns_state);
    socket->states.state = APE_SOCKET_ST_OFFLINE;

    if (socket->callbacks.on_disconnect != NULL) {
        socket->callbacks.on_disconnect(socket, ape, socket->callbacks.arg);
    }
#ifdef __WIN32
    closesocket(APE_SOCKET_FD(socket));
#else
    close(APE_SOCKET_FD(socket));
#endif

    return 1;
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
    ape_destroy_pool(socket->jobs.head);

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
    if (socket == NULL || socket->states.state == APE_SOCKET_ST_OFFLINE)
        return -1;

    ape_global *ape = socket->ape;

    timer_dispatch_async(_ape_socket_destroy, socket);
}

int ape_socket_destroy(ape_socket *socket)
{
    ape_global *ape;

    if (!ape_socket_close(socket)) {
        return -1;
    }

    ape = socket->ape;
    timer_dispatch_async(ape_socket_free, socket);
    /* TODO: Free any pending job !!! */

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
        printf("Failed to open %s - %s\n", file, strerror(errno));
        return 0;
    }

    if (socket->states.flags & APE_SOCKET_WOULD_BLOCK ||
            socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE) {

        socket->states.flags  |= APE_SOCKET_WOULD_BLOCK;
        job          = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SENDFILE);
        job->ptr.fd  = fd;

        return 1;
    }
#if (defined(__APPLE__) || defined(__FREEBSD__))
    nwrite = 4096;
    while (sendfile(fd, socket->s.fd, offset_file, &nwrite, NULL, 0) == 0 && nwrite != 0) {
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
        socket->states.flags  |= APE_SOCKET_WOULD_BLOCK;
        job          = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SENDFILE);
        job->ptr.fd  = fd;
        /* BSD/OSX require offset */
        job->offset  = offset_file;
    } else {
        //printf("File sent...\n");
        close(fd);
    }
    
    return 1;
}

int APE_socket_writev(ape_socket *socket, const struct iovec *iov, int iovcnt)
{
    if (socket->states.state != APE_SOCKET_ST_ONLINE ||
            iovcnt == 0) {
        return -1;        
    }
#ifdef _HAVE_SSL_SUPPORT
    if (APE_SOCKET_ISSECURE(socket)) {
        /* NOT IMPLEMENTED */
        return -1;
    }
#endif    
    writev(socket->s.fd, iov, iovcnt);
    
    return 0;
}
#endif

int APE_socket_write(ape_socket *socket, void *data,
    size_t len, ape_socket_data_autorelease data_type)
{
#ifdef __WIN32
  #define ssize_t int
#endif
    ssize_t t_bytes = 0, r_bytes = len, n = 0;
    int io_error = 0, rerrno = 0;

    if (socket->states.state != APE_SOCKET_ST_ONLINE ||
            len == 0) {

        ape_socket_release_data(data,
            (data_type == APE_DATA_COPY ? APE_DATA_OWN : data_type));
        return -1;
    }

    if (socket->states.flags & APE_SOCKET_WOULD_BLOCK ||
            socket->jobs.head->flags & APE_SOCKET_JOB_ACTIVE) {
        ape_socket_queue_data(socket, data, len, 0, data_type);
        //printf("Would block %d\n", len);
        return len;
    }
#ifdef _HAVE_SSL_SUPPORT
    if (APE_SOCKET_ISSECURE(socket)) {
        int w;
        //printf("Want write on a secure connection\n");
        
        ERR_clear_error();
        
        if ((w = ape_ssl_write(socket->SSL.ssl, data, len)) == -1) {
                unsigned long err = SSL_get_error(socket->SSL.ssl->con, w);
                switch(err) {
                    case SSL_ERROR_ZERO_RETURN:
                        break;
                    case SSL_ERROR_WANT_WRITE:
                    case SSL_ERROR_WANT_READ:
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
#endif
        while (t_bytes < len) {
#ifdef __WIN32
            if ((n = send(socket->s.fd, data + t_bytes, r_bytes, 0)) < 0) {
#else
            if ((n = write(socket->s.fd, data + t_bytes, r_bytes)) < 0) {
#endif
                if (errno == EAGAIN && r_bytes != 0) {
                    socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                    ape_socket_queue_data(socket, data, len, t_bytes, data_type);
                    return r_bytes;
                } else {
                    io_error = 1;
                    rerrno = errno;
                    break;
                }
            }
            
            t_bytes += n;
            r_bytes -= n;
        }
#ifdef _HAVE_SSL_SUPPORT
    }
#endif
    
    ape_socket_release_data(data,
        (data_type == APE_DATA_COPY ? APE_DATA_OWN : data_type));
    
    if (io_error) {
        printf("IO error (%d) : %s\n", APE_SOCKET_FD(socket), strerror(rerrno));
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

    char *ip = inet_ntoa(socket->sockaddr.sin_addr);

    return ip;
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
#elif (defined(__FreeBSD__) && __FreeBSD_version < 500000) || defined(__DragonFly__) || defined(__APPLE__)
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
    struct iovec *chunks = _alloca(sizeof(struct iovec)*max_chunks);
#endif
    job = (ape_socket_jobs_t *)socket->jobs.head;

    while(job != NULL && job->flags & APE_SOCKET_JOB_ACTIVE) {
        switch(job->flags & ~(APE_POOL_ALL_FLAGS | APE_SOCKET_JOB_ACTIVE)) {
        case APE_SOCKET_JOB_WRITEV:
        {
            int i;
            ssize_t n;
            ape_pool_list_t *plist = (ape_pool_list_t *)job->ptr.data;
            ape_socket_packet_t *packet = (ape_socket_packet_t *)plist->head;
#ifdef _HAVE_SSL_SUPPORT            
            if (APE_SOCKET_ISSECURE(socket)) {
                ERR_clear_error();
                
                while (packet != NULL && packet->pool.ptr.data != NULL) {

                    if ((n = ape_ssl_write(socket->SSL.ssl,
                                packet->pool.ptr.data, packet->len)) == -1) {
                    
                        unsigned long err = SSL_get_error(socket->SSL.ssl->con, n);
                        switch(err) {
                            case SSL_ERROR_ZERO_RETURN:
                                break;
                            case SSL_ERROR_WANT_WRITE:
                            case SSL_ERROR_WANT_READ:
                                socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                                return 0;
                            default:
                                APE_socket_shutdown_now(socket);
                                return 0;
                        }
                    }
                    ape_socket_release_data(packet->pool.ptr.data, packet->data_type);
                    packet->pool.ptr.data = NULL;
                    packet = (ape_socket_packet_t *)ape_pool_head_to_queue(plist);

                }                
            } else {
#endif            
                for (i = 0; packet != NULL && i < max_chunks; i++) {
                    if (packet->pool.ptr.data == NULL) {
                        break;
                    }
                    chunks[i].iov_base = (char *)packet->pool.ptr.data + packet->offset;
                    chunks[i].iov_len  = packet->len - packet->offset;

                    packet = (ape_socket_packet_t *)packet->pool.next;

                }

                /* TODO: loop until EAGAIN? */
                n = writev(socket->s.fd, chunks, i);
                /* ERR */
                /* TODO : Handle this */
                if (n == -1) {
                    socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                    //job = (ape_socket_jobs_t *)job->next; /* useless? */
                    return 0;
                }
                packet = (ape_socket_packet_t *)plist->head;

                while (packet != NULL && packet->pool.ptr.data != NULL) {
                    n -= packet->len - packet->offset;

                    /* packet not finished */
                    if (n < 0) {
                        packet->offset = packet->len + n;
                        break;
                    }
                    ape_socket_release_data(packet->pool.ptr.data, packet->data_type);
                    packet->pool.ptr.data = NULL;

                    packet = (ape_socket_packet_t *)ape_pool_head_to_queue(plist);

                    if (n == 0) {
                        break;
                    }
                }

                /* job not finished */
                if (packet->pool.ptr.data != NULL) {
                    socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                    return 0;
                }
#ifdef _HAVE_SSL_SUPPORT
            }
#endif
            break;
        }
#ifndef __WIN32
        case APE_SOCKET_JOB_SENDFILE:
        {
            off_t nwrite = 4096;
#if (defined(__APPLE__) || defined(__FREEBSD__))
            while (sendfile(job->ptr.fd, socket->s.fd, job->offset, &nwrite, NULL, 0) == 0 && nwrite != 0) {
                job->offset += nwrite;
                nwrite = 4096;
            }
            if (nwrite != 0) {
                nwrite = -1;
            }
#else
            do {
                nwrite = sendfile(socket->s.fd, job->ptr.fd, NULL, 4096);
            } while (nwrite > 0);
#endif
            /* Job not finished */
            if (nwrite == -1 && errno == EAGAIN) {
                socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
                return 0;
            }
            /* Job finished */
#ifndef __WIN32
            close(job->ptr.fd);
#else
            _close(job->ptr.fd);
#endif
            job->offset = 0;
            job->ptr.data = NULL;
            
            break;
        }
#endif
        case APE_SOCKET_JOB_SHUTDOWN:

            ape_shutdown(socket, 2);
            
            return -1;
        default:
            break;
        }

        job->flags &= ~APE_SOCKET_JOB_ACTIVE;
        job = (ape_socket_jobs_t *)ape_pool_head_to_current(&socket->jobs);

        njobsdone++;
    }

    return njobsdone;
}

static int ape_socket_queue_data(ape_socket *socket,
        unsigned char *data, size_t len, size_t offset, ape_socket_data_autorelease data_type)
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

    job = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_WRITEV);
    list = job->ptr.data;

    if (list == NULL) {
        list = ape_socket_new_packet_queue(8);
        job->ptr.data = list;
    }
    packets = (ape_socket_packet_t *)list->current;

    packets->pool.ptr.data = data;
    packets->len       = len;
    packets->offset    = offset;
    packets->data_type = data_type;

    /* Always have spare slots */
    if (packets->pool.next == NULL) {
        ape_grow_pool(list, sizeof(ape_socket_packet_t), 8);
    }

    list->current = packets->pool.next;

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
            printf("Got a ssl client\n");
            socket->SSL.ssl = ape_ssl_init_con(socket->ape->ssl_global_ctx, socket->s.fd, 0);
        }
#endif

    if (socket->callbacks.on_connected != NULL) {
        socket->callbacks.on_connected(socket, socket->ape, socket->callbacks.arg);
    }

    return 0;
}

int ape_socket_accept(ape_socket *socket)
{
    int fd, sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in their_addr;
    ape_socket *client;

    while(1) { /* walk through backlog */
        fd = accept(socket->s.fd,
            (struct sockaddr *)&their_addr,
            (unsigned int *)&sin_size);

        if (fd == -1) {
            if (errno == EINTR) continue;
            /* TODO : case ECONNABORTED */
            break;
        }

        client = APE_socket_new(socket->states.proto, fd, socket->ape);

        memcpy(&client->sockaddr, &their_addr, sizeof(struct sockaddr_in));

        /* clients inherits server callbacks */
        client->callbacks    = socket->callbacks;
        client->parent       = socket;

        client->states.state = APE_SOCKET_ST_ONLINE;
        client->states.type  = APE_SOCKET_TP_CLIENT;
#ifdef _HAVE_SSL_SUPPORT        
        if (APE_SOCKET_ISSECURE(socket)) {
            printf("Got a ssl client\n");
            client->SSL.ssl = ape_ssl_init_con(socket->SSL.ssl, client->s.fd, 1);
        }
#endif
        events_add(client->s.fd, client, EVENT_READ|EVENT_WRITE, socket->ape);

        if (socket->callbacks.on_connect != NULL) {
            socket->callbacks.on_connect(socket, client, socket->ape, socket->callbacks.arg);
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

    char buffer[MAX_PACKET_SIZE_UDP];
    int rlen = 0;

    while ((rlen = recvfrom(server->s.fd, buffer, MAX_PACKET_SIZE_UDP,
        0, (struct sockaddr *)&address, &address_len)) > 0) {

        if (rlen < 1) {
            return 0;
        }

        buffer[rlen] = '\0';

        if (server->callbacks.on_message != NULL) {
            server->callbacks.on_message(server, server->ape,
                (unsigned char *)buffer, rlen, &address, server->callbacks.arg);
        }
    }
#undef MAX_PACKET_SIZE_UDP

    return 1;
}

int ape_socket_write_udp(ape_socket *from, const char *data,
    size_t len, const char *ip, uint16_t port)
{
    if (from->states.proto != APE_SOCKET_PT_UDP) {
        printf("[Socket warning] Trying to call sendto from a non UDP socket\n");
        return -1;
    }

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    memset(&(addr.sin_zero), '\0', 8);

    int ret = sendto(from->s.fd, data, len, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (ret == -1) {
        printf("[Socket warning] UDP I/O Error (%d): %s\n", errno, strerror(errno));

        if (errno == EAGAIN) {
            printf("[Socket] EAGAIN on UDP sendto()\n");
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

/* Consume socket buffer */
int ape_socket_read(ape_socket *socket)
{
    ssize_t nread;

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

                switch(err) {
                    case SSL_ERROR_ZERO_RETURN:
                        nread = 0;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        printf("Want write\n");
                        break;
                    case SSL_ERROR_WANT_READ:
                        break;
                    default:
                        printf("Force shutdown %d\n", socket->s.fd);
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
                switch(errno) {
                    case EINTR:
                        goto socket_reread;
                    default:
                        break;
                }
            }

            socket->data_in.used += ape_max(nread, 0);
#ifdef _HAVE_SSL_SUPPORT
        }
#endif
    } while (nread > 0);

    if (socket->data_in.used != 0) {
        //buffer_append_char(&socket->data_in, '\0');
        if (socket->callbacks.on_read != NULL &&
            socket->states.state != APE_SOCKET_ST_SHUTDOWN) {

            socket->callbacks.on_read(socket, socket->ape, socket->callbacks.arg);
        }

        socket->data_in.used = 0;
    }
    if (nread == 0) {
        ape_socket_destroy(socket);

        return -1;
    }

    return socket->data_in.used;
}


static ape_socket_jobs_t *ape_socket_job_get_slot(ape_socket *socket, int type)
{
    ape_socket_jobs_t *jobs = (ape_socket_jobs_t *)socket->jobs.current;

    /* If we request a write job we can push the data to the iov list */
    if ((type == APE_SOCKET_JOB_WRITEV &&
            jobs->flags & APE_SOCKET_JOB_WRITEV) ||
            !(jobs->flags & APE_SOCKET_JOB_ACTIVE)) {

        jobs->flags |= APE_SOCKET_JOB_ACTIVE | type;
        
        return jobs;
    }

    jobs = (ape_socket_jobs_t *)(jobs == (ape_socket_jobs_t *)socket->jobs.queue ?
        ape_grow_pool(&socket->jobs, sizeof(ape_socket_jobs_t), 2) :
        jobs->next);

    socket->jobs.current = (ape_pool_t *)jobs;
    jobs->flags |= APE_SOCKET_JOB_ACTIVE | type;

    return jobs;
}

static void ape_init_job_list(ape_pool_list_t *list, size_t n)
{
    ape_init_pool_list(list, sizeof(ape_socket_jobs_t), n);
}

#if 0
static ape_socket_jobs_t *ape_socket_new_jobs_queue(size_t n)
{
    return (ape_socket_jobs_t *)ape_new_pool(sizeof(ape_socket_jobs_t), n);
}
#endif

static ape_pool_list_t *ape_socket_new_packet_queue(size_t n)
{
    return ape_new_pool_list(sizeof(ape_socket_packet_t), n);
}

// vim: ts=4 sts=4 sw=4 et

