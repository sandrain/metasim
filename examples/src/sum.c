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

static int rank;
static int nranks;
static pid_t pid;

static int server_rank;
static int server_nranks;

static metasim_t metasim;

int main(int argc, char **argv)
{
    int ret = 0;
    int sum_rank = -1;
    int32_t seed = -1;
    int32_t sum = 0;
    int32_t expected = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc == 2) {
        sum_rank = atoi(argv[1]);
        if (sum_rank >= nranks) {
            __debug("%d is not a valid rank, rank 0 will be used", sum_rank);
            sum_rank = 0;
        }
    }

    if (sum_rank >= 0)
        __debug("sum will be invoked from rank %d", sum_rank);
    else
        __debug("sum will be invoked from all %d ranks", nranks);

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

    seed = rank;
    expected = (server_nranks * (server_nranks - 1)) / 2;
    expected += server_nranks * seed;

    MPI_Barrier(MPI_COMM_WORLD);

    if (sum_rank < 0 || sum_rank == rank) {
        ret = metasim_invoke_sum(metasim, seed, &sum);
        __debug("[%d] RPC SUM (seed=%d) => (ret=%d,sum=%d), expected sum=%d",
                rank, seed, ret, sum, expected);
    }

    MPI_Barrier(MPI_COMM_WORLD);

out:
    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}

