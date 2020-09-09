/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include "metasim-comm.h"
#include "metasim-rpc-remote.h"

extern metasim_comm_t *metasim_comm;

static void metasim_rpc_remote_ping_handle(hg_handle_t handle)
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
DEFINE_MARGO_RPC_HANDLER(metasim_rpc_remote_ping_handle)

