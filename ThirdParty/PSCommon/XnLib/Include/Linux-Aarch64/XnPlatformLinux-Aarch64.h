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
#ifndef _XN_PLATFORM_LINUX_AARCH64_H_
#define _XN_PLATFORM_LINUX_AARCH64_H_

// Start with Linux-x86, and override what's different
#include "../Linux-x86/XnPlatformLinux-x86.h"

//---------------------------------------------------------------------------
// Platform Basic Definition
//---------------------------------------------------------------------------
#undef XN_PLATFORM
#undef XN_PLATFORM_STRING
#define XN_PLATFORM XN_PLATFORM_LINUX_AARCH64
#define XN_PLATFORM_STRING "Linux-Aarch64"

//---------------------------------------------------------------------------
// Platform Capabilities
//---------------------------------------------------------------------------
#undef XN_PLATFORM_ENDIAN_TYPE
#ifdef __ARM_BIG_ENDIAN
#define XN_PLATFORM_ENDIAN_TYPE XN_PLATFORM_IS_BIG_ENDIAN
#else
#define XN_PLATFORM_ENDIAN_TYPE XN_PLATFORM_IS_LITTLE_ENDIAN
#endif

#endif //_XN_PLATFORM_LINUX_AARCH64_H_

