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
    int count = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc == 2) {
        count = atoi(argv[1]);
        assert(count > 0);
    }

    pid = getpid();

    metasim = metasim_init();
    assert(metasim);

    ret = metasim_invoke_init(metasim, rank, (int32_t) pid,
                              &server_rank, &server_nranks);
    if (ret) {
        __error("[%d] RPC INIT failed, terminating..\n", rank);
        goto out;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (count == 0)
        count = server_nranks;

    for (i = 0; i < count; i++) {
        int32_t num = i;
        int32_t echo = 0;

        ret = metasim_invoke_echo(metasim, num, &echo);

        __debug("[%d] (%3d/%3d) RPC ECHO (num=%d,echo=%d)",
                rank, i, count, num, echo);
        fflush(stdout);
    }

out:
    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}


