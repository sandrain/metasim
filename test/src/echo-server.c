#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <mpi.h>
#include <margo.h>

#include "echo.h"

static margo_instance_id server_mid;
static hg_addr_t server_addr;
static hg_id_t echo_id;
static int maxrpcs = -1;
static int nrpcs;

static void rpc_handle_echo(hg_handle_t handle)
{
    int32_t num;
    int32_t echo;
    echo_in_t in;
    echo_out_t out;
    hg_return_t hret;

    nrpcs++;

    margo_get_input(handle, &in);
    num = in.num;

    echo = num;
    out.echo = echo;

    __debug("[RPC ECHO (%3d)] (num=%d) => (echo=%d)", nrpcs, num, echo);

    hret = margo_respond(handle, &out);
    assert(hret == HG_SUCCESS);

    hret = margo_free_input(handle, &in);
    assert(hret == HG_SUCCESS);

    hret = margo_destroy(handle);
    assert(hret == HG_SUCCESS);

    if (maxrpcs > 0 && maxrpcs <= nrpcs) {
        margo_addr_free(server_mid, server_addr);
        margo_finalize(server_mid);
    }
}
DEFINE_MARGO_RPC_HANDLER(rpc_handle_echo);

static int rpc_prepare(void)
{
    FILE *fp = NULL;
    char addrstr[128];
    size_t addrstr_len = 128;
    hg_addr_t addr = HG_ADDR_NULL;
    margo_instance_id mid = MARGO_INSTANCE_NULL;
    hg_return_t hret;

    mid = margo_init("na+sm://", MARGO_SERVER_MODE, 1, 2);
    assert(mid);

    hret = margo_addr_self(mid, &addr);
    assert(hret == HG_SUCCESS);

    hret = margo_addr_to_string(mid, addrstr, &addrstr_len, addr);
    assert(hret == HG_SUCCESS);

    fp = fopen(ECHO_SERVER_ADDR_FILE, "w");
    if (fp) {
        fprintf(fp, "%s", addrstr);
        fclose(fp);

        __debug("server address file created: %s (addr=%s)",
                ECHO_SERVER_ADDR_FILE, addrstr);
    } else {
        __error("failed to publish address file (%s)",
                ECHO_SERVER_ADDR_FILE);
        return errno;
    }

    echo_id = MARGO_REGISTER(mid, "echo", echo_in_t, echo_out_t,
                             rpc_handle_echo);

    server_mid = mid;
    server_addr = addr;

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;

    if (argc == 2)
        maxrpcs = atoi(argv[1]);

    rpc_prepare();

    margo_wait_for_finalize(server_mid);
    margo_finalize(server_mid);

    return ret;
}
