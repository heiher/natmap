/*
 ============================================================================
 Name        : hev-sock.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Sock
 ============================================================================
 */

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

static struct addrinfo *
get_addr (int family, const char *addr, const char *port, int passive)
{
    struct addrinfo *result = NULL;
    struct addrinfo hints = { 0 };
    int res;

    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
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
    int protocol;
    int res = 0;
    int fd;

    family = ai->ai_family;
    socktype = ai->ai_socktype;
    protocol = ai->ai_protocol;

    fd = hev_task_io_socket_socket (family, socktype, protocol);
    if (fd < 0) {
        LOG (E);
        return -1;
    }

    res |= setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (int));
    res |= setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof (int));
    if (res < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

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

int
hev_sock_client_http (int family, const char *saddr, const char *sport,
                      const char *daddr, const char *dport, const char *iface)
{
    struct addrinfo *sai;
    struct addrinfo *dai;
    int timeout = 30000;
    int res;
    int fd;

    sai = get_addr (family, saddr, sport, 0);
    if (!sai) {
        LOG (E);
        return -1;
    }

    dai = get_addr (sai->ai_family, daddr, dport, 0);
    if (!dai) {
        LOG (E);
        freeaddrinfo (sai);
        return -1;
    }

    fd = get_sock (sai);
    if (fd < 0) {
        LOG (E);
        freeaddrinfo (sai);
        freeaddrinfo (dai);
        return -1;
    }

    res = bind_iface (fd, sai->ai_family, iface);
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
            LOG (E);
            freeaddrinfo (sai);
            freeaddrinfo (dai);
            close (fd);
            return -1;
        }
    }
    freeaddrinfo (sai);

    hev_task_add_fd (hev_task_self (), fd, POLLIN | POLLOUT);

    res = hev_task_io_socket_connect (fd, dai->ai_addr, dai->ai_addrlen,
                                      io_yielder, &timeout);
    freeaddrinfo (dai);
    if (res < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

    return fd;
}

int
hev_sock_client_stun (int fd, const char *daddr, const char *dport, int *bport)
{
    struct addrinfo sai;
    struct addrinfo *dai;
    struct sockaddr_storage saddr;
    socklen_t saddrlen = sizeof (saddr);
    int timeout = 30000;
    int res;

    res = getsockname (fd, (struct sockaddr *)&saddr, &saddrlen);
    if (res < 0) {
        LOG (E);
        return -1;
    }

    sai.ai_family = saddr.ss_family;
    sai.ai_socktype = SOCK_STREAM;
    sai.ai_protocol = IPPROTO_TCP;

    dai = get_addr (sai.ai_family, daddr, dport, 0);
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

    res = bind (fd, (struct sockaddr *)&saddr, saddrlen);
    if (res < 0) {
        LOG (E);
        freeaddrinfo (dai);
        close (fd);
        return -1;
    }

    if (saddr.ss_family == AF_INET) {
        *bport = ((struct sockaddr_in *)&saddr)->sin_port;
    } else if (saddr.ss_family == AF_INET6) {
        *bport = ((struct sockaddr_in6 *)&saddr)->sin6_port;
    }

    hev_task_add_fd (hev_task_self (), fd, POLLIN | POLLOUT);

    res = hev_task_io_socket_connect (fd, dai->ai_addr, dai->ai_addrlen,
                                      io_yielder, &timeout);
    freeaddrinfo (dai);
    if (res < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

    return fd;
}

int
hev_sock_client_pfwd (const char *addr, const char *port)
{
    struct addrinfo *ai;
    int timeout = 30000;
    int res;
    int fd;

    ai = get_addr (AF_UNSPEC, addr, port, 0);
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

    hev_task_add_fd (hev_task_self (), fd, POLLIN | POLLOUT);

    res = hev_task_io_socket_connect (fd, ai->ai_addr, ai->ai_addrlen,
                                      io_yielder, &timeout);
    freeaddrinfo (ai);
    if (res < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

    return fd;
}

int
hev_sock_server_pfwd (int fd)
{
    struct addrinfo ai;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof (addr);
    int res;

    res = getsockname (fd, (struct sockaddr *)&addr, &addrlen);
    if (res < 0) {
        LOG (E);
        return -1;
    }

    ai.ai_family = addr.ss_family;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = IPPROTO_TCP;

    fd = get_sock (&ai);
    if (fd < 0) {
        LOG (E);
        return -1;
    }

    res |= bind (fd, (struct sockaddr *)&addr, addrlen);
    res |= listen (fd, 5);
    if (res < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

    hev_task_add_fd (hev_task_self (), fd, POLLIN);

    return fd;
}
