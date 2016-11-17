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
#ifndef XNSENSORFIXEDPARAMS_H
#define XNSENSORFIXEDPARAMS_H

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnStreamParams.h>
#include "XnDeviceSensor.h"

//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
class XnSensorFirmware;
struct XnDevicePrivateData;
typedef struct XnDevicePrivateData XnDevicePrivateData;

//---------------------------------------------------------------------------
// XnSensorFixedParams class
//---------------------------------------------------------------------------
class XnSensorFixedParams
{
public:
	XnSensorFixedParams(XnDevicePrivateData* pDevicePrivateData);

	XnStatus Init();

	inline XnUInt16 GetDepthCmosI2CBus() const { return m_nSensorDepthCMOSI2CBus; }
	inline XnUInt16 GetDepthCmosI2CSlaveAddress() const { return m_nSensorDepthCMOSI2CSlaveAddress; }
	inline XnUInt16 GetImageCmosI2CBus() const { return m_nSensorImageCMOSI2CBus; }
	inline XnUInt16 GetImageCmosI2CSlaveAddress() const { return m_nSensorImageCMOSI2CSlaveAddress; }

	inline XnDouble GetZeroPlaneDistance() const { return m_dZeroPlaneDistance; }
	inline XnDouble GetZeroPlanePixelSize() const { return m_dZeroPlanePixelSize; }
	inline XnDouble GetEmitterDCmosDistance() const { return m_dEmitterDCmosDistance; }
	inline XnDouble GetDCmosRCmosDistance() const { return m_dDCmosRCmosDistance; }

	inline const XnChar* GetSensorSerial() const { return m_strSensorSerial; }

	inline XnImageCMOSType GetImageCmosType() const { return (XnImageCMOSType)m_nImageCmosType; }
	inline XnDepthCMOSType GetDepthCmosType() const { return (XnDepthCMOSType)m_nDepthCmosType; }

	inline const XnChar* GetDeviceName() const { return m_deviceInfo.strDeviceName; }
	inline const XnChar* GetVendorData() const { return m_deviceInfo.strVendorData; }
	inline const XnChar* GetPlatformString() const { return m_strPlatformString; }

private:
	XnDevicePrivateData* m_pDevicePrivateData;

	XnUInt16 m_nSensorDepthCMOSI2CBus;
	XnUInt16 m_nSensorDepthCMOSI2CSlaveAddress;
	XnUInt16 m_nSensorImageCMOSI2CBus;
	XnUInt16 m_nSensorImageCMOSI2CSlaveAddress;

	XnDouble m_dZeroPlaneDistance;
	XnDouble m_dZeroPlanePixelSize;
	XnDouble m_dEmitterDCmosDistance;
	XnDouble m_dDCmosRCmosDistance;

	XnUInt32 m_nImageCmosType;
	XnUInt32 m_nDepthCmosType;

	XnChar m_strSensorSerial[XN_DEVICE_MAX_STRING_LENGTH];
	XnDeviceInformation m_deviceInfo;
	XnChar m_strPlatformString[XN_DEVICE_MAX_STRING_LENGTH];
};

#endif // XNSENSORFIXEDPARAMS_H
