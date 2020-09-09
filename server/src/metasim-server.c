/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "metasim-log.h"
#include "metasim-config.h"
#include "metasim-comm.h"

struct _metasim_ctx {
    metasim_config_t config;
    metasim_comm_t comm;
};

typedef struct _metasim_ctx metasim_ctx_t;

static metasim_ctx_t _metasim_ctx;
static metasim_ctx_t *metasim = &_metasim_ctx;

#define _config(m)   (&((m)->config))
#define _comm(m)     (&((m)->comm))


static int terminate;

static void daemonize(void)
{
    pid_t pid;
    pid_t sid;
    int rc;

    pid = fork();

    if (pid < 0) {
        __error("fork failed: %s", strerror(errno));
        exit(1);
    }

    if (pid > 0) {
        exit(0);
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
        __error("setsid failed: %s", strerror(errno));
        exit(1);
    }

    rc = chdir("/");
    if (rc < 0) {
        __error("chdir failed: %s", strerror(errno));
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid = fork();
    if (pid < 0) {
        __error("fork failed: %s", strerror(errno));
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }
}

static void exit_request(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
        terminate = 1;
        break;
    default:
        break;
    }
}

static void register_signal_handlers(void)
{
    int ret = 0;
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = exit_request;
    ret = sigemptyset(&sa.sa_mask);
    ret |= sigaction(SIGINT, &sa, NULL);
    ret |= sigaction(SIGQUIT, &sa, NULL);
    ret |= sigaction(SIGTERM, &sa, NULL);

    if (ret)
        __error("failed to register signal handlers");
}

static int cleanup(metasim_ctx_t *metasim)
{
    int ret = 0;

    if (metasim) {
        ret = metasim_comm_exit(_comm(metasim));

        metasim_log_close();
    }

    return ret;
}

/*
 * main program
 */

static void rpc_ping_test(metasim_comm_t *comm)
{
    int32_t targetrank = 0;
    int32_t nranks = 0;
    int32_t ping = 0;
    int32_t pong = 0;
    hg_handle_t handle = 0;
    hg_addr_t addr;
    hg_id_t rpc;
    metasim_ping_in_t in;
    metasim_ping_out_t out;

    __debug("testing if ping works");

    ping = comm->rank;
    nranks = comm->size;
    targetrank = (comm->rank + 1) % nranks;

    rpc = comm->remote_rpc.id_ping;
    addr = metasim_comm_get_rank_addr(comm, targetrank);
    in.ping = ping;

    hg_return_t hret = margo_create(comm->smid, addr, rpc, &handle);
    if (hret != HG_SUCCESS) {
        __error("failed to create margo instance");
        goto out;
    }

    __debug("sending pingpong (ping=%d) to rank %d", ping, targetrank);

    hret = margo_forward(handle, &in);

    margo_get_output(handle, &out);
    pong = out.pong;

    __debug("response from rank %d: pong=%d", targetrank, pong);

out:
    margo_free_output(handle, &out);

    if (handle != HG_HANDLE_NULL)
        margo_destroy(handle);
}

int main(int argc, char **argv)
{
    int ret = 0;
    char logfile[PATH_MAX];

    ret = metasim_config_process_arguments(_config(metasim), argc, argv);
    if (ret)
        return -1;

    /* daemonize the process */
    if (metasim->config.daemon)
        daemonize();

    //register_signal_handlers();

    ret = metasim_comm_init(_comm(metasim), _config(metasim));
    if (ret) {
        __error("failed to init the communication (ret=%d)\n", ret);
        return -1;
    }

    sprintf(logfile, "%s/metasimd.%d",
                     metasim->config.logdir, metasim->comm.rank);

    ret = metasim_log_open(logfile);
    if (ret) {
        __error("failed to open the log file (%s). "
                "messages will be printed in stderr", logfile);
    }

    __debug("metasimd initialized");

    //rpc_ping_test(_comm(metasim));

    __debug("ping test done, now waiting..");

    while (1)
        sleep(3);

    ret = cleanup(metasim);

    return ret;
}
