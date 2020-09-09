#ifndef __METASIM_COMM_H
#define __METASIM_COMM_H

#include <stdint.h>
#include <ssg.h>
#include <ssg-pmix.h>

#include "metasim-log.h"
#include "metasim-config.h"
#include "metasim-rpc-remote.h"

struct _metasim_comm {
    /* rpc context for local client */
    margo_instance_id cmid;
    metasim_rpc_set_t local_rpc;

    /* server to server rpc context */
    int rank;
    int size;
    margo_instance_id smid;
    ssg_group_id_t gid;
    hg_addr_t *addrs;
    metasim_remote_rpc_set_t remote_rpc;
};

typedef struct _metasim_comm metasim_comm_t;

int metasim_comm_init(metasim_comm_t *comm, metasim_config_t *config);

int metasim_comm_exit(metasim_comm_t *comm);

hg_addr_t metasim_comm_get_rank_addr(metasim_comm_t *comm, int rank);

#endif /* __METASIM_COMM_H */

