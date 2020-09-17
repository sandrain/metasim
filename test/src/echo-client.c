#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <mpi.h>
#include <margo.h>

#include "echo.h"

static margo_instance_id client_mid;
static hg_addr_t server_addr;
static hg_id_t echo_id;

static char *read_addr_proto(char *listener_addr)
{
    char *proto = strdup(listener_addr);

    if (proto) {
        char *pos = strchr(proto, ':');
        pos[0] = '\0';
    }

    return proto;
}

static char *get_server_addr(void)
{
    int ret = 0;
    FILE *fp = NULL;
    char *addr = NULL;
    char linebuf[LINE_MAX];

    /* wait untile the file becomes available */
    while (1) {
        ret = access(ECHO_SERVER_ADDR_FILE, R_OK);
        if (!ret || errno != ENOENT)
            break;

        sleep(2);
    }

    if (ret) {
        __error("failed to access the address file %s (err=%d, %s)",
                ECHO_SERVER_ADDR_FILE, errno, strerror(errno));
        return NULL;
    }

    fp = fopen(ECHO_SERVER_ADDR_FILE, "r");
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
        __error("failed to open %s (%s)", ECHO_SERVER_ADDR_FILE,
                strerror(errno));
    }

    return addr;
}

static int rpc_prepare(void)
{
    char *addr_str = NULL;
    char *proto = NULL;
    hg_addr_t addr = HG_ADDR_NULL;
    margo_instance_id mid = MARGO_INSTANCE_NULL;

    addr_str = get_server_addr();
    assert(addr_str);

    proto = read_addr_proto(addr_str);
    assert(proto);

    __debug("echo server at %s (protocol=%s)", addr_str, proto);

    mid = margo_init(proto, MARGO_CLIENT_MODE, 0, 0);
    assert(mid);

    margo_addr_lookup(mid, addr_str, &addr);
    assert(addr);

    echo_id = MARGO_REGISTER(mid, "echo", echo_in_t, echo_out_t, NULL);

    client_mid = mid;
    server_addr = addr;

    return 0;
}

static int rpc_invoke_echo(int32_t num, int32_t *echo)
{
    hg_handle_t handle;
    hg_return_t hret;
    echo_in_t in;
    echo_out_t out;

    in.num = num;

    margo_create(client_mid, server_addr, echo_id, &handle);
    hret = margo_forward(handle, &in);
    assert(hret == HG_SUCCESS);

    hret = margo_get_output(handle, &out);
    assert(hret == HG_SUCCESS);

    *echo = out.echo;

    margo_free_output(handle, &out);
    margo_destroy(handle);

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    int echo_count = 10;
    int rank = 0;
    int nranks = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    if (argc == 2) {
        echo_count = atoi(argv[1]);
        assert(echo_count > 0);
    }

    __debug("preparing rpc.. (echo_count=%d)", echo_count);

    MPI_Barrier(MPI_COMM_WORLD);

    rpc_prepare();

    if (echo_count == 0)
        echo_count = nranks;

    for (i = 0; i < echo_count; i++) {
        int32_t num = i;
        int32_t echo = 0;

        ret = rpc_invoke_echo(num, &echo);
        assert(0 == ret);

        __debug("[%d] (%3d/%3d) RPC ECHO (num=%d) => (echo=%d)",
                rank, i, echo_count, num, echo);
    }


    margo_addr_free(client_mid, server_addr);
    margo_finalize(client_mid);

    __debug("all done");

    MPI_Finalize();

    return ret;
}
