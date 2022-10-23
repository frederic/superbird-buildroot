/*
 * \file        imgpack.h
 * \brief       Amlogic resource image header define
 *
 * \version     1.0.0
 * \date        2013/10/24
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic Inc.. All Rights Reserved.
 *
 */
#ifndef __IMGPACK_H__
#define __IMGPACK_H__

typedef unsigned int    __u32;
typedef signed int      __s32;
typedef unsigned char   __u8;
typedef signed char     __s8;

#define IH_MAGIC	0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32	/* Image Name Length		*/

#define AML_RES_IMG_ITEM_ALIGN_SZ   16
#define AML_RES_IMG_V1_MAGIC_LEN    8
#define AML_RES_IMG_V1_MAGIC        "AML_RES!"//8 chars
#define AML_RES_IMG_HEAD_SZ         (AML_RES_IMG_ITEM_ALIGN_SZ * 4)//64
#define AML_RES_ITEM_HEAD_SZ        (AML_RES_IMG_ITEM_ALIGN_SZ * 4)//64

#define AML_RES_IMG_VERSION_V1      (0x01)
#define AML_RES_IMG_VERSION_V2      (0x02)

#pragma pack(push, 1)
typedef struct pack_header{
	unsigned int 	magic;	/* Image Header Magic Number	*/
	unsigned int 	hcrc;	/* Image Header CRC Checksum	*/
	unsigned int	size;	/* Image Data Size		*/
	unsigned int	start;	/* item data offset in the image*/
	unsigned int	end;		/* Entry Point Address		*/
	unsigned int	next;	/* Next item head offset in the image*/
	unsigned int	dcrc;	/* Image Data CRC Checksum	*/
	unsigned char	index;		/* Operating System		*/
	unsigned char	nums;	/* CPU architecture		*/
	unsigned char   type;	/* Image Type			*/
	unsigned char 	comp;	/* Compression Type		*/
	char 	name[IH_NMLEN];	/* Image Name		*/
}AmlResItemHead_t;
#pragma pack(pop)

//typedef for amlogic resource image
#pragma pack(push, 4)
typedef struct {
    __u32   crc;    //crc32 value for the resouces image
    __s32   version;//0x01 means 'AmlResItemHead_t' attach to each item , 0x02 means all 'AmlResItemHead_t' at the head

    __u8    magic[AML_RES_IMG_V1_MAGIC_LEN];  //resources images magic

    __u32   imgSz;  //total image size in byte
    __u32   imgItemNum;//total item packed in the image

    __u32   alignSz;//AML_RES_IMG_ITEM_ALIGN_SZ
    __u8    reserv[AML_RES_IMG_HEAD_SZ - 8 * 3 - 4];

}AmlResImgHead_t;
#pragma pack(pop)

/*The Amlogic resouce image is consisted of a AmlResImgHead_t and many 
 * 
 * |<---AmlResImgHead_t-->|<--AmlResItemHead_t-->---...--|<--AmlResItemHead_t-->---...--|....
 * 
 */

#endif//ifndef __IMGPACK_H__

