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

#include <mpi.h>
#include <margo.h>

#include "metasim-server.h"
#include "metasim-rpc.h"
#include "metasim-listener.h"

metasim_server_t _metasim;
metasim_server_t *metasim = &_metasim;

static void __fence(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    __debug(format, args);
    va_end(args);

    metasim_fence();
}

static char hostname[NAME_MAX];
static const char *server_addr_dir = "logs/addr";

static int write_addr_file(int rank, const char *str)
{
    int ret = 0;
    char path[PATH_MAX];
    FILE *fp = NULL;

    sprintf(path, "%s/%d", server_addr_dir, rank);

    fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%s\n", str);
        fclose(fp);
    } else {
        __error("failed to write address file %s (%s)",
                path, strerror(errno));
        return errno;
    }

    return ret;
}

static int read_addr_file(margo_instance_id mid, int nranks, hg_addr_t *addrs)
{
    int ret = 0;
    int i = 0;
    FILE *fp = NULL;
    char path[PATH_MAX];
    char addrstr[512];
    hg_addr_t addr;
    hg_return_t hret;

    for (i = 0; i < nranks; i++) {
        sprintf(path, "%s/%d", server_addr_dir, i);
        fp = fopen(path, "r");
        if (fp) {
            fgets(addrstr, 511, fp);
            addrstr[strlen(addrstr) - 1] = '\0';

            __debug("reading address of rank[%d] = %s", i, addrstr);

            hret = margo_addr_lookup(mid, addrstr, &addr);
            assert(hret == HG_SUCCESS);
            fclose(fp);

            addrs[i] = addr;
        } else {
            __error("failed to read address file %s (err=%s)",
                    path, strerror(errno));
            return errno;
        }
    }

    return ret;
}

static int comm_init(int mpi_rank, int mpi_nranks)
{
    int ret = 0;
    int rank = 0;
    int nranks = 0;
    margo_instance_id mid;
    char addrstr[512];
    size_t addrstr_len = 512;
    hg_addr_t addr_self;
    hg_addr_t *peer_addrs;

    __debug("initializa the communication");

    /* just take the mpi comm world */
    rank = mpi_rank;
    nranks = mpi_nranks;

    mid = margo_init("ofi+tcp://", MARGO_SERVER_MODE, 1, 4);
    if (mid == MARGO_INSTANCE_NULL) {
        __error("failed to initialize margo");
        return EIO;
    }

    margo_addr_self(mid, &addr_self);
    margo_addr_to_string(mid, addrstr, &addrstr_len, addr_self);

    __debug("margo initialized: %s", addrstr);

    ret = write_addr_file(rank, addrstr);
    assert(0 == ret);

    peer_addrs = calloc(nranks, sizeof(*peer_addrs));
    assert(peer_addrs);

    __fence("reading peer addresses");

    ret = read_addr_file(mid, nranks, peer_addrs);
    assert(0 == ret);

    /* initialize the global context */
    metasim->rank = rank;
    metasim->nranks = nranks;
    metasim->mid = mid;
    metasim->peer_addrs = peer_addrs;

    return 0;
}

static void cleanup(void)
{
    int i = 0;

    if (metasim) {
        for (i = 0; i < metasim->nranks; i++) {
            if (metasim->peer_addrs[i] != HG_ADDR_NULL)
                margo_addr_free(metasim->mid, metasim->peer_addrs[i]);
        }

        if (metasim->mid)
            margo_finalize(metasim->mid);
    }

    metasim_log_close();
}

static int comm_probe_peers(void)
{
    int ret = 0;
    int i = 0;
    char str[512];
    size_t len = 512; 

    __debug("probing peer addresses");

    for (i = 0; i < metasim->nranks; i++) {
        hg_addr_t addr = metasim_get_rank_addr(metasim, i);
        hg_return_t hret = margo_addr_to_string(metasim->mid, str, &len, addr);
        if (hret != HG_SUCCESS)
            __error("failed to examine the address of rank %d", i);

        __debug("rank[%d]: %s %s", i, str,
                i == metasim->rank ? "(myself)" : "");
    }

    return ret;
}

static int test_ping(int rank)
{
    int ret = 0;
    int32_t i = 0;
    int32_t pong = 0;

    if (rank >= 0 && rank != metasim->rank)
        return 0;

    __debug("rank %d, performing ping test to %d peers",
            metasim->rank, metasim->nranks);

    for (i = 0; i < metasim->nranks; i++) {
        if (i == metasim->rank)
            continue;

        ret = metasim_rpc_invoke_ping(i, metasim->rank, &pong);
        if (ret)
            __error("rpc ping failed");

        __debug("[RPC PING] (target=%d, ping=%d): pong=%d",
                i, metasim->rank, pong);
    }

    return ret;
}

static int test_sum(int rank)
{
    int ret = 0;
    int32_t sum = 0;
    int32_t expected = 0;

    if (rank >= 0 && rank != metasim->rank)
        return 0;

    __debug("rank %d, performing sum test", metasim->rank);

    ret = metasim_rpc_invoke_sum(0, &sum);
    if (ret) {
        __error("rpc sum failed");
        goto out;
    }

    expected = (metasim->nranks - 1) * metasim->nranks / 2;

    __debug("[RPC SUM] sum=%d (expected=%d)", sum, expected);

out:
    return ret;
}

static struct option l_opts[] = {
    { "help", 0, 0, 'h' },
    { "test", 0, 0, 't' },
    { 0, 0, 0, 0 },
};

static char *s_opts = "ht";

static const char *usage_str =
"\n"
"Usage: metasimd [options...]\n"
"\n"
"Availble options:\n"
"-h, --help  print this help message\n"
"-t, --test  perform self test on server start up\n"
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
    int selftest = 0;
    char *pos = NULL;
    char logfile[PATH_MAX];
    char loglink[PATH_MAX];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_nranks);

    __debug("using mpi to bootstrap servers");

    while ((ch = getopt_long(argc, argv, s_opts, l_opts, &ix)) >= 0) {
        switch (ch) {
        case 't':
            selftest = 1;
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
    system("mkdir -p logs/addr");
    gethostname(hostname, NAME_MAX);

    sprintf(logfile, "logs/hosts/metasimd.%s", hostname);
    ret = metasim_log_open(logfile);
    if (ret) {
        __error("failed to open the log file (%s). "
                "messages will be printed in stderr", logfile);
    }

    /* initialize communication */
    ret = comm_init(mpi_rank, mpi_nranks);
    if (ret) {
        __error("failed to initialize the communication with peers");
        goto out;
    }

    /* create a symlink log file with rank */
    pos = strchr(logfile, '/');
    sprintf(loglink, "logs/metasimd.%d", metasim->rank);
    symlink(&pos[1], loglink);

    __fence("created log symlinks using ssg rank");

    /* check if we can resolve all peer addresses */
    if (metasim->rank == 0) {
        ret = comm_probe_peers();
        if (ret) {
            __error("failed to probe peers");
            goto out;
        }
    }

    /* register rpcs */
    metasim_rpc_register();
    margo_diag_start(metasim->mid);

    /* wait until all are initialized */
    __fence("all peers are initialized");

    if (selftest) {
        __debug("## test[0]: ping from 0");
        test_ping(0);
        __fence("## ping test completed from rank 0");

        __debug("## test[1]: ping from all ranks");
        test_ping(-1);
        __fence("## ping test completed from all ranks");

        __debug("## test[2]: broadcast sum from rank 0");
        test_sum(0);
        __fence("## broadcast sum test completed from rank 0");

        __debug("## test[3]: broadcast sum from all ranks");
        test_sum(-1);
        __fence("## broadcast sum test completed from all ranks");
    }

    /* init listener to accept requests from local clients */
    metasim_listener_init();

    margo_diag_dump(metasim->mid, "logs/margo/diag", 1);
    margo_wait_for_finalize(metasim->mid);
out:
    cleanup();
    MPI_Finalize();

    return ret;
}
