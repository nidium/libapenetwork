/*
APE Network Library
Copyright (C) 2014 Anthony Catel <paraboul@gmail.com>

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

#ifndef __APE_PORT_WINDOWS_H && defined(_MSC_VER)
#define __APE_PORT_WINDOWS_H

#if defined(_WIN64)
  #define ssize_t __int64
#else
  #define ssize_t int
#endif

typedef int socklen_t;

#define sclose(x)  closesocket((x))

#define SOCKERRNO         ((int)WSAGetLastError())
#define SET_SOCKERRNO(x)  (WSASetLastError((int)(x)))


#undef  EBADF            /* override definition in errno.h */
#define EBADF            WSAEBADF
#undef  EINTR            /* override definition in errno.h */
#define EINTR            WSAEINTR
#undef  EINVAL           /* override definition in errno.h */
#define EINVAL           WSAEINVAL
#undef  EWOULDBLOCK      /* override definition in errno.h */
#define EWOULDBLOCK      WSAEWOULDBLOCK
#undef  EINPROGRESS      /* override definition in errno.h */
#define EINPROGRESS      WSAEINPROGRESS
#undef  EALREADY         /* override definition in errno.h */
#define EALREADY         WSAEALREADY
#undef  ENOTSOCK         /* override definition in errno.h */
#define ENOTSOCK         WSAENOTSOCK
#undef  EDESTADDRREQ     /* override definition in errno.h */
#define EDESTADDRREQ     WSAEDESTADDRREQ
#undef  EMSGSIZE         /* override definition in errno.h */
#define EMSGSIZE         WSAEMSGSIZE
#undef  EPROTOTYPE       /* override definition in errno.h */
#define EPROTOTYPE       WSAEPROTOTYPE
#undef  ENOPROTOOPT      /* override definition in errno.h */
#define ENOPROTOOPT      WSAENOPROTOOPT
#undef  EPROTONOSUPPORT  /* override definition in errno.h */
#define EPROTONOSUPPORT  WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT  WSAESOCKTNOSUPPORT
#undef  EOPNOTSUPP       /* override definition in errno.h */
#define EOPNOTSUPP       WSAEOPNOTSUPP
#define EPFNOSUPPORT     WSAEPFNOSUPPORT
#undef  EAFNOSUPPORT     /* override definition in errno.h */
#define EAFNOSUPPORT     WSAEAFNOSUPPORT
#undef  EADDRINUSE       /* override definition in errno.h */
#define EADDRINUSE       WSAEADDRINUSE
#undef  EADDRNOTAVAIL    /* override definition in errno.h */
#define EADDRNOTAVAIL    WSAEADDRNOTAVAIL
#undef  ENETDOWN         /* override definition in errno.h */
#define ENETDOWN         WSAENETDOWN
#undef  ENETUNREACH      /* override definition in errno.h */
#define ENETUNREACH      WSAENETUNREACH
#undef  ENETRESET        /* override definition in errno.h */
#define ENETRESET        WSAENETRESET
#undef  ECONNABORTED     /* override definition in errno.h */
#define ECONNABORTED     WSAECONNABORTED
#undef  ECONNRESET       /* override definition in errno.h */
#define ECONNRESET       WSAECONNRESET
#undef  ENOBUFS          /* override definition in errno.h */
#define ENOBUFS          WSAENOBUFS
#undef  EISCONN          /* override definition in errno.h */
#define EISCONN          WSAEISCONN
#undef  ENOTCONN         /* override definition in errno.h */
#define ENOTCONN         WSAENOTCONN
#define ESHUTDOWN        WSAESHUTDOWN
#define ETOOMANYREFS     WSAETOOMANYREFS
#undef  ETIMEDOUT        /* override definition in errno.h */
#define ETIMEDOUT        WSAETIMEDOUT
#undef  ECONNREFUSED     /* override definition in errno.h */
#define ECONNREFUSED     WSAECONNREFUSED
#undef  ELOOP            /* override definition in errno.h */
#define ELOOP            WSAELOOP
#ifndef ENAMETOOLONG     /* possible previous definition in errno.h */
#define ENAMETOOLONG     WSAENAMETOOLONG
#endif
#define EHOSTDOWN        WSAEHOSTDOWN
#undef  EHOSTUNREACH     /* override definition in errno.h */
#define EHOSTUNREACH     WSAEHOSTUNREACH
#ifndef ENOTEMPTY        /* possible previous definition in errno.h */
#define ENOTEMPTY        WSAENOTEMPTY
#endif
#define EPROCLIM         WSAEPROCLIM
#define EUSERS           WSAEUSERS
#define EDQUOT           WSAEDQUOT
#define ESTALE           WSAESTALE
#define EREMOTE          WSAEREMOTE

#endif