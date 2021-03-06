#ifndef __METASIM_LOG_H
#define __METASIM_LOG_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
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

#define __metasim_log(mask, ...)                                              \
        do {                                                                  \
            if (mask) {                                                       \
                time_t now = time(NULL);                                      \
                struct tm *ltime = localtime(&now);                           \
                char timestampstr[128];                                       \
                strftime(timestampstr, sizeof(timestampstr),                  \
                         "%Y-%m-%dT%H:%M:%S", ltime);                         \
                FILE *out = metasim_log_stream ? metasim_log_stream : stderr; \
                char *file = strrchr(__FILE__, '/');                          \
                fprintf(out, "%s tid=%d @ %s()[%s:%d] ", timestampstr,        \
                             __gettid(), __func__, &file[1], __LINE__);       \
                fprintf(out, __VA_ARGS__);                                    \
                fprintf(out, "\n");                                           \
                fflush(out);                                                  \
            }                                                                 \
        } while (0)

#define __error(...)  __metasim_log(metasim_log_error, __VA_ARGS__)
#define __debug(...)  __metasim_log(metasim_log_debug, __VA_ARGS__)

int metasim_log_open(const char *path);

void metasim_log_close(void);

static inline void metasim_log_disable(void)
{
    metasim_log_error = 1;
    metasim_log_debug = 0;
}

#endif /* __METASIM_LOG_H */

