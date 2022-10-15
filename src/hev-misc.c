/*
 ============================================================================
 Name        : hev-misc.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Misc
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include "hev-misc.h"

#if defined(__mips__)
#if defined(_LP64)
#define NR_Linux 5000
#else
#define NR_Linux 4000
#endif
#else
#define NR_Linux 0
#endif

#if !defined(SYS_pidfd_open)
#define SYS_pidfd_open (NR_Linux + 434)
#endif

#if !defined(SYS_pidfd_getfd)
#define SYS_pidfd_getfd (NR_Linux + 438)
#endif

#if defined(__linux__)
#define run_syscall syscall
#else
static long
run_syscall (long n, ...)
{
    return -1;
}
#endif

int
hev_io_yielder (HevTaskYieldType type, void *data)
{
    const int timeout = *(int *)data;

    if (type == HEV_TASK_YIELD) {
        hev_task_yield (HEV_TASK_YIELD);
        return 0;
    }

    if (timeout < 0) {
        hev_task_yield (HEV_TASK_WAITIO);
    } else {
        if (hev_task_sleep (timeout) <= 0) {
            return -1;
        }
    }

    return 0;
}

static unsigned long
get_inode (int port, int family)
{
    const char *paths[] = {
        "/proc/net/tcp",
        "/proc/net/tcp6",
    };
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *fp;
    int i;

    i = (family == AF_INET6) ? 1 : 0;

    fp = fopen (paths[i], "r");
    if (!fp) {
        return 0;
    }

    nread = getline (&line, &len, fp);
    if (nread < 0) {
        fclose (fp);
        return 0;
    }

    while ((nread = getline (&line, &len, fp)) != -1) {
        int res, local_port, rem_port, d, state, uid, timer_run, timeout;
        unsigned long rxq, txq, time_len, retr, inode;
        char rem_addr[128], local_addr[128];

        res = sscanf (line,
                      "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X "
                      "%X %lX:%lX %X:%lX %lX %d %d %lu %*s\n",
                      &d, local_addr, &local_port, rem_addr, &rem_port, &state,
                      &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout,
                      &inode);
        if ((res >= 14) && (state == 10) && (local_port == port)) {
            fclose (fp);
            return inode;
        }
    }

    fclose (fp);

    return 0;
}

static int
get_pid_fd (unsigned long inode, pid_t *pid, int *fd)
{
    struct dirent *dpe;
    char match[256];
    DIR *dp;

    dp = opendir ("/proc");
    if (!dp) {
        return -1;
    }

    snprintf (match, sizeof (match) - 1, "socket:[%lu]", inode);

    while ((dpe = readdir (dp))) {
        char path[1024];
        struct dirent *dfe;
        DIR *df;

        if (dpe->d_type != DT_DIR) {
            continue;
        }

        snprintf (path, sizeof (path) - 1, "/proc/%s/fd", dpe->d_name);
        df = opendir (path);
        if (!df) {
            continue;
        }

        while ((dfe = readdir (df))) {
            char name[256];
            int len;

            if (dfe->d_type != DT_LNK) {
                continue;
            }

            snprintf (path, sizeof (path) - 1, "/proc/%s/fd/%s", dpe->d_name,
                      dfe->d_name);
            len = readlink (path, name, sizeof (name) - 1);
            if (len < 0) {
                continue;
            }

            name[len] = '\0';
            if (strcmp (name, match) == 0) {
                closedir (df);
                closedir (dp);
                *fd = strtoul (dfe->d_name, NULL, 10);
                *pid = strtoul (dpe->d_name, NULL, 10);
                return 0;
            }
        }

        closedir (df);
    }

    closedir (dp);

    return -1;
}

static int
set_reuse_port (pid_t pid, int fd)
{
    const int reuse = 1;
    int pfd;
    int sfd;

    pfd = run_syscall (SYS_pidfd_open, pid, 0);
    if (pfd < 0) {
        LOG (E);
        return -1;
    }

    sfd = run_syscall (SYS_pidfd_getfd, pfd, fd, 0);
    if (sfd < 0) {
        LOG (E);
        close (pfd);
        return -1;
    }

    setsockopt (sfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof (int));

    close (sfd);
    close (pfd);
    return 0;
}

int
hev_reuse_port (const char *port)
{
    int types[] = { AF_INET, AF_INET6 };
    int result = 0;
    int p;
    int i;

    p = strtoul (port, NULL, 10);

    for (i = 0; i < ARRAY_SIZE (types); i++) {
        unsigned long inode;

        inode = get_inode (p, types[i]);
        if (inode > 0) {
            pid_t pid;
            int res;
            int sfd;

            res = get_pid_fd (inode, &pid, &sfd);
            if (res == 0) {
                result |= set_reuse_port (pid, sfd);
            }
        }
    }

    return result;
}

int
hev_run_daemon (void)
{
    switch (fork ()) {
    case -1:
        return -1;
    case 0:
        break;
    default:
        exit (0);
    }

    if (setsid () < 0) {
        return -1;
    }

    return 0;
}
