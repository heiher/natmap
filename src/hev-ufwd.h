/*
 ============================================================================
 Name        : hev-ufwd.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : UDP forwarder
 ============================================================================
 */

#ifndef __HEV_UFWD_H__
#define __HEV_UFWD_H__

/**
 * hev_ufwd_run:
 * @fd: socket file descriptor
 *
 * Run server for UDP port forwarding.
 */
void hev_ufwd_run (int fd);

/**
 * hev_ufwd_kill:
 *
 * Force stop UDP port forwarding server.
 */
void hev_ufwd_kill (void);

#endif /* __HEV_UFWD_H__ */
