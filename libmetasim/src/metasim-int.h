#ifndef __METASIM_INT_H
#define __METASIM_INT_H

#include <margo.h>
#include "metasim-rpc.h"

struct metasim_ctx {
    margo_instance_id mid;
    hg_addr_t server_addr;
    metasim_rpc_set_t rpc;
};

typedef struct metasim_ctx metasim_ctx_t;

#endif /* __METASIM_INT_H */

