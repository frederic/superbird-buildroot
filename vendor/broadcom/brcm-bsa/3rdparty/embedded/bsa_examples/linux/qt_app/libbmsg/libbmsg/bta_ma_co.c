/*****************************************************************************
**
**  Name:           bta_ma_co.c
**
**  Description:    This file implements the common call-out functions for the
**                  Message Access Profile.
**
**  Copyright (c) 2003-2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "bt_target.h"
#include "bta_fs_co.h"
#include "bta_ma_co.h"


#ifndef O_BINARY
#define O_BINARY 0x8000
#endif


/*******************************************************************************
**
** Function         bta_ma_co_open
**
** Description      Open a file.
**                             
** Parameters       p_path - Full path of file to open.
**                  oflags - file open flags.
**
** Returns          file descriptor.  BTA_FS_INVALID_FD if open fails.
**
*******************************************************************************/
int  bta_ma_co_open(const char *p_path, int oflags)
{
    int o_flags = 0; /* Initially read only */
    int fd;
    int err=0;

    if (oflags & BTA_FS_O_RDWR)
        o_flags |= O_RDWR;
    else if (oflags & BTA_FS_O_WRONLY)
        o_flags |= O_WRONLY;

    if (oflags & BTA_FS_O_CREAT)
        o_flags |= O_CREAT;

    if (oflags & BTA_FS_O_EXCL)
        o_flags |= O_EXCL;

    if (oflags & BTA_FS_O_TRUNC)
        o_flags |= O_TRUNC;

    //if ((fd = open((char *)p_fname, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)

    if ((fd = open ((char *)p_path, (o_flags | O_BINARY) , 0666)) < 0)
    {
        err = errno;
        APPL_TRACE_ERROR1("file open err=%d",err);
    }

#if (BTA_MSE_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    if (err)
    {
        APPL_TRACE_DEBUG4("bta_ma_co_open: handle:%d err:%d, o_flags:%x, oflags(fs flags) :%x",
                          fd, err, o_flags,oflags );
    }
#endif
    return(fd);
}

/*******************************************************************************
**
** Function         bta_ma_co_write
**
** Description      Write data to file.
**                             
** Parameters       fd - file descriptor.
**                  buffer - data to write.
**                  size - size of data to write (in bytes).
**
** Returns          Number of bytes written.
**
*******************************************************************************/
int bta_ma_co_write(int fd, const void *buffer, int size)
{
    return(write(fd, buffer, size));
}

/*******************************************************************************
**
** Function         bta_ma_co_read
**
** Description      Read data from file.
**                             
** Parameters       fd - file descriptor.
**                  buffer - to receive data.
**                  size - amount of data to read (in bytes).
**
** Returns          Number of bytes read.
**
*******************************************************************************/
int bta_ma_co_read(int fd, void *buffer, int size)
{
    return(read(fd, buffer, size ));
}

/*******************************************************************************
**
** Function         bta_ma_co_close
**
** Description      Close the file.
**                             
** Parameters       fd - file descriptor.
**
** Returns          void
**
*******************************************************************************/
void bta_ma_co_close(int fd)
{
    close(fd);
}

