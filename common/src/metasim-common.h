#ifndef __METASIM_COMMON_H
#define __METASIM_COMMON_H

#include <margo.h>

/* local rpc with metasim listener */

struct metasim_rpcset {
    hg_id_t ping;
};

typedef struct metasim_rpcset metasim_rpcset_t;

MERCURY_GEN_PROC(metasim_ping_in_t,
                 ((int32_t)(target))
                 ((int32_t)(ping)));
MERCURY_GEN_PROC(metasim_ping_out_t,
                 ((int32_t)(ret))
                 ((int32_t)(pong)));

#endif /* __METASIM_COMMON_H */

