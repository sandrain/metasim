#ifndef __METASIM_LOG_H
#define __METASIM_LOG_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

extern int metasim_log_error;
extern int metasim_log_debug;

extern FILE *metasim_log_stream;

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

#define metasim_log(...)                                                  \
        if (metasim_log_error) {                                          \
            FILE *out = metasim_log_stream ? metasim_log_stream : stderr; \
            char *file = strrchr(__FILE__, '/');                          \
            fprintf(out, "tid=%d @ %s()[%s:%d] ",                         \
                         __gettid(), __func__, &file[1], __LINE__);       \
            fprintf(out, __VA_ARGS__);                                    \
            fprintf(out, "\n");                                           \
            fflush(out);                                                  \
        }

#define __error metasim_log

#define __debug(...)                                                      \
        if (metasim_log_debug) {                                          \
            metasim_log(__VA_ARGS__);                                     \
        }

int metasim_log_open(const char *path);

void metasim_log_close(void);

#endif /* __METASIM_LOG_H */

