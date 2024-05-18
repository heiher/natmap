/*
 ============================================================================
 Name        : hev-conf.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Conf
 ============================================================================
 */

#ifndef __HEV_CONF_H__
#define __HEV_CONF_H__

/**
 * hev_conf_help:
 *
 * Get help message of config.
 *
 * Returns: returns help message string.
 */
const char *hev_conf_help (void);

/**
 * hev_conf_init:
 * @argc: argument count
 * @argv: argument vector
 *
 * Initialize config with arguments.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_conf_init (int argc, char *argv[]);

/**
 * hev_conf_mode:
 *
 * Get transmission protocol mode.
 *
 * Returns: returns SOCK_STREAM or SOCK_DGRAM.
 */
int hev_conf_mode (void);

/**
 * hev_conf_type:
 *
 * Get network family type.
 *
 * Returns: returns AF_INET or AF_INET6.
 */
int hev_conf_type (void);

/**
 * hev_conf_keep:
 *
 * Get keep-alive interval in millseconds.
 *
 * Returns: returns integer number.
 */
int hev_conf_keep (void);

/**
 * hev_conf_stun:
 *
 * Get STUN server address.
 *
 * Returns: returns string.
 */
const char *hev_conf_stun (void);

/**
 * hev_conf_http:
 *
 * Get HTTP server address.
 *
 * Returns: returns string.
 */
const char *hev_conf_http (void);

/**
 * hev_conf_path:
 *
 * Get script path that invoked on port mapping created.
 *
 * Returns: returns string.
 */
const char *hev_conf_path (void);

/**
 * hev_conf_mark:
 *
 * Get the fwmark value.
 *
 * Returns: returns string.
 */
unsigned int hev_conf_mark (void);

/**
 * hev_conf_baddr:
 *
 * Get bind address for port mapping.
 *
 * Returns: returns string.
 */
const char *hev_conf_baddr (void);

/**
 * hev_conf_bport:
 *
 * Get bind port for port mapping.
 *
 * Returns: returns string.
 */
const char *hev_conf_bport (void);

/**
 * hev_conf_taddr:
 *
 * Get target address for port forwarding.
 *
 * Returns: returns string.
 */
const char *hev_conf_taddr (void);

/**
 * hev_conf_tport:
 *
 * Get target port for port forwarding.
 *
 * Returns: returns string.
 */
const char *hev_conf_tport (void);

/**
 * hev_conf_tmsec:
 *
 * Get timeout millseconds for port forwarding.
 *
 * Returns: returns integer number.
 */
int hev_conf_tmsec (void);

/**
 * hev_conf_mport:
 * @port: port number
 *
 * Get/Set public port of port mapping.
 *
 * Returns: returns string.
 */
const char *hev_conf_mport (int port);

/**
 * hev_conf_iface:
 *
 * Get network interface name for binding as source.
 *
 * Returns: returns string.
 */
const char *hev_conf_iface (void);

/**
 * hev_conf_daemon:
 *
 * Get is run as daemon.
 *
 * Returns: returns non-zero on true, otherwise returns zero.
 */
int hev_conf_daemon (void);

/**
 * hev_conf_sport:
 *
 * Get port of STUN server.
 *
 * Returns: returns string.
 */
const char *hev_conf_sport (void);

/**
 * hev_conf_hport:
 *
 * Get port of HTTP server.
 *
 * Returns: returns string.
 */
const char *hev_conf_hport (void);

#endif /* __HEV_CONF_H__ */
