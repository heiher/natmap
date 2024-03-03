/*
 ============================================================================
 Name        : hev-conf.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 xyz
 Description : Conf
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "hev-misc.h"

#include "hev-conf.h"

static int mode = SOCK_STREAM;
static int type = AF_UNSPEC;
static int keep;
static int dmon;

static char mport[16];
static char sport[16] = "3478";
static char hport[16] = "80";
static char stun[256];
static char http[256];

static const char *path;
static const char *baddr;
static const char *bport;
static const char *taddr;
static const char *tport;
static const char *iface;

const char *
hev_conf_help (void)
{
    const char *help =
        "Usage:\n"
        " natmap [options]\n"
        "\n"
        "Options:\n"
        " -4                use IPv4\n"
        " -6                use IPv6\n"
        " -u                UDP mode\n"
        " -d                run as daemon\n"
        " -i <interface>    network interface\n"
        " -k <interval>     seconds between each keep-alive\n"
        " -s <address>      domain name or address to STUN server\n"
        " -h <address>      domain name or address to HTTP server\n"
        " -e <path>         script path for notify mapped address\n"
        "\n"
        "Bind options:\n"
        " -b <port>         port number for binding\n"
        "\n"
        "Forward options:\n"
        " -t <address>      domain name or address to forward target\n"
        " -p <port>         port number to forward target (0: use public port)\n";

    return help;
}

int
hev_conf_init (int argc, char *argv[])
{
    int opt;

    while ((opt = getopt (argc, argv, "46udk:s:h:e:b:t:p:i:")) != -1) {
        switch (opt) {
        case '4':
            type = AF_INET;
            break;
        case '6':
            type = AF_INET6;
            break;
        case 'u':
            mode = SOCK_DGRAM;
            break;
        case 'd':
            dmon = 1;
            break;
        case 'k':
            keep = strtoul (optarg, NULL, 10) * 1000;
            break;
        case 's':
            sscanf (optarg, "%255[^:]:%5[0123456789]", stun, sport);
            break;
        case 'h':
            sscanf (optarg, "%255[^:]:%5[0123456789]", http, hport);
            break;
        case 'e':
            path = optarg;
            break;
        case 'b':
            bport = optarg;
            break;
        case 't':
            taddr = optarg;
            break;
        case 'p':
            tport = optarg;
            break;
        case 'i':
            iface = optarg;
            break;
        default:
            return -1;
        }
    }

    if (!stun[0]) {
        return -1;
    }

    if ((mode == SOCK_STREAM) && !http[0]) {
        return -1;
    }

    if ((taddr && !tport) || (!taddr && tport)) {
        return -1;
    }

    if (keep <= 0) {
        keep = (mode == SOCK_STREAM) ? 30 : 10;
        keep *= 1000;
    }

    if (!bport) {
        bport = "0";
    }

    baddr = (type == AF_INET6) ? "::" : "0.0.0.0";

    return 0;
}

int
hev_conf_mode (void)
{
    return mode;
}

int
hev_conf_type (void)
{
    return type;
}

int
hev_conf_keep (void)
{
    return keep;
}

const char *
hev_conf_stun (void)
{
    return stun;
}

const char *
hev_conf_http (void)
{
    return http;
}

const char *
hev_conf_path (void)
{
    return path;
}

const char *
hev_conf_baddr (void)
{
    return baddr;
}

const char *
hev_conf_bport (void)
{
    return bport;
}

const char *
hev_conf_taddr (void)
{
    return taddr;
}

const char *
hev_conf_tport (void)
{
    return tport;
}

const char *
hev_conf_mport (int port)
{
    if (port > 0)
        snprintf (mport, sizeof (mport) - 1, "%u", port);

    return mport;
}

const char *
hev_conf_iface (void)
{
    return iface;
}

int
hev_conf_daemon (void)
{
    return dmon;
}

const char *
hev_conf_sport (void)
{
    return sport;
}

const char *
hev_conf_hport (void)
{
    return hport;
}
