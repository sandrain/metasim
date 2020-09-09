/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include "metasim-comm.h"
#include "metasim-rpc-local.h"

extern metasim_comm_t *metasim_comm;

static int invoke_remote_ping(int32_t rank, int32_t ping, int32_t *pong)
{
    int ret = 0;
    hg_handle_t handle = 0;
    hg_id_t rpc_id;
    metasim_ping_in_t in;
    metasim_ping_out_t out;
    hg_addr_t addr = metasim_comm_get_rank_addr(metasim_comm, rank);
    margo_instance_id mid = metasim_comm->smid;
    margo_request req;

    rpc_id = metasim_comm->remote_rpc.id_ping;
    in.ping = ping;

    hg_return_t hret = margo_create(mid, addr, rpc_id, &handle);
    if (hret != HG_SUCCESS) {
        __error("failed to create margo instance");
        ret = -1;
        goto out;
    }

    __debug("forwarding ping to remote rank %d", rank);

    hret = margo_iforward(handle, &in, &req);
    if (hret != HG_SUCCESS) {
        __error("failed to forward rpc");
        ret = -1;
        goto out;
    }

    margo_wait(req);

    margo_get_output(handle, &out);
    *pong = out.pong;

out:
    margo_free_output(handle, &out);

    if (handle != HG_HANDLE_NULL)
        margo_destroy(handle);

    return ret;
}

static void metasim_rpc_local_query_handle(hg_handle_t handle)
{
    hg_return_t ret = HG_SUCCESS;
    metasim_query_in_t in;
    metasim_query_out_t out;

    __debug("received query rpc");

    ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        __error("margo_get_input failed");
        out.localrank = (int32_t) -1;
    } else {
        int32_t localrank = metasim_comm->rank;
        int32_t nservers = metasim_comm->size;

        __debug("responding (localrank=%d, nservers=%d)",
                localrank, nservers);

        out.localrank = localrank;
        out.nservers = nservers;
    }

    ret = margo_respond(handle, &out);
    if (ret != HG_SUCCESS) {
        __error("margo_respond failed");
    }

    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_rpc_local_query_handle)

static void metasim_rpc_local_ping_handle(hg_handle_t handle)
{
    hg_return_t ret = HG_SUCCESS;
    metasim_ping_in_t in;
    metasim_ping_out_t out;

    __debug("received ping rpc");

    ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        __error("margo_get_input failed");
        out.pong = (int32_t) -1;
    } else {
        int32_t ping = in.ping;
        int32_t pong = ping;

        out.pong = pong;

        __debug("responding (ping=%d, pong=%d)", ping, pong);
    }

    ret = margo_respond(handle, &out);
    if (ret != HG_SUCCESS) {
        __error("margo_respond failed");
    }

    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_rpc_local_ping_handle)

static void metasim_rpc_local_rping_handle(hg_handle_t handle)
{
    hg_return_t ret = HG_SUCCESS;
    metasim_rping_in_t in;
    metasim_rping_out_t out;

    __debug("received rping rpc");

    ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        __error("margo_get_input failed");
        out.pong = (int32_t) -1;
    } else {
        int32_t ping = in.ping;
        int32_t server = in.server % metasim_comm->size;
        int32_t pong = 0;

        if (server == metasim_comm->rank) {
            __debug("target rank is me, will reply directly");

            pong = in.ping;
        } else {
            __debug("forwarding pingpong to rank %d", server);

            invoke_remote_ping(server, ping, &pong);

            __debug("response from rank %d: pong=%d", server, pong);
        }
        __debug("responding to client (ping=%d, pong=%d)", ping, server);
    }

    ret = margo_respond(handle, &out);
    if (ret != HG_SUCCESS) {
        __error("margo_respond failed");
    }

    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_rpc_local_rping_handle)

