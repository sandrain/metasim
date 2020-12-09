#ifndef __METASIM_H
#define __METASIM_H

#include <stdint.h>

typedef void * metasim_t;

metasim_t metasim_init(void);

void metasim_exit(metasim_t metasim);

int metasim_get_server_count(metasim_t metasim);

int metasim_invoke_ping(metasim_t metasim,
                        int32_t target, int32_t ping, int32_t *pong);

#endif /* __METASIM_H */
