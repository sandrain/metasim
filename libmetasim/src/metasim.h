#ifndef __METASIM_H
#define __METASIM_H

#include <stdint.h>

typedef void * metasim_t;

metasim_t metasim_init(void);

void metasim_exit(metasim_t metasim);

int metasim_invoke_init(metasim_t metasim,
                        int32_t rank, int32_t pid,
                        int32_t *localrank, int32_t *nservers);

int metasim_invoke_terminate(metasim_t metasim, int32_t rank, int32_t pid);

int metasim_invoke_echo(metasim_t metasim, int32_t num, int32_t *echo);

int metasim_invoke_ping(metasim_t metasim,
                        int32_t target, int32_t ping, int32_t *pong);

int metasim_invoke_sum(metasim_t metasim, int32_t seed, int32_t *sum,
                       uint64_t *elapsed_usec);

/* @elapsed_usec returns the total elapsed time */
int metasim_invoke_sumrepeat(metasim_t metasim, int32_t seed, int32_t repeat,
                             int32_t *sum, uint64_t *elapsed_usec);

#endif /* __METASIM_H */
