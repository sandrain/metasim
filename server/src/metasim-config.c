/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <unistd.h>

#include "metasim-config.h"
#include "metasim-log.h"

static int default_rpc_client_thread_pool_size = 4;
static int default_rpc_server_thread_pool_size = 4;

static struct option l_opts[] = {
    { "daemonize", 0, 0, 'd' },
    { "help", 0, 0, 'h' },
    { "logdir", 1, 0, 'l' },
    { "silent", 0, 0, 's' },
    { "verbose", 0, 0, 'v' },
    { 0, 0, 0, 0 },
};

static char *s_opts = "dhl:sv";

static const char *usage_str =
"\n"
"Usage: %s [options...]\n"
"\n"
"Available options:\n"
" -d, --daemonize       run as a deamon process\n"
" -h, --help            print this help message\n"
" -l, --logdir=<DIR>    directory where log files will be stored\n"
" -s, --silent          disable all log messages\n"
" -v, --verbose         print informative but noisy log messages\n"
"\n";

int metasim_config_process_arguments(metasim_config_t *config,
                                     int argc, char **argv)
{
    int ret = 0;
    int ch = 0;
    int ix = 0;
    int silent = 0;
    int verbose = 0;
    char *logdir = NULL;

    if (!config)
        return EINVAL;

    memset(config, 0, sizeof(*config));

    while ((ch = getopt_long(argc, argv, s_opts, l_opts, &ix)) >= 0) {
        switch (ch) {
        case 'd':
            config->daemon = 1;
            break;

        case 'l':
            logdir = optarg;
            break;

        case 's':
            silent = 1;
            break;

        case 'v':
            verbose = 1;
            break;

        case 'h':
        default:
            ret = EINVAL;
            break;
        }

        if (ret)
            break;
    }

    /* handle the logstream */
    if (silent + verbose > 1) {
        ret = EINVAL;
    } else if (silent + verbose == 0) {
        metasim_log_error = 1;
    } else {
        if (silent) {
            metasim_log_debug = 0;
            metasim_log_error = 0;
        }

        if (verbose) {
            metasim_log_debug = 1;
            metasim_log_error = 1;
        }
    }

    /* log directory */
    logdir = realpath(logdir, NULL);
    if (logdir) {
        ret = access(logdir, R_OK | W_OK | X_OK);
        if (0 == ret)
            config->logdir = logdir;
    }

    /* set default values if not provided */
    config->rpc_client_thread_pool_size = default_rpc_client_thread_pool_size;
    config->rpc_client_thread_pool_size = default_rpc_server_thread_pool_size;

    if (ret) {
        fputs(usage_str, stderr);
        return ret;
    }

    return ret;
}

