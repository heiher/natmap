/*
 ============================================================================
 Name        : hev-http.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Http
 ============================================================================
 */

#ifndef __HEV_HTTP_H__
#define __HEV_HTTP_H__

/**
 * hev_http_run:
 *
 * Run HTTP client to keep-alive.
 */
void hev_http_run (void);

/**
 * hev_http_kill:
 *
 * Force stop HTTP client.
 */
void hev_http_kill (void);

#endif /* __HEV_HTTP_H__ */
