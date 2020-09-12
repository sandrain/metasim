#ifndef __METASIM_RPC_H
#define __METASIM_RPC_H

#include <stdint.h>

void metasim_rpc_register(void);

/*
 * rpc wrappers
 */
int metasim_rpc_invoke_ping(int32_t targetrank, int32_t ping, int32_t *pong);

int metasim_rpc_invoke_sum(int32_t seed, int32_t *sum);

#endif /* __METASIM_RPC_H */

