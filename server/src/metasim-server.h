#ifndef __METASIM_SERVER_H
#define __METASIM_SERVER_H

#include <margo.h>
#include <mpi.h>
#include <abt.h>

#include "metasim-log.h"

/* global sever context */
struct metasim_server {
    int rank;
    int nranks;
    hg_addr_t *peer_addrs;

    margo_instance_id mid;
};

typedef struct metasim_server metasim_server_t;

static inline hg_addr_t metasim_get_rank_addr(metasim_server_t *m, int rank)
{
    return m->peer_addrs[rank];
}

static inline void metasim_fence(void)
{
    MPI_Barrier(MPI_COMM_WORLD);
}

static inline void
__print_margo_handler_pool_size(margo_instance_id mid, const char *tag)
{
    int ret = 0;
    size_t size = 0;
    size_t total_size = 0;
    ABT_pool pool = NULL;

    ret = margo_get_handler_pool(mid, &pool);
    if (ret < 0) {
        __error("failed to get handler pool");
        return;
    }

    ret = ABT_pool_get_size(pool, &size);
    if (ret != ABT_SUCCESS) {
        __error("failed to get pool size");
        return;
    }
    ret = ABT_pool_get_total_size(pool, &total_size);
    if (ret != ABT_SUCCESS) {
        __error("failed to get pool total size");
        return;
    }

    __debug("[%s] handler pool size = %zu/%zu", tag, size, total_size);
}

#define print_margo_handler_pool_size(mid) \
        __print_margo_handler_pool_size((mid), __func__)

static inline void
__print_margo_handler_pool_info(margo_instance_id mid)
{
    ABT_pool pool = NULL;

    margo_get_handler_pool(mid, &pool);
    ABT_info_print_pool(metasim_log_stream, pool);
}

#define print_margo_handler_pool_info(mid)                                   \
        do {                                                                 \
            __debug("margo handler pool info:");                             \
            __print_margo_handler_pool_info((mid));                          \
        } while (0)

#endif /* __METASIM_SERVER_H */

