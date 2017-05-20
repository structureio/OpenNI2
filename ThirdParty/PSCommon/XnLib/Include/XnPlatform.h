/*****************************************************************************
*                                                                            *
*  PrimeSense PSCommon Library                                               *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of PSCommon.                                            *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#ifndef _XN_PLATFORM_H_
#define _XN_PLATFORM_H_

#define XN_PLATFORM_WIN32 1
#define XN_PLATFORM_XBOX360 2
#define XN_PLATFORM_PS3 3
#define XN_PLATFORM_WII 4
#define XN_PLATFORM_LINUX_X86 5
#define XN_PLATFORM_FILES_ONLY 6
#define XN_PLATFORM_ARC 6
#define XN_PLATFORM_LINUX_ARM 7
#define XN_PLATFORM_MACOSX 8
#define XN_PLATFORM_ANDROID_ARM 9
#define XN_PLATFORM_LINUX_AARCH64 10

#define XN_PLATFORM_IS_LITTLE_ENDIAN 1
#define XN_PLATFORM_IS_BIG_ENDIAN    2

#define XN_PLATFORM_USE_NO_VAARGS 1
#define XN_PLATFORM_USE_WIN32_VAARGS_STYLE 2
#define XN_PLATFORM_USE_GCC_VAARGS_STYLE   3
#define XN_PLATFORM_USE_ARC_VAARGS_STYLE   4

#if (defined _WIN32)
#ifndef RC_INVOKED
#if _MSC_VER < 1300
#error OpenNI Platform Abstraction Layer - Win32 - Microsoft Visual Studio version below 2003 (7.0) are not supported!
#endif
#endif
#include "Win32/XnPlatformWin32.h"
#elif defined (ANDROID) && defined (__arm__)
#include "Android-Arm/XnPlatformAndroid-Arm.h"
#elif (__linux__ && (i386 || __x86_64__))
#include "Linux-x86/XnPlatformLinux-x86.h"
#elif (__linux__ && (__arm__ || __aarch64__))
#include "Linux-Arm/XnPlatformLinux-Arm.h"
#elif _ARC
#include "ARC/XnPlaformARC.h"
#elif (__APPLE__)
#include "MacOSX/XnPlatformMacOSX.h"
#else
#error Xiron Platform Abstraction Layer - Unsupported Platform!
#endif

#define XN_MAX_OS_NAME_LENGTH 255

typedef struct xnOSInfo
{
	XnChar csOSName[XN_MAX_OS_NAME_LENGTH];
	XnChar csCPUName[XN_MAX_OS_NAME_LENGTH];
	XnUInt32 nProcessorsCount;
	XnUInt64 nTotalMemory;
} xnOSInfo;

#ifdef __cplusplus
#define XN_C_API_EXPORT extern "C" XN_API_EXPORT
#define XN_C_API_IMPORT extern "C" XN_API_IMPORT
#define XN_CPP_API_EXPORT XN_API_EXPORT
#define XN_CPP_API_IMPORT XN_API_IMPORT
#define XN_C_API extern "C"
#define XN_CPP_API 
#else // __cplusplus
#define XN_C_API_EXPORT XN_API_EXPORT
#define XN_C_API_IMPORT XN_API_IMPORT
#define XN_C_API
#endif  // __cplusplus

struct XnCallbackHandleImpl;
typedef struct XnCallbackHandleImpl* XnCallbackHandle;
typedef int XnStatus;

#ifndef TRUE
#define TRUE 1
#endif //TRUE
#ifndef FALSE
#define FALSE 0
#endif //FALSE

#define XN_MIN(a,b)            (((a) < (b)) ? (a) : (b))

#define XN_MAX(a,b)            (((a) > (b)) ? (a) : (b))

typedef void (*XnFuncPtr)();

struct XnRegistrationHandleImpl;
typedef struct XnRegistrationHandleImpl* XnRegistrationHandle;

#endif // _XN_PLATFORM_H_
