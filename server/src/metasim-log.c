/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include <stdio.h>
#include <errno.h>

#include "metasim-log.h"

int metasim_log_error;
int metasim_log_debug;

FILE *metasim_log_stream;

int metasim_log_open(const char *path)
{
    int ret = 0;
    FILE *fp = NULL;

    if (metasim_log_error + metasim_log_debug == 0)
        return 0;

    fp = fopen(path, "w+");
    if (fp)
        metasim_log_stream = fp;
    else
        ret = errno;

    return ret;
}

void metasim_log_close(void)
{
    if (metasim_log_stream)
        fclose(metasim_log_stream);
}

