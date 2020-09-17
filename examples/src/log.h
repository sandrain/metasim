#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

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

#define __log(mask, ...)                                                     \
        do {                                                                 \
            if (mask) {                                                      \
                char *file = strrchr(__FILE__, '/');                         \
                fprintf(stderr, "tid=%d @ %s()[%s:%d] ",                     \
                             __gettid(), __func__, &file[1], __LINE__);      \
                fprintf(stderr, __VA_ARGS__);                                \
                fprintf(stderr, "\n");                                       \
            }                                                                \
        } while (0)

#define __error(...)  __log(1, __VA_ARGS__)
#define __debug(...)  __log(1, __VA_ARGS__)

#endif
