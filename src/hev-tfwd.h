/*
 ============================================================================
 Name        : hev-tfwd.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : TCP forwarder
 ============================================================================
 */

#ifndef __HEV_TFWD_H__
#define __HEV_TFWD_H__

/**
 * hev_tfwd_run:
 * @fd: http file descriptor
 *
 * Run server for TCP port forwarding.
 */
void hev_tfwd_run (int fd);

/**
 * hev_tfwd_kill:
 *
 * Force stop TCP port forwarding server.
 */
void hev_tfwd_kill (void);

#endif /* __HEV_TFWD_H__ */
