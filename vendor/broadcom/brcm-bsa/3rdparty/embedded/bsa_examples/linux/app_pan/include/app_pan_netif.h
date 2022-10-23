/*****************************************************************************
 **
 **  Name:           app_pan_netif.h
 **
 **  Description:    Bluetooth PAN application
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_PAN_NETIF_H
#define APP_PAN_NETIF_H

#include <sys/types.h>

typedef int (*tAPP_PAN_NETIF_INIT)(void);
typedef int (*tAPP_PAN_NETIF_OPEN)(BD_ADDR bd_addr);
typedef int (*tAPP_PAN_NETIF_CLOSE)(int fd);
typedef ssize_t (*tAPP_PAN_NETIF_READ)(int fd, void *buf, size_t count);
typedef ssize_t (*tAPP_PAN_NETIF_WRITE)(int fd, const void *buf, size_t count);

typedef struct
{
    tAPP_PAN_NETIF_INIT  init;
    tAPP_PAN_NETIF_OPEN  open;
    tAPP_PAN_NETIF_CLOSE close;
    tAPP_PAN_NETIF_READ  read;
    tAPP_PAN_NETIF_WRITE write;
} tAPP_PAN_NETIF;

extern const tAPP_PAN_NETIF app_pan_netif;

#endif /* APP_PAN_NETIF_H */
