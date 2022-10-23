/*
 * \file        res_pack_i.h
 * \brief       include file for res_pack
 *
 * \version     1.0.0
 * \date        25/10/2013   11:33
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic Inc.. All Rights Reserved.
 *
 */
#ifndef __RES_PACK_I_H__
#define __RES_PACK_I_H__

#ifndef WIN32
#include <unistd.h>
#include <dirent.h>

#define MAX_PATH    512 
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#else   
//For windows platform
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define fseeko  _fseeki64
#endif// #ifndef WIN32

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#include "res_pack.h"
#include "crc32.h"

#ifdef BUILD_DLL
#include "res_dll_if.h"
#endif// #ifdef BUILD_DLL

typedef void* __hdle;

#define debugP(...) printf("dbg:"),printf(__VA_ARGS__)
#define errorP(...) printf("ERR(L%d):", __LINE__),printf(__VA_ARGS__)

#endif // __RES_PACK_I$_H__
