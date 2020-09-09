#ifndef __METASIM_RPC_H
#define __METASIM_RPC_H

#include <margo.h>

#define METASIM_RPC_LOCAL_ADDR_FILE     "/tmp/metasim.server.address"

MERCURY_GEN_PROC(metasim_query_in_t,
                 ((int32_t)(dummay)));
MERCURY_GEN_PROC(metasim_query_out_t,
                 ((int32_t)(localrank))
                 ((int32_t)(nservers)));
DECLARE_MARGO_RPC_HANDLER(metasim_rpc_local_query_handle);

MERCURY_GEN_PROC(metasim_ping_in_t,
                 ((int32_t)(ping)));
MERCURY_GEN_PROC(metasim_ping_out_t,
                 ((int32_t)(pong)));
DECLARE_MARGO_RPC_HANDLER(metasim_rpc_local_ping_handle);

MERCURY_GEN_PROC(metasim_rping_in_t,
                 ((int32_t)(server))
                 ((int32_t)(ping)));
MERCURY_GEN_PROC(metasim_rping_out_t,
                 ((int32_t)(pong)));
DECLARE_MARGO_RPC_HANDLER(metasim_rpc_local_rping_handle);


struct metasim_rpc_set {
    hg_id_t id_query;
    hg_id_t id_ping;
    hg_id_t id_rping;
};

typedef struct metasim_rpc_set metasim_rpc_set_t;

static inline
void metasim_rpc_register_client(margo_instance_id mid, metasim_rpc_set_t *rpc)
{
    if (rpc) {
        rpc->id_query =
            MARGO_REGISTER(mid, "metasim_rpc_query",
                           metasim_query_in_t,
                           metasim_query_out_t,
                           NULL);
        rpc->id_ping =
            MARGO_REGISTER(mid, "metasim_rpc_ping",
                           metasim_ping_in_t,
                           metasim_ping_out_t,
                           NULL);
        rpc->id_rping =
            MARGO_REGISTER(mid, "metasim_rpc_rping",
                           metasim_rping_in_t,
                           metasim_rping_out_t,
                           NULL);
    }
}

#endif /* __METASIM_RPC_H */
