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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "hev-misc.h"

#include "hev-conf.h"

static int mode = SOCK_STREAM;
static int type = AF_INET;
static int keep;
static unsigned int ucount;
static int dmon;
static int tmsec;
static int bport[3];
static unsigned int mark;

static char mport[16];
static char sport[16] = "3478";
static char hport[16] = "80";
static char stun[256];
static char http[256];

static const char *path;
static const char *baddr;
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
        " -4                  use IPv4\n"
        " -6                  use IPv6\n"
        " -u                  UDP mode\n"
        " -d                  run as daemon\n"
        " -i <interface>      network interface or IP address\n"
        " -k <interval>       seconds between each keep-alive\n"
        " -c <count>          UDP STUN check cycle (every <count> intervals)\n"
        " -s <addr>[:port]    domain name or address of STUN server\n"
        " -h <addr>[:port]    domain name or address of HTTP server\n"
        " -e <path>           script path for notify mapped address\n"
        " -f <mark>           fwmark value (hex: 0x1, dec: 1, oct: 01)\n"
        "\n"
        "Bind options:\n"
        " -b <port>[-port]    port number range for binding\n"
        "                     - <0>: random allocation\n"
        "                     - <port>: specified\n"
        "                     - <port>-<port>: sequential allocation within the range\n"
        "\n"
        "Forward options:\n"
        " -T <timeout>        port forwarding timeout in seconds\n"
        " -t <address>        domain name or address of forward target\n"
        " -p <port>           port number of forward target (0: use public port)\n";

    return help;
}

int
hev_conf_init (int argc, char *argv[])
{
    int opt;
    struct sockaddr_in6 sa;

    while ((opt = getopt (argc, argv, "46udk:c:s:h:e:f:b:T:t:p:i:")) != -1) {
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
        case 'c':
            ucount = strtoul (optarg, NULL, 10);
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
        case 'f':
            mark = strtoul (optarg, NULL, 0);
            break;
        case 'b':
            sscanf (optarg, "%u-%u", &bport[0], &bport[1]);
            break;
        case 'T':
            tmsec = strtoul (optarg, NULL, 10) * 1000;
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

    if (ucount <= 0) {
        ucount = 10;
    }

    if (!bport[0]) {
        bport[0] = 0;
    }
    if (!bport[1]) {
        bport[1] = bport[0];
    }
    bport[2] = bport[0];

    if (iface && inet_pton (type, iface, &sa)) {
        baddr = iface;
        iface = NULL;
    } else {
        baddr = (type == AF_INET6) ? "::" : "0.0.0.0";
    }

    if (tmsec <= 0) {
        tmsec = 120000;
    }

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

unsigned int
hev_conf_ucount (void)
{
    return ucount;
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

unsigned int
hev_conf_mark (void)
{
    return mark;
}

const char *
hev_conf_baddr (void)
{
    return baddr;
}

const char *
hev_conf_bport (void)
{
    static char port[16];

    snprintf (port, sizeof (port) - 1, "%u", bport[2]);

    bport[2] = bport[2] + 1;
    if (bport[2] > bport[1]) {
        bport[2] = bport[0];
    }

    return port;
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

int
hev_conf_tmsec (void)
{
    return tmsec;
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
