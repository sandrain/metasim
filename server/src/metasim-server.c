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
#include <getopt.h>
#include <ctype.h>
#include <signal.h>
#include <libgen.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * We currently use mpi for bootstrapping only. MPI seems to be the most
 * reliable way to do this over using file system or pmix, etc.
 */
#include <mpi.h>
#include <margo.h>

#include "metasim-server.h"
#include "metasim-rpc.h"
#include "metasim-listener.h"

metasim_server_t _metasim;
metasim_server_t *metasim = &_metasim;

static int metasim_proto;
static char *metasim_proto_prefix[] = {
    "ofi+tcp://", "ofi+verbs://", 0
};

static inline void metasim_fence(void)
{
    MPI_Barrier(MPI_COMM_WORLD);
}

static void __fence(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    __debug(format, args);
    va_end(args);

    metasim_fence();
}

static char hostname[NAME_MAX];

#define ADDRSTRLEN 256

static char addrstr[ADDRSTRLEN];
static char *metasimd_addrs;

static int write_addr_file(int nranks)
{
    int ret = 0;
    int i = 0;
    char path[PATH_MAX];
    FILE *fp = NULL;

    sprintf(path, "logs/metasimd.address.txt");

    fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%d\n", nranks);

        for (i = 0; i < nranks; i++)
            fprintf(fp, "%d,%s\n", i, &metasimd_addrs[i*ADDRSTRLEN]);

        fclose(fp);
    } else {
        __error("failed to write address file %s (%s)",
                path, strerror(errno));
        return errno;
    }

    return ret;
}

static int comm_init(int mpi_rank, int mpi_nranks)
{
    int ret = 0;
    int rank = 0;
    int nranks = 0;
    margo_instance_id mid;
    hg_addr_t addr_self;
    size_t addrlen = ADDRSTRLEN;
    const char *protostr = metasim_proto_prefix[metasim_proto];

    __debug("initializa the communication (protocol: %s)", protostr);

    /* just take the mpi comm world */
    rank = mpi_rank;
    nranks = mpi_nranks;

    mid = margo_init(protostr, MARGO_SERVER_MODE, 1, 4);
    if (mid == MARGO_INSTANCE_NULL) {
        __error("failed to initialize margo");
        return EIO;
    }

    margo_addr_self(mid, &addr_self);
    margo_addr_to_string(mid, addrstr, &addrlen, addr_self);

    __debug("margo address = %s", addrstr);

    __fence("margo initialized..");
    if (rank == 0) {
        metasimd_addrs = calloc(nranks, ADDRSTRLEN);
        assert(metasimd_addrs);
    }

    ret = MPI_Gather(addrstr, ADDRSTRLEN, MPI_CHAR,
                     metasimd_addrs, ADDRSTRLEN, MPI_CHAR,
                     0, MPI_COMM_WORLD);
    assert(ret == MPI_SUCCESS);

    /* initialize the global context */
    metasim->rank = rank;
    metasim->nranks = nranks;
    metasim->mid = mid;

    return ret;
}

static void cleanup(void)
{
    if (metasim) {
        if (metasim->mid)
            margo_finalize(metasim->mid);
    }

    metasim_log_close();
}

static struct option l_opts[] = {
    { "help", 0, 0, 'h' },
    { "verbs", 0, 0, 'i' },
    { "silent", 0, 0, 's' },
    { 0, 0, 0, 0 },
};

static char *s_opts = "hist";

static const char *usage_str =
"\n"
"Usage: metasimd [options...]\n"
"\n"
"Availble options:\n"
"-h, --help        print this help message\n"
"-i, --verbs       use ibverbs transport (default: tcp)\n"
"-s, --silent      do not print any logs\n"
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
    int mpi_rank = 0;
    int mpi_nranks = 0;
    int silent = 0;
    char *pos = NULL;
    char logfile[PATH_MAX];
    char loglink[PATH_MAX];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_nranks);

    __debug("using mpi to bootstrap servers");

    while ((ch = getopt_long(argc, argv, s_opts, l_opts, &ix)) >= 0) {
        switch (ch) {
        case 'i':
            metasim_proto = 1;
            break;

        case 's':
            silent = 1;
            break;

        case 'h':
        default:
            print_usage(0);
            break;
        }
    }

    /* open the log file */
    system("mkdir -p logs/margo");
    system("mkdir -p logs/hosts");
    gethostname(hostname, NAME_MAX);

    if (silent) {
        metasim_log_disable();
    } else {
        sprintf(logfile, "logs/hosts/metasimd.%s", hostname);
        ret = metasim_log_open(logfile);
        if (ret) {
            __error("failed to open the log file (%s). "
                    "messages will be printed in stderr", logfile);
        }
    }

    /* initialize communication */
    ret = comm_init(mpi_rank, mpi_nranks);
    if (ret) {
        __error("failed to initialize the communication with peers");
        goto out;
    }

    /* create a symlink log file with rank */
    if (!silent) {
        pos = strchr(logfile, '/');
        sprintf(loglink, "logs/metasimd.%d", metasim->rank);
        symlink(&pos[1], loglink);
    }

    /* register rpcs */
    metasim_rpc_register();

    /* wait until all are initialized */
    __fence("all servers are initialized");
    if (mpi_rank == 0)
        write_addr_file(mpi_nranks);

    margo_wait_for_finalize(metasim->mid);
out:
    cleanup();
    MPI_Finalize();

    return ret;
}
