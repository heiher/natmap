/*
 ============================================================================
 Name        : hev-sock.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Sock
 ============================================================================
 */

#ifndef __HEV_SOCK_H__
#define __HEV_SOCK_H__

/**
 * hev_sock_client_base:
 * @family: network family
 * @type: socket type
 * @saddr: source addr
 * @sport: source port
 * @daddr: destination addr
 * @dport: destination port
 * @iface: network interface
 * @mark: fwmark
 * @baddr: [out] socket bind addr
 * @paddr: [out] socket peer addr
 *
 * Create a socket for TCP and UDP client.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_base (int family, int type, const char *saddr,
                          const char *sport, const char *daddr,
                          const char *dport, const char *iface,
                          unsigned int mark, struct sockaddr_storage *baddr,
                          struct sockaddr_storage *paddr);

/**
 * hev_sock_client_stun:
 * @saddr: source addr
 * @type: socket type
 * @daddr: destination addr
 * @dport: destination port
 * @iface: network interface
 * @mark: fwmark
 * @baddr: [out] bound addr
 * @bport: [out] bound port
 *
 * Create a socket for STUN client.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_stun (struct sockaddr *saddr, int type, const char *daddr,
                          const char *dport, const char *iface,
                          unsigned int mark, unsigned int baddr[4], int *bport);

/**
 * hev_sock_client_pfwd:
 * @type: socket type
 * @addr: destination addr
 * @port: destination port
 *
 * Create a socket for port forwarding client.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_pfwd (int type, const char *addr, const char *port);

/**
 * hev_sock_server_pfwd:
 * @saddr: source addr
 * @type: socket type
 * @iface: network interface
 * @mark: fwmark
 *
 * Create a socket for port forwarding server.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_server_pfwd (struct sockaddr *saddr, int type, const char *iface,
                          unsigned int mark);

#endif /* __HEV_SOCK_H__ */
