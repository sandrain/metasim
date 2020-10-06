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
#include <time.h>
#include <assert.h>
#include <margo.h>

#include "metasim-common.h"
#include "metasim-server.h"
#include "metasim-listener.h"
#include "metasim-rpc.h"

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

    print_margo_handler_pool_size(listener_mid);

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

    print_margo_handler_pool_size(listener_mid);

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
    int ret = 0;
    int32_t target;
    int32_t ping;
    int32_t pong;
    metasim_ping_in_t in;
    metasim_ping_out_t out;

    print_margo_handler_pool_size(listener_mid);

    margo_get_input(handle, &in);
    target = in.target;
    ping = in.ping;

    if (target >= metasim->nranks)
        target = target % metasim->nranks;

    __debug("[RPC PING] received & forwarding rpc (target=%d, ping=%d)",
            target, ping);

    ret = metasim_rpc_invoke_ping(target, ping, &pong);
    if (ret) {
        __error("metasim_rpc_invoke_ping failed, will return -1 (ret=%d)",
                ret);
        pong = -1;
    }

    out.pong = pong;

    __debug("[RPC PING] respoding rpc (pong=%d)", pong);

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_ping);

static uint64_t
calculate_elapsed_usec(struct timespec *t1, struct timespec *t2)
{
    double ns = .0f;

    ns = (t2->tv_sec*1e9 + t2->tv_nsec) - (t1->tv_sec*1e9 + t1->tv_nsec);

    return (uint64_t) ns*1e-3;
}

static void metasim_listener_handle_sum(hg_handle_t handle)
{
    int ret = 0;
    int32_t seed = 0;
    int32_t sum = 0;
    metasim_sum_in_t in;
    metasim_sum_out_t out;
    struct timespec start, stop;
    uint64_t usec = 0;

    print_margo_handler_pool_size(listener_mid);

    margo_get_input(handle, &in);
    seed = in.seed;

    __debug("[RPC SUM] received & forwarding rpc (seed=%d)", seed);

    clock_gettime(CLOCK_REALTIME, &start);

    ret = metasim_rpc_invoke_sum(seed, &sum);

    clock_gettime(CLOCK_REALTIME, &stop);

    if (ret) {
        __error("metasim_rpc_invoke_ping failed, will return -1 (ret=%d)",
                ret);
        sum = -1;
    }

    usec = calculate_elapsed_usec(&start, &stop);

    __debug("[RPC SUM] respoding rpc (sum=%d, usec=%llu)",
            sum, (unsigned long long) usec);

    out.ret = ret;
    out.sum = sum;
    out.elapsed_usec = usec;

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_sum);

static void metasim_listener_handle_sumrepeat(hg_handle_t handle)
{
    int ret = 0;
    int32_t i = 0;
    int32_t seed = 0;
    int32_t repeat = 0;
    int32_t sum = 0;
    metasim_sumrepeat_in_t in;
    metasim_sumrepeat_out_t out;
    struct timespec start, stop;
    uint64_t usec = 0;

    print_margo_handler_pool_size(listener_mid);

    margo_get_input(handle, &in);
    seed = in.seed;
    repeat = in.repeat;

    __debug("[RPC SUMREPEAT] received & forwarding rpc (seed=%d, repeat=%d)",
            seed, repeat);

    /* the 1st run took longer than the subsequent runs */
    clock_gettime(CLOCK_REALTIME, &start);

    ret = metasim_rpc_invoke_sum(seed, &sum);

    clock_gettime(CLOCK_REALTIME, &stop);

    if (ret) {
        __error("metasim_rpc_invoke_ping failed, will return -1 (ret=%d)",
                ret);
        sum = -1;
    }

    usec = calculate_elapsed_usec(&start, &stop);
    __debug("[RPC SUMREPEAT] first sum took %llu", (unsigned long long) usec);

    /* now repeat and measure the time */
    ret = 0;
    clock_gettime(CLOCK_REALTIME, &start);

    for (i = 0; i < repeat; i++)
        ret |= metasim_rpc_invoke_sum(seed, &sum);

    clock_gettime(CLOCK_REALTIME, &stop);

    if (ret) {
        __error("metasim_rpc_invoke_ping failed, will return -1 (ret=%d)",
                ret);
        sum = -1;
    }

    usec = calculate_elapsed_usec(&start, &stop);

    __debug("[RPC SUM] respoding rpc (sum=%d, usec=%llu)",
            sum, (unsigned long long) usec);

    out.ret = ret;
    out.sum = sum;
    out.elapsed_usec = usec;

    margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);
}
DEFINE_MARGO_RPC_HANDLER(metasim_listener_handle_sumrepeat);

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

    MARGO_REGISTER(mid, "listener_sumrepeat",
                   metasim_sumrepeat_in_t,
                   metasim_sumrepeat_out_t,
                   metasim_listener_handle_sumrepeat);
}

int metasim_listener_init(void)
{
    int ret = 0;
    char addrstr[512];
    size_t addrstr_len = 512;
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

