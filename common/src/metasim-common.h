#ifndef __METASIM_COMMON_H
#define __METASIM_COMMON_H

#include <margo.h>

#define METASIM_LISTENER_ADDR_FILE "/tmp/metasim-listener-addr"

/* local rpc with metasim listener */

struct metasim_rpcset {
    hg_id_t init;
    hg_id_t terminate;
    hg_id_t ping;
    hg_id_t sum;
};

typedef struct metasim_rpcset metasim_rpcset_t;

MERCURY_GEN_PROC(metasim_init_in_t,
                 ((int32_t)(rank))
                 ((int32_t)(pid)));
MERCURY_GEN_PROC(metasim_init_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(rank))
                 ((int32_t)(nranks)));
DECLARE_MARGO_RPC_HANDLER(metasim_listener_handle_init);

MERCURY_GEN_PROC(metasim_terminate_in_t,
                 ((int32_t)(rank))
                 ((int32_t)(pid)));
MERCURY_GEN_PROC(metasim_terminate_out_t,
                 ((int32_t)(ret)));
DECLARE_MARGO_RPC_HANDLER(metasim_listener_handle_terminate);

MERCURY_GEN_PROC(metasim_ping_in_t,
                 ((int32_t)(target))
                 ((int32_t)(ping)));
MERCURY_GEN_PROC(metasim_ping_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(pong)));
DECLARE_MARGO_RPC_HANDLER(metasim_listener_handle_ping);

MERCURY_GEN_PROC(metasim_sum_in_t,
                 ((int32_t)(seed)));
MERCURY_GEN_PROC(metasim_sum_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(sum)));
DECLARE_MARGO_RPC_HANDLER(metasim_listener_handle_sum);

#endif /* __METASIM_COMMON_H */

