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

static int rank;
static int nranks;
static pid_t pid;

static int server_rank;
static int server_nranks;

static metasim_t metasim;

int main(int argc, char **argv)
{
    int ret = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    pid = getpid();

    metasim = metasim_init();
    assert(metasim);

    ret = metasim_invoke_init(metasim, rank, (int32_t) pid,
                              &server_rank, &server_nranks);
    assert(0 == ret);

    printf("[%d] RPC INIT result: server_rank=%d, server_nranks=%d\n",
           rank, server_rank, server_nranks);

    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}

