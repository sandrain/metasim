#ifndef __METASIM_RPC_REMOTE_H
#define __METASIM_RPC_REMOTE_H

#include <margo.h>

#include "metasim-rpc.h"

DECLARE_MARGO_RPC_HANDLER(metasim_rpc_remote_ping_handle);

struct metasim_remote_rpc_set {
    hg_id_t id_ping;
};

typedef struct metasim_remote_rpc_set metasim_remote_rpc_set_t;

static inline void metasim_rpc_remote_register(margo_instance_id mid,
                                               metasim_remote_rpc_set_t *rpc)
{
    if (rpc) {
        rpc->id_ping = MARGO_REGISTER(mid, "metasim_rpc_ping",
                                      metasim_ping_in_t,
                                      metasim_ping_out_t,
                                      metasim_rpc_remote_ping_handle);
    }
}

#endif /* __METASIM_RPC_REMOTE_H */

