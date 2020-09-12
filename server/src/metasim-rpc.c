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
#include "metasim-rpc.h"
#include "metasim-rpc-tree.h"

struct rpc_set {
    hg_id_t ping;
    hg_id_t sum;
};

typedef struct rpc_set rpc_set_t;

static rpc_set_t rpcset;

extern metasim_server_t *metasim;

MERCURY_GEN_PROC(metasim_ping_in_t,
                 ((int32_t)(ping)));
MERCURY_GEN_PROC(metasim_ping_out_t,
                 ((int32_t)(pong)));
DECLARE_MARGO_RPC_HANDLER(metasim_rpc_handle_ping);

MERCURY_GEN_PROC(metasim_sum_in_t,
                 ((int32_t)(root))
                 ((int32_t)(seed)));
MERCURY_GEN_PROC(metasim_sum_out_t,
                 ((int32_t)(sum)));
DECLARE_MARGO_RPC_HANDLER(metasim_rpc_handle_sum);

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

/*
 * collective helpers
 */

typedef struct {
    hg_handle_t handle;
    margo_request req;
} corpc_req_t;

static int corpc_get_handle(hg_id_t rpc, int target, corpc_req_t *creq)
{
    int ret = 0;
    hg_return_t hret;
    hg_addr_t addr = metasim_get_rank_addr(metasim, target);

    hret = margo_create(metasim->mid, addr, rpc, &(creq->handle));
    if (hret != HG_SUCCESS) {
        __error("failed to create request (%p)", creq);
        ret = hret;
    }

    return ret;
}

static int corpc_forward_request(void *in, corpc_req_t *creq)
{
    int ret = 0;
    hg_return_t hret;

    hret = margo_iforward(creq->handle, in, &(creq->req));
    if (hret != HG_SUCCESS) {
        __error("failed to forward request (%p)", creq);
        ret = hret;
    }

    return ret;
}

static int corpc_wait_request(corpc_req_t *creq)
{
    int ret = 0;
    hg_return_t hret;

    hret = margo_wait(creq->req);
    if (hret != HG_SUCCESS) {
        __error("failed to wait for request (%p)", creq);
        ret = hret;
    }

    return ret;
}

/*
 * sum rpc (broadcasting)
 */

static int sum_forward(metasim_rpc_tree_t *tree,
                       metasim_sum_in_t *in, metasim_sum_out_t *out)
{
    int ret = 0;
    int i;
    int32_t partial_sum = 0;
    int child_count = tree->child_count;
    int *child_ranks = tree->child_ranks;
    corpc_req_t *req = NULL;

    if (child_count == 0) {
        out->sum = metasim->rank;
        __debug("i have no child (sum=%d)", metasim->rank);
        return 0;
    }

    __debug("bcasting sum to %d children:", child_count);

    for (i = 0; i < child_count; i++)
        __debug("child[%d] = %d", i, child_ranks[i]);

    /* forward requests to children in the rpc tree */
    req = calloc(child_count, sizeof(*req));
    if (!req) {
        __error("failed to allocate memory for corpc");
        return ENOMEM;
    }

    for (i = 0; i < child_count; i++) {
        corpc_req_t *r = &req[i];
        int child = child_ranks[i];

        ret = corpc_get_handle(rpcset.sum, child, r);
        if (ret) {
            __error("corpc_get_handle failed, abort rpc");
            goto out;
        }

        ret = corpc_forward_request((void *) in, r);
        if (ret) {
            __error("corpc_forward_request failed, abort rpc");
            goto out;
        }
    }

    /* collect results */
    for (i = 0; i < child_count; i++) {
        metasim_sum_out_t _out;
        corpc_req_t *r = &req[i];

        ret = corpc_wait_request(r);
        if (ret) {
            __error("corpc_wait_request failed, abort rpc");
            goto out;
        }

        /* TODO: check returns */
        margo_get_output(r->handle, &_out);
        partial_sum += _out.sum;

        __debug("sum from child[%d] (rank=%d): %d (sum=%d)",
                i, child_ranks[i], _out.sum, partial_sum);

        margo_free_output(r->handle, &_out);
        margo_destroy(r->handle);
    }

    out->sum = partial_sum + metasim->rank;

out:
    return ret;
}

static void metasim_rpc_handle_sum(hg_handle_t handle)
{
    int ret = 0;
    hg_return_t hret;
    metasim_rpc_tree_t tree;
    metasim_sum_in_t in;
    metasim_sum_out_t out;

    __debug("sum rpc handler");

    hret = margo_get_input(handle, &in);
    if (hret != HG_SUCCESS) {
        __error("margo_get_input failed");
        return;
    }

    /* TODO: check returns */
    metasim_rpc_tree_init(metasim->rank, metasim->nranks, in.root, 2, &tree);

    ret = sum_forward(&tree, &in, &out);
    if (ret)
        __error("sum_forward failed");

    metasim_rpc_tree_free(&tree);
    margo_free_input(handle, &in);

    margo_respond(handle, &out);

    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_rpc_handle_sum)

int metasim_rpc_invoke_sum(int32_t seed, int32_t *sum)
{
    int ret = 0;
    int32_t _sum = 0;
    metasim_rpc_tree_t tree;
    metasim_sum_in_t in;
    metasim_sum_out_t out;

    ret = metasim_rpc_tree_init(metasim->rank, metasim->nranks, metasim->rank,
                                2, &tree);
    if (ret) {
        __error("failed to initialize the rpc tree (ret=%d)", ret);
        return ret;
    }

    in.root = metasim->rank;
    in.seed = 0;

    ret = sum_forward(&tree, &in, &out);
    if (ret) {
        __error("sum_forward failed (ret=%d)", ret);
    } else {
       _sum = out.sum;
       __debug("rpc sum final result = %d", _sum);

       *sum = _sum;
    }

    metasim_rpc_tree_free(&tree);

    return ret;
}

/*
 * rpc: sum
 */

void metasim_rpc_register(void)
{
    rpcset.ping =
        MARGO_REGISTER(metasim->mid, "metasim_rpc_ping",
                       metasim_ping_in_t,
                       metasim_ping_out_t,
                       metasim_rpc_handle_ping);
    rpcset.sum =
        MARGO_REGISTER(metasim->mid, "metasim_rpc_sum",
                       metasim_sum_in_t,
                       metasim_sum_out_t,
                       metasim_rpc_handle_sum);
}

