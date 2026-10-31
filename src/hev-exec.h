/*
 ============================================================================
 Name        : hev-exec.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Exec
 ============================================================================
 */

#ifndef __HEV_EXEC_H__
#define __HEV_EXEC_H__

/**
 * hev_exec_run:
 * @family: network family
 * @maddr: mapped addr
 * @mport: mapped port
 * @baddr: bound addr
 * @bport: bound port
 *
 * Run script to notify the mapped address is changed.
 */
void hev_exec_run (int family, unsigned int maddr[4], unsigned short mport,
                   unsigned int baddr[4], unsigned short bport);

#endif /* __HEV_EXEC_H__ */
