
/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#define __USE_GNU
#define _GNU_SOURCE
#include <pthread.h>
#include "vpuconfig.h"
#include "vdi_osal.h"

#include <time.h>
#include <termios.h>
static struct termios initial_settings, new_settings;
static int peek_character = -1;

#define ANSI_COLOR_ERR      "\x1b[31m"       // RED
#define ANSI_COLOR_TRACE    "\x1b[32m"       // GREEN
#define ANSI_COLOR_WARN     "\x1b[33m"       // YELLOW
#define ANSI_COLOR_BLUE     "\x1b[34m"       // BLUE
#define ANSI_COLOR_INFO     ""
// For future
#define ANSI_COLOR_MAGENTA  "\x1b[35m"       // MAGENTA
#define ANSI_COLOR_CYAN     "\x1b[36m"       // CYAN
#define ANSI_COLOR_RESET    "\x1b[0m"        // RESET

static unsigned log_decor = LOG_HAS_TIME | LOG_HAS_FILE | LOG_HAS_MICRO_SEC |
			    LOG_HAS_NEWLINE |
			    LOG_HAS_SPACE | LOG_HAS_COLOR;
static int max_log_level = MAX_LOG_LEVEL;
static FILE *fpLog  = NULL;

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
static pthread_mutex_t s_log_mutex;
#endif

debug_log_level_t g_log_level = ERR;

void vp5_set_log_level(debug_log_level_t level)
{
    char *log_level = getenv("VP5_LOG_LEVEL");
    if (log_level) {
        g_log_level = (debug_log_level_t)atoi(log_level);
	printf("Set log level by environment to %d\n", g_log_level);
    } else {
        g_log_level = level;
	printf("Set log level to %d\n", g_log_level);
    }
    return;
}

int InitLog()
{
	fpLog = osal_fopen("ErrorLog.txt", "w");
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	pthread_mutex_init(&s_log_mutex, NULL);
#endif
	return 1;
}

void DeInitLog()
{
	if (fpLog)
	{
		osal_fclose(fpLog);
		fpLog = NULL;
	}
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	pthread_mutex_destroy(&s_log_mutex);
#endif
}

void SetMaxLogLevel(int level)
{
	max_log_level = level;
}

int GetMaxLogLevel()
{
	return max_log_level;
}

void LogMsg(int level, const char *format, ...)
{
    va_list ptr;
    char    logBuf[MAX_PRINT_LENGTH] = {0};
    char*   prefix = "";
    char*   postfix= "";

    if (level > max_log_level)
        return;
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	pthread_mutex_lock(&s_log_mutex);
#endif

    if ((log_decor & LOG_HAS_COLOR)) {
        postfix = ANSI_COLOR_RESET;
        switch (level) {
        case INFO:  prefix = ANSI_COLOR_INFO;  break;
        case ERR:   prefix = ANSI_COLOR_ERR "[ERROR]";   break;
        case TRACE: prefix = ANSI_COLOR_TRACE; break;
        case WARN:  prefix = ANSI_COLOR_WARN"[WARN ]";  break;
        default:    prefix = "";               break;
        }
    }

    va_start( ptr, format );
    vsnprintf( logBuf, MAX_PRINT_LENGTH, format, ptr );
    va_end(ptr);

#ifdef ANDROID
    if (level == ERR) ALOGE("%s", logBuf);
    else              ALOGI("%s", logBuf);
    fputs(logBuf, stderr);
#else
    fputs(prefix,  stderr);
    fputs(logBuf,  stderr);
    fputs(postfix, stderr);
#endif

    if ((log_decor & LOG_HAS_FILE) && fpLog)
    {
        osal_fwrite(logBuf, strlen(logBuf), 1,fpLog);
        osal_fflush(fpLog);
    }

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	pthread_mutex_unlock(&s_log_mutex);
#endif
}

void osal_init_keyboard()
{
    tcgetattr(0,&initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    //new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
	peek_character = -1;
}

void osal_close_keyboard()
{
    tcsetattr(0, TCSANOW, &initial_settings);
}

int osal_kbhit()
{
	unsigned char ch;
	int nread;

    if (peek_character != -1) return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1)
    {
        peek_character = (int)ch;
        return 1;
    }
    return 0;
}

int osal_getch()
{
	int val;
	char ch;

    if(peek_character != -1)
    {
    	val = peek_character;
        peek_character = -1;
        return val;
    }
    read(0,&ch,1);
    return ch;
}

int osal_flush_ch(void)
{
	fflush(stdout);
	return 1;
}

void * osal_memcpy(void * dst, const void * src, int count)
{
	return memcpy(dst, src, count);//lint !e670
}

int osal_memcmp(const void* src, const void* dst, int size)
{
    return memcmp(src, dst, size);
}

void * osal_memset(void *dst, int val, int count)
{
	return memset(dst, val, count);
}

void * osal_malloc(int size)
{
	return malloc(size);
}

void * osal_realloc(void* ptr, int size)
{
    return realloc(ptr, size);
}

void osal_free(void *p)
{
	free(p);//lint -e{424}
}

int osal_fflush(osal_file_t fp)
{
	return fflush(fp);
}

int osal_feof(osal_file_t fp)
{
    return feof((FILE *)fp);
}

osal_file_t osal_fopen(const char * osal_file_tname, const char * mode)
{
	return fopen(osal_file_tname, mode);
}
size_t osal_fwrite(const void * p, int size, int count, osal_file_t fp)
{
	return fwrite(p, size, count, fp);
}
size_t osal_fread(void *p, int size, int count, osal_file_t fp)
{
	return fread(p, size, count, fp);
}

long osal_ftell(osal_file_t fp)
{
	return ftell(fp);
}

int osal_fseek(osal_file_t fp, long offset, int origin)
{
	return fseek(fp, offset, origin);
}
int osal_fclose(osal_file_t fp)
{
	return fclose(fp); //lint !e482
}

int osal_fscanf(osal_file_t fp, const char * _Format, ...)
{
    int ret;

    va_list arglist;
    va_start(arglist, _Format);

    ret = vfscanf(fp, _Format, arglist);

    va_end(arglist);

    return ret;
}

int osal_fprintf(osal_file_t fp, const char * _Format, ...)
{
	int ret;
	va_list arglist;
	va_start(arglist, _Format);

	ret = vfprintf(fp, _Format, arglist);

	va_end(arglist);

	return ret;
}

void osal_msleep(Uint32 millisecond)
{
    usleep(millisecond*1000);
}

osal_thread_t osal_thread_create(void(*start_routine)(void*), void*arg)
{
    Int32           ret;
    pthread_t*      thread = (pthread_t*)osal_malloc(sizeof(pthread_t));
    osal_thread_t   handle = NULL;

    if ((ret=pthread_create(thread, NULL, (void*(*)(void*))start_routine, arg)) != 0) {
        osal_free(thread);
        VLOG(ERR, "<%s:%d> Failed to pthread_create ret(%d)\n", __FUNCTION__, __LINE__, ret);
    }
    else {
        handle = (osal_thread_t)thread;
    }

    return handle;  //lint !e593
}

Int32 osal_thread_join(osal_thread_t thread, void** retval)
{
    Int32       ret;
    pthread_t   pthreadHandle;

    if (thread == NULL) {
        VLOG(ERR, "%s:%d invalid thread handle\n", __FUNCTION__, __LINE__);
        return 2;
    }

    pthreadHandle = *(pthread_t*)thread;

    if ((ret=pthread_join(pthreadHandle, retval)) != 0) {
        osal_free(thread);
        VLOG(ERR, "%s:%d Failed to pthread_join ret(%d)\n", __FUNCTION__, __LINE__, ret);
        return 2;
    }

    osal_free(thread);

    return 0;
}

Int32 osal_thread_timedjoin(osal_thread_t thread, void** retval, Uint32 second)
{
#if 0
    Int32           ret;
#endif
    pthread_t       pthreadHandle;
    struct timespec absTime;

    if (thread == NULL) {
        VLOG(ERR, "%s:%d invalid thread handle\n", __FUNCTION__, __LINE__);
        return 2;
    }

    pthreadHandle = *(pthread_t*)thread;

    absTime.tv_sec  = second;
    absTime.tv_nsec = 0;
#if 1
*retval = NULL;
return 2;
#else
    if ((ret=pthread_timedjoin_np(pthreadHandle, retval, &absTime)) == 0) {
        osal_free(thread);
        return 0;
    }
    else if (ret == ETIMEDOUT) {
        return 1;
    }
    else {
        return 2;
    }
#endif
}

osal_mutex_t osal_mutex_create(void)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)osal_malloc(sizeof(pthread_mutex_t));

    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (pthread_mutex_init(mutex, NULL) < 0) {
        osal_free(mutex);
        VLOG(ERR, "<%s:%d> failed to pthread_mutex_init() errno(%d)\n", __FUNCTION__, __LINE__, errno);
        return NULL; //lint !e429
    }

    return (osal_mutex_t)mutex;
}

void osal_mutex_destroy(osal_mutex_t mutex)
{
    Int32   ret;

    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> Invalid mutex handle\n", __FUNCTION__, __LINE__);
        return;
    }

    if ((ret=pthread_mutex_destroy(mutex)) != 0) {
        VLOG(ERR, "<%s:%d> Failed to pthread_mutex_destroy(). ret(%d)\n", __FUNCTION__, __LINE__, ret);
    }

    osal_free(mutex);
}

BOOL osal_mutex_lock(osal_mutex_t mutex)
{
    Int32   ret;

    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> Invalid mutex handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if ((ret=pthread_mutex_lock((pthread_mutex_t*)mutex)) != 0) {
        VLOG(ERR, "<%s:%d> Failed to pthread_mutex_lock() ret(%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;   //lint !e454
    }

    return TRUE;    //lint !e454
}

BOOL osal_mutex_unlock(osal_mutex_t mutex)
{
    Int32 ret;

    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> Invalid mutex handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if ((ret=pthread_mutex_unlock((pthread_mutex_t*)mutex)) != 0) { //lint !e455
        VLOG(ERR, "<%s:%d> Failed to pthread_mutex_unlock(). ret(%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    return TRUE;
}

Uint64 osal_gettime(void)
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    return (tp.tv_sec*1000 + tp.tv_nsec/1000000);
}

