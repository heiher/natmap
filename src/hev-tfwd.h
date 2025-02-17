/*
 ============================================================================
 Name        : hev-tfwd.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : TCP forwarder
 ============================================================================
 */

#ifndef __HEV_TFWD_H__
#define __HEV_TFWD_H__

/**
 * hev_tfwd_run:
 * @saddr: source addr
 *
 * Run server for TCP port forwarding.
 */
void hev_tfwd_run (struct sockaddr *saddr);

/**
 * hev_tfwd_kill:
 *
 * Force stop TCP port forwarding server.
 */
void hev_tfwd_kill (void);

#endif /* __HEV_TFWD_H__ */
