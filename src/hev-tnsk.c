/*
 ============================================================================
 Name        : hev-tnsk.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : TCP NAT session keeper
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-conf.h"
#include "hev-misc.h"
#include "hev-sock.h"
#include "hev-stun.h"
#include "hev-tfwd.h"
#include "hev-xnsk.h"

#include "hev-tnsk.h"

static struct sockaddr_storage saddr;
static char saved_port[16];
static time_t last_stun_time = 0;
static HevTask *task;
static int timeout;
static int fd;

static void
tnsk_keep_alive (int fd, const char *http)
{
    static char buffer[8192];
    struct msghdr mh = { 0 };
    struct iovec iov[3];
    int misscnt = 0;

    mh.msg_iov = iov;
    mh.msg_iovlen = 3;

    iov[0].iov_base = "HEAD / HTTP/1.1\r\nHost: ";
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
stun_handler (void)
{
    const char *tfwd = hev_conf_taddr ();

    last_stun_time = time (NULL);
    
    if (saddr.ss_family == AF_INET) {
        struct sockaddr_in *ad = (struct sockaddr_in *)&saddr;
        snprintf (saved_port, sizeof (saved_port), "%u", ntohs (ad->sin_port));
    } else {
        struct sockaddr_in6 *ad = (struct sockaddr_in6 *)&saddr;
        snprintf (saved_port, sizeof (saved_port), "%u", ntohs (ad->sin6_port));
    }

    if (tfwd) {
        hev_tfwd_run ((struct sockaddr *)&saddr);
    }
}

static void
tnsk_run (void)
{
    const char *http;
    const char *tfwd;
    const char *addr;
    const char *port;
    const char *hport;
    const char *iface;
    unsigned int mark;
    int twait;
    int type;

    time_t now = time (NULL);

    type = hev_conf_type ();
    http = hev_conf_http ();
    tfwd = hev_conf_taddr ();
    addr = hev_conf_baddr ();
    hport = hev_conf_hport ();
    iface = hev_conf_iface ();
    mark = hev_conf_mark ();
    twait = hev_conf_twait ();

    if (last_stun_time != 0 && (now - last_stun_time <= twait)) {
        port = saved_port;
    } else {
        port = hev_conf_bport ();
    }

    fd = hev_sock_client_base (type, SOCK_STREAM, addr, port, http, hport,
                               iface, mark, &saddr, NULL);
    if (fd < 0) {
        LOGV (E, "%s", "Start TCP keep-alive service failed.");
        return;
    }

    hev_stun_run ((struct sockaddr *)&saddr, stun_handler);

    tnsk_keep_alive (fd, http);

    if (tfwd) {
        hev_tfwd_kill ();
    }
    hev_task_del_fd (hev_task_self (), fd);
    close (fd);
}

static void
task_entry (void *data)
{
    for (;;) {
        tnsk_run ();
        hev_task_sleep (5000);
    }
}

static void
tnsk_kill (void)
{
    timeout = 0;
    if (task) {
        hev_task_wakeup (task);
    }
}

void
hev_tnsk_run (void)
{
    hev_xnsk_init (tnsk_kill);

    task = hev_task_new (-1);
    hev_task_run (task, task_entry, NULL);
}
