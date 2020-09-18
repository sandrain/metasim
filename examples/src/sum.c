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
#include <getopt.h>
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

static struct option l_opts[] = {
    { "help", 0, 0, 'h' },
    { "rank", 1, 0, 'r' },
    { "serial", 0, 0, 's' },
    { 0, 0, 0, 0 },
};

static char *s_opts = "hr:s";

static char *usage_str =
"\n"
"Usage: sum [options...]\n"
"\n"
"-h, --help         print this help message\n"
"-r, --rank=<R>     execute sum operation only on rank <R>\n"
"                   (default: from all ranks)\n"
"-s, --serial       execute from target ranks one by one\n"
"                   (default: running in parallel)\n"
"\n";

static void print_usage(int ec)
{
    fputs(usage_str, stderr);
    exit(ec);
}

static void do_sum(int32_t seed, int32_t expected)
{
    int ret = 0;
    int32_t sum = 0;

    ret = metasim_invoke_sum(metasim, seed, &sum);

    __debug("[%d] RPC SUM (seed=%d) => (ret=%d, sum=%d), expected sum=%d",
            rank, seed, ret, sum, expected);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int serial = 0;
    int sum_rank = -1;
    int ch = 0;
    int ix = 0;
    int32_t seed = -1;
    int32_t expected = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while ((ch = getopt_long(argc, argv, s_opts, l_opts, &ix)) >= 0) {
        switch (ch) {
        case 'r':
            sum_rank = atoi(optarg);
            break;

        case 's':
            serial = 1;
            break;

        case 'h':
        default:
            print_usage(0);
            break;
        }
    }

    if (sum_rank >= nranks) {
        __debug("%d is not a valid rank, rank 0 will be used", sum_rank);
        sum_rank = 0;
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

    if (sum_rank < 0 && serial) {
        for (int i = 0; i < nranks; i++) {
            MPI_Barrier(MPI_COMM_WORLD);

            if (i == rank)
                do_sum(seed, expected);
        }
    } else {
        MPI_Barrier(MPI_COMM_WORLD);

        if (sum_rank < 0 || sum_rank == rank)
            do_sum(seed, expected);

        MPI_Barrier(MPI_COMM_WORLD);
    }

out:
    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}

