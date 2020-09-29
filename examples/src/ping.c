/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include <sys/types.h>
#include <unistd.h>
#include <metasim.h>

#include "log.h"

int log_error = 1;
int log_debug = 1;

static int rank;
static int nranks;
static pid_t pid;

static int server_rank;
static int server_nranks;

static metasim_t metasim;

int main(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    int ping_count = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc == 2) {
        ping_count = atoi(argv[1]);
        assert(ping_count > 0);
    }

    pid = getpid();

    metasim = metasim_init();
    assert(metasim);

    ret = metasim_invoke_init(metasim, rank, (int32_t) pid,
                              &server_rank, &server_nranks);
    __debug("[%d] RPC INIT (rank=%d,pid=%d) => "
            "(server_rank=%d, server_nranks=%d)",
            rank, rank, pid, server_rank, server_nranks);

    if (ret) {
        __error("[%d] rpc failed, terminating..", rank);
        fflush(stdout);
        goto out;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (ping_count == 0)
        ping_count = server_nranks;

    for (i = 0; i < ping_count; i++) {
        int32_t ping = i;
        int32_t pong = 0;
        int32_t target = i % server_nranks;

        ret = metasim_invoke_ping(metasim, target, ping, &pong);

        __debug("[%d] (%3d/%3d) RPC PING (target=%d,ping=%d) => "
                "(ret=%d, pong=%d)",
                rank, i, ping_count, i, ping, ret, pong);
    }

out:
    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}

