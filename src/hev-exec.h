/*
 ============================================================================
 Name        : hev-exec.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Exec
 ============================================================================
 */

#ifndef __HEV_EXEC_H__
#define __HEV_EXEC_H__

/**
 * hev_exec_run:
 * @family: network family
 * @addr: mapped addr
 * @port: mapped port
 *
 * Run script to notify the mapped address is changed.
 */
void hev_exec_run (int family, unsigned int addr[4], unsigned short port);

#endif /* __HEV_EXEC_H__ */
