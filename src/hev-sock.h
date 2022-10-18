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
 * hev_sock_client_http:
 * @family: network family
 * @saddr: source addr
 * @sport: source port
 * @daddr: destination addr
 * @dport: destination port
 * @iface: network interface
 *
 * Create a socket for client HTTP.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_http (int family, const char *saddr, const char *sport,
                          const char *daddr, const char *dport,
                          const char *iface);

/**
 * hev_sock_client_stun:
 * @fd: http socket file descriptor
 * @daddr: destination addr
 * @dport: destination port
 * @iface: network interface
 * @bport: [out] bound port
 *
 * Create a socket for client STUN.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_stun (int fd, const char *daddr, const char *dport,
                          const char *iface, int *bport);

/**
 * hev_sock_client_pfwd:
 * @addr: destination addr
 * @port: destination port
 *
 * Create a socket for client TCP forwarding.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_client_pfwd (const char *addr, const char *port);

/**
 * hev_sock_server_pfwd:
 * @fd: http socket file descriptor
 *
 * Create a socket for server.
 *
 * Returns: returns file descriptor on successful, otherwise returns -1.
 */
int hev_sock_server_pfwd (int fd);

#endif /* __HEV_SOCK_H__ */
