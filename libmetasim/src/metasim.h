#ifndef __METASIM_H
#define __METASIM_H

#include <stdint.h>

typedef void * metasim_t;

metasim_t metasim_init(void);

void metasim_exit(metasim_t metasim);

int metasim_invoke_query(metasim_t metasim,
                         int32_t *localrank, int32_t *nservers);

int metasim_invoke_ping(metasim_t metasim, int32_t ping, int32_t *pong);

int metasim_invoke_rping(metasim_t metasim,
                         int32_t server, int32_t ping, int32_t *pong);

#endif /* __METASIM_H */
