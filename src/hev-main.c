/*
 ============================================================================
 Name        : hev-main.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Main
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <hev-task-system.h>

#include "hev-conf.h"
#include "hev-exec.h"
#include "hev-misc.h"
#include "hev-winc.h"
#include "hev-xnsk.h"

#include "hev-main.h"

int
main (int argc, char *argv[])
{
    char stack[HEV_EXEC_STACK_SIZE];
    struct rlimit limit = {
        .rlim_cur = 65536,
        .rlim_max = 65536,
    };
    struct timeval tv;
    int res;

#if defined(__MSYS__)
    res = hev_winc_setup_ctrlc ();
    if (res < 0) {
        LOG (E);
        return -3;
    }
#endif

    res = hev_conf_init (argc, argv);
    if (res < 0) {
        fprintf (stderr, "%s", hev_conf_help ());
#ifdef COMMIT_ID
        fprintf (stderr, "\nVersion: %s\n", COMMIT_ID);
#endif
        return -1;
    }

    res = hev_conf_daemon ();
    if (res) {
        hev_run_daemon ();
    }

    gettimeofday (&tv, 0);
    srand (tv.tv_sec ^ tv.tv_usec);

    signal (SIGPIPE, SIG_IGN);
    setrlimit (RLIMIT_NOFILE, &limit);
    hev_exec_init (&stack[HEV_EXEC_STACK_SIZE]);

    res = hev_task_system_init ();
    if (res < 0) {
        LOG (E);
        return -2;
    }

    hev_xnsk_run ();
    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
