/*
 ============================================================================
 Name        : hev-stun.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Stun
 ============================================================================
 */

#ifndef __HEV_STUN_H__
#define __HEV_STUN_H__

typedef void (*HevStunHandler) (void);
typedef struct _HevStunHandlerGroup HevStunHandlerGroup;

struct _HevStunHandlerGroup
{
    HevStunHandler ready;
    HevStunHandler done;
};

/**
 * hev_stun_run:
 * @fd: socket file descriptor
 * @handlers: callbacks for stun ready / done
 *
 * Run STUN client to get mapped address.
 */
void hev_stun_run (int fd, HevStunHandlerGroup *handlers);

#endif /* __HEV_STUN_H__ */
