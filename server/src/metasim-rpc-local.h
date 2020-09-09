#ifndef __METASIM_RPC_LOCAL_H
#define __METASIM_RPC_LOCAL_H

#include "metasim-rpc.h"

static inline
void metasim_rpc_register_server(margo_instance_id mid, metasim_rpc_set_t *rpc)
{
    if (rpc) {
        rpc->id_query =
            MARGO_REGISTER(mid, "metasim_rpc_query",
                           metasim_query_in_t,
                           metasim_query_out_t,
                           metasim_rpc_local_query_handle);
        rpc->id_ping =
            MARGO_REGISTER(mid, "metasim_rpc_ping",
                           metasim_ping_in_t,
                           metasim_ping_out_t,
                           metasim_rpc_local_ping_handle);
        rpc->id_rping =
            MARGO_REGISTER(mid, "metasim_rpc_rping",
                           metasim_rping_in_t,
                           metasim_rping_out_t,
                           metasim_rpc_local_rping_handle);
    }
}

#endif /* __METASIM_RPC_LOCAL_H */
