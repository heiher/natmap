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

static int sfd = -1;
static int bport;

static int
cmp_addr (int family, unsigned int *maddr, unsigned short mport,
          unsigned short bport)
{
    static unsigned int pmaddr[4];
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
        break;
    case AF_INET6:
        res |= memcmp (pmaddr, maddr, 16);
        memcpy (pmaddr, maddr, 16);
        break;
    }

    return res;
}

static int
stun_bind (int fd, int mode, int bport)
{
    const int bufsize = 2048;
    char buf[bufsize + 32];
    unsigned int *maddr;
    unsigned short mport;
    int timeout = 30000;
    StunMessage msg;
    int family = 0;
    int flags = 0;
    int exec;
    int len;
    int pos;
    int i;

    msg.type = htons (0x0001);
    msg.size = htons (0x0000);
    msg.magic = htonl (MAGIC);
    msg.tid[0] = rand ();
    msg.tid[1] = rand ();
    msg.tid[2] = rand ();

    len = hev_task_io_socket_send (fd, &msg, sizeof (msg), MSG_WAITALL,
                                   io_yielder, &timeout);
    if (len <= 0) {
        LOG (E);
        return -1;
    }

    if (mode == SOCK_STREAM) {
        len = hev_task_io_socket_recv (fd, &msg, sizeof (msg), MSG_WAITALL,
                                       io_yielder, &timeout);
        if (len <= 0) {
            LOG (E);
            return -1;
        }

        len = htons (msg.size);
        if ((len <= 0) || (len > bufsize)) {
            LOG (E);
            return -1;
        }
        flags = MSG_WAITALL;
        pos = 0;
    } else {
        pos = sizeof (msg);
        len = bufsize;
    }

    len = hev_task_io_socket_recv (fd, buf, len, flags, io_yielder, &timeout);
    if (len <= 0) {
        LOG (E);
        return -1;
    }

    for (i = pos; i < len;) {
        StunAttribute *a = (StunAttribute *)&buf[i];
        StunMappedAddr *m = (StunMappedAddr *)(a + 1);
        int size;

        size = ntohs (a->size);
        if (size <= 0) {
            LOG (E);
            return -1;
        }

        if (a->type == htons (MAPPED_ADDR)) {
            family = m->family;
            mport = m->port;
            maddr = m->addr;
            break;
        } else if (a->type == htons (XOR_MAPPED_ADDR)) {
            family = m->family;
            mport = m->port;
            maddr = m->addr;
            mport ^= msg.magic;
            maddr[0] ^= msg.magic;
            maddr[1] ^= msg.tid[0];
            maddr[2] ^= msg.tid[1];
            maddr[3] ^= msg.tid[2];
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

    exec = cmp_addr (family, maddr, mport, bport);

    if (exec) {
        hev_exec_run (family, maddr, mport, bport);
    }

    return 0;
}

static void
task_entry (void *data)
{
    HevTask *task;
    int mode;
    int res;

    task = hev_task_self ();
    mode = hev_conf_mode ();

    if (sfd < 0) {
        int fd = (intptr_t)data;
        const char *iface;
        const char *stun;

        stun = hev_conf_stun ();
        iface = hev_conf_iface ();

        sfd = hev_sock_client_stun (fd, mode, stun, "3478", iface, &bport);
        if (sfd < 0) {
            LOG (E);
            hev_xnsk_kill ();
            return;
        }
    } else {
        hev_task_add_fd (task, sfd, POLLIN | POLLOUT);
    }

    res = stun_bind (sfd, mode, bport);
    if (res < 0) {
        LOG (E);
        close (sfd);
        hev_xnsk_kill ();
        return;
    }

    if (mode == SOCK_STREAM) {
        close (sfd);
        sfd = -1;
    } else {
        hev_task_del_fd (task, sfd);
    }
}

void
hev_stun_run (int fd)
{
    HevTask *task;

    task = hev_task_new (-1);
    hev_task_run (task, task_entry, (void *)(intptr_t)fd);
}
