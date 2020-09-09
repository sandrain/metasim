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

#include "metasim-rpc.h"
#include "metasim-int.h"

#include "metasim.h"

#define DEBUG 1

#ifdef DEBUG
#define __debug(...) fprintf(stderr, __VA_ARGS__) 
#else
#define __debug(...)
#endif

#define metasim_ctx(m)  ((metasim_ctx_t *) (m))

static char *get_local_server_addr(void)
{
    FILE *fp = NULL;
    char *addr = NULL;
    char linebuf[LINE_MAX];

    fp = fopen(METASIM_RPC_LOCAL_ADDR_FILE, "r");
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
        __debug("failed to open %s (%s)", METASIM_RPC_LOCAL_ADDR_FILE,
                strerror(errno));
    }

    return addr;
}

static void cleanup(metasim_ctx_t *self)
{
    if (self)
        free(self);
}

static char *read_addr_proto(char *server_addr)
{
    char *proto = strdup(server_addr);

    if (proto) {
        char *pos = strchr(proto, ':');
        pos[0] = '\0';
    }

    return proto;
}

static int init_rpc(metasim_ctx_t *self)
{
    int ret = 0;
    char *addr_str = NULL;
    char *proto = NULL;
    hg_addr_t addr = HG_ADDR_NULL;
    margo_instance_id mid = MARGO_INSTANCE_NULL;

    addr_str = get_local_server_addr();
    if (!addr_str)
        return ENOTCONN;

    proto = read_addr_proto(addr_str);
    if (!proto) {
        free(addr_str);
        ret = ENOTCONN;
        goto out_free;
    }

    __debug("local server address = %s, proto = %s\n", addr_str, proto);

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
    self->server_addr = addr;
    metasim_rpc_register_client(mid, &self->rpc);

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
            cleanup(self);
            self = NULL;
        }
    }

    return (metasim_t) self;
}

void metasim_exit(metasim_t metasim)
{
    metasim_ctx_t *self = metasim_ctx(metasim);

    if (self) {
        if (self->mid != MARGO_INSTANCE_NULL)
            margo_finalize(self->mid);

        cleanup(self);
    }
}

int metasim_invoke_query(metasim_t metasim,
                         int32_t *localrank, int32_t *nservers)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_query_in_t in;
    metasim_query_out_t out;

    if (!self)
        return EINVAL;
    
    rpc_id = self->rpc.id_query;

    margo_create(self->mid, self->server_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);

    *localrank = out.localrank;
    *nservers = out.nservers;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_ping(metasim_t metasim, int32_t ping, int32_t *pong)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    metasim_ping_in_t in;
    metasim_ping_out_t out;

    if (!self)
        return EINVAL;
    
    rpc_id = self->rpc.id_ping;
    in.ping = ping;

    margo_create(self->mid, self->server_addr, rpc_id, &handle);
    margo_forward(handle, &in);

    margo_get_output(handle, &out);
    *pong = out.pong;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

int metasim_invoke_rping(metasim_t metasim,
                         int32_t server, int32_t ping, int32_t *pong)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    margo_request req;
    hg_id_t rpc_id;
    metasim_rping_in_t in;
    metasim_rping_out_t out;

    if (!self)
        return EINVAL;
    
    rpc_id = self->rpc.id_rping;
    in.server = server;
    in.ping = ping;

    margo_create(self->mid, self->server_addr, rpc_id, &handle);

    margo_iforward(handle, &in, &req);
    margo_wait(req);

    margo_get_output(handle, &out);
    *pong = out.pong;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return ret;
}

