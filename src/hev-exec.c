/*
 ============================================================================
 Name        : hev-exec.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Exec
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <hev-task-call.h>

#include "hev-conf.h"
#include "hev-misc.h"

#include "hev-exec.h"

static HevTaskCall call;
static const char *mode;
static const char *path;
static char oaddr[INET6_ADDRSTRLEN];
static char iaddr[INET6_ADDRSTRLEN];
static char oport[32];
static char iport[32];
static char ip4p[32];

static void
signal_handler (int signum)
{
    waitpid (-1, NULL, WNOHANG);
}

static void
hev_exec_fork_exec (HevTaskCall *call)
{
    pid_t pid;

    pid = fork ();
    if (pid < 0) {
        LOG (E);
        return;
    } else if (pid != 0) {
        return;
    }

    execl (path, path, oaddr, oport, ip4p, iport, mode, iaddr, NULL);

    LOGV (E, "%s", "Run script failed, Please check is it executable?");
    exit (-1);
}

void
hev_exec_init (void *stack)
{
    call.stack_top = stack;
}

void
hev_exec_run (int family, unsigned int maddr[4], unsigned short mport,
              unsigned int baddr[4], unsigned short bport)
{
    unsigned char *q;
    unsigned char *p;
    const char *fmt;

    path = hev_conf_path ();
    signal (SIGCHLD, signal_handler);

    q = (unsigned char *)maddr;
    p = (unsigned char *)&mport;

    inet_ntop (family, maddr, oaddr, sizeof (oaddr));
    inet_ntop (family, baddr, iaddr, sizeof (iaddr));

    fmt = "%u";
    snprintf (oport, sizeof (oport), fmt, ntohs (mport));
    snprintf (iport, sizeof (iport), fmt, ntohs (bport));

    if (family == AF_INET) {
        fmt = "2001::%02x%02x:%02x%02x:%02x%02x";
        snprintf (ip4p, sizeof (ip4p), fmt, p[0], p[1], q[0], q[1], q[2], q[3]);
    } else {
        ip4p[0] = '\0';
    }

    switch (hev_conf_mode ()) {
    case SOCK_STREAM:
        mode = "tcp";
        break;
    case SOCK_DGRAM:
        mode = "udp";
        break;
    default:
        mode = "";
    }

    if (!path) {
        printf ("%s %s %s %s %s %s\n", oaddr, oport, ip4p, iport, mode, iaddr);
        fflush (stdout);
        return;
    }

#ifdef __MSYS__
    hev_task_call_jump (&call, hev_exec_fork_exec);
#else
    hev_exec_fork_exec (&call);
#endif
}
