/* Copyright (C) 2020, 2021 - UT-Battelle, LLC. All right reserved.
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

static int warmup;

static metasim_t metasim;

static double do_sum(int32_t seed, int32_t expected)
{
    int ret = 0;
    int32_t sum = 0;
    uint64_t usec = 0;
    double elapsed = .0f;

    ret = metasim_invoke_sum(metasim, seed, &sum, &usec);

    elapsed = usec*1e-6;

    __debug("[%d] RPC SUM (seed=%d) => (ret=%d, sum=%d), expected sum=%d (%s),"
            " %.6f seconds",
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
    int i = 0;
    int32_t expected = 0;
    double server_elapsed = .0f;
    double elapsed = .0f;
    double start = .0f;
    double stop = .0f;

    if (rank > 0)
        goto wait;

    expected = calculate_expected_sum(0);

    /* warm up run (the 1st run takes significantly longer than the rest) */
    if (warmup)
        elapsed = do_sum(rank, expected);

    start = MPI_Wtime();

    for (i = 0; i < repeat; i++) {
        elapsed = do_sum(0, expected);
        printf("%.6lf\n", elapsed);

        server_elapsed += elapsed;
    }

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

static int do_sum_limited(int repeat, int limited)
{
    int i = 0;
    int j = 0;
    int rounds = 1;
    int32_t expected = 0;
    double elapsed = .0f;
    double *all_elapsed = NULL;
    double start = .0f;
    double stop = .0f;

    if (rank == 0) {
        all_elapsed = calloc(nranks, sizeof(*all_elapsed));
        assert(all_elapsed);
    }

    if (limited < nranks) {
        rounds = nranks / limited;
        if (nranks % limited)
            rounds++;
    }

    __debug("hash run: %d rounds\n", rounds);

    expected = calculate_expected_sum(rank);

    /* warm up run (the 1st run takes significantly longer than the rest) */
    if (warmup)
        elapsed = do_sum(rank, expected);

    start = MPI_Wtime();

    for (i = 0; i < repeat; i++) {


        for (j = 0; j < rounds; j++) {
            MPI_Barrier(MPI_COMM_WORLD);

            if (rank % rounds == j) {
                printf("## rank %d running at round %d", rank, j);
                do_sum(rank, expected);
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }

        MPI_Gather(&elapsed, 1, MPI_DOUBLE,
                   all_elapsed, 1, MPI_DOUBLE,
                   0, MPI_COMM_WORLD);
    }

    stop = MPI_Wtime();

    if (rank == 0) {
        double total_runtime = stop - start;
        double avg = total_runtime / repeat;

        printf("## %d,%.6lf,%.6lf (%d rounds)\n",
                repeat, total_runtime, avg, rounds);
    }

    return 0;
}


static int do_sum_parallel(int repeat)
{
    int i = 0;
    int32_t expected = 0;
    double elapsed = .0f;
    double *all_elapsed = NULL;
    double start = .0f;
    double stop = .0f;

    if (rank == 0) {
        all_elapsed = calloc(nranks, sizeof(*all_elapsed));
        assert(all_elapsed);
    }

    expected = calculate_expected_sum(rank);

    /* warm up run (the 1st run takes significantly longer than the rest) */
    if (warmup)
        elapsed = do_sum(rank, expected);

    start = MPI_Wtime();

    for (i = 0; i < repeat; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        elapsed = do_sum(rank, expected);

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Gather(&elapsed, 1, MPI_DOUBLE,
                   all_elapsed, 1, MPI_DOUBLE,
                   0, MPI_COMM_WORLD);

        if (rank == 0) {
            for (int r = 0; r < nranks; r++) {
                printf("%.6lf%c", all_elapsed[r],
                                  r == nranks - 1 ? '\n' : ',');
            }
        }
    }

    stop = MPI_Wtime();

    if (rank == 0) {
        double total_runtime = stop - start;
        double avg = total_runtime / repeat;

        printf("## %d,%.6lf,%.6lf\n", repeat, total_runtime, avg);
    }

    return 0;
}

static struct option l_opts[] = {
    { "help", 0, 0, 'h' },
    { "limited", 1, 0, 'l' },
    { "repeat", 1, 0, 'r' },
    { "serial", 0, 0, 's' },
    { "verbose", 0, 0, 'v' },
    { "warmup", 0, 0, 'w' },
    { 0, 0, 0, 0 },
};

static char *s_opts = "hl:r:svw";

static char *usage_str =
"\n"
"Usage: sum [options...]\n"
"\n"
"-h, --help         print this help message\n"
"-l, --limited=<N>  at most <N> sum operations are executed in parallel\n"
"-r, --repeat=<N>   repeat <N> times (default=1)\n"
"-s, --serial       execute sum only from rank 0\n"
"                   (default: running in parallel from all ranks)\n"
"-v, --verbose      print debugging messages\n"
"-w, --warmup       perform an extra warmup operation before measuring\n"
"\n";

static void print_usage(int ec)
{
    fputs(usage_str, stderr);
    exit(ec);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int serial = 0;
    int ch = 0;
    int ix = 0;
    int repeat = 1;
    int limited = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while ((ch = getopt_long(argc, argv, s_opts, l_opts, &ix)) >= 0) {
        switch (ch) {
        case 'r':
            repeat = atoi(optarg);
            break;

        case 'l':
            limited = atoi(optarg);
            break;

        case 's':
            serial = 1;
            break;

        case 'v':
            log_error = 1;
            log_debug = 1;
            break;

        case 'w':
            warmup = 1;
            break;

        case 'h':
        default:
            print_usage(0);
            break;
        }
    }

    if (serial)
        __debug("sum will be invoked only from rank 0");
    else if (limited)
        __debug("sum will be invoked from all %d ranks "
                "(%d inflight operations at a time)", nranks, limited);
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

    if (serial)
        do_sum_serial(repeat);
    else if (limited)
        do_sum_limited(repeat, limited);
    else
        do_sum_parallel(repeat);

out:
    metasim_exit(metasim);

    MPI_Finalize();

    return ret;
}

