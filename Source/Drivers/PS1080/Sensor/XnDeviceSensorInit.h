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
#ifndef XNDEVICESENSORINIT_H
#define XNDEVICESENSORINIT_H

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "XnDeviceSensor.h"
#include "XnDeviceSensorProtocol.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

#if XN_PLATFORM == XN_PLATFORM_WIN32
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_MULTIPLIER_ISO				8*10
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_MULTIPLIER_BULK				120
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_MULTIPLIER_LOWBAND_ISO		8*5
	#define XN_SENSOR_USB_IMAGE_BUFFERS									8

	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE_MULTIPLIER_ISO				8*10
	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE_MULTIPLIER_BULK				120
	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE_MULTIPLIER_LOWBAND_ISO		8*5
	#define XN_SENSOR_USB_DEPTH_BUFFERS									8
	#define XN_SENSOR_USB_DEPTH_BUFFERS_LOW_ISO							4

	#define XN_SENSOR_USB_MISC_BUFFER_SIZE_MULTIPLIER_ISO				104
	#define XN_SENSOR_USB_MISC_BUFFER_SIZE_MULTIPLIER_BULK				20
	#define XN_SENSOR_USB_MISC_BUFFER_SIZE_MULTIPLIER_LOWBAND_ISO		52
	#define XN_SENSOR_USB_MISC_BUFFERS									8
#elif XN_PLATFORM == XN_PLATFORM_PS3
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_ISO		0x1E000
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_BULK	0x4000
	#define XN_SENSOR_USB_IMAGE_BUFFERS		1

	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE	0x4000
	#define XN_SENSOR_USB_DEPTH_BUFFERS		2

	#define XN_SENSOR_USB_MISC_BUFFER_SIZE	0x1000
	#define XN_SENSOR_USB_MISC_BUFFERS		1
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86 || XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_MACOSX || XN_PLATFORM == XN_PLATFORM_ANDROID_ARM || XN_PLATFORM_LINUX_GENERIC)
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_MULTIPLIER_ISO				32
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_MULTIPLIER_BULK				40
	#define XN_SENSOR_USB_IMAGE_BUFFER_SIZE_MULTIPLIER_LOWBAND_ISO		16
	#define XN_SENSOR_USB_IMAGE_BUFFERS									16

	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE_MULTIPLIER_ISO				32
	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE_MULTIPLIER_BULK				40
	#define XN_SENSOR_USB_DEPTH_BUFFER_SIZE_MULTIPLIER_LOWBAND_ISO		16
	#define XN_SENSOR_USB_DEPTH_BUFFERS									16
	#define XN_SENSOR_USB_DEPTH_BUFFERS_LOW_ISO							4

	#define XN_SENSOR_USB_MISC_BUFFER_SIZE_MULTIPLIER_ISO				104
	#define XN_SENSOR_USB_MISC_BUFFER_SIZE_MULTIPLIER_BULK				20
	#define XN_SENSOR_USB_MISC_BUFFER_SIZE_MULTIPLIER_LOWBAND_ISO		52
	#define XN_SENSOR_USB_MISC_BUFFERS									5
#endif

#define XN_SENSOR_READ_THREAD_TIMEOUT_ISO	100
#define XN_SENSOR_READ_THREAD_TIMEOUT_BULK	1000

//---------------------------------------------------------------------------
// Functions Declaration
//---------------------------------------------------------------------------
XnStatus XnDeviceSensorInit(XnDevicePrivateData* pDevicePrivateData);

XnStatus XnDeviceSensorAllocateBuffers(XnDevicePrivateData* pDevicePrivateData);
XnStatus XnDeviceSensorFreeBuffers(XnDevicePrivateData* pDevicePrivateData);

XnStatus XnDeviceSensorConfigureVersion(XnDevicePrivateData* pDevicePrivateData);

XnStatus XnDeviceSensorOpenInputThreads(XnDevicePrivateData* pDevicePrivateData);

XnStatus XnDeviceSensorConfigure(XnDevicePrivateData* pDevicePrivateData);

XnStatus XnDeviceSensorInitCmosData(XnDevicePrivateData* pDevicePrivateData);

#endif // XNDEVICESENSORINIT_H
