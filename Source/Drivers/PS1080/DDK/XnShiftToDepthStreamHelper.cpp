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
#include "XnShiftToDepthStreamHelper.h"
#include <XnCore.h>

XnShiftToDepthStreamHelper::XnShiftToDepthStreamHelper() :
	m_ShiftToDepthTable(XN_STREAM_PROPERTY_S2D_TABLE, "S2D", NULL, 0, NULL),
	m_DepthToShiftTable(XN_STREAM_PROPERTY_D2S_TABLE, "D2S", NULL, 0, NULL),
	m_pModule(NULL),
	m_bPropertiesAdded(FALSE)
{
	m_ShiftToDepthTable.UpdateGetCallback(GetShiftToDepthTableCallback, this);
	m_DepthToShiftTable.UpdateGetCallback(GetDepthToShiftTableCallback, this);
	xnOSMemSet(&m_ShiftToDepthTables, 0, sizeof(XnShiftToDepthTables));
}

XnShiftToDepthStreamHelper::~XnShiftToDepthStreamHelper()
{
	XnShiftToDepthStreamHelper::Free();
}

XnStatus XnShiftToDepthStreamHelper::Init(XnDeviceModule* pModule)
{
	XnStatus nRetVal = XN_STATUS_OK;

	XN_VALIDATE_INPUT_PTR(pModule);
	m_pModule = pModule;
	
	// old depth streams did not have S2D tables as actual properties. Add these properties
	XnBool bDoesExist = FALSE;
	nRetVal = m_pModule->DoesPropertyExist(XN_STREAM_PROPERTY_S2D_TABLE, &bDoesExist);
	XN_IS_STATUS_OK(nRetVal);

	if (!bDoesExist)
	{
		// add properties to the module
		XN_VALIDATE_ADD_PROPERTIES(m_pModule, &m_ShiftToDepthTable, &m_DepthToShiftTable);

		m_bPropertiesAdded = TRUE;

		// now create tables and register to properties
		nRetVal = InitShiftToDepth();
		XN_IS_STATUS_OK(nRetVal);
	}
	else
	{
		m_ShiftToDepthTables.pShiftToDepthTable = (OniDepthPixel*)m_ShiftToDepthTable.GetValue().data;
		m_ShiftToDepthTables.pDepthToShiftTable = (XnUInt16*)m_DepthToShiftTable.GetValue().data;
	}
	
	return (XN_STATUS_OK);
}

XnStatus XnShiftToDepthStreamHelper::Free()
{
	XnShiftToDepthFree(&m_ShiftToDepthTables);
	return XN_STATUS_OK;
}

XnStatus XnShiftToDepthStreamHelper::InitShiftToDepth()
{
	XnStatus nRetVal = XN_STATUS_OK;

	// register to any shift-to-depth property (so tables can be updated if needed)
	XnUInt32 propIds[] = 
	{
		XN_STREAM_PROPERTY_MIN_DEPTH,
		XN_STREAM_PROPERTY_MAX_DEPTH,
		XN_STREAM_PROPERTY_CONST_SHIFT,
		XN_STREAM_PROPERTY_PIXEL_SIZE_FACTOR,
		XN_STREAM_PROPERTY_PARAM_COEFF,
		XN_STREAM_PROPERTY_SHIFT_SCALE,
		XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE,
		XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE,
		XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE,
		XN_STREAM_PROPERTY_OUTPUT_FORMAT,
	};

	XnUInt32 nPropCount = sizeof(propIds) / sizeof(propIds[0]);

	XnCallbackHandle hDummy;

	XnProperty* pProperty = NULL;
	for (XnUInt32 i = 0; i < nPropCount; ++i)
	{
		nRetVal = m_pModule->GetProperty(propIds[i], &pProperty);
		XN_IS_STATUS_OK(nRetVal);

		nRetVal = pProperty->OnChangeEvent().Register(ShiftToDepthPropertyValueChangedCallback, this, hDummy);
		XN_IS_STATUS_OK(nRetVal);
	}

	// register for tables size properties
	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_MAX_SHIFT, &pProperty);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = pProperty->OnChangeEvent().Register(DeviceS2DTablesSizeChangedCallback, this, hDummy);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_DEVICE_MAX_DEPTH, &pProperty);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = pProperty->OnChangeEvent().Register(DeviceS2DTablesSizeChangedCallback, this, hDummy);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_OUTPUT_FORMAT, &pProperty);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = pProperty->OnChangeEvent().Register(DeviceS2DTablesSizeChangedCallback, this, hDummy);
	XN_IS_STATUS_OK(nRetVal);

	// now init the tables
	XnShiftToDepthConfig Config;
	nRetVal = GetShiftToDepthConfig(Config);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnShiftToDepthInit(&m_ShiftToDepthTables, &Config);
	XN_IS_STATUS_OK(nRetVal);

	// replace tables buffers
	m_ShiftToDepthTable.ReplaceBuffer(m_ShiftToDepthTables.pShiftToDepthTable, m_ShiftToDepthTables.nShiftsCount * sizeof(OniDepthPixel));
	m_DepthToShiftTable.ReplaceBuffer(m_ShiftToDepthTables.pDepthToShiftTable, m_ShiftToDepthTables.nDepthsCount * sizeof(XnUInt16));

	return (XN_STATUS_OK);
}

XnStatus XnShiftToDepthStreamHelper::GetShiftToDepthConfig(XnShiftToDepthConfig& Config)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	XnUInt64 nTemp;
	XnDouble dTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE, &dTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.fZeroPlaneDistance = (XnFloat)dTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE, &dTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.fZeroPlanePixelSize = (XnFloat)dTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE, &dTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.fEmitterDCmosDistance = (XnFloat)dTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_MAX_SHIFT, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nDeviceMaxShiftValue = (XnUInt32)nTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_DEVICE_MAX_DEPTH, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nDeviceMaxDepthValue = (XnUInt32)nTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_CONST_SHIFT, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nConstShift = (XnUInt32)nTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_PIXEL_SIZE_FACTOR, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nPixelSizeFactor = (XnUInt32)nTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_PARAM_COEFF, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nParamCoeff = (XnUInt32)nTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_SHIFT_SCALE, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nShiftScale = (XnUInt32)nTemp;

	// change scale according to output format
	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_OUTPUT_FORMAT, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	switch (nTemp)
	{
	case ONI_PIXEL_FORMAT_DEPTH_1_MM:
	case ONI_PIXEL_FORMAT_SHIFT_9_2:
		break;
	case ONI_PIXEL_FORMAT_DEPTH_100_UM:
		Config.nShiftScale *= 10;
		break;
	default:
		XN_ASSERT(FALSE);
	}

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_MIN_DEPTH, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nDepthMinCutOff = (OniDepthPixel)nTemp;

	nRetVal = m_pModule->GetProperty(XN_STREAM_PROPERTY_MAX_DEPTH, &nTemp);
	XN_IS_STATUS_OK(nRetVal);

	Config.nDepthMaxCutOff = (OniDepthPixel)nTemp;

	return (XN_STATUS_OK);
}

XnStatus XnShiftToDepthStreamHelper::RaiseChangeEvents()
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	nRetVal = m_ShiftToDepthTable.UnsafeUpdateValue(XnGeneralBufferPack(m_ShiftToDepthTables.pShiftToDepthTable, m_ShiftToDepthTables.nShiftsCount * sizeof(OniDepthPixel)));
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_DepthToShiftTable.UnsafeUpdateValue(XnGeneralBufferPack(m_ShiftToDepthTables.pDepthToShiftTable, m_ShiftToDepthTables.nDepthsCount * sizeof(XnUInt16)));
	XN_IS_STATUS_OK(nRetVal);
	
	return (XN_STATUS_OK);
}

XnStatus XnShiftToDepthStreamHelper::OnShiftToDepthPropertyValueChanged()
{
	XnStatus nRetVal = XN_STATUS_OK;

	XnShiftToDepthConfig Config;
	nRetVal = GetShiftToDepthConfig(Config);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnShiftToDepthUpdate(&m_ShiftToDepthTables, &Config);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = RaiseChangeEvents();
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnShiftToDepthStreamHelper::OnDeviceS2DTablesSizeChanged()
{
	XnStatus nRetVal = XN_STATUS_OK;

	// free the tables, and re-init them
	XnShiftToDepthFree(&m_ShiftToDepthTables);

	XnShiftToDepthConfig Config;
	nRetVal = GetShiftToDepthConfig(Config);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnShiftToDepthInit(&m_ShiftToDepthTables, &Config);
	XN_IS_STATUS_OK(nRetVal);

	// replace tables buffers
	m_ShiftToDepthTable.ReplaceBuffer(m_ShiftToDepthTables.pShiftToDepthTable, m_ShiftToDepthTables.nShiftsCount * sizeof(OniDepthPixel));
	m_DepthToShiftTable.ReplaceBuffer(m_ShiftToDepthTables.pDepthToShiftTable, m_ShiftToDepthTables.nDepthsCount * sizeof(XnUInt16));

	nRetVal = RaiseChangeEvents();
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnShiftToDepthStreamHelper::GetShiftToDepthTableImpl(const OniGeneralBuffer& gbValue) const
{
	XnInt32 nTableSize = m_ShiftToDepthTables.nShiftsCount * sizeof(OniDepthPixel);
	if (gbValue.dataSize < nTableSize)
	{
		return XN_STATUS_OUTPUT_BUFFER_OVERFLOW;
	}

	xnOSMemCopy(gbValue.data, m_ShiftToDepthTables.pShiftToDepthTable, nTableSize);
	return XN_STATUS_OK;
}

XnStatus XnShiftToDepthStreamHelper::GetDepthToShiftTableImpl(const OniGeneralBuffer& gbValue) const
{
	XnInt32 nTableSize = m_ShiftToDepthTables.nDepthsCount * sizeof(XnUInt16);
	if (gbValue.dataSize < nTableSize)
	{
		return XN_STATUS_OUTPUT_BUFFER_OVERFLOW;
	}

	xnOSMemCopy(gbValue.data, m_ShiftToDepthTables.pDepthToShiftTable, nTableSize);
	return XN_STATUS_OK;
}

XnStatus XN_CALLBACK_TYPE XnShiftToDepthStreamHelper::GetShiftToDepthTableCallback(const XnActualGeneralProperty* /*pSender*/, const OniGeneralBuffer& gbValue, void* pCookie)
{
	XnShiftToDepthStreamHelper* pStream = (XnShiftToDepthStreamHelper*)pCookie;
	return pStream->GetShiftToDepthTableImpl(gbValue);	
}

XnStatus XN_CALLBACK_TYPE XnShiftToDepthStreamHelper::GetDepthToShiftTableCallback(const XnActualGeneralProperty* /*pSender*/, const OniGeneralBuffer& gbValue, void* pCookie)
{
	XnShiftToDepthStreamHelper* pStream = (XnShiftToDepthStreamHelper*)pCookie;
	return pStream->GetDepthToShiftTableImpl(gbValue);	
}

XnStatus XN_CALLBACK_TYPE XnShiftToDepthStreamHelper::ShiftToDepthPropertyValueChangedCallback(const XnProperty* /*pSender*/, void* pCookie)
{
	XnShiftToDepthStreamHelper* pStream = (XnShiftToDepthStreamHelper*)pCookie;
	return pStream->OnShiftToDepthPropertyValueChanged();
}

XnStatus XN_CALLBACK_TYPE XnShiftToDepthStreamHelper::DeviceS2DTablesSizeChangedCallback(const XnProperty* /*pSender*/, void* pCookie)
{
	XnShiftToDepthStreamHelper* pStream = (XnShiftToDepthStreamHelper*)pCookie;
	return pStream->OnDeviceS2DTablesSizeChanged();
}

