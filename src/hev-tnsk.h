/*
 ============================================================================
 Name        : hev-tnsk.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : TCP NAT session keeper
 ============================================================================
 */

#ifndef __HEV_TNSK_H__
#define __HEV_TNSK_H__

/**
 * hev_tnsk_run:
 *
 * Run HTTP client to keep-alive.
 */
void hev_tnsk_run (void);

/**
 * hev_tnsk_kill:
 *
 * Force stop HTTP client.
 */
void hev_tnsk_kill (void);

#endif /* __HEV_TNSK_H__ */
