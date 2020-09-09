#ifndef __METASIM_CONFIG_H
#define __METASIM_CONFIG_H

struct _metasim_config {
    int daemon;

    const char *logdir;

    int rpc_client_thread_pool_size;
    int rpc_server_thread_pool_size;
};

typedef struct _metasim_config metasim_config_t;

int metasim_config_process_arguments(metasim_config_t *config,
                                     int argc, char **argv);

#endif /* __METASIM_CONFIG_H */
