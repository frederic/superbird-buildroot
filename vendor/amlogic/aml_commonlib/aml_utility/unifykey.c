/*
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define KEYUNIFY_ATTACH     _IO('f', 0x60)
#define KEYUNIFY_GET_INFO   _IO('f', 0x62)

#define KEY_UNIFY_NAME_LEN  (48)

struct key_item_info_t {
    unsigned int id;
    char name[KEY_UNIFY_NAME_LEN];
    unsigned int size;
    unsigned int permit;
    unsigned int flag;
    unsigned int reserve;
};

int Aml_Util_UnifyKeyInit(const char *path)
{
    int fp;
    int ret;
    struct key_item_info_t key_item_info;

    fp = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }

    ret = ioctl(fp, KEYUNIFY_ATTACH, &key_item_info);
    close(fp);
    return ret;
}

void dump_mem(char *buffer, int count)
{
    int i;

    if (NULL == buffer || count == 0) {
        printf("%s() %d: %p, %d", __func__, __LINE__, buffer, count);
        return;
    }
    for (i=0; i<count ; i++) {
        if (i % 16 == 0)
            printf("dump data: \n");
        printf("%02x ", buffer[i]);
    }
    printf("\n");
}

void dump_keyitem_info(struct key_item_info_t *info)
{
    if (info == NULL)
        return;
    printf("id:	%d\n", info->id);
    printf("name:	%s\n", info->name);
    printf("size:	%d\n", info->size);
    printf("permit:	0x%x\n", info->permit);
    printf("flag:	0x%x\n", info->flag);
    return;
}

int Aml_Util_UnifyKeyRead(const char *path, char *name, char* refbuf)
{
    int ret = 0;
    char buffer[4096] = {0};
    unsigned long ppos;
    size_t readsize;
    int fp;
    struct key_item_info_t key_item_info;

    if ((NULL == path) || (NULL == name)) {
        printf("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }

    fp  = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -2;
    }

    /* seek the key index need operate. */
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    if (ret < 0) {
        close(fp);
        return ret;
    }

    // dump_keyitem_info(&key_item_info);

    ppos = key_item_info.id;
    lseek(fp, ppos, SEEK_SET);
    if (key_item_info.flag) {
        ret = read(fp, buffer, key_item_info.size);
        if (ret < 0) {
            close(fp);
            return ret;
        }
        // dump_mem(buffer, ret);
        strcpy(refbuf, buffer);
    }

    close(fp);
    return ret;
}

int Aml_Util_UnifyKeyWrite(const char *path, char *buff, char *name)
{
    int ret = 0;
    char buffer[4096] = {0};
    unsigned long ppos;
    size_t readsize, writesize;
    int fp;
    struct key_item_info_t key_item_info;

    if ((NULL == path) || (NULL == buff) || (NULL == name)) {
        printf("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }

    fp = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }

    /* seek the key index need operate. */
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    if (ret < 0) {
        close(fp);
        return ret;
    }

    // dump_keyitem_info(&key_item_info);

    ppos = key_item_info.id;
    lseek(fp, ppos, SEEK_SET);
    writesize = write(fp, buff, strlen(buff));
    if (writesize != strlen(buff)) {
        printf("%s() %d: write %s failed!\n", __func__, __LINE__,
            key_item_info.name);
        ret = -1;
    }

    close(fp);
    return ret;
}

