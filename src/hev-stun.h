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

/**
 * hev_stun_run:
 * @fd: http file descriptor
 *
 * Run STUN client to get mapped address.
 */
void hev_stun_run (int fd);

#endif /* __HEV_STUN_H__ */
