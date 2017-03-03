/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_PORT_WINDOWS_H

#define __APE_PORT_WINDOWS_H

#include <winsock2.h>

#if defined(_WIN64)
#define ssize_t __int64
#else
#define ssize_t int
#endif

#define strncasecmp _strnicmp
#define strcasecmp _stricmp

typedef int socklen_t;
typedef const char sockopt_t;

#define sclose(x) closesocket((x))
#define swrite(fd, data, bytes) send(fd, (char *)data, bytes, 0)
#define sread(fd, buf, len) recv(fd, buf, len, 0)

#define SOCKERRNO ((int)WSAGetLastError())
#define SET_SOCKERRNO(x) (WSASetLastError((int)(x)))

#define _INC_ERRNO

#undef EBADF /* override definition in errno.h */
#define EBADF WSAEBADF
#undef EINTR /* override definition in errno.h */
#define EINTR WSAEINTR
#undef EINVAL /* override definition in errno.h */
#define EINVAL WSAEINVAL
#undef EWOULDBLOCK /* override definition in errno.h */
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EAGAIN /* override definition in errno.h */
#define EAGAIN WSAEWOULDBLOCK
#undef EINPROGRESS /* override definition in errno.h */
#define EINPROGRESS WSAEINPROGRESS
#undef EALREADY /* override definition in errno.h */
#define EALREADY WSAEALREADY
#undef ENOTSOCK /* override definition in errno.h */
#define ENOTSOCK WSAENOTSOCK
#undef EDESTADDRREQ /* override definition in errno.h */
#define EDESTADDRREQ WSAEDESTADDRREQ
#undef EMSGSIZE /* override definition in errno.h */
#define EMSGSIZE WSAEMSGSIZE
#undef EPROTOTYPE /* override definition in errno.h */
#define EPROTOTYPE WSAEPROTOTYPE
#undef ENOPROTOOPT /* override definition in errno.h */
#define ENOPROTOOPT WSAENOPROTOOPT
#undef EPROTONOSUPPORT /* override definition in errno.h */
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#undef EOPNOTSUPP /* override definition in errno.h */
#define EOPNOTSUPP WSAEOPNOTSUPP
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#undef EAFNOSUPPORT /* override definition in errno.h */
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#undef EADDRINUSE /* override definition in errno.h */
#define EADDRINUSE WSAEADDRINUSE
#undef EADDRNOTAVAIL /* override definition in errno.h */
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#undef ENETDOWN /* override definition in errno.h */
#define ENETDOWN WSAENETDOWN
#undef ENETUNREACH /* override definition in errno.h */
#define ENETUNREACH WSAENETUNREACH
#undef ENETRESET /* override definition in errno.h */
#define ENETRESET WSAENETRESET
#undef ECONNABORTED /* override definition in errno.h */
#define ECONNABORTED WSAECONNABORTED
#undef ECONNRESET /* override definition in errno.h */
#define ECONNRESET WSAECONNRESET
#undef ENOBUFS /* override definition in errno.h */
#define ENOBUFS WSAENOBUFS
#undef EISCONN /* override definition in errno.h */
#define EISCONN WSAEISCONN
#undef ENOTCONN /* override definition in errno.h */
#define ENOTCONN WSAENOTCONN
#define ESHUTDOWN WSAESHUTDOWN
#define ETOOMANYREFS WSAETOOMANYREFS
#undef ETIMEDOUT /* override definition in errno.h */
#define ETIMEDOUT WSAETIMEDOUT
#undef ECONNREFUSED /* override definition in errno.h */
#define ECONNREFUSED WSAECONNREFUSED
#undef ELOOP /* override definition in errno.h */
#define ELOOP WSAELOOP
#ifndef ENAMETOOLONG /* possible previous definition in errno.h */
#define ENAMETOOLONG WSAENAMETOOLONG
#endif
#define EHOSTDOWN WSAEHOSTDOWN
#undef EHOSTUNREACH /* override definition in errno.h */
#define EHOSTUNREACH WSAEHOSTUNREACH
#ifndef ENOTEMPTY /* possible previous definition in errno.h */
#define ENOTEMPTY WSAENOTEMPTY
#endif
#define EPROCLIM WSAEPROCLIM
#define EUSERS WSAEUSERS
#define EDQUOT WSAEDQUOT
#define ESTALE WSAESTALE
#define EREMOTE WSAEREMOTE

#endif

