#ifndef __AUDIO_TRACE_H_
#define __AUDIO_TRACE_H_

//#define USE_FTRACE

#ifdef USE_FTRACE

extern int audio_trace_init();

extern int audio_trace(const char *fmt, ...);

extern void audio_trace_deinit();

#else

#include "log.h"

#define audio_trace_init()

#define audio_trace(fmt, args...) LOG_LEVEL_DEBUG(fmt, ##args)
//#define audio_trace(fmt, args...)

#define audio_trace_deinit()

#endif

#endif /* __AUDIO_TRACE_H_ */

