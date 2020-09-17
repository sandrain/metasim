/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 * 
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
/* listener is responsible for listening local clients via shm margo server.
 */
#include <config.h>

#include <errno.h>
#include <assert.h>
#include <margo.h>
#include <abt.h>

#include "metasim-common.h"
#include "metasim-server.h"
#include "metasim-listener.h"

static margo_instance_id listener_mid;
static const int listener_default_pool_size = 4;

static hg_addr_t listener_addr;

/*
 * listener rpc handlers
 */

extern metasim_server_t *metasim;

static void metasim_listener_handle_init(hg_handle_t handle)
{
    metasim_init_in_t in;
    metasim_init_out_t out;

    margo_get_input(handle, &in);

    __debug("[RPC INIT] received rpc (rank=%d, pid=%d)", in.rank, in.pid);

    out.ret = 0;
    out.rank = metasim->rank;
    out.nranks = metasim->nranks;

    __debug("[RPC INIT] respoding rpc (rank=%d, nranks=%d)",
            metasim->rank, metasim->nranks);

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_init);

static void metasim_listener_handle_terminate(hg_handle_t handle)
{
    metasim_terminate_in_t in;
    metasim_terminate_out_t out;

    margo_get_input(handle, &in);

    __debug("[RPC TERMINATE] received rpc (rank=%d, pid=%d)", in.rank, in.pid);

    out.ret = 0;

    /* terminate server */

    __debug("[RPC TERMINATE] respoding rpc (ret=0)");

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_terminate);

static void metasim_listener_handle_echo(hg_handle_t handle)
{
    int32_t num;
    int32_t echo;
    metasim_echo_in_t in;
    metasim_echo_out_t out;

    margo_get_input(handle, &in);
    num = in.num;

    echo = num;
    out.echo = echo;

    __debug("[RPC ECHO] (num=%d) => (echo=%d)", num, echo);

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_echo);

static void metasim_listener_handle_ping(hg_handle_t handle)
{
    int target;
    int ping;
    metasim_ping_in_t in;
    metasim_ping_out_t out;

    margo_get_input(handle, &in);
    target = in.target;
    ping = in.ping;

    __debug("[RPC PING] received rpc (target=%d, ping=%d)", target, ping);

    out.ret = 0;
    out.pong = ping;

    __debug("[RPC PING] respoding rpc (ret=0, pong=%d)", ping);

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_ping);

static void metasim_listener_handle_sum(hg_handle_t handle)
{
    int seed;
    metasim_sum_in_t in;
    metasim_sum_out_t out;

    margo_get_input(handle, &in);
    seed = in.seed;

    __debug("[RPC SUM] received rpc (seed=%d)", seed);

    out.ret = 0;
    out.sum = seed;

    __debug("[RPC SUM] respoding rpc (ret=0, sum=%d)", seed);

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_sum);

static void listener_register_rpc(margo_instance_id mid)
{
    MARGO_REGISTER(mid, "listener_init",
                   metasim_init_in_t,
                   metasim_init_out_t,
                   metasim_listener_handle_init);

    MARGO_REGISTER(mid, "listener_terminate",
                   metasim_terminate_in_t,
                   metasim_terminate_out_t,
                   metasim_listener_handle_terminate);

    MARGO_REGISTER(mid, "listener_echo",
                   metasim_echo_in_t,
                   metasim_echo_out_t,
                   metasim_listener_handle_echo);

    MARGO_REGISTER(mid, "listener_ping",
                   metasim_ping_in_t,
                   metasim_ping_out_t,
                   metasim_listener_handle_ping);

    MARGO_REGISTER(mid, "listener_sum",
                   metasim_sum_in_t,
                   metasim_sum_out_t,
                   metasim_listener_handle_sum);
}

int metasim_listener_init(void)
{
    int ret = 0;
    char addrstr[128];
    size_t addrstr_len = 128;
    FILE *fp = NULL;
    margo_instance_id mid;
    hg_addr_t addr;
    hg_return_t hret;

    __debug("launching listener");

    mid = margo_init("na+sm://", MARGO_SERVER_MODE, 1,
                     listener_default_pool_size);
    if (mid == MARGO_INSTANCE_NULL) {
        __error("failed to initialize margo for listener");
        return EIO;
    }

    __debug("listener margo initialized");

    hret = margo_addr_self(mid, &addr);
    assert(hret == HG_SUCCESS);

    hret = margo_addr_to_string(mid, addrstr, &addrstr_len, addr);
    assert(hret == HG_SUCCESS);

    __debug("listener address: %s", addrstr);

    listener_register_rpc(mid);

    __debug("publishing the address (%s)", addrstr);

    fp = fopen(METASIM_LISTENER_ADDR_FILE, "w");
    if (fp) {
        fprintf(fp, "%s", addrstr);
        fclose(fp);

        __debug("listener address file created: %s (addr=%s)",
                METASIM_LISTENER_ADDR_FILE, addrstr);
    } else {
        __error("failed to publish address file (%s)",
                METASIM_LISTENER_ADDR_FILE);
        return errno;
    }

    listener_mid = mid;
    listener_addr = addr;

    __debug("listener launched, listening on %s", addrstr);

    return ret;
}

int metasim_listener_exit(void)
{
    int ret = 0;

    margo_finalize(listener_mid);

    return ret;
}

