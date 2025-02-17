/*
 ============================================================================
 Name        : hev-stun.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Stun
 ============================================================================
 */

#ifndef __HEV_STUN_H__
#define __HEV_STUN_H__

typedef void (*HevStunHandler) (void);

/**
 * hev_stun_run:
 * @saddr: socket source address
 * @keep: enable keep-alive (udp only)
 * @handler: callback for stun done
 *
 * Run STUN client to get mapped address.
 */
void hev_stun_run (struct sockaddr *saddr, int keep, HevStunHandler handler);

#endif /* __HEV_STUN_H__ */
