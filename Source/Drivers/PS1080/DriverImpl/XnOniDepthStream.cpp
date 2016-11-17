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
#include "XnOniDepthStream.h"
#include "../Sensor/XnSensorDepthStream.h"

//---------------------------------------------------------------------------
// XnOniDepthStream class
//---------------------------------------------------------------------------

XnOniDepthStream::XnOniDepthStream(XnSensor* pSensor, XnOniDevice* pDevice) : 
	XnOniMapStream(pSensor, XN_STREAM_TYPE_DEPTH, ONI_SENSOR_DEPTH, pDevice)
{
}

OniStatus XnOniDepthStream::getProperty(int propertyId, void* data, int* pDataSize)
{
	switch (propertyId)
	{
	case ONI_STREAM_PROPERTY_MAX_VALUE:
		if (*pDataSize != sizeof(int))
		{
			return ONI_STATUS_BAD_PARAMETER;
		}

		XnUInt64 nValue;
		m_pSensor->GetProperty(m_strType, XN_STREAM_PROPERTY_DEVICE_MAX_DEPTH, &nValue);

		*(int*)data = (int)nValue;
		return ONI_STATUS_OK;
	case ONI_STREAM_PROPERTY_MIN_VALUE:
		if (*pDataSize != sizeof(int))
		{
			return ONI_STATUS_BAD_PARAMETER;
		}

		*(int*)data = 0;
		return ONI_STATUS_OK;
	case XN_STREAM_PROPERTY_DEPTH_SENSOR_CALIBRATION_INFO:
		return ((XnSensorDepthStream*)m_pDeviceStream)->GetSensorCalibrationInfo(data, pDataSize);
	default:
		return XnOniMapStream::getProperty(propertyId, data, pDataSize);
	}
}

OniBool XnOniDepthStream::isPropertySupported(int propertyId)
{
	return (
		propertyId == ONI_STREAM_PROPERTY_MAX_VALUE ||
		propertyId == ONI_STREAM_PROPERTY_MIN_VALUE ||
		propertyId == XN_STREAM_PROPERTY_DEPTH_SENSOR_CALIBRATION_INFO ||
		XnOniMapStream::isPropertySupported(propertyId));
}

void XnOniDepthStream::notifyAllProperties()
{
	XnOniMapStream::notifyAllProperties();

	XnUInt32 nValue;
	int size = sizeof(nValue);

	// white balance
	getProperty(XN_STREAM_PROPERTY_WHITE_BALANCE_ENABLED, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_WHITE_BALANCE_ENABLED, &nValue, size);

	// gain
	getProperty(XN_STREAM_PROPERTY_GAIN, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_GAIN, &nValue, size);

	// hole filter
	getProperty(XN_STREAM_PROPERTY_HOLE_FILTER, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_HOLE_FILTER, &nValue, size);

	// registration type
	getProperty(XN_STREAM_PROPERTY_REGISTRATION_TYPE, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_REGISTRATION_TYPE, &nValue, size);

	// const shift
	getProperty(XN_STREAM_PROPERTY_CONST_SHIFT, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_CONST_SHIFT, &nValue, size);

	// pixel size factor
	getProperty(XN_STREAM_PROPERTY_PIXEL_SIZE_FACTOR, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_PIXEL_SIZE_FACTOR, &nValue, size);

	// max shift
	getProperty(XN_STREAM_PROPERTY_MAX_SHIFT, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_MAX_SHIFT, &nValue, size);

	// param coeff
	getProperty(XN_STREAM_PROPERTY_PARAM_COEFF, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_PARAM_COEFF, &nValue, size);

	// shift scale
	getProperty(XN_STREAM_PROPERTY_SHIFT_SCALE, &nValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_SHIFT_SCALE, &nValue, size);

	XnDouble dValue;
	size = sizeof(dValue);

	// zero plane distance
	getProperty(XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE, &dValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE, &dValue, size);

	// zero plane pixel size
	getProperty(XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE, &dValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE, &dValue, size);

	// emitter/depth-cmos distance
	getProperty(XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE, &dValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE, &dValue, size);

	// depth-cmos/RGB-cmos distance
	getProperty(XN_STREAM_PROPERTY_DCMOS_RCMOS_DISTANCE, &dValue, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_DCMOS_RCMOS_DISTANCE, &dValue, size);

	XnSensorDepthStream* pDepthStream = (XnSensorDepthStream*)m_pDeviceStream;

	// shift-to-depth table
	raisePropertyChanged(XN_STREAM_PROPERTY_S2D_TABLE, pDepthStream->GetShiftToDepthTable(), sizeof(OniDepthPixel)*(pDepthStream->GetMaxShift()+1));

	// depth-to-shift table
	raisePropertyChanged(XN_STREAM_PROPERTY_D2S_TABLE, pDepthStream->GetDepthToShiftTable(), sizeof(XnUInt16)*(pDepthStream->GetDeviceMaxDepth()+1));

	DepthUtilsSensorCalibrationInfo calibrationInfo;
	size = sizeof(DepthUtilsSensorCalibrationInfo);
	((XnSensorDepthStream*)m_pDeviceStream)->GetSensorCalibrationInfo(&calibrationInfo, &size);
	raisePropertyChanged(XN_STREAM_PROPERTY_DEPTH_SENSOR_CALIBRATION_INFO, &calibrationInfo, size);
}

OniStatus XnOniDepthStream::convertDepthToColorCoordinates(StreamBase* colorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY)
{
	// take video mode from the color stream
	XnOniMapStream* pColorStream = (XnOniMapStream*)colorStream;

	OniVideoMode videoMode;
	XnStatus retVal = pColorStream->GetVideoMode(&videoMode);
	if (retVal != XN_STATUS_OK)
	{
		XN_ASSERT(FALSE);
		return ONI_STATUS_ERROR;
	}

	// translate it to the internal property
	XnPixelRegistration pixelArgs;
	pixelArgs.nDepthX = depthX;
	pixelArgs.nDepthY = depthY;
	pixelArgs.nDepthValue = depthZ;
	pixelArgs.nImageXRes = videoMode.resolutionX;
	pixelArgs.nImageYRes = videoMode.resolutionY;
	int pixelArgsSize = sizeof(pixelArgs);

	if (ONI_STATUS_OK != getProperty(XN_STREAM_PROPERTY_PIXEL_REGISTRATION, &pixelArgs, &pixelArgsSize))
	{
		return ONI_STATUS_ERROR;
	}

	// take output
	*pColorX = pixelArgs.nImageX;
	*pColorY = pixelArgs.nImageY;

	return ONI_STATUS_OK;
}

