/****************************************************************************
 *
 *		Utility types
 *		-------------
 *
 ****************************************************************************
 *
 *	Description:	Utility types
 *
 *	Copyright:		Copyright DEMA Consulting 2008-2012, All rights reserved.
 *	Owner:			DEMA Consulting.
 *					93 Richardson Road
 *					Dublin, NH 03444
 *
 ***************************************************************************/

/**
 * @addtogroup Debugging
 * @{
 */

#ifndef _UTILTYPES_H_INCLUDED
#define _UTILTYPES_H_INCLUDED

#ifndef WIN32

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef IN_OUT
#define IN_OUT
#endif

#ifndef E_FAIL
#define E_FAIL			(-1)
#endif

#ifndef S_OK
#define S_OK			(0)
#endif

#ifndef LRESULT
#define LRESULT			int
#endif

#ifndef afx_msg
#define afx_msg
#endif

#ifndef FAILED
#define FAILED(hr)		((hr) < 0)
#endif

#ifndef DLLSYMBOL
#define DLLSYMBOL
#endif

#ifndef HRESULT
#define HRESULT			int
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr)	((hr) >= 0)
#endif

#ifndef HWND
#define HWND			unsigned int
#endif

#ifndef BYTE
#define BYTE			unsigned char
#endif

#ifndef WORD
#define WORD			unsigned short
#endif

#ifndef DWORD
#define DWORD			unsigned long
#endif

#ifndef UINT
#define UINT			unsigned int
#endif

#ifndef HIWORD
#define HIWORD(x)		(((x) >> 16) & 0xffff)
#endif

#ifndef LOWORD
#define LOWORD(x)		((x) & 0xffff)
#endif

#endif	// !WIN32

#endif	// _UTILTYPES_H_INCLUDED

/**
 * @}
 * End of file.
 */
