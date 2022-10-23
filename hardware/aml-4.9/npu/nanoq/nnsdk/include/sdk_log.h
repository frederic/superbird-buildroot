#ifndef __SDK_LOG__
#define __SDK_LOG__

#define SDK_STATUS_OK                              0
#define SDK_STATUS_ERROR                          -1
#define SDK_STATUS_PARAM_ERROR                    -2
#define SDK_STATUS_MEMORY_ALLOC_FAIL              -3
#define SDK_STATUS_CREATE_NETWORK_FAIL            -4
#define SDK_STATUS_VERIFY_NETWORK_FAIL            -5
#define SDK_STATUS_PROCESS_NETWORK_FAIL           -6

typedef enum{
  SDK_LOG_NULL = -1,	        ///< close all log output
  SDK_LOG_TERMINAL,	            ///< set log print to terminal
  SDK_LOG_FILE,		            ///< set log print to file
  SDK_LOG_SYSTEM                ///< set log print to system
}sdk_log_format_t;

typedef enum{
	SDK_DEBUG_LEVEL_RELEASE = -1,	///< close debug
	SDK_DEBUG_LEVEL_ERROR,		    ///< error level,hightest level system will exit and crash
	SDK_DEBUG_LEVEL_WARN,		    ///< warning, system continue working,but something maybe wrong
	SDK_DEBUG_LEVEL_INFO,		    ///< info some value if needed
	SDK_DEBUG_LEVEL_PROCESS,	    ///< default,some process print
	SDK_DEBUG_LEVEL_DEBUG		    ///< debug level,just for debug
}sdk_debug_level_t;

#define DEBUG_BUFFER_LEN 1280
#define SDK_API  "NN_SDK: "

#define LOGE( fmt, ... ) \
    nn_sdk_LogMsg(SDK_DEBUG_LEVEL_ERROR, "E %s[%s:%d]" fmt, SDK_API, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW( fmt, ... ) \
    nn_sdk_LogMsg(SDK_DEBUG_LEVEL_WARN,  "W %s[%s:%d]" fmt, SDK_API, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI( fmt, ... ) \
    nn_sdk_LogMsg(SDK_DEBUG_LEVEL_INFO,  "I %s[%s:%d]" fmt, SDK_API, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGP( fmt, ... ) \
    nn_sdk_LogMsg(SDK_DEBUG_LEVEL_PROCESS, "P %s[%s:%d]" fmt, SDK_API, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD( fmt, ... ) \
    nn_sdk_LogMsg(SDK_DEBUG_LEVEL_DEBUG, "D %s[%s:%d]" fmt, SDK_API, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define __LOG__( fmt, ... ) \
    nn_sdk_LogMsg(SDK_DEBUG_LEVEL_DEBUG, "%s[%s:%d]" fmt, SDK_API, __FUNCTION__, __LINE__, ##__VA_ARGS__)

void nn_sdk_LogMsg(sdk_debug_level_t level, const char *fmt, ...);
void det_set_log_level(sdk_debug_level_t level,sdk_log_format_t output_format);

#endif