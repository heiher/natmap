/*
 ============================================================================
 Name        : hev-xnsk.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : NAT session keeper
 ============================================================================
 */

#include <stdio.h>
#include <sys/socket.h>

#include "hev-conf.h"
#include "hev-misc.h"
#include "hev-tnsk.h"
#include "hev-unsk.h"

#include "hev-xnsk.h"

static void (*killer) (void);

void
hev_xnsk_init (void *kill)
{
    killer = kill;
}

void
hev_xnsk_run (void)
{
    int mode = hev_conf_mode ();

    switch (mode) {
    case SOCK_STREAM:
        hev_tnsk_run ();
        break;
    case SOCK_DGRAM:
        hev_unsk_run ();
        break;
    default:
        LOG (E);
    }
}

void
hev_xnsk_kill (void)
{
    killer ();
}
