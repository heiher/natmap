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

#include <hev-sys.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#include "hev-conf.h"
#include "hev-misc.h"

#include "hev-exec.h"

static const char *mode;
static const char *path;
static char oaddr[INET6_ADDRSTRLEN];
static char iaddr[INET6_ADDRSTRLEN];
static char oport[32];
static char iport[32];
static char ip4p[32];

#ifdef _WIN32

static void
hev_exec_spawn (void)
{
    STARTUPINFOA si = { .cb = sizeof (si) };
    PROCESS_INFORMATION pi;
    char cmd[1024];
    int len;

    len = snprintf (cmd, sizeof (cmd), "\"%s\" %s %s %s %s %s %s", path, oaddr,
                    oport, ip4p[0] ? ip4p : "\"\"", iport, mode, iaddr);
    if (len < 0 || len >= sizeof (cmd)) {
        LOGV (E, "%s", "Script command line is too long.");
        return;
    }

    if (!CreateProcessA (NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si,
                         &pi)) {
        LOGV (E, "%s", "Run script failed, Please check is it executable?");
        return;
    }

    CloseHandle (pi.hThread);
    CloseHandle (pi.hProcess);
}

#else

static void
signal_handler (int signum)
{
    waitpid (-1, NULL, WNOHANG);
}

static void
hev_exec_spawn (void)
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

#endif

void
hev_exec_run (int family, unsigned int maddr[4], unsigned short mport,
              unsigned int baddr[4], unsigned short bport)
{
    unsigned char *q;
    unsigned char *p;
    const char *fmt;

    path = hev_conf_path ();
#ifndef _WIN32
    signal (SIGCHLD, signal_handler);
#endif

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

    hev_exec_spawn ();
}
