/*****************************************************************************
 **
 **  Name:           app_pan_tap.c
 **
 **  Description:    Sample application for TAP interface.
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include <signal.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/sockios.h>
#include <sys/prctl.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include "data_types.h"
#include "app_utils.h"
#include "app_pan_netif.h"

#ifndef APP_PAN_TAP_IF_NAME
#define APP_PAN_TAP_IF_NAME "tap%d"
#endif

#ifndef APP_PAN_TAP_CLONE_DEV
#define APP_PAN_TAP_CLONE_DEV "/dev/net/tun"
#endif

#define APP_PAN_TAP_BAD_FD (-1)

typedef struct
{
    int tap_fd;
    char tap_name[IFNAMSIZ];
} tAPP_PAN_TAP_CB;

tAPP_PAN_TAP_CB app_pan_tap_cb;

/*
 * Local functions
 */
static int app_pan_tap_init(void);
static int app_pan_tap_open(BD_ADDR bd_addr);
static int app_pan_tap_close(int fd);
static ssize_t app_pan_tap_read(int fd, void *buf, size_t count);
static ssize_t app_pan_tap_write(int fd, const void *buf, size_t count);

/*
 * Function table
 */
const tAPP_PAN_NETIF app_pan_netif = {
    app_pan_tap_init,
    app_pan_tap_open,
    app_pan_tap_close,
    app_pan_tap_read,
    app_pan_tap_write,
};

/******************************************************************************
 **
 ** Function         app_pan_tap_init
 **
 ** Description      init TAP interface
 **
 ** Returns          status
 **
 ******************************************************************************/
static int app_pan_tap_init(void)
{
    memset(&app_pan_tap_cb, 0, sizeof(app_pan_tap_cb));
    app_pan_tap_cb.tap_fd = APP_PAN_TAP_BAD_FD;

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_tap_open
 **
 ** Description      open and up TAP interface
 **
 ** Returns          tap fd (-1 = error)
 **
 ******************************************************************************/
static int app_pan_tap_open(BD_ADDR bd_addr)
{
    int fd;
    struct ifreq ifr;
    int err;
    int sk;

    if (app_pan_tap_cb.tap_fd != APP_PAN_TAP_BAD_FD)
    {
        APP_ERROR1("TAP already opened fd:%d", app_pan_tap_cb.tap_fd);
        return -1;
    }

    /* open the clone device */
    fd = open(APP_PAN_TAP_CLONE_DEV, O_RDWR);
    if (fd < 0)
    {
        APP_ERROR1("could not open %s, err:%d", APP_PAN_TAP_CLONE_DEV, errno);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, APP_PAN_TAP_IF_NAME, IFNAMSIZ);

    /* try to create the device */
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0)
    {
        APP_ERROR1("ioctl error:%d, errno:%s", err, strerror(errno));
        close(fd);
        return -1;
    }

#ifndef TUNGETIFF
    APP_ERROR0("TUNGETIFF was not defined!");
    close(fd);
    return -1;
#else
    if ((err = ioctl(fd, TUNGETIFF, (void *)&ifr)) < 0)
    {
        APP_ERROR1("ioctl error:%d, errno:%s", err, strerror(errno));

        close(fd);

        return -1;
    }
#endif
    strncpy(app_pan_tap_cb.tap_name, ifr.ifr_name, IFNAMSIZ - 1);

    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        APP_ERROR1("socket error:%d, errno:%s", sk, strerror(errno));
        close(fd);
        return -1;
    }

    /* set mac addr */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, app_pan_tap_cb.tap_name, IFNAMSIZ - 1);
    err = ioctl(sk, SIOCGIFHWADDR, &ifr);
    if(err < 0)
    {
        APP_ERROR1("Could not get network hardware for interface:%s, errno:%s",
                app_pan_tap_cb.tap_name, strerror(errno));

        close(sk);
        close(fd);

        return -1;
    }
    strncpy(ifr.ifr_name, app_pan_tap_cb.tap_name, IFNAMSIZ - 1);
    memcpy(ifr.ifr_hwaddr.sa_data, bd_addr, 6);
    err = ioctl(sk, SIOCSIFHWADDR, (caddr_t)&ifr);
    if (err < 0)
    {
        APP_ERROR1("Could not set bt address for interface:%s, errno:%s",
                app_pan_tap_cb.tap_name, strerror(errno));

        close(sk);
        close(fd);

        return -1;
    }

    /* bring it up */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, app_pan_tap_cb.tap_name, IFNAMSIZ - 1);
    ifr.ifr_flags |= IFF_UP;
    ifr.ifr_flags |= IFF_MULTICAST;
    err = ioctl(sk, SIOCSIFFLAGS, (caddr_t) &ifr);
    if (err < 0) {
        APP_ERROR1("Could not bring up network interface:%s, errno:%d",
                app_pan_tap_cb.tap_name, errno);

        close(sk);
        close(fd);

        return -1;
    }

    close(sk);

    app_pan_tap_cb.tap_fd = fd;

    return app_pan_tap_cb.tap_fd;
}

/******************************************************************************
 **
 ** Function         app_pan_tap_close
 **
 ** Description      close TAP interface
 **
 ** Returns          status (0 = success, -1 = error)
 **
 ******************************************************************************/
static int app_pan_tap_close(int fd)
{
    struct ifreq ifr;
    int sk;

    if (app_pan_tap_cb.tap_fd != fd ||
            app_pan_tap_cb.tap_fd == APP_PAN_TAP_BAD_FD)
    {
        return -1;
    }

    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        APP_ERROR1("socket error:%d, errno:%s", sk, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, app_pan_tap_cb.tap_name, IFNAMSIZ - 1);
    ifr.ifr_flags &= ~IFF_UP;
    ioctl(sk, SIOCSIFFLAGS, (caddr_t) &ifr);
    close(sk);

    close(fd);

    app_pan_tap_cb.tap_fd = APP_PAN_TAP_BAD_FD;

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_tap_read
 **
 ** Description      read data from TAP interface
 **
 ** Returns          void
 **
 ******************************************************************************/
static ssize_t app_pan_tap_read(int fd, void *buf, size_t count)
{
    int ret;

    if (app_pan_tap_cb.tap_fd != fd ||
            app_pan_tap_cb.tap_fd == APP_PAN_TAP_BAD_FD)
    {
        return -1;
    }

    ret = read(fd, buf, count);
    if (ret < 0)
    {
        APP_DEBUG1("read data from TAP. Status:%d", ret);

        return ret;
    }

    return ret;
}

/******************************************************************************
 **
 ** Function         app_pan_tap_write
 **
 ** Description      write data to TAP interface
 **
 ** Returns          stats (0 = success, -1 = error)
 **
 ******************************************************************************/
static ssize_t app_pan_tap_write(int fd, const void *buf, size_t count)
{
    int ret;

    if (app_pan_tap_cb.tap_fd != fd ||
            app_pan_tap_cb.tap_fd == APP_PAN_TAP_BAD_FD)
    {
        return -1;
    }

    ret = write(fd, buf, count);
    if (ret < 0 || (size_t)ret != count)
    {
        APP_ERROR1("TAP write ret:%d (expected %d)", ret, count);

        return ret;
    }

    return ret;
}
