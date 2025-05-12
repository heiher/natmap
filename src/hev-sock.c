/*
 ============================================================================
 Name        : hev-sock.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Sock
 ============================================================================
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>

#include <hev-task.h>
#include <hev-task-dns.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-misc.h"

#include "hev-sock.h"

typedef struct _SockConnectCtx SockConnectCtx;

struct _SockConnectCtx
{
    int retries;
    void *data;
};

static int
connect_io_yielder (HevTaskYieldType type, void *data)
{
    SockConnectCtx *ctx = data;

    ctx->retries++;
    if (ctx->retries >= 100) {
        errno = EADDRNOTAVAIL;
        return -1;
    }

    return io_yielder (type, ctx->data);
}

static int
sock_connect (int fd, const struct sockaddr *addr, socklen_t addr_len,
              SockConnectCtx *ctx)
{
    int res;

    ctx->retries = 0;
    res = hev_task_io_socket_connect (fd, addr, addr_len, connect_io_yielder,
                                      ctx);
    if (res < 0) {
        if (errno == EADDRNOTAVAIL) {
            LOGV (E, "%s",
                  "Cannot assign requested address, "
                  "Please check is another instance exists or wait a minute. "
                  "More: https://github.com/heiher/natmap/issues/27");
        } else {
            LOGV (E, "%s", strerror (errno));
        }
    }

    return res;
}

static struct addrinfo *
get_addr (int family, int type, const char *addr, const char *port, int passive)
{
    struct addrinfo *result = NULL;
    struct addrinfo hints = { 0 };
    int res;

    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_flags = passive ? AI_PASSIVE : 0;

    res = hev_task_dns_getaddrinfo (addr, port, &hints, &result);
    if (res < 0) {
        LOG (E);
        return NULL;
    }

    return result;
}

static int
get_sock (struct addrinfo *ai)
{
    const int reuse = 1;
    int family;
    int socktype;
    int res;
    int fd;

    family = ai->ai_family;
    socktype = ai->ai_socktype;

    fd = hev_task_io_socket_socket (family, socktype, 0);
    if (fd < 0) {
        LOG (E);
        return -1;
    }

    res = fcntl (fd, F_SETFD, fcntl (fd, F_GETFD) | FD_CLOEXEC);
    if (res < 0) {
        LOG (W);
    }

    setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (int));
    setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof (int));

    return fd;
}

static int
bind_iface (int fd, int family, const char *iface)
{
    if (!iface) {
        return 0;
    }

#if defined(__linux__)
    struct ifreq ifr = { 0 };

    strncpy (ifr.ifr_name, iface, sizeof (ifr.ifr_name) - 1);
    return setsockopt (fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr));
#elif defined(__APPLE__) || defined(__MACH__)
    int i;

    i = if_nametoindex (iface);
    if (i == 0) {
        return -1;
    }

    switch (family) {
    case AF_INET:
        return setsockopt (fd, IPPROTO_IP, IP_BOUND_IF, &i, sizeof (i));
    case AF_INET6:
        return setsockopt (fd, IPPROTO_IPV6, IPV6_BOUND_IF, &i, sizeof (i));
    }
#endif

    return -1;
}

static int
bind_fwmark (int fd, unsigned int mark)
{
#if defined(__linux__)
    return setsockopt (fd, SOL_SOCKET, SO_MARK, &mark, sizeof (mark));
#elif defined(__FreeBSD__)
    return setsockopt (fd, SOL_SOCKET, SO_USER_COOKIE, &mark, sizeof (mark));
#endif
    return 0;
}

int
hev_sock_client_base (int family, int type, const char *saddr,
                      const char *sport, const char *daddr, const char *dport,
                      const char *iface, unsigned int mark,
                      struct sockaddr_storage *baddr,
                      struct sockaddr_storage *paddr)
{
    HevTask *task = hev_task_self ();
    struct addrinfo *sai;
    struct addrinfo *dai;
    int timeout = 30000;
    SockConnectCtx ctx;
    socklen_t addrlen;
    int res;
    int fd;

    sai = get_addr (family, type, saddr, sport, 0);
    if (!sai) {
        LOG (E);
        return -1;
    }

    dai = get_addr (sai->ai_family, type, daddr, dport, 0);
    if (!dai) {
        LOG (E);
        freeaddrinfo (sai);
        return -1;
    }

    if (paddr) {
        memcpy (paddr, dai->ai_addr, dai->ai_addrlen);
    }

    fd = get_sock (sai);
    if (fd < 0) {
        LOG (E);
        freeaddrinfo (sai);
        freeaddrinfo (dai);
        return -1;
    }

    res = bind_iface (fd, sai->ai_family, iface);
    if (mark) {
        res |= bind_fwmark (fd, mark);
    }
    if (res < 0) {
        LOG (E);
        freeaddrinfo (sai);
        freeaddrinfo (dai);
        close (fd);
        return -1;
    }

    res = bind (fd, sai->ai_addr, sai->ai_addrlen);
    if (res < 0) {
        hev_reuse_port (sport);
        res = bind (fd, sai->ai_addr, sai->ai_addrlen);
        if (res < 0) {
            LOGV (E, "%s", strerror (errno));
            freeaddrinfo (sai);
            freeaddrinfo (dai);
            close (fd);
            return -1;
        }
    }
    freeaddrinfo (sai);

    hev_task_add_fd (task, fd, POLLIN | POLLOUT);

    ctx.data = &timeout;
    res = sock_connect (fd, dai->ai_addr, dai->ai_addrlen, &ctx);
    freeaddrinfo (dai);
    if (res < 0) {
        hev_task_del_fd (task, fd);
        close (fd);
        return -1;
    }

    addrlen = sizeof (struct sockaddr_storage);
    res = getsockname (fd, (struct sockaddr *)baddr, &addrlen);
    if (res < 0) {
        LOG (E);
        hev_task_del_fd (task, fd);
        close (fd);
        return -1;
    }

    return fd;
}

int
hev_sock_client_stun (struct sockaddr *saddr, int type, const char *daddr,
                      const char *dport, const char *iface, unsigned int mark,
                      unsigned int baddr[4], int *bport)
{
    HevTask *task = hev_task_self ();
    struct sockaddr_storage taddr;
    struct addrinfo sai, *dai;
    int timeout = 30000;
    SockConnectCtx ctx;
    socklen_t addrlen;
    int res;
    int fd;

    switch (saddr->sa_family) {
    case AF_INET:
        addrlen = sizeof (struct sockaddr_in);
        break;
    case AF_INET6:
        addrlen = sizeof (struct sockaddr_in6);
        break;
    }

    sai.ai_family = saddr->sa_family;
    sai.ai_socktype = type;

    dai = get_addr (sai.ai_family, sai.ai_socktype, daddr, dport, 0);
    if (!dai) {
        LOG (E);
        return -1;
    }

    fd = get_sock (&sai);
    if (fd < 0) {
        LOG (E);
        freeaddrinfo (dai);
        return -1;
    }

    res = bind_iface (fd, sai.ai_family, iface);
    if (mark) {
        res |= bind_fwmark (fd, mark);
    }
    if (res < 0) {
        LOG (E);
        freeaddrinfo (dai);
        close (fd);
        return -1;
    }

    res = bind (fd, saddr, addrlen);
    if (res < 0) {
        LOG (E);
        freeaddrinfo (dai);
        close (fd);
        return -1;
    }

    hev_task_add_fd (task, fd, POLLIN | POLLOUT);

    ctx.data = &timeout;
    res = sock_connect (fd, dai->ai_addr, dai->ai_addrlen, &ctx);
    freeaddrinfo (dai);
    if (res < 0) {
        hev_task_del_fd (task, fd);
        close (fd);
        return -1;
    }

    res = getsockname (fd, (struct sockaddr *)&taddr, &addrlen);
    if (res < 0) {
        LOG (E);
        hev_task_del_fd (task, fd);
        close (fd);
        return -1;
    }

    if (taddr.ss_family == AF_INET) {
        struct sockaddr_in *pa = (struct sockaddr_in *)&taddr;
        memcpy (baddr, &pa->sin_addr, 4);
        *bport = pa->sin_port;
    } else if (taddr.ss_family == AF_INET6) {
        struct sockaddr_in6 *pa = (struct sockaddr_in6 *)&taddr;
        memcpy (baddr, &pa->sin6_addr, 16);
        *bport = pa->sin6_port;
    }

    return fd;
}

int
hev_sock_client_pfwd (int type, const char *addr, const char *port)
{
    HevTask *task = hev_task_self ();
    struct addrinfo *ai;
    int timeout = 30000;
    int res;
    int fd;

    ai = get_addr (AF_UNSPEC, type, addr, port, 0);
    if (!ai) {
        LOG (E);
        return -1;
    }

    fd = get_sock (ai);
    if (fd < 0) {
        LOG (E);
        freeaddrinfo (ai);
        return -1;
    }

    hev_task_add_fd (task, fd, POLLIN | POLLOUT);

    res = hev_task_io_socket_connect (fd, ai->ai_addr, ai->ai_addrlen,
                                      io_yielder, &timeout);
    freeaddrinfo (ai);
    if (res < 0) {
        LOG (E);
        hev_task_del_fd (task, fd);
        close (fd);
        return -1;
    }

    return fd;
}

int
hev_sock_server_pfwd (struct sockaddr *saddr, int type, const char *iface,
                      unsigned int mark)
{
    struct sockaddr_storage baddr;
    struct addrinfo ai;
    socklen_t addrlen;
    int res;
    int fd;

    switch (saddr->sa_family) {
    case AF_INET:
        addrlen = sizeof (struct sockaddr_in);
        break;
    case AF_INET6:
    default:
        addrlen = sizeof (struct sockaddr_in6);
        break;
    }

    memcpy (&baddr, saddr, addrlen);
    ai.ai_family = baddr.ss_family;
    ai.ai_socktype = type;

    fd = get_sock (&ai);
    if (fd < 0) {
        LOG (E);
        return -1;
    }

#ifdef __MSYS__
    if (type == SOCK_DGRAM) {
        if (baddr.ss_family == AF_INET) {
            struct sockaddr_in *pa = (struct sockaddr_in *)&baddr;
            memset (&pa->sin_addr, 0, 4);
        } else if (baddr.ss_family == AF_INET6) {
            struct sockaddr_in6 *pa = (struct sockaddr_in6 *)&baddr;
            memset (&pa->sin6_addr, 0, 16);
        }
    }
#endif

    res = bind (fd, (struct sockaddr *)&baddr, addrlen);
    res |= bind_iface (fd, baddr.ss_family, iface);
    if (mark) {
        res |= bind_fwmark (fd, mark);
    }
    if (type == SOCK_STREAM) {
        res |= listen (fd, 5);
    }
    if (res < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

    hev_task_add_fd (hev_task_self (), fd, POLLIN);

    return fd;
}
