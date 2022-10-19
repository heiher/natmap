/*
 ============================================================================
 Name        : hev-sock.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Sock
 ============================================================================
 */

#ifndef __HEV_SOCK_H__
#define __HEV_SOCK_H__

/**
 * hev_sock_client_tcp:
 * @family: network family
 * @saddr: source addr
 * @sport: source port
 * @daddr: destination addr
 * @dport: destination port
 * @iface: network interface
 *
 * Create a socket for TCP client.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_tcp (int family, const char *saddr, const char *sport,
                         const char *daddr, const char *dport,
                         const char *iface);

/**
 * hev_sock_client_udp:
 * @family: network family
 * @saddr: source addr
 * @sport: source port
 * @iface: network interface
 *
 * Create a socket for UDP client.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_udp (int family, const char *saddr, const char *sport,
                         const char *iface);

/**
 * hev_sock_client_stun:
 * @fd: http socket file descriptor
 * @type: socket type
 * @daddr: destination addr
 * @dport: destination port
 * @iface: network interface
 * @bport: [out] bound port
 *
 * Create a socket for STUN client.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_stun (int fd, int type, const char *daddr,
                          const char *dport, const char *iface, int *bport);

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
 * @fd: http socket file descriptor
 * @type: socket type
 *
 * Create a socket for port forwarding server.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_server_pfwd (int fd, int type);

#endif /* __HEV_SOCK_H__ */
