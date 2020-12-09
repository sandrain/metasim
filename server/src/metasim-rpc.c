/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include <errno.h>
#include <margo.h>

#include "metasim-server.h"
#include "metasim-common.h"
#include "metasim-rpc.h"

struct rpc_set {
    hg_id_t ping;
};

typedef struct rpc_set rpc_set_t;

static rpc_set_t rpcset;

extern metasim_server_t *metasim;

DECLARE_MARGO_RPC_HANDLER(metasim_rpc_handle_ping);

/*
 * rpc: ping
 */
static void metasim_rpc_handle_ping(hg_handle_t handle)
{
    hg_return_t ret = HG_SUCCESS;
    metasim_ping_in_t in;
    metasim_ping_out_t out;
    int32_t ping = 0;
    int32_t pong = 0;

    __debug("received ping request");

    ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        __error("margo_get_input failed");
        out.pong = (int32_t) -1;
        goto out;
    } else {
        ping = in.ping;
        pong = ping;
        out.pong = pong;
    }

    __debug("responding (ping=%d, pong=%d)", ping, pong);

    ret = margo_respond(handle, &out);
    if (ret != HG_SUCCESS)
        __error("margo_respond failed");

out:
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_rpc_handle_ping)

#if 0
int metasim_rpc_invoke_ping(int32_t targetrank, int32_t ping, int32_t *pong)
{
    int ret = 0;
    hg_handle_t handle = 0;
    hg_addr_t addr;
    metasim_ping_in_t in;
    metasim_ping_out_t out;
    int32_t nranks = metasim->nranks;

    if (targetrank > nranks - 1)
        return EINVAL;

    addr = metasim_get_rank_addr(metasim, targetrank);
    in.ping = ping;

    hg_return_t hret = margo_create(metasim->mid, addr, rpcset.ping, &handle);
    if (hret != HG_SUCCESS) {
        __error("failed to create margo instance");
        goto out;
    }

    hret = margo_forward(handle, &in);

    margo_get_output(handle, &out);
    *pong = out.pong;

out:
    margo_free_output(handle, &out);

    if (handle != HG_HANDLE_NULL)
        margo_destroy(handle);

    return ret;
}
#endif

void metasim_rpc_register(void)
{
    rpcset.ping =
        MARGO_REGISTER(metasim->mid, "ping",
                       metasim_ping_in_t,
                       metasim_ping_out_t,
                       metasim_rpc_handle_ping);
}

