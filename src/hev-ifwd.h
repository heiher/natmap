/*
 ============================================================================
 Name        : hev-ifwd.c
 Author      : Mike Wang <mikewang000000@gmail.com>
               hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 xyz
 Description : iptables forwarder
 ============================================================================
 */
#ifndef __HEV_IFWD_H__
#define __HEV_IFWD_H__

/**
 * hev_ifwd_run:
 * @fd: file desc
 *
 * Start iptables port forwarding.
 */
void hev_ifwd_run (int fd);

/**
 * hev_ifwd_kill:
 *
 * Stop iptables port forwarding.
 */
void hev_ifwd_kill (void);

#endif /* __HEV_IFWD_H__ */
