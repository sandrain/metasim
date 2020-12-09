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
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "metasim-common.h"
#include "metasim.h"

struct metasim_ctx {
    margo_instance_id mid;
    metasim_rpcset_t rpc;
    int nservers;
    hg_addr_t *server_addrs;
};

typedef struct metasim_ctx metasim_ctx_t;

static int metasim_log_error = 1;
static int metasim_log_debug = 1;

static inline pid_t __gettid(void)
{
#if defined(gettid)
    return gettid();
#elif defined(SYS_gettid)
    return syscall(SYS_gettid);
#else
#error no gettid()
#endif
    return -1;
}

#define __libmetasim_log(mask, ...)                                           \
        do {                                                                  \
            if (mask) {                                                       \
                time_t now = time(NULL);                                      \
                struct tm *ltime = localtime(&now);                           \
                char timestampstr[128];                                       \
                strftime(timestampstr, sizeof(timestampstr),                  \
                         "%Y-%m-%dT%H:%M:%S", ltime);                         \
                FILE *out = stderr;                                           \
                char *file = strrchr(__FILE__, '/');                          \
                fprintf(out, "%s tid=%d @ %s()[%s:%d] ", timestampstr,        \
                             __gettid(), __func__, &file[1], __LINE__);       \
                fprintf(out, __VA_ARGS__);                                    \
                fprintf(out, "\n");                                           \
                fflush(out);                                                  \
            }                                                                 \
        } while (0)

#define __error(...)  __libmetasim_log(metasim_log_error, __VA_ARGS__)
#define __debug(...)  __libmetasim_log(metasim_log_debug, __VA_ARGS__)

#define metasim_ctx(m)  ((metasim_ctx_t *) (m))

/*
 * sending rpc to local listener
 */

int metasim_invoke_ping(metasim_t metasim,
                        int32_t id, int32_t ping, int32_t *pong)
{
    int ret = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);
    hg_handle_t handle;
    hg_id_t rpc_id;
    hg_addr_t addr;
    hg_return_t hret;
    metasim_ping_in_t in;
    metasim_ping_out_t out;
    int target = id % self->nservers;

    if (!self)
        return EINVAL;

    addr = self->server_addrs[target];

    rpc_id = self->rpc.ping;
    in.target = target;
    in.ping = ping;

    hret = margo_create(self->mid, addr, rpc_id, &handle);
    if (hret != HG_SUCCESS) {
        __error("failed to create margo instance");
        ret = EIO;
        goto out;
    }

    hret = margo_forward(handle, &in);
    if (hret != HG_SUCCESS) {
        __error("failed to forward rpc (ret=%d)", hret);
        ret = EIO;
        goto out;
    }

    margo_get_output(handle, &out);
    *pong = out.pong;

    margo_free_output(handle, &out);
    margo_destroy(handle);

out:
    return ret;
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

    rpc->ping =
        MARGO_REGISTER(mid, "ping",
                       metasim_ping_in_t,
                       metasim_ping_out_t,
                       NULL);
}

static int init_rpc(metasim_ctx_t *self)
{
    int ret = 0;
    int i = 0;
    int nservers = 0;
    hg_addr_t *server_addrs = NULL;
    margo_instance_id mid = MARGO_INSTANCE_NULL;
    const char *server_address_file = NULL;
    FILE *fp = NULL;
    char *line = NULL;
    char linebuf[256];

    __debug("initializing client rpc");

    server_address_file = getenv("METASIMD_ADDRESS_FILE");
    if (!server_address_file) {
        __error("failed to lookup env[METASIM_ADDRESS_FILE]");
        return ENOTCONN;
    }

    while (i < 10) {
        fp = fopen(server_address_file, "r");
        if (!fp) {
            if (errno == ENOENT) {
                /* allow some more time .. */
                i++;
                __debug("address file (%s) not found, waiting 5 sec "
                        "(try: %d)..", server_address_file, i);
                sleep(5);
            } else {
                __error("failed to open server address file %s (%s)",
                        server_address_file, strerror(errno));
                return errno;
            }
        } else {
            break;
        }
    }

    if (NULL == (line = fgets(linebuf, 256, fp))) {
        __error("failed to read a line (%s)", strerror(errno));
        ret = errno;
        goto out_close;
    }

    nservers = atoi(line);
    server_addrs = calloc(nservers, sizeof(hg_addr_t));
    if (!server_addrs) {
        __error("failed to allocate memory to store server addresses");
        ret = errno;
        goto out_close;
    }

    for (i = 0; i < nservers; i++) {
        char *addr_str = NULL;
        hg_addr_t addr = HG_ADDR_NULL;

        if (NULL == (line = fgets(linebuf, 256, fp))) {
            /* TODO: check errors */
        }

        addr_str = strchr(line, ',') + 1;
        addr_str[strlen(addr_str)-1] = '\0';

        if (i == 0) {
            char *proto = read_addr_proto(addr_str);
            if (!proto) {
                __error("failed to read the network protocol");
                ret = ENOTCONN;
                goto out_free_addr;
            }

            /* initialize margo using the protocol */
            mid = margo_init(proto, MARGO_CLIENT_MODE, 0, 0);

            free(proto);

            if (mid == MARGO_INSTANCE_NULL) {
                __error("failed to initialize margo");
                ret = ENOTCONN;
                goto out_free_addr;
            }

            __debug("margo initialized..");
        }

        margo_addr_lookup(mid, addr_str, &addr);
        if (addr == HG_ADDR_NULL) {
            __debug("margo_addr_lookup failed for %s", addr_str);

            ret = ENOTCONN;
            margo_finalize(mid);
            goto out_free_addr;
        }

        server_addrs[i] = addr;

        __debug("found server %d: %s", i, addr_str);
    }

    fclose(fp);

    self->mid = mid;
    self->nservers = nservers;
    self->server_addrs = server_addrs;

    register_rpc(self);

    return ret;

out_free_addr:
    if (server_addrs)
        free(server_addrs);
out_close:
    fclose(fp);

    return ret;
}

int metasim_get_server_count(metasim_t metasim)
{
    int count = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);

    if (self)
        count = self->nservers;

    return count;
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
    } else {
        __error("failed to allocate memory");
    }

    return (metasim_t) self;
}

void metasim_exit(metasim_t metasim)
{
    int i = 0;
    metasim_ctx_t *self = metasim_ctx(metasim);

    if (self) {
        for (i = 0; i < self->nservers; i++)
            margo_addr_free(self->mid, self->server_addrs[i]);

        if (self->mid != MARGO_INSTANCE_NULL)
            margo_finalize(self->mid);

        if (self->server_addrs)
            free(self->server_addrs);

        free(self);
        self = NULL;
    }
}

