/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file provides extensions for functions not defined in <string.h>
 */

#ifndef STRING_EXT_H
#define STRING_EXT_H

#include <stddef.h>
#include <sys/cdefs.h>

/*
 * Copy src to string dst of siz size.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

/* A constant-time version of memcmp() */
int consttime_memcmp(const void *p1, const void *p2, size_t nb);

/* Deprecated. For backward compatibility. */
static inline int buf_compare_ct(const void *s1, const void *s2, size_t n)
{
	return consttime_memcmp(s1, s2, n);
}

/*
 * Like memset(s, 0, count) but prevents the compiler from optimizing the call
 * away. Such "dead store elimination" optimizations typically occur when
 * clearing a *local* variable that is not used after it is cleared; but
 * link-time optimization (LTO) can also trigger code elimination in other
 * circumstances. See "Dead Store Elimination (Still) Considered Harmful" [1]
 * for details and examples (and note that the Cland compiler enables LTO by
 * default!).
 *
 * [1] https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-yang.pdf
 *
 * Practically speaking:
 *
 * - Use memzero_explicit() to *clear* (as opposed to initialize) *sensitive*
 *   data (such as keys, passwords, cryptographic state);
 * - Otherwise, use memset().
 */
void memzero_explicit(void *s, size_t count);

#endif /* STRING_EXT_H */
