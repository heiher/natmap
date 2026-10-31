/*
 ============================================================================
 Name        : hev-compat.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2026 xyz
 Description : Compatibility
 ============================================================================
 */

#ifndef __HEV_COMPAT_H__
#define __HEV_COMPAT_H__

#ifndef _WIN32

#include <unistd.h>

#else

extern char *optarg;
extern int optind;

/**
 * getopt:
 * @argc: argument count
 * @argv: argument vector
 * @optstring: option characters, followed by a colon if it takes an argument
 *
 * Parse command-line options, as with the POSIX getopt.
 *
 * Returns: the option character, '?' if unknown or missing argument, or -1
 * if there are no more options.
 */
int getopt (int argc, char *const argv[], const char *optstring);

#endif

#endif /* __HEV_COMPAT_H__ */
