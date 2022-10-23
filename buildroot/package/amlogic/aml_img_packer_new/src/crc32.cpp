/*
 * crc32.c
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */
#include "res_pack_i.h"

#define BUFSIZE     1024*16

static unsigned int crc_table[256];


static void init_crc_table(void)
{
    unsigned int c;
    unsigned int i, j;

    for (i = 0; i < 256; i++) {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[i] = c;
    }
}

//generate crc32 with buffer data
unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++) {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ;
}

//Generate crc32 value with file steam, which from 'offset' to end
unsigned calc_img_crc(FILE* fp, off_t offset)
{

    int nread;
    unsigned char buf[BUFSIZE];
    /*第一次传入的值需要固定,如果发送端使用该值计算crc校验码,
    **那么接收端也同样需要使用该值进行计算*/
    unsigned int crc = 0xffffffff;

    if (fp == NULL) {
        fprintf(stderr,"bad param!!\n");
        return -1;
    }

    init_crc_table();
    fseeko(fp,offset,SEEK_SET);
    while ((nread = fread(buf,1, BUFSIZE,fp)) > 0) {
        crc = crc32(crc, buf, nread);
    }

    if (nread < 0) {
        fprintf(stderr,"%d:read %s.\n", __LINE__, strerror(errno));
        return -1;
    }

    return crc;
}

int check_img_crc(FILE* fp, off_t offset, const unsigned orgCrc)
{
    const unsigned genCrc = calc_img_crc(fp, offset);

    if(genCrc != orgCrc)
    {
        fprintf(stderr,"%d:genCrc 0x%x != orgCrc 0x%x, error(%s).\n", __LINE__, genCrc, orgCrc, strerror(errno));
        return -1;
    }

    return 0;
}


