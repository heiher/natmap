/*
 ============================================================================
 Name        : hev-tfwd.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : TCP forwarder
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
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
    HevTask *task = hev_task_self ();
    const char *addr;
    const char *port;
    int timeout;
    int mode;
    int sfd;
    int dfd;

    sfd = (intptr_t)data;
    mode = hev_conf_mode ();
    addr = hev_conf_taddr ();
    port = hev_conf_tport ();
    timeout = hev_conf_tmsec ();

    if (strtoul (port, NULL, 10) == 0)
        port = hev_conf_mport (-1);

    dfd = hev_sock_client_pfwd (mode, addr, port);
    if (dfd < 0) {
        LOG (W);
        close (sfd);
        return;
    }

    hev_task_add_fd (task, sfd, POLLIN | POLLOUT);
    hev_task_io_splice (sfd, sfd, dfd, dfd, 8192, io_yielder, &timeout);

    hev_task_del_fd (task, sfd);
    hev_task_del_fd (task, dfd);
    close (sfd);
    close (dfd);
}

static void
server_task_entry (void *data)
{
    const char *iface;
    int mode;
    int mark;
    int fd;

    mode = hev_conf_mode ();
    mark = hev_conf_mark ();
    iface = hev_conf_iface ();
    fd = hev_sock_server_pfwd (data, mode, iface, mark);
    if (fd < 0) {
        LOGV (E, "%s", "Start TCP forward service failed.");
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
hev_tfwd_run (struct sockaddr *saddr)
{
    if (task) {
        return;
    }

    task = hev_task_new (-1);
    hev_task_run (task, server_task_entry, saddr);
}

void
hev_tfwd_kill (void)
{
    quit = -1;
    if (task) {
        hev_task_wakeup (task);
    }
}
