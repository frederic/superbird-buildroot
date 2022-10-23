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

#ifndef __UNIFYKEYS_H
#define __UNIFYKEYS_H

#define UNIFYKEYS_PATH "/dev/unifykeys"

int Aml_Util_UnifyKeyInit(const char *path);
int Aml_Util_UnifyKeyRead(const char *path, char *name, char *refbuf);
int Aml_Util_UnifyKeyWrite(const char *path, char *buff, char *name);

#endif
