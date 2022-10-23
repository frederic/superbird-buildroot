/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AML_IPC_LOG_H

#define AML_IPC_LOG_H

#include <alloca.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief amlogic log util
 * Usage:
 *  // define the log category in one source file:
 *  AML_LOG_DEFINE(my_log);
 *  // set my_log as the default log category
 *  // AML_LOGX will log to default category, and AML_LOGCATX will log to specified category
 *  #define AML_LOG_DEFAULT AML_LOG_GET_CAT(my_log)
 *  // in source code, use AML_LOGX to log message under my_log category
 *  AML_LOGV("this log is controled under my_log category")
 *
 *  // if a message needs to log under another category (defined in another source file)
 *  // use AML_LOG_EXTERN to import that category
 *  // i.e. to use my_log category in another file
 *  AML_LOG_EXTERN(my_log);
 *  AML_LOGV_CAT(my_log, "this log is under my_log category too");
 *
 * AML_SCTIME_LOGX will log the time it takes when it leave the scope where AML_SCTIME_LOGX is
 * defined
 */

#define AML_LOG_LEVEL_INVALID 0x7fffffff

#define AML_LOG_GET_CAT(name) AML_LOG_##name
#define AML_LOG_DEFINE(name)                                                                       \
    struct AmlIPCLogCat AML_LOG_GET_CAT(name) = {#name, AML_LOG_LEVEL_INVALID,                     \
                                                 AML_LOG_LEVEL_INVALID, NULL}
#define AML_LOG_EXTERN(name) extern struct AmlIPCLogCat AML_LOG_GET_CAT(name)
extern struct AmlIPCLogCat AML_LOG_DEFAULT;

struct AmlIPCLogCat {
    const char *name;
    int log_level;
    int trace_level;
    struct AmlIPCLogCat *next;
};

#define AML_LOG_CAT_ENABLE(cat, l) ((cat)->log_level > l)

#define AML_LOG_GET_CAT_(name) AML_LOG_##name
#define AML_LOG_CAT(cat, level, fmt, ...)                                                          \
    do {                                                                                           \
        if (AML_LOG_CAT_ENABLE(&(AML_LOG_GET_CAT_(cat)), level)) {                                 \
            aml_ipc_log_msg(&(AML_LOG_GET_CAT_(cat)), level, __FILE__, __func__, __LINE__, fmt,    \
                            ##__VA_ARGS__);                                                        \
        }                                                                                          \
    } while (0)

#define AML_LOGVVV(fmt, ...) AML_LOG_CAT(DEFAULT, 6, fmt, ##__VA_ARGS__)
#define AML_LOGVV(fmt, ...) AML_LOG_CAT(DEFAULT, 5, fmt, ##__VA_ARGS__)
#define AML_LOGV(fmt, ...) AML_LOG_CAT(DEFAULT, 4, fmt, ##__VA_ARGS__)
#define AML_LOGD(fmt, ...) AML_LOG_CAT(DEFAULT, 3, fmt, ##__VA_ARGS__)
#define AML_LOGI(fmt, ...) AML_LOG_CAT(DEFAULT, 2, fmt, ##__VA_ARGS__)
#define AML_LOGW(fmt, ...) AML_LOG_CAT(DEFAULT, 1, fmt, ##__VA_ARGS__)
#define AML_LOGE(fmt, ...) AML_LOG_CAT(DEFAULT, 0, fmt, ##__VA_ARGS__)

#define AML_LOGCATVVV(cat, fmt, ...) AML_LOG_CAT(cat, 6, fmt, ##__VA_ARGS__)
#define AML_LOGCATVV(cat, fmt, ...) AML_LOG_CAT(cat, 5, fmt, ##__VA_ARGS__)
#define AML_LOGCATV(cat, fmt, ...) AML_LOG_CAT(cat, 4, fmt, ##__VA_ARGS__)
#define AML_LOGCATD(cat, fmt, ...) AML_LOG_CAT(cat, 3, fmt, ##__VA_ARGS__)
#define AML_LOGCATI(cat, fmt, ...) AML_LOG_CAT(cat, 2, fmt, ##__VA_ARGS__)
#define AML_LOGCATW(cat, fmt, ...) AML_LOG_CAT(cat, 1, fmt, ##__VA_ARGS__)
#define AML_LOGCATE(cat, fmt, ...) AML_LOG_CAT(cat, 0, fmt, ##__VA_ARGS__)

// log the time spend in current scope
struct AmlIPCScopeTimer {
    char buf[256];
    struct timespec start_time;
    struct timespec start_ttime;
    struct AmlIPCLogCat *cat;
    const char *file;
    const char *func;
    int level;
    int line;
};
extern __thread int sc_level__;
void stop_scope_timer__(struct AmlIPCScopeTimer **timer);

#define AML_TRACE_CAT_ENABLE(cat, l) ((cat)->trace_level > l)
#define AML_SCTIME_LOGCAT_(_id, _cat, _level, _fmt, ...)                                           \
    __attribute__((cleanup(stop_scope_timer__))) struct AmlIPCScopeTimer *_id = NULL;              \
    do {                                                                                           \
        ++sc_level__;                                                                              \
        if (AML_TRACE_CAT_ENABLE(&(_cat), _level)) {                                               \
            _id = alloca(sizeof(*_id));                                                            \
            _id->cat = &_cat;                                                                      \
            _id->level = _level;                                                                   \
            _id->file = __FILE__;                                                                  \
            _id->func = __func__;                                                                  \
            _id->line = __LINE__;                                                                  \
            clock_gettime(CLOCK_MONOTONIC, &_id->start_time);                                      \
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &_id->start_ttime);                             \
            snprintf(_id->buf, sizeof(_id->buf), _fmt, ##__VA_ARGS__);                             \
        }                                                                                          \
    } while (0)

#define _MCAT_(x, y) x##y
#define _MCAT_2_(x, y) _MCAT_(x, y)
#define AML_SCTIME_LOG_CAT(cat, level, fmt, ...)                                                   \
    AML_SCTIME_LOGCAT_(_MCAT_2_(_sctime_, __COUNTER__), AML_LOG_GET_CAT_(cat), level, fmt,         \
                       ##__VA_ARGS__)
#define AML_SCTIME_LOGVV(fmt, ...) AML_SCTIME_LOG_CAT(DEFAULT, 5, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGV(fmt, ...) AML_SCTIME_LOG_CAT(DEFAULT, 4, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGD(fmt, ...) AML_SCTIME_LOG_CAT(DEFAULT, 3, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGI(fmt, ...) AML_SCTIME_LOG_CAT(DEFAULT, 2, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGW(fmt, ...) AML_SCTIME_LOG_CAT(DEFAULT, 1, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGE(fmt, ...) AML_SCTIME_LOG_CAT(DEFAULT, 0, fmt, ##__VA_ARGS__)

#define AML_SCTIME_LOGCATVV(cat, fmt, ...) AML_SCTIME_LOG_CAT(cat, 5, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGCATV(cat, fmt, ...) AML_SCTIME_LOG_CAT(cat, 4, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGCATD(cat, fmt, ...) AML_SCTIME_LOG_CAT(cat, 3, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGCATI(cat, fmt, ...) AML_SCTIME_LOG_CAT(cat, 2, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGCATW(cat, fmt, ...) AML_SCTIME_LOG_CAT(cat, 1, fmt, ##__VA_ARGS__)
#define AML_SCTIME_LOGCATE(cat, fmt, ...) AML_SCTIME_LOG_CAT(cat, 0, fmt, ##__VA_ARGS__)

#define AML_TRACE_EVENT_CAT(c, n, ph, id, _cat, _level, _fmt, ...)                                 \
    do {                                                                                           \
        if (AML_TRACE_CAT_ENABLE(&(AML_LOG_GET_CAT_(_cat)), _level)) {                             \
            aml_ipc_trace_log(&(AML_LOG_GET_CAT_(_cat)), _level, c, n, ph, id, _fmt,               \
                              ##__VA_ARGS__);                                                      \
        }                                                                                          \
    } while (0)

#define AML_FLOW_START_TRACECATV(cat, c, n, id, fmt, ...)                                          \
    AML_TRACE_EVENT_CAT(c, n, 's', id, cat, 4, fmt, ##__VA_ARGS__)
#define AML_FLOW_START_TRACECATD(cat, c, n, id, fmt, ...)                                          \
    AML_TRACE_EVENT_CAT(c, n, 's', id, cat, 3, fmt, ##__VA_ARGS__)

#define AML_FLOW_STEP_TRACECATV(cat, c, n, id, fmt, ...)                                           \
    AML_TRACE_EVENT_CAT(c, n, 't', id, cat, 4, fmt, ##__VA_ARGS__)
#define AML_FLOW_STEP_TRACECATD(cat, c, n, id, fmt, ...)                                           \
    AML_TRACE_EVENT_CAT(c, n, 't', id, cat, 3, fmt, ##__VA_ARGS__)

#define AML_FLOW_END_TRACECATV(cat, c, n, id, fmt, ...)                                            \
    AML_TRACE_EVENT_CAT(c, n, 'f', id, cat, 4, fmt, ##__VA_ARGS__)
#define AML_FLOW_END_TRACECATD(cat, c, n, id, fmt, ...)                                            \
    AML_TRACE_EVENT_CAT(c, n, 'f', id, cat, 3, fmt, ##__VA_ARGS__)

#define AML_ASYNC_START_TRACECATV(cat, c, n, id, fmt, ...)                                         \
    AML_TRACE_EVENT_CAT(c, n, 'b', id, cat, 4, fmt, ##__VA_ARGS__)
#define AML_ASYNC_START_TRACECATD(cat, c, n, id, fmt, ...)                                         \
    AML_TRACE_EVENT_CAT(c, n, 'b', id, cat, 3, fmt, ##__VA_ARGS__)

#define AML_ASYNC_STEP_TRACECATV(cat, c, n, id, fmt, ...)                                          \
    AML_TRACE_EVENT_CAT(c, n, 'n', id, cat, 4, fmt, ##__VA_ARGS__)
#define AML_ASYNC_STEP_TRACECATD(cat, c, n, id, fmt, ...)                                          \
    AML_TRACE_EVENT_CAT(c, n, 'n', id, cat, 3, fmt, ##__VA_ARGS__)

#define AML_ASYNC_END_TRACECATV(cat, c, n, id, fmt, ...)                                           \
    AML_TRACE_EVENT_CAT(c, n, 'e', id, cat, 4, fmt, ##__VA_ARGS__)
#define AML_ASYNC_END_TRACECATD(cat, c, n, id, fmt, ...)                                           \
    AML_TRACE_EVENT_CAT(c, n, 'e', id, cat, 3, fmt, ##__VA_ARGS__)

__attribute__((format(printf, 6, 7))) void aml_ipc_log_msg(struct AmlIPCLogCat *cat, int level,
                                                           const char *file, const char *function,
                                                           int lineno, const char *fmt, ...);
/**
 * @brief set log level from a string
 *
 * @param str, ',' seperate categories and levels
 * cat1:level1,cat2:level2...
 *
 */
void aml_ipc_log_set_from_string(const char *str);
void aml_ipc_trace_set_from_string(const char *str);

void aml_ipc_log_set_output_file(FILE *fp);
/**
 * @brief write json format trace information to stream fp
 *   this json format trace information can be viewed by trace viewer such as chrome browser
 *
 * @param fp
 */
void aml_ipc_trace_set_json_output(FILE *fp);
__attribute__((format(printf, 7, 8))) void aml_ipc_trace_log(struct AmlIPCLogCat *cat, int level,
                                                             const char *evcat, const char *evname,
                                                             int evph, void *evid, const char *fmt,
                                                             ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_LOG_H */

