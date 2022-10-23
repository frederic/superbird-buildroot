/*
 * STB params variable manipulation
 *
 * Copyright (C) 1999-2016, Broadcom Corporation
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
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id:stbutils.h $
 */

#ifndef _stbutils_h_
#define _stbutils_h_

#ifndef _LANGUAGE_ASSEMBLY

#include <typedefs.h>
#include <bcmdefs.h>

struct param_tuple {
	char *name;
	char *value;
	struct param_tuple *next;
};

/*
 * Initialize STB params file access.
 */
extern int stbpriv_init(osl_t *osh);
extern void stbpriv_exit(osl_t *osh);

/*
 * Append a chunk of nvram variables to the list
 */
extern int stbparam_append(osl_t *osh, char *paramlst, uint paramsz);

/*
 * Get the value of STB param variable
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */
extern char * stbparam_get(const char *name);

/*
 * Match STB param variable.
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is string equal
 *		to match or FALSE otherwise
 */
static INLINE int
stbparam_match(const char *name, const char *match)
{
	const char *value = stbparam_get(name);
	return (value && !strcmp(value, match));
}

/*
 * Get all STB variables (format name=value\0 ... \0\0).
 * @param	buf	buffer to store variables
 * @param	count	size of buffer in bytes
 * @return	0 on success and errno on failure
 */
extern int stbparams_getall(char *buf, int count);


#endif /* _LANGUAGE_ASSEMBLY */

#endif /* _stbutils_h_ */
