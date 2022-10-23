/*
 * Misc useful routines to access NIC local SROM/OTP .
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: bcmsrom.h 471127 2014-04-17 23:24:23Z $
 */

#ifndef	_bcmsrom_h_
#define	_bcmsrom_h_

#include <typedefs.h>
#include <osl_decl.h>
#include <siutils.h>

#include <bcmsrom_fmt.h>

/* Prototypes */
extern int srom_var_init(si_t *sih, uint bus, void *curmap, osl_t *osh,
                         char **vars, uint *count);
extern void srom_var_deinit(si_t *sih);

extern int srom_read(si_t *sih, uint bus, void *curmap, osl_t *osh,
                     uint byteoff, uint nbytes, uint16 *buf,
                     bool check_crc);

extern int srom_write(si_t *sih, uint bus, void *curmap, osl_t *osh,
                      uint byteoff, uint nbytes, uint16 *buf);

extern int srom_write_short(si_t *sih, uint bustype, void *curmap, osl_t *osh,
                            uint byteoff, uint16 value);
extern int srom_otp_cisrwvar(si_t *sih, osl_t *osh, char *vars, int *count);
extern int srom_otp_write_region_crc(si_t *sih, uint nbytes, uint16* buf16, bool write);

/* parse standard PCMCIA cis, normally used by SB/PCMCIA/SDIO/SPI/OTP
 *   and extract from it into name=value pairs
 */
extern int srom_probe_boardtype(si_t *sih, uint8 *pcis[], uint ciscnt);
extern int srom_parsecis(si_t *sih, osl_t *osh, uint8 **pcis, uint ciscnt,
                         char **vars, uint *count);


#endif	/* _bcmsrom_h_ */
