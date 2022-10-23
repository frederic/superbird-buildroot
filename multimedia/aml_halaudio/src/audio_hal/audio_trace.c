#include "audio_trace.h"

#ifdef USE_FTRACE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#define LOG_TAG "AUDIO_TRACE"
#include "log.h"

#define TRACE_NEED_LOCK
#define TRACE_PATH "/sys/kernel/tracing/trace_marker"

#ifdef TRACE_NEED_LOCK
static pthread_mutex_t lock;
#endif

static int trace_fd = -1;

int audio_trace_init()
{
    pthread_mutex_init(&lock, NULL);

    trace_fd = open(TRACE_PATH, O_WRONLY);

    if (trace_fd >= 0) {
        printf("audio_trace_init ok\n");
        return 0;
    }

    printf("audio_trace_init failed\n");
    return -1;
}

int audio_trace(const char *fmt, ...)
{
    int r = 0;
    char buf[256];
    if (trace_fd >= 0) {
        int len;
        va_list args;

        pthread_mutex_lock(&lock);

        va_start(args, fmt);
        len = vsnprintf(buf, sizeof(buf), fmt, args);
        if (len > 0)
            r = write(trace_fd, buf, len);

        pthread_mutex_unlock(&lock);
    }

    return r;
}

void audio_trace_deinit()
{
    if (trace_fd >= 0) {
        close(trace_fd);
        trace_fd = -1;
    }
}

#endif

