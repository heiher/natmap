/*
 ============================================================================
 Name        : hev-ifwd.c
 Author      : Mike Wang <mikewang000000@gmail.com>
               hev <r@hev.cc>
 Copyright   : Copyright (c) 2024 xyz
 Description : iptables forwarder
 ============================================================================
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hev-conf.h"
#include "hev-ifwd.h"
#include "hev-misc.h"
#include "hev-exec.h"

#define BUFFLEN 1024

typedef struct _IptNatRule IptNatRule;

struct _IptNatRule
{
    char table[16];
    char chain[32];
    char proto[8];
    char ipv4[16];
    char port[16];
    char toipv4[16];
    char toport[16];
};

static int ipt_initialized = 0;
static int ipt_atexit      = 0;
static IptNatRule nat_rule;

static int
call_subprocess (const char *file, char *const argv[], char *sstdout,
                 size_t nstdout, char *sstderr, size_t nstderr)
{
    int pipefd_o[2], pipefd_e[2];
    char blackhole[512];
    int status = -1;
    int nbyte;
    pid_t pid;

    signal(SIGCHLD, SIG_DFL);

    if (pipe (pipefd_o) == -1 || pipe (pipefd_e) == -1) {
        LOGV (E, "%s", strerror (errno));
        goto err_exit;
    }

    pid = fork ();

    if (pid == -1) {
        LOGV (E, "%s", strerror (errno));
        goto err_close;
    }

    if (pid == 0) {
        if (dup2 (pipefd_o[1], STDOUT_FILENO) == -1 ||
            dup2 (pipefd_e[1], STDERR_FILENO) == -1) {
            LOGV (E, "%s", strerror (errno));
            _exit (EXIT_FAILURE);
        }
        close (pipefd_o[0]);
        close (pipefd_o[1]);
        close (pipefd_e[0]);
        close (pipefd_e[1]);
        execvp (file, argv);
        LOGV (E, "%s", strerror (errno));
        _exit (EXIT_FAILURE);
    }

    close (pipefd_o[1]);
    close (pipefd_e[1]);

    nbyte = read (pipefd_o[0], sstdout, nstdout ? nstdout - 1 : 0);
    if (nbyte == -1) {
        LOGV (E, "%s", strerror (errno));
        goto err_close_read;
    }

    if (nstdout)
        sstdout[nbyte] = 0;

    while (read (pipefd_o[0], blackhole, sizeof (blackhole)) > 0) { }

    nbyte = read (pipefd_e[0], sstderr, nstderr ? nstderr - 1 : 0);
    if (nbyte == -1) {
        LOGV (E, "%s", strerror (errno));
        goto err_close_read;
    }

    if (nstderr)
        sstderr[nbyte] = 0;

    while (read (pipefd_e[0], blackhole, sizeof (blackhole)) > 0) { }

    if (waitpid (pid, &status, 0) == -1) {
        LOGV (E, "%s", strerror (errno));
        goto err_close_read;
    }

    close (pipefd_o[0]);
    close (pipefd_e[0]);

    signal(SIGCHLD, hev_exec_signal_handler);

    return WEXITSTATUS (status);

err_close:
    close (pipefd_o[1]);
    close (pipefd_e[1]);

err_close_read:
    close (pipefd_o[0]);
    close (pipefd_e[0]);

err_exit:
    signal(SIGCHLD, hev_exec_signal_handler);
    return -1;
}

static int
vercmp (int a_major, int a_minor, int a_fix,
        int b_major, int b_minor, int b_fix)
{
    if (a_major < b_major) {
        return -1;
    }
    if (a_major > b_major) {
        return 1;
    }
    if (a_minor < b_minor) {
        return -1;
    }
    if (a_minor > b_minor) {
        return 1;
    }
    if (a_fix < b_fix) {
        return -1;
    }
    if (a_fix > b_fix) {
        return 1;
    }

    return 0;
}

static int
ipt_precheck (void)
{
    char *const cmd_ver[]  = { "iptables", "--version", 0 };

    char sstdout[BUFFLEN], sstderr[BUFFLEN];
    int ret;
    int v_major, v_minor, v_fix, m;

    ret = call_subprocess ("iptables", cmd_ver, sstdout, sizeof (sstdout),
                           sstderr, sizeof (sstderr));
    fprintf (stderr, "%s", sstderr);

    if (ret)
        return -1;

    m = sscanf (sstdout, "iptables v%d.%d.%d", &v_major, &v_minor, &v_fix);

    if (m != 3) {
        LOGV (E, "%s", "failed to check iptables version");
        return -1;
    }

    if (vercmp (v_major, v_minor, v_fix, 1, 4, 20) <= 0) {
        LOGV (E, "%s", "iptables version >= 1.4.20 is required");
        return -1;
    }

    return 0;
}

static int
ipt_chain_exists (char *table, char *chain)
{
    char *const cmd[] = { "iptables", "--wait", "-t", table,
                          "--list-rules", chain, 0 };

    char sstdout[BUFFLEN], sstderr[BUFFLEN];
    int ret;

    ret = call_subprocess ("iptables", cmd, sstdout, sizeof (sstdout),
                           sstderr, sizeof (sstderr));

    if (ret == 1) {
        return 0;
    } else if (ret) {
        fprintf (stderr, "%s", sstderr);
        return -1;
    }
    return 1;
}

static int
ipt_create_chain (char *table, char *chain)
{
    char *const cmd[] = { "iptables", "--wait", "-t", table,
                          "-N", chain, 0 };

    char sstdout[BUFFLEN], sstderr[BUFFLEN];
    int ret;

    ret = call_subprocess ("iptables", cmd, sstdout, sizeof (sstdout),
                           sstderr, sizeof (sstderr));
    fprintf (stderr, "%s", sstderr);

    return ret;
}

static int
ipt_insert_jump (char *table, char *chain, char *dest_chain)
{
    char *const cmd[] = { "iptables", "--wait", "-t", table,
                          "-I", chain, "-j", dest_chain, 0 };

    char sstdout[BUFFLEN], sstderr[BUFFLEN];
    int ret;

    ret = call_subprocess ("iptables", cmd, sstdout, sizeof (sstdout),
                           sstderr, sizeof (sstderr));
    fprintf (stderr, "%s", sstderr);

    return ret;
}

static int
ipt_insert_dnat (IptNatRule *rule)
{
    char todest[32];
    char *const cmd[] = {
        "iptables",         "--wait",
        "-t",               rule->table,
        "-I",               rule->chain,
        "-p",               rule->proto,
        "--dst",            rule->ipv4,
        "--dport",          rule->port,
        "-j",               "DNAT",
        "--to-destination", todest,
        0
    };

    char sstdout[BUFFLEN], sstderr[BUFFLEN];
    int ret;

    snprintf (todest, sizeof (todest), "%s:%s", rule->toipv4, rule->toport);

    ret = call_subprocess ("iptables", cmd, sstdout, sizeof (sstdout),
                           sstderr, sizeof (sstderr));
    fprintf (stderr, "%s", sstderr);

    return ret;
}

static int
ipt_delete_dnat (IptNatRule *rule)
{
    char todest[32];
    char *const cmd[] = {
        "iptables",         "--wait",
        "-t",               rule->table,
        "-D",               rule->chain,
        "-p",               rule->proto,
        "--dst",            rule->ipv4,
        "--dport",          rule->port,
        "-j",               "DNAT",
        "--to-destination", todest,
        0
    };

    char sstdout[BUFFLEN], sstderr[BUFFLEN];
    int ret;

    snprintf (todest, sizeof (todest), "%s:%s", rule->toipv4, rule->toport);

    ret = call_subprocess ("iptables", cmd, sstdout, sizeof (sstdout), sstderr,
                           sizeof (sstderr));
    fprintf (stderr, "%s", sstderr);

    return ret;
}

static int
ipt_init (void)
{
    int ret;

    if (ipt_initialized)
        return 0;

    if (ipt_precheck ())
        return -1;

    ret = ipt_chain_exists ("nat", "NATMAP");

    if (ret == -1)
        return -1;

    if (!ret) {
        if (ipt_create_chain ("nat", "NATMAP"))
            return -1;
        if (ipt_insert_jump ("nat", "PREROUTING", "NATMAP"))
            return -1;
        if (ipt_insert_jump ("nat", "OUTPUT", "NATMAP"))
            return -1;
    }

    ipt_initialized = 1;
    return 0;
}

static int
get_bind_info (int fd, char baddr[16], char bport[16])
{
    int ret;
    struct sockaddr_storage addr;
    struct sockaddr_in *paddr;
    socklen_t len = sizeof (addr);

    ret = getsockname (fd, (struct sockaddr *) &addr, &len);
    if (ret < 0) {
        LOG (E);
        close (fd);
        return -1;
    }

    if (addr.ss_family != AF_INET) {
        LOGV (E, "%s", "IPv6 is currently not supported.");
        return -1;
    }

    paddr = (struct sockaddr_in *) &addr;
    inet_ntop (paddr->sin_family, &paddr->sin_addr, baddr, 16);
    snprintf(bport, 16, "%u", ntohs(paddr->sin_port));

    return 0;
}

static int
ipt_create_nat_rule (int fd)
{
    struct in_addr addr_target, addr_local;
    const char *proto;
    const char *taddr;
    const char *tport;
    char baddr[16];
    char bport[16];
    int mode;

    mode = hev_conf_mode ();
    taddr = hev_conf_taddr ();
    tport = hev_conf_tport ();
    proto = (mode == SOCK_STREAM) ? "tcp" : "udp";

    if (get_bind_info(fd, baddr, bport))
        return -1;

    if (inet_pton (AF_INET, taddr, &addr_target) != 1)
        return -1;

    if (inet_pton (AF_INET, "127.0.0.1", &addr_local) != 1)
        return -1;

    if (addr_target.s_addr == addr_local.s_addr)
        taddr = baddr;

    snprintf(nat_rule.table,  sizeof(nat_rule.table),  "nat");
    snprintf(nat_rule.chain,  sizeof(nat_rule.chain),  "NATMAP");
    snprintf(nat_rule.proto,  sizeof(nat_rule.proto),  "%s", proto);
    snprintf(nat_rule.ipv4,   sizeof(nat_rule.ipv4),   "%s", baddr);
    snprintf(nat_rule.port,   sizeof(nat_rule.port),   "%s", bport);
    snprintf(nat_rule.toipv4, sizeof(nat_rule.toipv4), "%s", taddr);
    snprintf(nat_rule.toport, sizeof(nat_rule.toport), "%s", tport);

    return 0;
}

static void
exit_sig_handler(int sig)
{
    hev_ifwd_kill ();
    signal (sig, SIG_DFL);
    raise (sig);
}

static int
ipt_register_atexit (void)
{
    int ret;

    if (ipt_atexit)
        return 0;

    ret = atexit(hev_ifwd_kill);
    if (ret)
        LOG (E);

    signal(SIGINT, exit_sig_handler);
    signal(SIGTERM, exit_sig_handler);

    ipt_atexit = 1;
    return ret;
}

void
hev_ifwd_run (int fd)
{
    if (ipt_init ()) {
        LOGV (E, "%s", "iptables initialization failed.");
        return;
    }
    if (ipt_create_nat_rule (fd)) {
        LOGV (E, "%s", "Failed to create iptables rule.");
        return;
    }
    if (ipt_insert_dnat (&nat_rule)) {
        LOGV (E, "%s", "Failed to insert iptables rule.");
        return;
    }
    if (ipt_register_atexit ()) {
        LOGV (E, "%s", "Failed to register iptables cleanup function.");
        return;
    }
}

void
hev_ifwd_kill (void)
{
    if (ipt_delete_dnat (&nat_rule)) {
        LOGV (E, "%s", "Failed to delete iptables rule.");
        return;
    }
}
