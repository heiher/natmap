/*
 ============================================================================
 Name        : hev-ufwd.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : UDP forwarder
 ============================================================================
 */

#ifndef __HEV_UFWD_H__
#define __HEV_UFWD_H__

/**
 * hev_ufwd_run:
 * @saddr: source addr
 *
 * Run server for UDP port forwarding.
 */
void hev_ufwd_run (struct sockaddr *saddr);

/**
 * hev_ufwd_kill:
 *
 * Force stop UDP port forwarding server.
 */
void hev_ufwd_kill (void);

/**
 * hev_ufwd_fd:
 *
 * Get socket fd of UDP port forwarding server.
 */
int hev_ufwd_fd (void);

#endif /* __HEV_UFWD_H__ */
