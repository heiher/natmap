/*
 ============================================================================
 Name        : hev-misc.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Misc
 ============================================================================
 */

#ifndef __HEV_MISC_H__
#define __HEV_MISC_H__

#include <hev-task.h>
#include <hev-task-io.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))

#define LOG(T) \
    fprintf (stderr, "[" #T "] %s %s:%d\n", __FUNCTION__, __FILE__, __LINE__);

#define LOGV(T, F, ...)                                                   \
    fprintf (stderr, "[" #T "] %s %s:%d " F "\n", __FUNCTION__, __FILE__, \
             __LINE__, __VA_ARGS__);

#ifndef container_of
#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *)(ptr);                 \
        ((type *)(__mptr - offsetof (type, member))); \
    })
#endif

#define io_yielder hev_io_yielder

/**
 * hev_io_yielder:
 * @type: yield type
 * @data: data
 *
 * The task io yielder.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_io_yielder (HevTaskYieldType type, void *data);

/**
 * hev_reuse_port:
 * @port: port number
 *
 * Set reuse port flag for remote socket file descriptor in other process.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_reuse_port (const char *port);

/**
 * hev_tcp_cca:
 * @algo: congestion control algorithm
 *
 * Set the TCP congestion control algorithm.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_tcp_cca (int fd, const char *algo);

/**
 * hev_run_daemon:
 *
 * Run as daemon.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_run_daemon (void);

#endif /* __HEV_MISC_H__ */
