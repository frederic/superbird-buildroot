/*****************************************************************************
**
**  Name:           hh_bthid.h
**
**  Description:    This file contains the functions implementation
**                  to pass HID reports/descriptor received from peer HID
**                  devices to the Linux kernel (via bthid module)
**
**  Copyright (c) 2010, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef _HH_BTHID_H_
#define _HH_BTHID_H_

/*******************************************************************************
**
** Function         hh_bthid_open
**
** Description      Open BTHID driver.
**
** Returns          int
**
*******************************************************************************/
int hh_bthid_open(void);

/*******************************************************************************
**
** Function         hh_bthid_close
**
** Description      This function is executed by app HH when connection is closed
**
** Parameters       BTHID file descriptor
**
**
** Returns          int
**
*******************************************************************************/
int hh_bthid_close(int bthid_fd);


/******************************************************************************
**                                                                            *
** Function:    hh_bthid_report_write                                         *
**                                                                            *
** Description: Send Bluetooth HID report to bthid driver for processing      *
**              bthid expects that application has sent HID DSCP to driver    *
**              this tells Linux system how to parse the reports              *
**              The bthid.ko driver will send the report to the Linux kernel  *
**                                                                            *
** Returns:     int                                                          *
**                                                                            *
*******************************************************************************/
int hh_bthid_report_write(int fd, unsigned char *p_rpt, int size);

/******************************************************************************
**                                                                            *
** Function:    hh_bthid_desc_write                                           *
**                                                                            *
** Description: Send Bluetooth HID Descriptor to bthid driver for processing  *
**                                                                            *
** Returns:     int                                                          *
**                                                                            *
*******************************************************************************/
int hh_bthid_desc_write(int fd, unsigned char *p_dscp, unsigned int size);

#endif

