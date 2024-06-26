﻿/// @file common.h
///
/// common.h
///

#ifndef _COMMON_H_
#define _COMMON_H_

#include <wx/platform.h>
//#include "version.h"


//#define _UNICODE
//#define wxUSE_UNICODE 1
//#define wxUSE_WCHAR_T 1

#ifndef _MAX_PATH
#define _MAX_PATH	260
#endif

#if defined(__WXMSW__)

#include <stdint.h>

// ignore warning
#ifndef __GNUC__
//#pragma warning(disable:4482)
#pragma warning(disable:4996)
#endif

#else

#include "tchar.h"
#include "typedef.h"

#if defined(__WXOSX__)

wchar_t *_wgetenv(const wchar_t *);
int _wsystem(const wchar_t *);

#elif defined(__WXGTK__)

wchar_t *_wgetenv(const wchar_t *);
int _wsystem(const wchar_t *);

#endif
#endif

#endif /* _COMMON_H_ */
