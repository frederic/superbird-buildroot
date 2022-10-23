#ifndef LOG_H
#define LOG_H
#if 0//def ANDROID
#include <cutils/log.h>
#define LEVEL_VERBOSE   0
#define LEVEL_INFO      1
#define LEVEL_DEBUG     2
#define LEVEL_WARN      3
#define LEVEL_ERROR     4
#define LEVEL_FATAL     5
#define LEVEL_MAX       0xF

#define ALOGF(...) printf(__VA_ARGS__)
#define ALOGA(...) printf(__VA_ARGS__)
#endif

#if  1//ndef ANDROID
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>


extern int aml_audio_debug_level;


static inline const char *get_cur_time_str()
{
    static char timestring[128] = {0};

    time_t timep;
    struct  timeb stTimeb;
    struct tm *p;

    time(&timep);
    p = localtime(&timep);
    ftime(&stTimeb);

    memset(timestring, 0, sizeof(timestring));
    sprintf(timestring, "[%d-%02d-%02d %02d:%02d:%02d.%03d] ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec, stTimeb.millitm);

    return (const char *)&timestring[0];
}


#define LEVEL_VERBOSE   0
#define LEVEL_INFO      1
#define LEVEL_DEBUG     2
#define LEVEL_WARN      3
#define LEVEL_ERROR     4
#define LEVEL_FATAL     5
#define LEVEL_MAX       0xF

#define LOG_LEVEL_VERBOSE(fmt, args...)     printf("%s[V] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
#define LOG_LEVEL_INFO(fmt, args...)        printf("%s[I] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
#define LOG_LEVEL_DEBUG(fmt, args...)       printf("%s[D] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
#define LOG_LEVEL_WARN(fmt, args...)        printf("%s[W] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
#define LOG_LEVEL_ERROR(fmt, args...)       printf("%s[E] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
#define LOG_LEVEL_FATAL(fmt, args...)       printf("%s[F] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)

// #define LOG_LEVEL_INFO(fmt, args...)   printf("%s[I] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
// #define LOG_LEVEL_WARN(fmt, args...)   printf("%s[W] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
// #define LOG_LEVEL_ERROR(fmt, args...)  printf("%s[E] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)
// #define LOG_LEVEL_FATAL(fmt, args...)  printf("%s[F] %s [%s, #%d] " fmt "\n", get_cur_time_str(), LOG_TAG, __func__, __LINE__, ##args)

#define PRINT_LEVEL     aml_audio_debug_level


#define LOG(level, fmt, args...)  \
{ \
    if(level >= PRINT_LEVEL) { \
        LOG_##level(fmt, ##args);\
    } \
}

// #define ALOGD(...) printf(__VA_ARGS__)
// #define ALOGE(...) printf(__VA_ARGS__)
// #define ALOGI(...) printf(__VA_ARGS__)
// #define ALOGW(...) printf(__VA_ARGS__)
// #define ALOGV(...) printf(__VA_ARGS__)

#define ALOGD(...) LOG(LEVEL_DEBUG, __VA_ARGS__)
#define ALOGE(...) LOG(LEVEL_ERROR, __VA_ARGS__)
#define ALOGI(...) LOG(LEVEL_INFO, __VA_ARGS__)
#define ALOGW(...) LOG(LEVEL_WARN, __VA_ARGS__)
#define ALOGV(...) LOG(LEVEL_VERBOSE, __VA_ARGS__)
#define ALOGF(...) LOG(LEVEL_FATAL, __VA_ARGS__)

// always print
#define ALOGA(...) printf(__VA_ARGS__)

#define ALOG_ASSERT
#define LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL_IF(cond, ...)
#define ALOGD_IF(cond, ...)
#define ALOGI_IF(cond, ...)

#endif


#endif
