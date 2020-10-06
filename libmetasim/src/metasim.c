/* Copyright (C) 2020 - UT-Battelle, LLC. All right reserved.
 *
 * Please refer to COPYING for the license.
 * ------------------------------------------------------------------------
 * Written by: Hyogi Sim <simh@ornl.gov>
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "metasim-common.h"
#include "metasim.h"

struct metasim_ctx {
    margo_instance_id mid;
    hg_addr_t listener_addr;
    metasim_rpcset_t rpc;
};

typedef struct metasim_ctx metasim_ctx_t;

#define DEBUG 1

#ifdef DEBUG
#define __debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define __debug(...)
#endif

#define __error __debug

#define metasim_ctx(m)  ((metasim_ctx_t *) (m))

/*
 * sending rpc to local listener
 */

int metasim_invoke_init(metasim_t metasim,
                        int32_t rank, int32_t pid,
                        int32_t *localrank, int32_t *nservers)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_init_in_t in;
    metasim_init_out_t out;

    if (!self)
        return EINVAL;

    rpc_id = self->rpc.init;
    in.rank = rank;
    in.pid = pid;

    margo_create(self->mid, self->listener_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);

    *localrank = out.rank;
    *nservers = out.nranks;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_terminate(metasim_t metasim, int32_t rank, int32_t pid)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_terminate_in_t in;
    metasim_terminate_out_t out;

    if (!self)
        return EINVAL;

    rpc_id = self->rpc.terminate;
    in.rank = rank;
    in.pid = pid;

    margo_create(self->mid, self->listener_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);

    ret = out.ret;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_echo(metasim_t metasim, int32_t num, int32_t *echo)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_echo_in_t in;
    metasim_echo_out_t out;

    if (!self)
        return EINVAL;

    rpc_id = self->rpc.echo;
    in.num = num;

    margo_create(self->mid, self->listener_addr, rpc_id, &handle);
    margo_forward(handle, &in);

     margo_get_output(handle, &out);

    *echo = out.echo;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_ping(metasim_t metasim,
                        int32_t target, int32_t ping, int32_t *pong)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_ping_in_t in;
    metasim_ping_out_t out;

    if (!self)
        return EINVAL;

    rpc_id = self->rpc.ping;
    in.target = target;
    in.ping = ping;

    margo_create(self->mid, self->listener_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);
    *pong = out.pong;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_sum(metasim_t metasim, int32_t seed, int32_t *sum,
                       uint64_t *elapsed_usec)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_sum_in_t in;
    metasim_sum_out_t out;

    if (!self)
        return EINVAL;

    rpc_id = self->rpc.sum;
    in.seed = seed;

    margo_create(self->mid, self->listener_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);
    ret = out.ret;
    *sum = out.sum;
    *elapsed_usec = out.elapsed_usec;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_sumrepeat(metasim_t metasim, int32_t seed, int32_t repeat,
                             int32_t *sum, uint64_t *elapsed_usec)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_sumrepeat_in_t in;
    metasim_sumrepeat_out_t out;

    if (!self)
        return EINVAL;

    rpc_id = self->rpc.sumrepeat;
    in.seed = seed;
    in.repeat = repeat;

    margo_create(self->mid, self->listener_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);
    ret = out.ret;
    *sum = out.sum;
    *elapsed_usec = out.elapsed_usec;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

static char *get_local_listener_addr(void)
{
    int ret = 0;
    FILE *fp = NULL;
    char *addr = NULL;
    char linebuf[LINE_MAX];

    /* wait untile the file becomes available */
    while (1) {
        ret = access(METASIM_LISTENER_ADDR_FILE, R_OK);
        if (!ret || errno != ENOENT)
            break;

        __debug("cannot access the addrfile yet, waiting..");

        sleep(2);
    }

    if (ret) {
        __error("failed to access the address file %s (err=%d, %s)",
                METASIM_LISTENER_ADDR_FILE, errno, strerror(errno));
        return NULL;
    }

    __debug("opening the addrfile");

    fp = fopen(METASIM_LISTENER_ADDR_FILE, "r");
    if (fp) {
        char *pos = fgets(linebuf, LINE_MAX-1, fp);

        if (pos) {
            pos = &linebuf[strlen(linebuf) - 1];
            if (isspace(pos[0]))
                pos[0] = '\0';

            addr = strdup(linebuf);
        }

        fclose(fp);
    } else {
        __debug("failed to open %s (%s)", METASIM_LISTENER_ADDR_FILE,
                strerror(errno));
    }

    return addr;
}

static char *read_addr_proto(char *listener_addr)
{
    char *proto = strdup(listener_addr);

    if (proto) {
        char *pos = strchr(proto, ':');
        pos[0] = '\0';
    }

    return proto;
}

static void register_rpc(metasim_ctx_t *self)
{
    margo_instance_id mid = self->mid;
    metasim_rpcset_t *rpc = &self->rpc;

    rpc->init =
        MARGO_REGISTER(mid, "listener_init",
                       metasim_init_in_t,
                       metasim_init_out_t,
                       NULL);
    rpc->terminate =
        MARGO_REGISTER(mid, "listener_terminate",
                       metasim_terminate_in_t,
                       metasim_terminate_out_t,
                       NULL);
    rpc->echo =
        MARGO_REGISTER(mid, "listener_echo",
                       metasim_echo_in_t,
                       metasim_echo_out_t,
                       NULL);
    rpc->ping =
        MARGO_REGISTER(mid, "listener_ping",
                       metasim_ping_in_t,
                       metasim_ping_out_t,
                       NULL);
    rpc->sum =
        MARGO_REGISTER(mid, "listener_sum",
                       metasim_sum_in_t,
                       metasim_sum_out_t,
                       NULL);
    rpc->sumrepeat =
        MARGO_REGISTER(mid, "listener_sumrepeat",
                       metasim_sumrepeat_in_t,
                       metasim_sumrepeat_out_t,
                       NULL);
}

static int init_rpc(metasim_ctx_t *self)
{
    int ret = 0;
    char *addr_str = NULL;
    char *proto = NULL;
    hg_addr_t addr = HG_ADDR_NULL;
    margo_instance_id mid = MARGO_INSTANCE_NULL;

    addr_str = get_local_listener_addr();
    if (!addr_str)
        return ENOTCONN;

    proto = read_addr_proto(addr_str);
    if (!proto) {
        free(addr_str);
        ret = ENOTCONN;
        goto out_free;
    }

    __debug("local listener address = %s, proto = %s\n", addr_str, proto);

    mid = margo_init(proto, MARGO_CLIENT_MODE, 0, 0);
    if (mid == MARGO_INSTANCE_NULL) {
        ret = ENOTCONN;
        goto out_free;
    }

    margo_addr_lookup(mid, addr_str, &addr);
    if (addr == HG_ADDR_NULL) {
        __debug("margo_addr_lookup failed");

        ret = ENOTCONN;
        margo_finalize(mid);
        goto out_free;
    }

    self->mid = mid;
    self->listener_addr = addr;

    register_rpc(self);

out_free:
    if (proto)
        free(proto);
    if (addr_str)
        free(addr_str);

    return ret;
}

metasim_t metasim_init(void)
{
    int ret = 0;
    metasim_ctx_t *self = NULL;

    self = calloc(1, sizeof(*self));
    if (self) {
        ret = init_rpc(self);
        if (ret) {
            free(self);
            self = NULL;
        }
    }

    return (metasim_t) self;
}

void metasim_exit(metasim_t metasim)
{
    metasim_ctx_t *self = metasim_ctx(metasim);

    if (self) {
        if (self->mid != MARGO_INSTANCE_NULL) {
            margo_addr_free(self->mid, self->listener_addr);
            margo_finalize(self->mid);
        }

        free(self);
        self = NULL;
    }
}

