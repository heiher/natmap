/*
 ============================================================================
 Name        : hev-exec.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Exec
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "hev-conf.h"
#include "hev-misc.h"

#include "hev-exec.h"

static void
signal_handler (int signum)
{
    waitpid (-1, NULL, WNOHANG);
}

void
hev_exec_run (int family, unsigned int addr[4], unsigned short port)
{
    unsigned char *q;
    unsigned char *p;
    const char *path;
    const char *fmt;
    char saddr[32];
    char sport[32];
    char ip4p[32];
    pid_t pid;

    signal (SIGCHLD, signal_handler);

    pid = fork ();
    if (pid < 0) {
        LOG (E);
        return;
    } else if (pid != 0) {
        return;
    }

    q = (unsigned char *)addr;
    p = (unsigned char *)&port;

    inet_ntop (family, addr, saddr, sizeof (saddr));

    fmt = "%u";
    snprintf (sport, sizeof (sport), fmt, ntohs (port));

    if (family == AF_INET) {
        fmt = "2001::%02x%02x:%02x%02x:%02x%02x";
        snprintf (ip4p, sizeof (ip4p), fmt, p[0], p[1], q[0], q[1], q[2], q[3]);
    } else {
        ip4p[0] = '\0';
    }

    path = hev_conf_path ();
    execl (path, path, saddr, sport, ip4p, NULL);

    LOG (E);
    exit (-1);
}
