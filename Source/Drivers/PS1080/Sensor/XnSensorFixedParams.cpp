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
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "XnSensorFixedParams.h"
#include "XnHostProtocol.h"

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------
XnSensorFixedParams::XnSensorFixedParams(XnDevicePrivateData* pDevicePrivateData) :
	m_pDevicePrivateData(pDevicePrivateData),
	m_nSensorDepthCMOSI2CBus(0),
	m_nSensorDepthCMOSI2CSlaveAddress(0),
	m_nSensorImageCMOSI2CBus(0),
	m_nSensorImageCMOSI2CSlaveAddress(0),
	m_dZeroPlaneDistance(0),
	m_dZeroPlanePixelSize(0),
	m_dEmitterDCmosDistance(0),
	m_dDCmosRCmosDistance(0),
	m_nImageCmosType(0),
	m_nDepthCmosType(0)
{
	m_strSensorSerial[0] = '\0';
	m_deviceInfo.strDeviceName[0] = '\0';
	m_deviceInfo.strVendorData[0] = '\0';
	m_strPlatformString[0] = '\0';
}

XnStatus XnSensorFixedParams::Init()
{
	XnStatus nRetVal = XN_STATUS_OK;

	// get fixed params
	XnFixedParams FixedParams;
	nRetVal = XnHostProtocolGetFixedParams(m_pDevicePrivateData, FixedParams);
	if (nRetVal != XN_STATUS_OK)
	{
		// Ugly patch since get param is not supported in maintenance mode!
		if (nRetVal != XN_STATUS_DEVICE_PROTOCOL_INVALID_COMMAND)
		{
			return nRetVal;
		}
		return nRetVal;
	}

	if (m_pDevicePrivateData->FWInfo.nFWVer < XN_SENSOR_FW_VER_5_4)
	{
		sprintf(m_strSensorSerial, "%d", FixedParams.nSerialNumber);
	}
	else
	{	
		nRetVal = XnHostProtocolGetSerialNumber(m_pDevicePrivateData, m_strSensorSerial);

		if (nRetVal != XN_STATUS_OK)
		{
			return nRetVal;
		}
	}

	xnLogVerbose(XN_MASK_DEVICE_SENSOR, "Sensor serial number: %s", m_strSensorSerial);

	// fill in properties
	m_dZeroPlaneDistance = FixedParams.fReferenceDistance;
	m_dZeroPlanePixelSize = FixedParams.fReferencePixelSize;
	m_dEmitterDCmosDistance = FixedParams.fDCmosEmitterDistance;
	m_dDCmosRCmosDistance = FixedParams.fDCmosRCmosDistance;

	m_nSensorDepthCMOSI2CBus = (XnUInt16)FixedParams.nDepthCmosI2CBus;
	m_nSensorDepthCMOSI2CSlaveAddress = (XnUInt16)FixedParams.nDepthCmosI2CAddress;
	m_nSensorImageCMOSI2CBus = (XnUInt16)FixedParams.nImageCmosI2CBus;
	m_nSensorImageCMOSI2CSlaveAddress = (XnUInt16)FixedParams.nImageCmosI2CAddress;
	
	m_nImageCmosType = (XnUInt32)FixedParams.nImageCmosType;
	m_nDepthCmosType = (XnUInt32)FixedParams.nDepthCmosType;

	nRetVal = XnHostProtocolAlgorithmParams(m_pDevicePrivateData, XN_HOST_PROTOCOL_ALGORITHM_DEVICE_INFO, 
		&m_deviceInfo, sizeof(m_deviceInfo), (XnResolutions)0, 0);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnHostProtocolGetPlatformString(m_pDevicePrivateData, m_strPlatformString);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}
