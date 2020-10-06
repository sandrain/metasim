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

static int rank;
static int nranks;
static int verbose;
static int repeat = 100;

static int32_t calculate_expected_sum(int rank)
{
    int32_t seed;
    int32_t expected;

    seed = rank;
    expected = (nranks * (nranks - 1)) / 2;
    expected += nranks * seed;

    return expected;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int sum = 0;
    int seed = 0;
    int local_sum = 0;
    int expected = 0;
    int i = 0;
    double start, stop;
    double lstart, lstop;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    if (rank == 0) {
        seed = rank;
        expected = calculate_expected_sum(rank);
        start = MPI_Wtime();
    }

    for (i = 0; i < repeat; i++) {
        if (verbose && rank == 0)
            lstart = MPI_Wtime();

        MPI_Bcast(&seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
        local_sum = rank + seed;
        MPI_Reduce(&local_sum, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


        if (verbose && rank == 0) {
            lstop = MPI_Wtime();
            printf("%.6f\n", lstop-lstart);
        }
    }

    if (rank == 0) {
        stop = MPI_Wtime();
        printf("## sum = %d (expected=%d), %.6f seconds\n",
               sum, expected, (stop-start)/(1.0*repeat));
    }

    MPI_Barrier(MPI_COMM_WORLD);

    return ret;
}

