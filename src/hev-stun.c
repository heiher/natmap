/*
 ============================================================================
 Name        : hev-stun.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Stun
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-conf.h"
#include "hev-exec.h"
#include "hev-misc.h"
#include "hev-sock.h"
#include "hev-xnsk.h"

#include "hev-stun.h"

typedef struct _StunMessage StunMessage;
typedef struct _StunAttribute StunAttribute;
typedef struct _StunMappedAddr StunMappedAddr;

enum
{
    IPV4 = 1,
    IPV6 = 2,
};

enum
{
    MAGIC = 0x2112A442,
    MAPPED_ADDR = 0x0001,
    XOR_MAPPED_ADDR = 0x0020,
};

struct _StunMessage
{
    unsigned short type;
    unsigned short size;
    unsigned int magic;
    unsigned int tid[3];
};

struct _StunAttribute
{
    unsigned short type;
    unsigned short size;
};

struct _StunMappedAddr
{
    unsigned char reserved;
    unsigned char family;
    unsigned short port;
    union
    {
        unsigned int addr[0];
        unsigned int ipv4[1];
        unsigned int ipv6[4];
    };
};

static HevTask *task;
static HevStunHandler handler;

static int
cmp_addr (int family, unsigned int maddr[4], unsigned short mport,
          unsigned int baddr[4], unsigned short bport)
{
    static unsigned int pmaddr[4];
    static unsigned int pbaddr[4];
    static unsigned short pmport;
    static unsigned short pbport;
    int res = 0;

    if (pbport != bport) {
        res |= -1;
    }
    pbport = bport;

    if (pmport != mport) {
        res |= -1;
    }
    pmport = mport;

    switch (family) {
    case AF_INET:
        res |= memcmp (pmaddr, maddr, 4);
        memcpy (&pmaddr[0], maddr, 4);
        memset (&pmaddr[1], 0, 12);
        res |= memcmp (pbaddr, baddr, 4);
        memcpy (&pbaddr[0], baddr, 4);
        memset (&pbaddr[1], 0, 12);
        break;
    case AF_INET6:
        res |= memcmp (pmaddr, maddr, 16);
        memcpy (pmaddr, maddr, 16);
        res |= memcmp (pbaddr, baddr, 16);
        memcpy (pbaddr, baddr, 16);
        break;
    }

    return res;
}

static ssize_t
stun_tcp (int fd, StunMessage *msg, void *buf, size_t size)
{
    int timeout = 30000;
    ssize_t len;

    len = hev_task_io_socket_send (fd, msg, sizeof (StunMessage), MSG_WAITALL,
                                   io_yielder, &timeout);
    if (len <= 0) {
        LOGV (E, "%s", "STUN TCP send failed.");
        return -1;
    }

    len = hev_task_io_socket_recv (fd, msg, sizeof (StunMessage), MSG_WAITALL,
                                   io_yielder, &timeout);
    if (len <= 0) {
        LOGV (E, "%s", "STUN TCP recv failed.");
        return -1;
    }

    len = htons (msg->size);
    if ((len <= 0) || (len > size)) {
        LOG (E);
        return -1;
    }

    len = hev_task_io_socket_recv (fd, buf, len, MSG_WAITALL, io_yielder,
                                   &timeout);
    return len;
}

static ssize_t
stun_udp (int fd, StunMessage *msg, void *buf, size_t size)
{
    ssize_t len;
    int i;

    for (i = 0; i < 10; i++) {
        int timeout = 3000;

        len = hev_task_io_socket_send (fd, msg, sizeof (StunMessage), 0,
                                       io_yielder, &timeout);
        if (len <= 0) {
            LOGV (E, "%s", "STUN UDP send failed.");
            return -1;
        }

        len = hev_task_io_socket_recv (fd, buf, size, 0, io_yielder, &timeout);
        if (len > 0) {
            break;
        }
    }

    return len;
}

static void
stun_pack (StunMessage *msg)
{
    msg->type = htons (0x0001);
    msg->size = htons (0x0000);
    msg->magic = htonl (MAGIC);
    msg->tid[0] = rand ();
    msg->tid[1] = rand ();
    msg->tid[2] = rand ();
}

static int
stun_unpack (StunMessage *msg, void *body, size_t len, int pos,
             unsigned int *addr, unsigned short *port)
{
    int family = -1;
    int i;

    for (i = pos; i < len;) {
        StunAttribute *a = (StunAttribute *)(body + i);
        StunMappedAddr *m = (StunMappedAddr *)(a + 1);
        int size;

        size = ntohs (a->size);
        if (size <= 0) {
            LOG (E);
            return -1;
        }

        if (a->type == htons (MAPPED_ADDR)) {
            *port = m->port;
            addr[0] = m->addr[0];
            addr[1] = m->addr[1];
            addr[2] = m->addr[2];
            addr[3] = m->addr[3];
            family = m->family;
            break;
        } else if (a->type == htons (XOR_MAPPED_ADDR)) {
            *port = m->port ^ msg->magic;
            addr[0] = m->addr[0] ^ msg->magic;
            addr[1] = m->addr[1] ^ msg->tid[0];
            addr[2] = m->addr[2] ^ msg->tid[1];
            addr[3] = m->addr[3] ^ msg->tid[2];
            family = m->family;
            break;
        }

        i += sizeof (StunAttribute) + size;
    }

    switch (family) {
    case IPV4:
        family = AF_INET;
        break;
    case IPV6:
        family = AF_INET6;
        break;
    default:
        LOG (E);
        return -1;
    }

    return family;
}

static int
stun_bind (int fd, int mode, unsigned int baddr[4], int bport)
{
    const int bufsize = 2048;
    char buf[bufsize + 32];
    unsigned int maddr[4];
    unsigned short mport;
    StunMessage msg;
    int family = 0;
    int exec;
    int len;
    int pos;

    stun_pack (&msg);

    if (mode == SOCK_STREAM) {
        len = stun_tcp (fd, &msg, buf, bufsize);
        pos = 0;
    } else {
        len = stun_udp (fd, &msg, buf, bufsize);
        pos = sizeof (msg);
    }
    if (len <= 0) {
        LOG (E);
        return -1;
    }

    family = stun_unpack (&msg, buf, len, pos, maddr, &mport);
    if (family < 0) {
        LOG (E);
        return -1;
    }

    handler ();

    exec = cmp_addr (family, maddr, mport, baddr, bport);
    if (exec) {
        hev_exec_run (family, maddr, mport, baddr, bport);
    }

    return 0;
}

static void
task_entry (void *data)
{
    unsigned int baddr[4];
    const char *iface;
    const char *stun;
    int bport;
    int mode;
    int tfd;
    int res;
    int fd;

    tfd = (intptr_t)data;
    mode = hev_conf_mode ();
    stun = hev_conf_stun ();
    iface = hev_conf_iface ();

    fd = hev_sock_client_stun (tfd, mode, stun, "3478", iface, baddr, &bport);
    close (tfd);
    if (fd < 0) {
        LOGV (E, "%s", "Start STUN service failed.");
        hev_xnsk_kill ();
        task = NULL;
        return;
    }

    if (mode == SOCK_STREAM) {
        res = stun_bind (fd, mode, baddr, bport);
        if (res < 0) {
            LOG (E);
            hev_xnsk_kill ();
        }
    } else {
        for (;;) {
            res = stun_bind (fd, mode, baddr, bport);
            if (res < 0) {
                LOG (E);
                hev_xnsk_kill ();
                break;
            }
            hev_task_yield (HEV_TASK_WAITIO);
        }
    }

    close (fd);
    task = NULL;
}

void
hev_stun_run (int fd, HevStunHandler _handler)
{
    if (task) {
        hev_task_wakeup (task);
        return;
    }

    handler = _handler;
    task = hev_task_new (-1);
    fd = hev_task_io_dup (fd);
    hev_task_run (task, task_entry, (void *)(intptr_t)fd);
}
