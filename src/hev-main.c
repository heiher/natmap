/*
 ============================================================================
 Name        : hev-main.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Main
 ============================================================================
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>

#include <hev-task-system.h>

#include "hev-conf.h"
#include "hev-http.h"

#include "hev-main.h"

int
main (int argc, char *argv[])
{
    struct rlimit limit = {
        .rlim_cur = 65536,
        .rlim_max = 65536,
    };
    int res;

    res = hev_conf_init (argc, argv);
    if (res < 0) {
        fprintf (stderr, "%s", hev_conf_help ());
#ifdef COMMIT_ID
        printf ("\nVersion: %s\n", COMMIT_ID);
#endif
        return -1;
    }

    res = hev_conf_daemon ();
    if (res) {
        if (daemon (1, 1)) {
        }
    }

    signal (SIGPIPE, SIG_IGN);
    setrlimit (RLIMIT_NOFILE, &limit);

    hev_task_system_init ();

    hev_http_run ();
    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
