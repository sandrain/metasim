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
    int repeat = 5;
    int32_t i = 0;
    int32_t pong = 0;
    metasim_t metasim;

    if (argc == 2)
        repeat = atoi(argv[1]);

    metasim = metasim_init();

    for (i = 0; i < repeat; i++) {
        ret = metasim_invoke_ping(metasim, i, &pong);
        if (ret)
            perror("rpc ping failed");
        else
            printf("ping=%d, pong=%d\n", i, pong);
    }

    metasim_exit(metasim);

    return ret;
}
