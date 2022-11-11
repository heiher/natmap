/*
 ============================================================================
 Name        : hev-ufwd.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : UDP forwarder
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>
#include <hev-rbtree.h>

#include "hev-conf.h"
#include "hev-misc.h"
#include "hev-sock.h"
#include "hev-xnsk.h"

#include "hev-ufwd.h"

typedef struct _Session Session;

struct _Session
{
    HevRBTreeNode node;
    HevTask *task;
    struct sockaddr_in6 addr;
    socklen_t alen;
    int active;
    int fd;
};

static HevRBTree sessions;
static HevTask *task;
static int quit;
static int sfd;

static int
yielder (HevTaskYieldType type, void *data)
{
    hev_task_yield (type);

    return quit;
}

static Session *
session_find (struct sockaddr *saddr, socklen_t len)
{
    HevRBTreeNode *node = sessions.root;

    while (node) {
        Session *this;
        int res;

        this = container_of (node, Session, node);
        res = memcmp (&this->addr, saddr, len);

        if (res < 0)
            node = node->left;
        else if (res > 0)
            node = node->right;
        else
            return this;
    }

    return NULL;
}

static void
session_add (Session *s)
{
    HevRBTreeNode **new = &sessions.root, *parent = NULL;

    while (*new) {
        Session *this;
        int res;

        this = container_of (*new, Session, node);
        res = memcmp (&this->addr, &s->addr, s->alen);

        parent = *new;
        if (res < 0)
            new = &((*new)->left);
        else if (res > 0)
            new = &((*new)->right);
    }

    hev_rbtree_node_link (&s->node, parent, new);
    hev_rbtree_insert_color (&sessions, &s->node);
}

static void
session_del (Session *s)
{
    hev_rbtree_erase (&sessions, &s->node);
}

static Session *
session_new (struct sockaddr *saddr, socklen_t len)
{
    const char *addr;
    const char *port;
    Session *s;
    int mode;
    int fd;

    mode = hev_conf_mode ();
    addr = hev_conf_taddr ();
    port = hev_conf_tport ();

    fd = hev_sock_client_pfwd (mode, addr, port);
    if (fd < 0) {
        LOG (E);
        return NULL;
    }

    s = hev_malloc0 (sizeof (Session));
    if (!s) {
        LOG (E);
        close (fd);
        return NULL;
    }

    s->fd = fd;
    s->alen = len;
    s->task = hev_task_new (-1);
    memcpy (&s->addr, saddr, len);

    return s;
}

static void
session_free (Session *s)
{
    close (s->fd);
    hev_free (s);
}

static void
client_task_entry (void *data)
{
    const int bufsize = 2048;
    int timeout = 120000;
    struct sockaddr *pa;
    char buf[bufsize];
    Session *s = data;

    pa = (struct sockaddr *)&s->addr;
    hev_task_add_fd (hev_task_self (), s->fd, POLLIN);

    for (;;) {
        int len;

        s->active = 0;
        len = hev_task_io_socket_recvfrom (s->fd, buf, bufsize, 0, NULL, NULL,
                                           io_yielder, &timeout);
        if (len <= 0) {
            if ((len == -2) && s->active) {
                continue;
            }
            break;
        }

        len = sendto (sfd, buf, len, 0, pa, s->alen);
        if (len <= 0) {
            break;
        }
    }

    session_del (s);
    session_free (s);
}

static void
server_task_entry (void *data)
{
    int mode;
    int tfd;

    tfd = (intptr_t)data;
    mode = hev_conf_mode ();

    sfd = hev_sock_server_pfwd (tfd, mode);
    close (tfd);
    if (sfd < 0) {
        LOG (E);
        hev_xnsk_kill ();
        task = NULL;
        return;
    }

    quit = 0;
    for (;;) {
        struct sockaddr_storage addr = { 0 };
        socklen_t alen = sizeof (addr);
        const int bufsize = 2048;
        struct sockaddr *pa;
        char buf[bufsize];
        Session *s;
        int len;

        pa = (struct sockaddr *)&addr;
        len = hev_task_io_socket_recvfrom (sfd, buf, bufsize, 0, pa, &alen,
                                           yielder, NULL);
        if (len < 0) {
            break;
        }

        s = session_find (pa, alen);
        if (!s) {
            s = session_new (pa, alen);
            if (!s) {
                LOG (E);
                continue;
            }
            session_add (s);
            hev_task_del_fd (task, s->fd);
            hev_task_run (s->task, client_task_entry, s);
        }

        send (s->fd, buf, len, 0);
        s->active = 1;
    }

    close (sfd);
    task = NULL;
    sfd = -1;
}

void
hev_ufwd_run (int fd)
{
    task = hev_task_new (-1);
    fd = hev_task_io_dup (fd);
    hev_task_run (task, server_task_entry, (void *)(intptr_t)fd);
}

void
hev_ufwd_kill (void)
{
    quit = -1;
    if (task) {
        hev_task_wakeup (task);
    }
}
