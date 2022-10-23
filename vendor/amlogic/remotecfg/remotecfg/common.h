#ifndef  _COMMON_H_
#define  _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <strings.h>
#include "keydefine.h"
#include "rc_common.h"

#define MAX_LINE_LEN 1024

#define MAPCODE(scancode, keycode)\
		((((scancode) & 0xFFFF)<<16) | ((keycode) & 0xFFFF))

#define MATCH(s,d) (strcasecmp(s, d) == 0)

#define IS_NULL(s) (strlen(s) == 0)

extern unsigned char dbg_flag;

#define prDbg(fmt, ...)       \
{				    		  \
	if (dbg_flag)             \
		fprintf(stdout, fmt, ##__VA_ARGS__); \
}

enum {
	SUCC = 0,
	FAIL = -1,
};

enum {
    CONFIG_LEVEL,
    KEYMAP_LEVEL,
    MOUSEMAP_LEVEL,
    ADCMAP_LEVEL,
};

typedef int (*pfileHandle)(char *, char *, void *);

typedef struct {
	struct ir_map_tab tab;
}S_TAB_FILE_T;

typedef struct {
	unsigned int workMode;
	unsigned int repeatEnable;
	unsigned int debugEnable;
	/*software decode*/
	struct ir_sw_decode_para sw_data;
} S_CFG_FILE_T;


int SetTabPara(int devFd, S_TAB_FILE_T *tabFile);
int SetCfgPara(int devFd, const char *sysDir, S_CFG_FILE_T *cfgFile);
int ParseFile(const char *file, pfileHandle handler, void *data);
int CheckVersion(int fd);
int OpenDevice(char *filename);
int CloseDevice(int fd);



#endif
