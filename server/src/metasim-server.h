#ifndef __METASIM_SERVER_H
#define __METASIM_SERVER_H

#include <margo.h>
#include <ssg.h>
#include <mpi.h>

#include "metasim-log.h"

/* global sever context */
struct metasim_server {
    int rank;
    int nranks;
    hg_addr_t *peer_addrs;

    margo_instance_id mid;
    ssg_group_id_t gid;
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

#endif /* __METASIM_SERVER_H */

