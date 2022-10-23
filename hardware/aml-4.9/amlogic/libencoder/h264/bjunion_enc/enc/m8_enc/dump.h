#ifndef _DUMP_H_
#define _DUMP_H_
#include <stdio.h>
#include "m8venclib.h"

enum DumpType{
    DUMP_REFERENCE = 0x1, 		//dump reference buffer
    DUMP_ENCODE_DATA = 0x2,		//dump qp data or encode data
    DUMP_MV_BITS = 0x4, 		//dump mv bits info
    DUMP_IE_BITS = 0x8,			//dump ie
};

typedef struct{
    unsigned mode;
    FILE *ref_fp;
    unsigned char *little_endian_buffer;
    unsigned char *convert_buffer;
    int start_frm_count;
    int end_frm_count;
}dump_struct_t;

extern unsigned dump_init(int enc_width, int enc_height);
extern void dump_reference_buffer(m8venc_drv_t *p);
extern void dump_encode_data(m8venc_drv_t* p, int type, int size);
extern void dump_mv_bits(m8venc_drv_t *p);
extern void dump_ie_bits(m8venc_drv_t *p);
extern void dump_release(void);

#endif
