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

int log_error;
int log_debug;

static int rank;
static int nranks;
static pid_t pid;

static int server_rank;
static int server_nranks;

static metasim_t metasim;

static double do_sum_server(int32_t seed, int32_t expected, int32_t repeat)
{
    int ret = 0;
    int32_t sum = 0;
    uint64_t usec = 0;
    double elapsed = .0f;

    ret = metasim_invoke_sumrepeat(metasim, seed, repeat, &sum, &usec);

    elapsed = usec*1e-6;

    __debug("[%d] RPC SUMREPEAT (seed=%d) => (ret=%d, sum=%d), "
            "expected sum=%d (%s), %.6f seconds",
            rank, seed, ret, sum, expected,
            sum == expected ? "success" : "fail",
            elapsed);

    return elapsed;
}

static int32_t calculate_expected_sum(int rank)
{
    int32_t seed;
    int32_t expected;

    seed = rank;
    expected = (server_nranks * (server_nranks - 1)) / 2;
    expected += server_nranks * seed;

    return expected;
}

static int do_sum_serial(int repeat)
{
    int32_t expected = 0;
    double server_elapsed = .0f;
    double start = .0f;
    double stop = .0f;

    if (rank > 0)
        goto wait;

    expected = calculate_expected_sum(0);

    start = MPI_Wtime();

    server_elapsed = do_sum_server(0, expected, repeat);

    stop = MPI_Wtime();

    if (rank == 0) {
        double total_runtime = stop - start;
        double avg = total_runtime / repeat;

        printf("## %d,%.6lf,%.6lf,%.6lf\n",
               repeat, total_runtime, avg, server_elapsed/repeat);
    }

wait:
    MPI_Barrier(MPI_COMM_WORLD);
    return 0;
}

static struct option l_opts[] = {
    { "help", 0, 0, 'h' },
    { "repeat", 1, 0, 'r' },
    { "verbose", 0, 0, 'v' },
    { 0, 0, 0, 0 },
};

static char *s_opts = "hr:v";

static char *usage_str =
"\n"
"Usage: sum [options...]\n"
"\n"
"-h, --help         print this help message\n"
"-r, --repeat=<N>   repeat <N> times (default=1)\n"
"-v, --verbose      print debugging messages\n"
"\n";

static void print_usage(int ec)
{
    fputs(usage_str, stderr);
    exit(ec);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int ix = 0;
    int repeat = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while ((ch = getopt_long(argc, argv, s_opts, l_opts, &ix)) >= 0) {
        switch (ch) {
        case 'r':
            repeat = atoi(optarg);
            break;

        case 'v':
            log_error = 1;
            log_debug = 1;
            break;

        case 'h':
        default:
            print_usage(0);
            break;
        }
    }

    pid = getpid();

    metasim = metasim_init();
    assert(metasim);

    ret = metasim_invoke_init(metasim, rank, (int32_t) pid,
                              &server_rank, &server_nranks);
    if (ret) {
        __error("[%d] rpc failed, terminating..", rank);
        fflush(stdout);
        goto out;
    } else {
        __debug("[%d] RPC INIT (rank=%d,pid=%d) => "
                "(server_rank=%d, server_nranks=%d)",
                rank, rank, pid, server_rank, server_nranks);
    }

    do_sum_serial(repeat);

out:
    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}

