/*
 ============================================================================
 Name        : hev-pfwd.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Port Fwd
 ============================================================================
 */

#ifndef __HEV_PFWD_H__
#define __HEV_PFWD_H__

/**
 * hev_pfwd_run:
 * @fd: http file descriptor
 *
 * Run server for TCP port forwarding.
 */
void hev_pfwd_run (int fd);

/**
 * hev_pfwd_kill:
 *
 * Force stop HTTP client.
 */
void hev_pfwd_kill (void);

#endif /* __HEV_PFWD_H__ */
