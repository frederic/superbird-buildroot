#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ANDROID
#include <cutils/log.h>
#endif

#ifndef ANDROID
#include <stdio.h>
#define ALOGD(...) printf(__VA_ARGS__)
#define ALOGE(...) printf(__VA_ARGS__)
#define ALOGI(...) printf(__VA_ARGS__)
#define ALOGW(...) printf(__VA_ARGS__)
#define ALOGV(...) printf(__VA_ARGS__)
#define ALOG_ASSERT
#define LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL_IF(cond, ...)
#define ALOGD_IF(cond, ...)
#define ALOGI_IF(cond, ...)

#endif

#ifdef __cplusplus
}
#endif

#endif
