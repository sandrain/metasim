/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <metasim.h>

int main(int argc, char **argv)
{
    int ret = 0;
    int32_t repeat = 5;
    int32_t i = 0;
    int32_t localrank = 0;
    int32_t nservers = 0;
    int32_t pong = 0;
    metasim_t metasim;

    if (argc == 2)
        repeat = atoi(argv[1]);

    metasim = metasim_init();

    /* query the local server ranks and the count of servers */
    ret = metasim_invoke_query(metasim, &localrank, &nservers);
    if (ret) {
        perror("rpc query failed");
        goto out;
    }

    printf("local server rank = %d (out of %d servers)\n", localrank, nservers);
    fflush(stdout);

    for (i = 0; i < repeat; i++) {
        int32_t targetrank = (localrank + 1) % nservers;

        printf("sending ping=%d to server %d\n", i, targetrank);
        fflush(stdout);

        ret = metasim_invoke_rping(metasim, targetrank, i, &pong);
        if (ret)
            perror("rpc ping failed");
        else {
            printf(" => response from server %d: ping=%d, pong=%d\n",
                   targetrank, i, pong);
            fflush(stdout);
        }
    }

out:
    metasim_exit(metasim);

    return ret;
}
