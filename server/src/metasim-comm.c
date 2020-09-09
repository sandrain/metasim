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
#include <errno.h>
#include <assert.h>
#include <limits.h>

#include "metasim-rpc-local.h"
#include "metasim-rpc-remote.h"
#include "metasim-comm.h"

metasim_comm_t *metasim_comm;

static ssg_group_config_t ssg_config = {
    .swim_period_length_ms = 3000,
    .swim_suspect_timeout_periods = 5,
    .swim_subgroup_member_count = -1,
    .ssg_credential = -1,
};

static void ssg_group_update_cb(void *unused, ssg_member_id_t id,
                                ssg_member_update_type_t type)
{
    switch (type) {
    case SSG_MEMBER_JOINED:
        __debug("member %ld joined", id);
        break;

    case SSG_MEMBER_LEFT:
        __debug("member %ld left", id);
        break;

    case SSG_MEMBER_DIED:
        __debug("member %ld died", id);
        break;
    }
}

static int comm_init_remote(metasim_comm_t *self, metasim_config_t *config)
{
    int ret = 0;
    int size = 0;
    int i = 0;
    pmix_proc_t proc;
    ssg_member_id_t rank;
    hg_addr_t *addr = NULL;

    ret = PMIx_Init(&proc, NULL, 0);
    if (ret != PMIX_SUCCESS) {
        __error("PMIx_Init failed: %s", PMIx_Error_string(ret));
        return ret;
    }

    margo_instance_id mid = margo_init("bmi+tcp://", MARGO_SERVER_MODE, 1,
                                       config->rpc_server_thread_pool_size);
    if (mid == MARGO_INSTANCE_NULL) {
        __error("failed to initialize margo");
        return EIO;
    }

    metasim_rpc_remote_register(mid, &self->remote_rpc);

    ret = ssg_init();
    if (ret != SSG_SUCCESS) {
        __error("ssg_init() failed");
        return ret;
    }

    ssg_group_id_t gid = ssg_group_create_pmix(mid, "metasim_ssg",
            proc, &ssg_config, ssg_group_update_cb, NULL);

    rank = ssg_get_group_self_rank(gid);
    size = ssg_get_group_size(gid);

    __debug("ssg group (gid=%lu, rank=%d, size=%d)",
            (unsigned long) gid, (int) rank, size);

    addr = calloc(size, sizeof(*addr));
    if (!addr) {
        __error("failed to allocate memory");
        return ENOMEM;
    }

    for (i = 0; i < size; i++) {
        ssg_member_id_t memid = ssg_get_group_member_id_from_rank(gid, i);
        addr[i] = ssg_get_group_member_addr(gid, memid);
#if 0
        char hg_addr_str[128];
        size_t hg_addr_str_size = 128;

        margo_addr_to_string(mid, hg_addr_str, &hg_addr_str_size, addr);

        __debug("[rank=%d] (member_id=%lu, addr=%s)",
                i, (unsigned long) memid, hg_addr_str);
#endif
    }

    self->smid = mid;
    self->rank = rank;
    self->size = size;
    self->gid = gid;
    self->addrs = addr;

    return ret;
}

static int comm_publish_local_addr(const char *addr_str)
{
    int ret = 0;
    FILE *fp = NULL;

    fp = fopen(METASIM_RPC_LOCAL_ADDR_FILE, "w+");
    if (fp) {
        fprintf(fp, "%s\n", addr_str);
        fclose(fp);
    } else {
        ret = errno;
    }

    return ret;
}

static int comm_init_local(metasim_comm_t *self, metasim_config_t *config)
{
    int ret = 0;
    margo_instance_id mid = MARGO_INSTANCE_NULL;
    hg_addr_t addr = HG_ADDR_NULL;
    size_t addr_str_len = 128;
    char addr_str[128];

    mid = margo_init("na+sm://", MARGO_SERVER_MODE, 1,
                     config->rpc_client_thread_pool_size);
    if (mid == MARGO_INSTANCE_NULL) {
        __error("failed to initialize margo");
        return EIO;
    }

    margo_addr_self(mid, &addr);
    margo_addr_to_string(mid, addr_str, &addr_str_len, addr);

    __debug("shm margo rpc server: %s", addr_str);

    ret = comm_publish_local_addr(addr_str);
    if (ret) {
        __error("failed to publish addr file %s (%s)",
                METASIM_RPC_LOCAL_ADDR_FILE, strerror(ret));
        return ret;
    }

    metasim_rpc_register_server(mid, &self->local_rpc);

    self->cmid = mid;

    return ret;
}


int metasim_comm_init(metasim_comm_t *comm, metasim_config_t *config)
{
    int ret = 0;

    if (!comm || !config)
        return EINVAL;

    memset(comm, sizeof(*comm), 0);

#if 0
    ret = comm_init_local(comm, config);
    if (ret) {
        __error("failed to initialize local client connections");
        goto out;
    }
#endif

    ret = comm_init_remote(comm, config);
    if (ret) {
        __error("failed to initialize remote server connections");
        goto out;
    }

    metasim_comm = comm;

out:
    return ret;
}

int metasim_comm_exit(metasim_comm_t *comm)
{
    int ret = 0;

    if (comm) {
        ssg_group_leave(comm->gid);
        margo_finalize(comm->smid);
        PMIx_Finalize(NULL, 0);
    }

    return ret;
}

hg_addr_t metasim_comm_get_rank_addr(metasim_comm_t *comm, int rank)
{
    hg_addr_t addr = HG_ADDR_NULL;
    char addrstr[256];
    size_t len = 256;

    if (comm) {
#if 0
        ssg_group_id_t gid = comm->gid;
        ssg_member_id_t memid = ssg_get_group_member_id_from_rank(gid, rank);

        if (memid == SSG_MEMBER_ID_INVALID) {
            __error("failed to get member id from group %lu and rank %d",
                    (unsigned long) gid, rank);
            goto out;
        }

        addr = ssg_get_group_member_addr(gid, memid);

#endif
        addr = comm->addrs[rank];
        margo_addr_to_string(comm->smid, addrstr, &len, addr);

        __debug("address requested, rank[%d] = %s", rank, addrstr);
    }

    return addr;
}

