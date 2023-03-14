/*
 ============================================================================
 Name        : hev-tfwd.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : TCP forwarder
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-conf.h"
#include "hev-misc.h"
#include "hev-sock.h"
#include "hev-xnsk.h"

#include "hev-tfwd.h"

static HevTask *task;
static int quit;

static int
yielder (HevTaskYieldType type, void *data)
{
    hev_task_yield (type);

    return quit;
}

static void
client_task_entry (void *data)
{
    int timeout = 120000;
    const char *addr;
    const char *port;
    int mode;
    int sfd;
    int dfd;

    sfd = (intptr_t)data;
    mode = hev_conf_mode ();
    addr = hev_conf_taddr ();
    port = hev_conf_tport ();

    dfd = hev_sock_client_pfwd (mode, addr, port);
    if (dfd < 0) {
        LOG (W);
        close (sfd);
        return;
    }

    hev_task_add_fd (hev_task_self (), sfd, POLLIN | POLLOUT);
    hev_task_io_splice (sfd, sfd, dfd, dfd, 8192, io_yielder, &timeout);

    close (sfd);
    close (dfd);
}

static void
server_task_entry (void *data)
{
    int fd = (intptr_t)data;
    int mode;

    mode = hev_conf_mode ();
    fd = hev_sock_server_pfwd (fd, mode);
    if (fd < 0) {
        LOG (E);
        hev_xnsk_kill ();
        task = NULL;
        return;
    }

    quit = 0;
    for (;;) {
        HevTask *task;
        int nfd;

        nfd = hev_task_io_socket_accept (fd, NULL, NULL, yielder, NULL);
        if (nfd < 0) {
            if (nfd != -2) {
                LOG (E);
            }
            break;
        }

        task = hev_task_new (-1);
        hev_task_run (task, client_task_entry, (void *)(intptr_t)nfd);
    }

    close (fd);
    task = NULL;
}

void
hev_tfwd_run (int fd)
{
    if (task) {
        return;
    }

    task = hev_task_new (-1);
    hev_task_run (task, server_task_entry, (void *)(intptr_t)fd);
}

void
hev_tfwd_kill (void)
{
    quit = -1;
    if (task) {
        hev_task_wakeup (task);
    }
}
