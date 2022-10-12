/*
 ============================================================================
 Name        : hev-http.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Http
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-conf.h"
#include "hev-pfwd.h"
#include "hev-misc.h"
#include "hev-sock.h"
#include "hev-stun.h"

#include "hev-http.h"

static HevTask *task;
static int timeout;

static void
http_keep_alive (int fd, const char *http)
{
    static char buffer[8192];
    struct msghdr mh = { 0 };
    struct iovec iov[3];
    int misscnt = 0;

    mh.msg_iov = iov;
    mh.msg_iovlen = 3;

    iov[0].iov_base = "GET /~ HTTP/1.1\r\nHost: ";
    iov[0].iov_len = strlen (iov[0].iov_base);
    iov[1].iov_base = (void *)http;
    iov[1].iov_len = strlen (iov[1].iov_base);
    iov[2].iov_base = "\r\nConnection: keep-alive\r\n\r\n";
    iov[2].iov_len = strlen (iov[2].iov_base);

    for (;;) {
        int res;

        timeout = 30000;
        res = hev_task_io_socket_sendmsg (fd, &mh, MSG_WAITALL, io_yielder,
                                          &timeout);
        if (res <= 0) {
            return;
        }

        timeout = hev_conf_keep ();
        for (;;) {
            res = hev_task_io_socket_recv (fd, buffer, sizeof (buffer), 0,
                                           io_yielder, &timeout);
            if ((res == -2) && (misscnt++ == 0) && timeout) {
                break;
            } else if (res <= 0) {
                return;
            } else {
                misscnt = 0;
            }
        }
    }
}

static void
http_run (void)
{
    const char *http;
    const char *pfwd;
    const char *addr;
    const char *port;
    const char *iface;
    int type;
    int fd;

    type = hev_conf_type ();
    http = hev_conf_http ();
    pfwd = hev_conf_taddr ();
    addr = hev_conf_baddr ();
    port = hev_conf_bport ();
    iface = hev_conf_iface ();

    fd = hev_sock_client_http (type, addr, port, http, "80", iface);
    if (fd < 0) {
        LOG (E);
        return;
    }

    hev_stun_run (fd);
    if (pfwd) {
        hev_pfwd_run (fd);
    }

    timeout = 1;
    http_keep_alive (fd, http);

    if (pfwd) {
        hev_pfwd_kill ();
    }
    close (fd);
}

static void
task_entry (void *data)
{
    for (;;) {
        http_run ();
        hev_task_sleep (5000);
    }
}

void
hev_http_run (void)
{
    task = hev_task_new (-1);
    hev_task_run (task, task_entry, NULL);
}

void
hev_http_kill (void)
{
    timeout = 0;
    if (task) {
        hev_task_wakeup (task);
    }
}
