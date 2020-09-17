#ifndef __ECHO_H
#define __ECHO_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <mercury.h>
#include <mercury_macros.h>

MERCURY_GEN_PROC(echo_in_t, ((int32_t)(num)));

MERCURY_GEN_PROC(echo_out_t, ((int32_t)(echo)));

#define ECHO_SERVER_ADDR_FILE "/tmp/echo-server-addr"

/* logging */

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
