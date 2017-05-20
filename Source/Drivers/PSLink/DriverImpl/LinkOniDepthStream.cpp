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
#include "LinkOniDepthStream.h"

//---------------------------------------------------------------------------
// LinkOniDepthStream class
//---------------------------------------------------------------------------
#define XN_MASK_LINK_STREAM "LinkDepthStream"

LinkOniDepthStream::LinkOniDepthStream(const char* configFile, xn::PrimeClient* pSensor, LinkOniDevice* pDevice) : 
	LinkOniMapStream(configFile, "Depth", pSensor, ONI_SENSOR_DEPTH, pDevice)
{
}

OniStatus LinkOniDepthStream::getProperty(int propertyId, void* data, int* pDataSize)
{
	const XnShiftToDepthTables* pTables = NULL;
	XnStatus nRetVal;
	XnFloat fValue = 0;
	XnUInt32 nTableSize;

	switch (propertyId)
	{
	// int props
	case ONI_STREAM_PROPERTY_MIN_VALUE:
		ENSURE_PROP_SIZE(*pDataSize, int);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, 0);
		break;

	case ONI_STREAM_PROPERTY_MAX_VALUE:
		ENSURE_PROP_SIZE(*pDataSize, int);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().nDeviceMaxDepthValue);
		break;

	case LINK_PROP_MAX_SHIFT:
		ENSURE_PROP_SIZE(*pDataSize, int);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().nDeviceMaxShiftValue);
		break;
		
	case LINK_PROP_ZERO_PLANE_DISTANCE:
		ENSURE_PROP_SIZE(*pDataSize, OniDepthPixel);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().nZeroPlaneDistance);
		break;

	case LINK_PROP_CONST_SHIFT:
		ENSURE_PROP_SIZE(*pDataSize, int);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize,  m_pInputStream->GetShiftToDepthConfig().nConstShift);
		break;

	case LINK_PROP_PARAM_COEFF:
		ENSURE_PROP_SIZE(*pDataSize, int);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().nParamCoeff);
		break;

	case LINK_PROP_SHIFT_SCALE:
		ENSURE_PROP_SIZE(*pDataSize, int);
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().nShiftScale);
		break;

	// real props
	//TODO: consider moving these two to MapStream
	case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
		ENSURE_PROP_SIZE(*pDataSize, XnFloat);
		m_pInputStream->GetFieldOfView(&fValue, NULL);
		ASSIGN_PROP_VALUE_FLOAT(data, *pDataSize, fValue);
		break;

	case ONI_STREAM_PROPERTY_VERTICAL_FOV:
		ENSURE_PROP_SIZE(*pDataSize, XnFloat);
		m_pInputStream->GetFieldOfView(NULL, &fValue);
		ASSIGN_PROP_VALUE_FLOAT(data, *pDataSize, fValue);
		break;

	case LINK_PROP_ZERO_PLANE_PIXEL_SIZE:
		ENSURE_PROP_SIZE(*pDataSize, XnFloat);
		ASSIGN_PROP_VALUE_FLOAT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().fZeroPlanePixelSize);
		break;
		
	case LINK_PROP_ZERO_PLANE_OUTPUT_PIXEL_SIZE:
		ENSURE_PROP_SIZE(*pDataSize, XnDouble);
		ASSIGN_PROP_VALUE_FLOAT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().nZeroPlaneDistance / m_pInputStream->GetCameraIntrinsics().m_fEffectiveFocalLengthInPixels);
		break;

	case LINK_PROP_EMITTER_DEPTH_CMOS_DISTANCE:
		ENSURE_PROP_SIZE(*pDataSize, XnFloat);
		ASSIGN_PROP_VALUE_FLOAT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().fEmitterDCmosDistance);
		break;

	case LINK_PROP_DEPTH_SCALE:
		ENSURE_PROP_SIZE(*pDataSize, XnDouble);
		ASSIGN_PROP_VALUE_FLOAT(data, *pDataSize, m_pInputStream->GetShiftToDepthConfig().dDepthScale);
		break;

	// tables
	case LINK_PROP_SHIFT_TO_DEPTH_TABLE:
		nRetVal = m_pInputStream->GetShiftToDepthTables(pTables);
		XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

		nTableSize = pTables->nShiftsCount * sizeof(OniDepthPixel);
		if (*pDataSize < (int)nTableSize)
		{
			xnLogError(XN_MASK_LINK_STREAM, "Get S2D table - buffer too small (expected %d, got %d)", nTableSize, *pDataSize);
			return ONI_STATUS_BAD_PARAMETER;
		}

		xnOSMemCopy(data, pTables->pShiftToDepthTable, nTableSize);
		break;
	
	case LINK_PROP_DEPTH_TO_SHIFT_TABLE:
		nRetVal = m_pInputStream->GetShiftToDepthTables(pTables);
		XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

		nTableSize = pTables->nDepthsCount * sizeof(XnUInt16);
		if (*pDataSize < (int)nTableSize)
		{
			xnLogError(XN_MASK_LINK_STREAM, "Get D2S table - buffer too small (expected %d, got %d)", nTableSize, *pDataSize);
			return ONI_STATUS_BAD_PARAMETER;
		}

		xnOSMemCopy(data, pTables->pDepthToShiftTable, nTableSize);
		break;

	default:
		return LinkOniMapStream::getProperty(propertyId, data, pDataSize);
	}

	return ONI_STATUS_OK;
}

OniBool LinkOniDepthStream::isPropertySupported(int propertyId)
{
	switch(propertyId)
	{
	// int props
	case ONI_STREAM_PROPERTY_MIN_VALUE:
	case ONI_STREAM_PROPERTY_MAX_VALUE:
	case LINK_PROP_MAX_SHIFT:
	case LINK_PROP_ZERO_PLANE_DISTANCE:
	case LINK_PROP_CONST_SHIFT:
	case LINK_PROP_PARAM_COEFF:
	case LINK_PROP_SHIFT_SCALE:
	// real props
		//TODO: consider moving these two to MapStream
	case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
	case ONI_STREAM_PROPERTY_VERTICAL_FOV:
	case LINK_PROP_ZERO_PLANE_PIXEL_SIZE:
	case LINK_PROP_ZERO_PLANE_OUTPUT_PIXEL_SIZE:
	case LINK_PROP_EMITTER_DEPTH_CMOS_DISTANCE:
	case LINK_PROP_DEPTH_SCALE:
	// tables
	case LINK_PROP_SHIFT_TO_DEPTH_TABLE:
	case LINK_PROP_DEPTH_TO_SHIFT_TABLE:
		return true;
	default:
		return LinkOniMapStream::isPropertySupported(propertyId);
	}
}

void LinkOniDepthStream::notifyAllProperties()
{
	LinkOniMapStream::notifyAllProperties();
	
	// int props
	int nValue;
	int size = sizeof(nValue);

	getProperty(LINK_PROP_MAX_SHIFT, &nValue, &size);
	raisePropertyChanged(LINK_PROP_MAX_SHIFT, &nValue, size);

	getProperty(LINK_PROP_ZERO_PLANE_DISTANCE, &nValue, &size);
	raisePropertyChanged(LINK_PROP_ZERO_PLANE_DISTANCE, &nValue, size);

	getProperty(LINK_PROP_CONST_SHIFT, &nValue, &size);
	raisePropertyChanged(LINK_PROP_CONST_SHIFT, &nValue, size);

	getProperty(LINK_PROP_PARAM_COEFF, &nValue, &size);
	raisePropertyChanged(LINK_PROP_PARAM_COEFF, &nValue, size);

	getProperty(LINK_PROP_SHIFT_SCALE, &nValue, &size);
	raisePropertyChanged(LINK_PROP_SHIFT_SCALE, &nValue, size);

	// real props
	XnDouble dValue;
	size = sizeof(dValue);
	//TODO: consider moving these two to MapStream
	getProperty(ONI_STREAM_PROPERTY_VERTICAL_FOV, &dValue, &size);
	raisePropertyChanged(ONI_STREAM_PROPERTY_VERTICAL_FOV, &dValue, size);

	getProperty(LINK_PROP_ZERO_PLANE_PIXEL_SIZE, &dValue, &size);
	raisePropertyChanged(LINK_PROP_ZERO_PLANE_PIXEL_SIZE, &dValue, size);

	getProperty(LINK_PROP_ZERO_PLANE_OUTPUT_PIXEL_SIZE, &dValue, &size);
	raisePropertyChanged(LINK_PROP_ZERO_PLANE_OUTPUT_PIXEL_SIZE, &dValue, size);

	getProperty(LINK_PROP_EMITTER_DEPTH_CMOS_DISTANCE, &dValue, &size);
	raisePropertyChanged(LINK_PROP_EMITTER_DEPTH_CMOS_DISTANCE, &dValue, size);

	getProperty(LINK_PROP_DEPTH_SCALE, &dValue, &size);
	raisePropertyChanged(LINK_PROP_DEPTH_SCALE, &dValue, size);

	// tables
	const XnShiftToDepthTables* pTables = NULL;
	m_pInputStream->GetShiftToDepthTables(pTables);

	raisePropertyChanged(LINK_PROP_SHIFT_TO_DEPTH_TABLE, pTables->pShiftToDepthTable, pTables->nShiftsCount * sizeof(OniDepthPixel));

	raisePropertyChanged(LINK_PROP_DEPTH_TO_SHIFT_TABLE, pTables->pDepthToShiftTable, pTables->nDepthsCount * sizeof(XnUInt16));
}

XnStatus LinkOniDepthStream::GetDefaultVideoMode( OniVideoMode* pVideoMode )
{
	if(pVideoMode != NULL)
	{
		//ARM cannot handle QVGA, so we default to QQVGA
#if (XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_ANDROID_ARM || XN_PLATFORM == XN_PLATFORM_LINUX_AARCH64)
		pVideoMode->resolutionX = 160;
		pVideoMode->resolutionY = 120;
#else
		pVideoMode->resolutionX = 320;
		pVideoMode->resolutionY = 240;
#endif		

		return XN_STATUS_OK;
	}
	else
	{
		return LinkOniMapStream::GetDefaultVideoMode(pVideoMode);
	}
}
