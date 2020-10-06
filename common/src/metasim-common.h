#ifndef __METASIM_COMMON_H
#define __METASIM_COMMON_H

#include <margo.h>

#define METASIM_LISTENER_ADDR_FILE "/tmp/metasim-listener-addr"

/* local rpc with metasim listener */

struct metasim_rpcset {
    hg_id_t init;
    hg_id_t terminate;
    hg_id_t echo;
    hg_id_t ping;
    hg_id_t sum;
    hg_id_t sumrepeat;
};

typedef struct metasim_rpcset metasim_rpcset_t;

MERCURY_GEN_PROC(metasim_init_in_t,
                 ((int32_t)(rank))
                 ((int32_t)(pid)));
MERCURY_GEN_PROC(metasim_init_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(rank))
                 ((int32_t)(nranks)));

MERCURY_GEN_PROC(metasim_terminate_in_t,
                 ((int32_t)(rank))
                 ((int32_t)(pid)));
MERCURY_GEN_PROC(metasim_terminate_out_t,
                 ((int32_t)(ret)));

MERCURY_GEN_PROC(metasim_echo_in_t,
                 ((int32_t)(num)));
MERCURY_GEN_PROC(metasim_echo_out_t,
                 ((int32_t)(echo)));

MERCURY_GEN_PROC(metasim_ping_in_t,
                 ((int32_t)(target))
                 ((int32_t)(ping)));
MERCURY_GEN_PROC(metasim_ping_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(pong)));

MERCURY_GEN_PROC(metasim_sum_in_t,
                 ((int32_t)(seed)));
MERCURY_GEN_PROC(metasim_sum_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(sum))
                 ((uint64_t)(elapsed_usec)));

MERCURY_GEN_PROC(metasim_sumrepeat_in_t,
                 ((int32_t)(seed))
                 ((int32_t)(repeat)));
MERCURY_GEN_PROC(metasim_sumrepeat_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(sum))
                 ((uint64_t)(elapsed_usec)));

#endif /* __METASIM_COMMON_H */

