/*
 ============================================================================
 Name        : hev-xnsk.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : NAT session keeper
 ============================================================================
 */

#include "hev-xnsk.h"

static void (*killer) (void);

void
hev_xnsk_init (void *kill)
{
    killer = kill;
}

void
hev_xnsk_kill (void)
{
    killer ();
}
