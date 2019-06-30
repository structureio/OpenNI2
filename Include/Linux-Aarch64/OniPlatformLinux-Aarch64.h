/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
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
#ifndef _ONI_PLATFORM_LINUX_AARCH64_H_
#define _ONI_PLATFORM_LINUX_AARCH64_H_

// Start with Linux-x86, and override what's different
#include "../Linux-x86/OniPlatformLinux-x86.h"

//---------------------------------------------------------------------------
// Platform Basic Definition
//---------------------------------------------------------------------------
#undef ONI_PLATFORM
#undef ONI_PLATFORM_STRING
#define ONI_PLATFORM ONI_PLATFORM_LINUX_AARCH64
#define ONI_PLATFORM_STRING "Linux-Aarch64"

//---------------------------------------------------------------------------
// Platform Capabilities
//---------------------------------------------------------------------------
#undef ONI_PLATFORM_ENDIAN_TYPE
#ifdef __ARM_BIG_ENDIAN
#define ONI_PLATFORM_ENDIAN_TYPE ONI_PLATFORM_IS_BIG_ENDIAN
#else
#define ONI_PLATFORM_ENDIAN_TYPE ONI_PLATFORM_IS_LITTLE_ENDIAN
#endif

#endif //_ONI_PLATFORM_LINUX_AARCH64_H_
