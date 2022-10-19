/*
 ============================================================================
 Name        : hev-xnsk.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : NAT session keeper
 ============================================================================
 */

#ifndef __HEV_XNSK_H__
#define __HEV_XNSK_H__

/**
 * hev_xnsk_init:
 *
 * Initialize keeper.
 */
void hev_xnsk_init (void *kill);

/**
 * hev_xnsk_kill:
 *
 * Force stop keeper.
 */
void hev_xnsk_kill (void);

#endif /* __HEV_XNSK_H__ */
