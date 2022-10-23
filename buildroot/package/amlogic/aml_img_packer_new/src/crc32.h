/*
 * crc32.h
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */

#ifndef CRC32_H_
#define CRC32_H_

unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size);

unsigned calc_img_crc(FILE* fd, off_t offset);

int check_img_crc(FILE* fp, off_t offset, const unsigned orgCrc);

#endif /* CRC32_H_ */
