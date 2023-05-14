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
#include <sys/socket.h>
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
hev_exec_run (int family, unsigned int maddr[4], unsigned short mport,
              unsigned short bport)
{
    unsigned char *q;
    unsigned char *p;
    const char *mode;
    const char *path;
    const char *fmt;
    char saddr[32];
    char sport[32];
    char lport[32];
    char ip4p[32];
    pid_t pid;

    path = hev_conf_path ();
    signal (SIGCHLD, signal_handler);

    q = (unsigned char *)maddr;
    p = (unsigned char *)&mport;

    inet_ntop (family, maddr, saddr, sizeof (saddr));

    fmt = "%u";
    snprintf (sport, sizeof (sport), fmt, ntohs (mport));
    snprintf (lport, sizeof (lport), fmt, ntohs (bport));

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
        printf ("%s %s %s %s %s\n", saddr, sport, ip4p, lport, mode);
        fflush (stdout);
        return;
    }

    pid = fork ();
    if (pid < 0) {
        LOG (E);
        return;
    } else if (pid != 0) {
        return;
    }

    execl (path, path, saddr, sport, ip4p, lport, mode, NULL);

    LOGV (E, "%s", "Run script failed, Please check is it executable?");
    exit (-1);
}
