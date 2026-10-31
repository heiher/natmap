/*
 ============================================================================
 Name        : hev-compat.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2026 xyz
 Description : Compatibility
 ============================================================================
 */

#ifdef _WIN32

#include <stdio.h>
#include <string.h>

#include "hev-compat.h"

char *optarg;
int optind = 1;

int
getopt (int argc, char *const argv[], const char *optstring)
{
    static int pos;
    const char *opt;
    int c;

    if (!pos) {
        if (optind >= argc || argv[optind][0] != '-' || !argv[optind][1])
            return -1;
        if (strcmp (argv[optind], "--") == 0) {
            optind++;
            return -1;
        }
        pos = 1;
    }

    c = argv[optind][pos++];
    opt = strchr (optstring, c);
    if (c == ':' || !opt) {
        fprintf (stderr, "%s: invalid option -- '%c'\n", argv[0], c);
        c = '?';
    } else if (opt[1] == ':') {
        if (argv[optind][pos]) {
            optarg = &argv[optind][pos];
        } else if (optind + 1 < argc) {
            optarg = argv[++optind];
        } else {
            fprintf (stderr, "%s: option requires an argument -- '%c'\n",
                     argv[0], c);
            c = '?';
        }
        pos = strlen (argv[optind]);
    }

    if (!argv[optind][pos]) {
        optind++;
        pos = 0;
    }

    return c;
}

#endif /* _WIN32 */
