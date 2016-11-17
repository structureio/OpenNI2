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
#include "XnDeviceSensorInit.h"
#include "XnSensorDepthStream.h"
#include "XnUncompressedDepthProcessor.h"
#include "XnPSCompressedDepthProcessor.h"
#include "XnPacked11DepthProcessor.h"
#include "XnPacked12DepthProcessor.h"
#include "XnCmosInfo.h"
#include <XnOS.h>
#include <XnProfiling.h>
#include <XnFormatsStatus.h>
#include "XnSensor.h"
#include <math.h>

#include "XnGMCDebugProcessor.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define XN_SHIFTS_MAX_SHIFT									2047
#define XN_SHIFTS_PIXEL_SIZE_FACTOR							1
#define XN_SHIFTS_PARAM_COEFFICIENT							4
#define XN_SHIFTS_SHIFT_SCALE								10

#define XN_DEPTH_STREAM_AGC_NUMBER_OF_BINS					4

//---------------------------------------------------------------------------
// XnSensorDepthStream class
//---------------------------------------------------------------------------
XnSensorDepthStream::XnSensorDepthStream(const XnChar* strName, XnSensorObjects* pObjects) : 
	XnDepthStream(strName, FALSE, XN_DEVICE_SENSOR_MAX_DEPTH_1_MM, XN_SHIFTS_MAX_SHIFT),
	m_Helper(pObjects),
	m_InputFormat(XN_STREAM_PROPERTY_INPUT_FORMAT, "InputFormat", XN_DEPTH_STREAM_DEFAULT_INPUT_FORMAT),
	m_DepthRegistration(XN_STREAM_PROPERTY_REGISTRATION, "Registration", XN_DEPTH_STREAM_DEFAULT_REGISTRATION),
	m_HoleFilter(XN_STREAM_PROPERTY_HOLE_FILTER, "HoleFilter", XN_DEPTH_STREAM_DEFAULT_HOLE_FILLER),
	m_WhiteBalance(XN_STREAM_PROPERTY_WHITE_BALANCE_ENABLED, "WhiteBalanceEnabled", XN_DEPTH_STREAM_DEFAULT_WHITE_BALANCE),
	m_Gain(XN_STREAM_PROPERTY_GAIN, "Gain", XN_DEPTH_STREAM_DEFAULT_GAIN_OLD),
	m_RegistrationType(XN_STREAM_PROPERTY_REGISTRATION_TYPE, "RegistrationType", XN_DEPTH_STREAM_DEFAULT_REGISTRATION_TYPE),
	m_CroppingMode(XN_STREAM_PROPERTY_CROPPING_MODE, "CroppingMode", XN_CROPPING_MODE_NORMAL),
	m_AGCBin(XN_STREAM_PROPERTY_AGC_BIN, "AGCBin", NULL, ReadAGCBinsFromFile),
	m_FirmwareMirror(0, "FirmwareMirror", FALSE, strName),
	m_FirmwareRegistration(0, "FirmwareRegistration", FALSE, strName),
	m_FirmwareCropSizeX(0, "FirmwareCropSizeX", 0, strName),
	m_FirmwareCropSizeY(0, "FirmwareCropSizeY", 0, strName),
	m_FirmwareCropOffsetX(0, "FirmwareCropOffsetX", 0, strName),
	m_FirmwareCropOffsetY(0, "FirmwareCropOffsetY", 0, strName),
	m_FirmwareCropMode(0, "FirmwareCropMode", XN_FIRMWARE_CROPPING_MODE_DISABLED, strName),
	m_ActualRead(XN_STREAM_PROPERTY_ACTUAL_READ_DATA, "ActualReadData", FALSE),
	m_GMCMode(XN_STREAM_PROPERTY_GMC_MODE, "GMCMode", XN_DEPTH_STREAM_DEFAULT_GMC_MODE),
	m_CloseRange(XN_STREAM_PROPERTY_CLOSE_RANGE, "CloseRange", XN_DEPTH_STREAM_DEFAULT_CLOSE_RANGE),
	m_PixelRegistration(XN_STREAM_PROPERTY_PIXEL_REGISTRATION, "PixelRegistration"),
	m_HorizontalFOV(ONI_STREAM_PROPERTY_HORIZONTAL_FOV, "HorizontalFov"),
	m_VerticalFOV(ONI_STREAM_PROPERTY_VERTICAL_FOV, "VerticalFov"),
	m_GMCDebug(XN_STREAM_PROPERTY_GMC_DEBUG, "GMCDebug", XN_DEPTH_STREAM_DEFAULT_GMC_DEBUG),
	m_WavelengthCorrection(XN_STREAM_PROPERTY_WAVELENGTH_CORRECTION, "WavelengthCorrection", XN_DEPTH_STREAM_DEFAULT_WAVELENGTH_CORRECTION),
	m_WavelengthCorrectionDebug(XN_STREAM_PROPERTY_WAVELENGTH_CORRECTION_DEBUG, "WavelengthCorrectionDebug", XN_DEPTH_STREAM_DEFAULT_WAVELENGTH_CORRECTION_DEBUG),
	m_depthUtilsHandle(NULL),
	m_hReferenceSizeChangedCallback(NULL)
{
	m_ActualRead.UpdateSetCallback(SetActualReadCallback, this);
}

XnStatus XnSensorDepthStream::Init()
{
	XnStatus nRetVal = XN_STATUS_OK;

	// init base
	nRetVal = XnDepthStream::Init();
	XN_IS_STATUS_OK(nRetVal);

	// start with no cut-off
	nRetVal = MaxDepthProperty().UnsafeUpdateValue(XN_DEVICE_SENSOR_MAX_DEPTH_100_UM);
	XN_IS_STATUS_OK(nRetVal);

	m_InputFormat.UpdateSetCallback(SetInputFormatCallback, this);
	m_DepthRegistration.UpdateSetCallback(SetRegistrationCallback, this);
	m_HoleFilter.UpdateSetCallback(SetHoleFilterCallback, this);
	m_WhiteBalance.UpdateSetCallback(SetWhiteBalanceCallback, this);
	m_Gain.UpdateSetCallback(SetGainCallback, this);
	m_RegistrationType.UpdateSetCallback(SetRegistrationTypeCallback, this);
	m_AGCBin.UpdateSetCallback(SetAGCBinCallback, this);
	m_AGCBin.UpdateGetCallback(GetAGCBinCallback, this);
	m_GMCMode.UpdateSetCallback(SetGMCModeCallback, this);
	m_CloseRange.UpdateSetCallback(SetCloseRangeCallback, this);
	m_CroppingMode.UpdateSetCallback(SetCroppingModeCallback, this);
	m_PixelRegistration.UpdateGetCallback(GetPixelRegistrationCallback, this);
	m_GMCDebug.UpdateSetCallback(SetGMCDebugCallback, this);
	m_WavelengthCorrection.UpdateSetCallback(SetWavelengthCorrectionCallback, this);
	m_WavelengthCorrectionDebug.UpdateSetCallback(SetWavelengthCorrectionDebugCallback, this);

	XN_VALIDATE_ADD_PROPERTIES(this, &m_InputFormat, &m_DepthRegistration, &m_HoleFilter, 
		&m_WhiteBalance, &m_Gain, &m_AGCBin, &m_ActualRead, &m_GMCMode, 
		&m_CloseRange, &m_CroppingMode, &m_RegistrationType, &m_PixelRegistration,
		&m_HorizontalFOV, &m_VerticalFOV, &m_GMCDebug, &m_WavelengthCorrection, &m_WavelengthCorrectionDebug);

	// register supported modes
	XnCmosPreset* pSupportedModes = m_Helper.GetPrivateData()->FWInfo.depthModes.GetData();
	XnUInt32 nSupportedModes = m_Helper.GetPrivateData()->FWInfo.depthModes.GetSize();
	nRetVal = AddSupportedModes(pSupportedModes, nSupportedModes);
	XN_IS_STATUS_OK(nRetVal);

	if (m_Helper.GetPrivateData()->pSensor->IsLowBandwidth())
	{
		nRetVal = m_InputFormat.UnsafeUpdateValue(XN_IO_DEPTH_FORMAT_COMPRESSED_PS);
		XN_IS_STATUS_OK(nRetVal);
	}

	// make sure default resolution is supported
	XnBool bResSupported = FALSE;
	for (XnUInt8 i = 0; i < nSupportedModes; ++i)
	{
		if (pSupportedModes[i].nResolution == XN_DEPTH_STREAM_DEFAULT_RESOLUTION)
		{
			bResSupported = TRUE;
			break;
		}
	}

	XnUInt64 nDefaultResolution = XN_DEPTH_STREAM_DEFAULT_RESOLUTION;
	if (!bResSupported)
	{
		// QVGA was always supported
		nDefaultResolution = XN_RESOLUTION_QVGA;
	}

	nRetVal = ResolutionProperty().UnsafeUpdateValue(nDefaultResolution);
	XN_IS_STATUS_OK(nRetVal);

	// set other properties default values
	nRetVal = FPSProperty().UnsafeUpdateValue(XN_DEPTH_STREAM_DEFAULT_FPS);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = OutputFormatProperty().UnsafeUpdateValue(XN_DEPTH_STREAM_DEFAULT_OUTPUT_FORMAT);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = ParamCoefficientProperty().UnsafeUpdateValue(XN_SHIFTS_PARAM_COEFFICIENT);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = ShiftScaleProperty().UnsafeUpdateValue(XN_SHIFTS_SHIFT_SCALE);
	XN_IS_STATUS_OK(nRetVal);

	// read some data from firmware
	XnDepthInformation DepthInformation;
	nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_DEPTH_INFO, &DepthInformation, sizeof(XnDepthInformation), XN_RESOLUTION_VGA, 30);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = ConstShiftProperty().UnsafeUpdateValue(DepthInformation.nConstShift);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = ZeroPlaneDistanceProperty().UnsafeUpdateValue(m_Helper.GetFixedParams()->GetZeroPlaneDistance());
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = ZeroPlanePixelSizeProperty().UnsafeUpdateValue(m_Helper.GetFixedParams()->GetZeroPlanePixelSize());
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = EmitterDCmosDistanceProperty().UnsafeUpdateValue(m_Helper.GetFixedParams()->GetEmitterDCmosDistance());
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = GetDCmosRCmosDistanceProperty().UnsafeUpdateValue(m_Helper.GetFixedParams()->GetDCmosRCmosDistance());
	XN_IS_STATUS_OK(nRetVal);

	XnDouble fZPPS = m_Helper.GetFixedParams()->GetZeroPlanePixelSize();
	XnDouble fZPD = m_Helper.GetFixedParams()->GetZeroPlaneDistance();

	nRetVal = m_HorizontalFOV.UnsafeUpdateValue(2*atan(fZPPS*XN_SXGA_X_RES/2/fZPD));
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_VerticalFOV.UnsafeUpdateValue(2*atan(fZPPS*XN_VGA_Y_RES*2/2/fZPD));
	XN_IS_STATUS_OK(nRetVal);

	// init helper
	nRetVal = m_Helper.Init(this, this);
	XN_IS_STATUS_OK(nRetVal);

	if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_3_0)
	{
		nRetVal = m_GMCMode.UnsafeUpdateValue(FALSE);
		XN_IS_STATUS_OK(nRetVal);
	}

	if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_5_2)
	{
		nRetVal = m_WavelengthCorrection.UnsafeUpdateValue(FALSE);
		XN_IS_STATUS_OK(nRetVal);
	}

	if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_4_0)
	{
		nRetVal = m_WhiteBalance.UnsafeUpdateValue(FALSE);
		XN_IS_STATUS_OK(nRetVal);
	}

	// on old firmwares, the host decides on the default gain. On new firmwares, we read it from firmware
	if (m_Helper.GetFirmware()->GetInfo()->nFWVer > XN_SENSOR_FW_VER_1_2)
	{
		nRetVal = m_Gain.UnsafeUpdateValue(GetFirmwareParams()->m_DepthGain.GetValue());
		XN_IS_STATUS_OK(nRetVal);
	}

	// registration
	XnCallbackHandle hCallbackDummy;	
	nRetVal = ResolutionProperty().OnChangeEvent().Register(DecideFirmwareRegistrationCallback, this, hCallbackDummy);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = DecideFirmwareRegistration((XnBool)m_DepthRegistration.GetValue(), (XnProcessingType)m_RegistrationType.GetValue(), GetResolution());
	XN_IS_STATUS_OK(nRetVal);

	// data processor
	nRetVal = m_Helper.RegisterDataProcessorProperty(m_InputFormat);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.RegisterDataProcessorProperty(ResolutionProperty());
	XN_IS_STATUS_OK(nRetVal);

	// pixel size factor
	nRetVal = GetFirmwareParams()->m_ReferenceResolution.OnChangeEvent().Register(DecidePixelSizeFactorCallback, this, m_hReferenceSizeChangedCallback);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = DecidePixelSizeFactor();
	XN_IS_STATUS_OK(nRetVal);

	// initialize registration
	if (m_Helper.GetFirmwareVersion() > XN_SENSOR_FW_VER_5_3)
	{
		nRetVal = PopulateSensorCalibrationInfo();
		XN_IS_STATUS_OK(nRetVal);
		nRetVal = DepthUtilsInitialize(&m_calibrationInfo, &m_depthUtilsHandle);
		XN_IS_STATUS_OK(nRetVal);
		nRetVal = DepthUtilsSetDepthConfiguration(m_depthUtilsHandle, GetXRes(), GetYRes(), GetOutputFormat(), IsMirrored());
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::Free()
{
	DepthUtilsShutdown(&m_depthUtilsHandle);

	// unregister from external properties (internal ones will be destroyed anyway...)
	if (m_hReferenceSizeChangedCallback != NULL)
	{
		GetFirmwareParams()->m_ReferenceResolution.OnChangeEvent().Unregister(m_hReferenceSizeChangedCallback);
		m_hReferenceSizeChangedCallback = NULL;
	}

	m_Helper.Free();

	XnDepthStream::Free();

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::MapPropertiesToFirmware()
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	nRetVal = m_Helper.MapFirmwareProperty(m_InputFormat, GetFirmwareParams()->m_DepthFormat, FALSE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(ResolutionProperty(), GetFirmwareParams()->m_DepthResolution, FALSE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(FPSProperty(), GetFirmwareParams()->m_DepthFPS, FALSE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_HoleFilter, GetFirmwareParams()->m_DepthHoleFilter, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_Gain, GetFirmwareParams()->m_DepthGain, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_WhiteBalance, GetFirmwareParams()->m_DepthWhiteBalance, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareMirror, GetFirmwareParams()->m_DepthMirror, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareRegistration, GetFirmwareParams()->m_RegistrationEnabled, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropSizeX, GetFirmwareParams()->m_DepthCropSizeX, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropSizeY, GetFirmwareParams()->m_DepthCropSizeY, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropOffsetX, GetFirmwareParams()->m_DepthCropOffsetX, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropOffsetY, GetFirmwareParams()->m_DepthCropOffsetY, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropMode, GetFirmwareParams()->m_DepthCropMode, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_GMCMode, GetFirmwareParams()->m_GMCMode, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_CloseRange, GetFirmwareParams()->m_DepthCloseRange, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_GMCDebug, GetFirmwareParams()->m_GMCDebug, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_WavelengthCorrection, GetFirmwareParams()->m_WavelengthCorrection, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_WavelengthCorrectionDebug, GetFirmwareParams()->m_WavelengthCorrectionDebug, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::ConfigureStreamImpl()
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnUSBShutdownReadThread(GetHelper()->GetPrivateData()->pSpecificDepthUsb->pUsbConnection->UsbEp);

	nRetVal = SetActualRead(TRUE);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.ConfigureFirmware(m_InputFormat);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(ResolutionProperty());
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(FPSProperty());
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_HoleFilter);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_Gain);
	XN_IS_STATUS_OK(nRetVal);;

	// we need to turn decimation on when resolution is QVGA, and FPS is different than 60
	// NOTE: this is ugly as hell. This logic should be moved to firmware.
	XnBool bDecimation = (GetResolution() == XN_RESOLUTION_QVGA && GetFPS() != 60);
	nRetVal = GetFirmwareParams()->m_DepthDecimation.SetValue(bDecimation);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.ConfigureFirmware(m_FirmwareRegistration);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_FirmwareMirror);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_GMCMode);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_GMCDebug);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_WavelengthCorrection);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_WavelengthCorrectionDebug);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(m_WhiteBalance);
	XN_IS_STATUS_OK(nRetVal);;

	nRetVal = m_Helper.GetCmosInfo()->SetCmosConfig(XN_CMOS_TYPE_DEPTH, GetResolution(), GetFPS());
	XN_IS_STATUS_OK(nRetVal);

	return XN_STATUS_OK;
}

XnStatus XnSensorDepthStream::SetActualRead(XnBool bRead)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if ((XnBool)m_ActualRead.GetValue() != bRead)
	{
		if (bRead)
		{
			xnLogVerbose(XN_MASK_DEVICE_SENSOR, "Creating USB depth read thread...");
			XnSpecificUsbDevice* pUSB = GetHelper()->GetPrivateData()->pSpecificDepthUsb;
			nRetVal = xnUSBInitReadThread(pUSB->pUsbConnection->UsbEp, pUSB->nChunkReadBytes, pUSB->nNumberOfBuffers, pUSB->nTimeout, XnDeviceSensorProtocolUsbEpCb, pUSB);
			XN_IS_STATUS_OK(nRetVal);
		}
		else
		{
			xnLogVerbose(XN_MASK_DEVICE_SENSOR, "Shutting down USB depth read thread...");
			xnUSBShutdownReadThread(GetHelper()->GetPrivateData()->pSpecificDepthUsb->pUsbConnection->UsbEp);
		}

		nRetVal = m_ActualRead.UnsafeUpdateValue(bRead);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::OpenStreamImpl()
{
	XnStatus nRetVal = XN_STATUS_OK;

	// Turn stream on
	nRetVal = GetFirmwareParams()->m_Stream1Mode.SetValue(XN_VIDEO_STREAM_DEPTH);
	XN_IS_STATUS_OK(nRetVal);

	// CloseRange
	if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_5_6)
	{
		CloseRangeControl((XnBool)m_CloseRange.GetValue());
	}
	else
	{
		nRetVal = m_Helper.ConfigureFirmware(m_CloseRange);
		XN_IS_STATUS_OK(nRetVal);
	}

	// Cropping
	if (m_FirmwareCropMode.GetValue() != XN_FIRMWARE_CROPPING_MODE_DISABLED)
	{
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropSizeX);
		XN_IS_STATUS_OK(nRetVal);;
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropSizeY);
		XN_IS_STATUS_OK(nRetVal);;
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropOffsetX);
		XN_IS_STATUS_OK(nRetVal);;
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropOffsetY);
		XN_IS_STATUS_OK(nRetVal);;
	}
	nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropMode);
	XN_IS_STATUS_OK(nRetVal);;

	nRetVal = XnDepthStream::Open();
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::CloseStreamImpl()
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = GetFirmwareParams()->m_Stream1Mode.SetValue(XN_VIDEO_STREAM_OFF);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = SetActualRead(FALSE);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnDepthStream::Close();
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetOutputFormat(OniPixelFormat nOutputFormat)
{
	XnStatus nRetVal = XN_STATUS_OK;

	switch (nOutputFormat)
	{
	case ONI_PIXEL_FORMAT_SHIFT_9_2:
	case ONI_PIXEL_FORMAT_DEPTH_1_MM:
		nRetVal = DeviceMaxDepthProperty().UnsafeUpdateValue(XN_DEVICE_SENSOR_MAX_DEPTH_1_MM);
		break;
	case ONI_PIXEL_FORMAT_DEPTH_100_UM:
		nRetVal = DeviceMaxDepthProperty().UnsafeUpdateValue(XN_DEVICE_SENSOR_MAX_DEPTH_100_UM);
		break;
	default:
		XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Unsupported depth output format: %d", nOutputFormat);
	}
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.BeforeSettingDataProcessorProperty();
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnDepthStream::SetOutputFormat(nOutputFormat);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.AfterSettingDataProcessorProperty();
	XN_IS_STATUS_OK(nRetVal);

	if (m_depthUtilsHandle != NULL)
	{	
		nRetVal = DepthUtilsSetDepthConfiguration(m_depthUtilsHandle, GetXRes(), GetYRes(), GetOutputFormat(), IsMirrored());
		XN_IS_STATUS_OK(nRetVal);
	}
		
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetMirror(XnBool bIsMirrored)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnOSEnterCriticalSection(GetLock());

	// set firmware mirror
	XnBool bFirmwareMirror = (bIsMirrored == TRUE && m_Helper.GetFirmwareVersion() >= XN_SENSOR_FW_VER_5_0);

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareMirror, (XnUInt16)bFirmwareMirror);
	if (nRetVal != XN_STATUS_OK)
	{
		xnOSLeaveCriticalSection(GetLock());
		return (nRetVal);
	}

	// update prop
	nRetVal = XnDepthStream::SetMirror(bIsMirrored);

	xnOSLeaveCriticalSection(GetLock());
	XN_IS_STATUS_OK(nRetVal);

	if (m_depthUtilsHandle != NULL)
	{
		nRetVal = DepthUtilsSetDepthConfiguration(m_depthUtilsHandle, GetXRes(), GetYRes(), GetOutputFormat(), IsMirrored());
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetFPS(XnUInt32 nFPS)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	nRetVal = m_Helper.BeforeSettingFirmwareParam(FPSProperty(), (XnUInt16)nFPS);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnDepthStream::SetFPS(nFPS);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.AfterSettingFirmwareParam(FPSProperty());
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetResolution(XnResolutions nResolution)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.BeforeSettingFirmwareParam(ResolutionProperty(), (XnUInt16)nResolution);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnDepthStream::SetResolution(nResolution);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.AfterSettingFirmwareParam(ResolutionProperty());
	XN_IS_STATUS_OK(nRetVal);

	if (m_depthUtilsHandle != NULL)
	{
		nRetVal = DepthUtilsSetDepthConfiguration(m_depthUtilsHandle, GetXRes(), GetYRes(), GetOutputFormat(), IsMirrored());
		XN_IS_STATUS_OK(nRetVal);
	}
		
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetInputFormat(XnIODepthFormats nInputFormat)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	switch (nInputFormat)
	{
	case XN_IO_DEPTH_FORMAT_COMPRESSED_PS:
	case XN_IO_DEPTH_FORMAT_UNCOMPRESSED_16_BIT:
		break;
	case XN_IO_DEPTH_FORMAT_UNCOMPRESSED_11_BIT:
		if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_4_0)
		{
			XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_UNSUPPORTED_MODE, XN_MASK_DEVICE_SENSOR, "11-bit depth is not supported on this sensor!");
		}
		break;
	case XN_IO_DEPTH_FORMAT_UNCOMPRESSED_12_BIT:
		if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_4_0)
		{
			XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_UNSUPPORTED_MODE, XN_MASK_DEVICE_SENSOR, "12-bit depth is not supported on this sensor!");
		}
		break;
	default:
		XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Unknown depth input format: %d", nInputFormat);
	}
	
	nRetVal = m_Helper.SimpleSetFirmwareParam(m_InputFormat, (XnUInt16)nInputFormat);
	XN_IS_STATUS_OK(nRetVal);
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetRegistration(XnBool bRegistration)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (bRegistration != (XnBool)m_DepthRegistration.GetValue())
	{
		nRetVal = DecideFirmwareRegistration(bRegistration, (XnProcessingType)m_RegistrationType.GetValue(), GetResolution());
		XN_IS_STATUS_OK(nRetVal);

		nRetVal = m_DepthRegistration.UnsafeUpdateValue(bRegistration);
		XN_IS_STATUS_OK(nRetVal);
	}
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetHoleFilter(XnBool bHoleFilter)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_HoleFilter, (XnUInt16)bHoleFilter);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetWhiteBalance(XnBool bWhiteBalance)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_WhiteBalance, (XnUInt16)bWhiteBalance);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetGain(XnUInt32 nGain)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_Gain, (XnUInt16)nGain);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetRegistrationType(XnProcessingType type)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (type != (XnProcessingType)m_RegistrationType.GetValue())
	{
		nRetVal = DecideFirmwareRegistration((XnBool)m_DepthRegistration.GetValue(), type, GetResolution());
		XN_IS_STATUS_OK(nRetVal);

		nRetVal = m_RegistrationType.UnsafeUpdateValue(type);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetGMCMode(XnBool bGMCMode)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_GMCMode, (XnUInt16)bGMCMode);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetCloseRange(XnBool bCloseRange)
{
	XnStatus nRetVal = XN_STATUS_OK;

    if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_5_6)
	{
		CloseRangeControl(bCloseRange);

		nRetVal = m_CloseRange.UnsafeUpdateValue(bCloseRange);
		XN_IS_STATUS_OK(nRetVal);
	}
	else
	{
		nRetVal = m_Helper.SimpleSetFirmwareParam(m_CloseRange, (XnUInt16)bCloseRange);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetCroppingMode(XnCroppingMode mode)
{
	switch (mode)
	{
	case XN_CROPPING_MODE_NORMAL:
	case XN_CROPPING_MODE_INCREASED_FPS:
	case XN_CROPPING_MODE_SOFTWARE_ONLY:
		break;
	default:
		XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Bad cropping mode: %u", mode);
	}

	return SetCroppingImpl(GetCropping(), mode);
}

XnStatus XnSensorDepthStream::SetGMCDebug(XnBool bGMCDebug)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_GMCDebug, (XnUInt16)bGMCDebug);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetWavelengthCorrection(XnBool bWavelengthCorrection)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	nRetVal = m_Helper.SimpleSetFirmwareParam(m_WavelengthCorrection, (XnUInt16)bWavelengthCorrection);
	XN_IS_STATUS_OK(nRetVal);
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetWavelengthCorrectionDebug(XnBool bWavelengthCorrectionDebug)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_WavelengthCorrectionDebug, (XnUInt16)bWavelengthCorrectionDebug);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetAGCBin(const XnDepthAGCBin* pBin)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	nRetVal = ValidateDepthValue(pBin->nMin);
	XN_IS_STATUS_OK(nRetVal);
	nRetVal = ValidateDepthValue(pBin->nMax);
	XN_IS_STATUS_OK(nRetVal);

	// translate to shifts
	XnUInt16* pDepthToShift = GetDepthToShiftTable();

	XnUInt16 nMinShift = pDepthToShift[pBin->nMin];
	XnUInt16 nMaxShift = pDepthToShift[pBin->nMax];

	// update firmware
	nRetVal = XnHostProtocolSetDepthAGCBin(m_Helper.GetPrivateData(), pBin->nBin, nMinShift, nMaxShift);
	XN_IS_STATUS_OK(nRetVal);

	// update prop
	nRetVal = m_AGCBin.UnsafeUpdateValue(XnGeneralBufferPack((void*)pBin, sizeof(XnDepthAGCBin)));
	XN_IS_STATUS_OK(nRetVal);
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::GetAGCBin(XnDepthAGCBin* pBin)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	// get from firmware
	XnUInt16 nMinShift;
	XnUInt16 nMaxShift;

	nRetVal = XnHostProtocolGetDepthAGCBin(m_Helper.GetPrivateData(), pBin->nBin, &nMinShift, &nMaxShift);
	XN_IS_STATUS_OK(nRetVal);

	// translate to depth
	OniDepthPixel* pShiftToDepth = GetShiftToDepthTable();

	pBin->nMin = pShiftToDepth[nMinShift];
	pBin->nMax = pShiftToDepth[nMaxShift];

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetCroppingImpl(const OniCropping* pCropping, XnCroppingMode mode)
{
	XnStatus nRetVal = XN_STATUS_OK;

	XnFirmwareCroppingMode firmwareMode = m_Helper.GetFirmwareCroppingMode(mode, pCropping->enabled);

	nRetVal = ValidateCropping(pCropping);
	XN_IS_STATUS_OK(nRetVal);

	xnOSEnterCriticalSection(GetLock());

	if (m_Helper.GetFirmwareVersion() > XN_SENSOR_FW_VER_3_0)
	{
		nRetVal = m_Helper.StartFirmwareTransaction();
		if (nRetVal != XN_STATUS_OK)
		{
			xnOSLeaveCriticalSection(GetLock());
			return (nRetVal);
		}

		if (pCropping->enabled)
		{
			nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropSizeX,       (XnUInt16) pCropping->width);

			if (nRetVal == XN_STATUS_OK)
				nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropSizeY,   (XnUInt16) pCropping->height);

			if (nRetVal == XN_STATUS_OK)
				nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropOffsetX, (XnUInt16) pCropping->originX);

			if (nRetVal == XN_STATUS_OK)
				nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropOffsetY, (XnUInt16) pCropping->originY);
		}

		if (nRetVal == XN_STATUS_OK)
		{
			nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropMode, (XnUInt16)firmwareMode);
		}

		if (nRetVal != XN_STATUS_OK)
		{
			m_Helper.RollbackFirmwareTransaction();
			m_Helper.UpdateFromFirmware(m_FirmwareCropMode);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetY);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeY);
			xnOSLeaveCriticalSection(GetLock());
			return (nRetVal);
		}

		nRetVal = m_Helper.CommitFirmwareTransactionAsBatch();
		if (nRetVal != XN_STATUS_OK)
		{
			m_Helper.UpdateFromFirmware(m_FirmwareCropMode);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetY);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeY);
			xnOSLeaveCriticalSection(GetLock());
			return (nRetVal);
		}
	}

	nRetVal = m_CroppingMode.UnsafeUpdateValue(mode);
	XN_ASSERT(nRetVal == XN_STATUS_OK);

	nRetVal = XnDepthStream::SetCropping(pCropping);
	xnOSLeaveCriticalSection(GetLock());
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::CloseRangeControl(XnBool bEnabled)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (bEnabled)
	{
		nRetVal = XnHostProtocolWriteAHB(m_Helper.GetPrivateData(), 0x2a0038d4, 0x0, 0xFFF); 	
		XN_IS_STATUS_OK(nRetVal);  

		nRetVal = XnHostProtocolWriteAHB(m_Helper.GetPrivateData(), 0x2a003820, 0x00001009, 0xFFFFFFFF); 	
		XN_IS_STATUS_OK(nRetVal);  

		nRetVal = m_Helper.SimpleSetFirmwareParam(m_Gain, 1);
		XN_IS_STATUS_OK(nRetVal); 
	}
	else
	{
		if (m_CloseRange.GetValue() == TRUE)
		{
			nRetVal = XnHostProtocolWriteAHB(m_Helper.GetPrivateData(), 0x2a0038d4, 0x190, 0xFFF); 	
			XN_IS_STATUS_OK(nRetVal);  

			nRetVal = XnHostProtocolWriteAHB(m_Helper.GetPrivateData(), 0x2a003820, 0x00001051, 0xFFFFFFFF); 	
			XN_IS_STATUS_OK(nRetVal);  

			nRetVal = m_Helper.SimpleSetFirmwareParam(m_Gain, 42);
			XN_IS_STATUS_OK(nRetVal); 
		}
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::SetCropping(const OniCropping* pCropping)
{
	return SetCroppingImpl(pCropping, (XnCroppingMode)m_CroppingMode.GetValue());
}

XnStatus XnSensorDepthStream::CropImpl(OniFrame* pFrame, const OniCropping* pCropping)
{
	XnStatus nRetVal = XN_STATUS_OK;

	// if firmware cropping is disabled, crop
	if (m_FirmwareCropMode.GetValue() == XN_FIRMWARE_CROPPING_MODE_DISABLED)
	{
		nRetVal = XnDepthStream::CropImpl(pFrame, pCropping);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::Mirror(OniFrame* pFrame) const
{
	XnStatus nRetVal = XN_STATUS_OK;

	// only perform mirror if it's our job. if mirror is performed by FW, we don't need to do anything.
	if (m_FirmwareMirror.GetValue() == FALSE)
	{
		nRetVal = XnDepthStream::Mirror(pFrame);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::CreateDataProcessor(XnDataProcessor** ppProcessor)
{
	XnStatus nRetVal = XN_STATUS_OK;

	XnFrameBufferManager* pBufferManager;
	nRetVal = StartBufferManager(&pBufferManager);
	XN_IS_STATUS_OK(nRetVal);

	XnStreamProcessor* pNew;

	switch (m_InputFormat.GetValue())
	{
	case XN_IO_DEPTH_FORMAT_UNCOMPRESSED_16_BIT:
		XN_VALIDATE_NEW_AND_INIT(pNew, XnUncompressedDepthProcessor, this, &m_Helper, pBufferManager);
		break;
	case XN_IO_DEPTH_FORMAT_COMPRESSED_PS:
		XN_VALIDATE_NEW_AND_INIT(pNew, XnPSCompressedDepthProcessor, this, &m_Helper, pBufferManager);
		break;
	case XN_IO_DEPTH_FORMAT_UNCOMPRESSED_11_BIT:
		XN_VALIDATE_NEW_AND_INIT(pNew, XnPacked11DepthProcessor, this, &m_Helper, pBufferManager);
		break;
	case XN_IO_DEPTH_FORMAT_UNCOMPRESSED_12_BIT:
		XN_VALIDATE_NEW_AND_INIT(pNew, XnPacked12DepthProcessor, this, &m_Helper, pBufferManager);
		break;
	default:
		return XN_STATUS_IO_INVALID_STREAM_DEPTH_FORMAT;
	}

	*ppProcessor = pNew;

	return XN_STATUS_OK;
}

XnStatus XnSensorDepthStream::DecideFirmwareRegistration(XnBool bRegistration, XnProcessingType registrationType, XnResolutions nRes)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	// start with request
	XnBool bFirmwareRegistration = bRegistration;

	if (bFirmwareRegistration)
	{
		// old chip (PS1000) does not support registration for VGA
		XnBool bHardwareRegistrationSupported = 
			m_Helper.GetPrivateData()->ChipInfo.nChipVer != XN_SENSOR_CHIP_VER_PS1000 || nRes == XN_RESOLUTION_QVGA;

		switch (registrationType)
		{
		case XN_PROCESSING_HARDWARE:
			if (!bHardwareRegistrationSupported)
			{
				XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Sensor does not support hardware registration for current configuration!");
			}
			break;
		case XN_PROCESSING_SOFTWARE:
			if (GetFPS() == 60)
			{
				XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Software registration is not supported in 60 FPS mode!");
			}
			bFirmwareRegistration = FALSE;
			break;
		case XN_PROCESSING_DONT_CARE:
			bFirmwareRegistration = bHardwareRegistrationSupported;
			break;
		default:
			XN_LOG_ERROR_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Unknown registration type: %d", registrationType);
		}
	}

	nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareRegistration, (XnUInt16)bFirmwareRegistration);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::DecidePixelSizeFactor()
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	XnUInt32 nPixelSizeFactor;
	switch (GetFirmwareParams()->m_ReferenceResolution.GetValue())
	{
	case XN_RESOLUTION_SXGA:
		nPixelSizeFactor = 1;
		break;
	case XN_RESOLUTION_VGA:
		nPixelSizeFactor = 2;
		break;
	default:
		XN_LOG_WARNING_RETURN(XN_STATUS_ERROR, XN_MASK_DEVICE_SENSOR, "Can't resolve pixel size for reference resolution %llu", GetFirmwareParams()->m_ReferenceResolution.GetValue());
	}
	
	if (m_Helper.GetFirmwareVersion() < XN_SENSOR_FW_VER_3_0)
	{
		// due to some weird bug (we don't know the reason), DevKits older than 3.0 uses
		// a smaller pixel size, but const shift remains the same. To work around this bug,
		// we will just update pixel size, instead of updating pixel size factor (so that const
		// shift will not be updated)
		nRetVal = ZeroPlanePixelSizeProperty().UnsafeUpdateValue(m_Helper.GetFixedParams()->GetZeroPlanePixelSize() * nPixelSizeFactor);
		XN_IS_STATUS_OK(nRetVal);
	}
	else
	{
		PixelSizeFactorProperty().UnsafeUpdateValue(nPixelSizeFactor);
	}
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorDepthStream::GetImageCoordinatesOfDepthPixel(XnUInt32 x, XnUInt32 y, OniDepthPixel z, XnUInt32 imageXRes, XnUInt32 imageYRes, XnUInt32& imageX, XnUInt32& imageY)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	nRetVal = DepthUtilsSetColorResolution(m_depthUtilsHandle, imageXRes, imageYRes);
	XN_IS_STATUS_OK(nRetVal);

	// first translate to same resolution
	nRetVal = DepthUtilsTranslatePixel(m_depthUtilsHandle, x, y, z, &imageX, &imageY);
	return nRetVal;
}

OniStatus XnSensorDepthStream::GetSensorCalibrationInfo(void* data, int* pDataSize)
{
	if ((size_t)*pDataSize < sizeof(DepthUtilsSensorCalibrationInfo))
	{
		return ONI_STATUS_BAD_PARAMETER;
	}

	if (m_depthUtilsHandle == NULL)
	{
		// Depth utils not initialized - probably not supported for this device
		return ONI_STATUS_NOT_SUPPORTED;
	}

	*pDataSize = sizeof(DepthUtilsSensorCalibrationInfo);
	xnOSMemCopy(data, &m_calibrationInfo, sizeof(DepthUtilsSensorCalibrationInfo));
	return ONI_STATUS_OK;
}


XnStatus XnSensorDepthStream::ApplyRegistration(OniDepthPixel* pDetphmap)
{
	DepthUtilsTranslateDepthMap(m_depthUtilsHandle, pDetphmap);
	return XN_STATUS_OK;
}

#define RGB_REG_X_RES 640
#define RGB_REG_Y_RES 512
#define XN_CMOS_VGAOUTPUT_XRES 1280
#define XN_SENSOR_WIN_OFFET_X 1
#define XN_SENSOR_WIN_OFFET_Y 1
#define RGB_REG_X_VAL_SCALE 16
#define S2D_PEL_CONST 10
#define S2D_CONST_OFFSET 0.375

XnStatus XnSensorDepthStream::PopulateSensorCalibrationInfo()
{
	XnDouble dPlanePixelSize;
	GetProperty(XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE, &dPlanePixelSize);

	XnDouble dPlaneDsr;
	GetProperty(XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE, &dPlaneDsr);

	XnDouble dDCRCDist;
	GetProperty(XN_STREAM_PROPERTY_DCMOS_RCMOS_DISTANCE, &dDCRCDist);

	m_calibrationInfo.magic = ONI_DEPTH_UTILS_CALIBRATION_INFO_MAGIC;
	m_calibrationInfo.version = 1;
	m_calibrationInfo.params1080.zpd = dPlaneDsr;
	m_calibrationInfo.params1080.zpps = dPlanePixelSize;
	m_calibrationInfo.params1080.dcrcdist = dDCRCDist;

	xnOSStrCopy(m_calibrationInfo.deviceName, "PS1080", 80);
	xnOSMemSet(m_calibrationInfo.serial, 0, 80);

	m_calibrationInfo.params1080.rgbRegXRes = RGB_REG_X_RES;
	m_calibrationInfo.params1080.rgbRegYRes = RGB_REG_Y_RES;
	m_calibrationInfo.params1080.cmosVGAOutputXRes = XN_CMOS_VGAOUTPUT_XRES;
	m_calibrationInfo.params1080.sensorWinOffsetX = XN_SENSOR_WIN_OFFET_X;
	m_calibrationInfo.params1080.sensorWinOffsetY = XN_SENSOR_WIN_OFFET_Y;
	m_calibrationInfo.params1080.rgbRegXValScale = RGB_REG_X_VAL_SCALE;
	m_calibrationInfo.params1080.s2dPelConst = S2D_PEL_CONST;
	m_calibrationInfo.params1080.s2dConstOffset = S2D_CONST_OFFSET;

	XnStatus nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_REGISTRATION, &m_calibrationInfo.params1080.registrationInfo_QQVGA, sizeof(m_calibrationInfo.params1080.registrationInfo_QQVGA), XN_RESOLUTION_QQVGA, 30);
	if (nRetVal != XN_STATUS_OK)
	{
		xnOSMemSet(&m_calibrationInfo.params1080.registrationInfo_QQVGA, 0, sizeof(m_calibrationInfo.params1080.registrationInfo_QQVGA));
	}
	
	nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_REGISTRATION, &m_calibrationInfo.params1080.registrationInfo_QVGA, sizeof(m_calibrationInfo.params1080.registrationInfo_QVGA), XN_RESOLUTION_QVGA, 30);
	XN_IS_STATUS_OK(nRetVal);
	nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_REGISTRATION, &m_calibrationInfo.params1080.registrationInfo_VGA, sizeof(m_calibrationInfo.params1080.registrationInfo_VGA), XN_RESOLUTION_VGA, 30);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_PADDING, &m_calibrationInfo.params1080.padInfo_QQVGA, sizeof(m_calibrationInfo.params1080.padInfo_QQVGA), XN_RESOLUTION_QQVGA, 30);
	if (nRetVal != XN_STATUS_OK)
	{	
		xnOSMemSet(&m_calibrationInfo.params1080.padInfo_QQVGA, 0, sizeof(m_calibrationInfo.params1080.padInfo_QQVGA));
	}
	
	xnOSMemSet(&m_calibrationInfo.params1080.padInfo_QVGA, 0, sizeof(m_calibrationInfo.params1080.padInfo_QVGA));
	nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_PADDING, &m_calibrationInfo.params1080.padInfo_QVGA, sizeof(m_calibrationInfo.params1080.padInfo_QVGA), XN_RESOLUTION_QVGA, 30);
	XN_IS_STATUS_OK(nRetVal);
	xnOSMemSet(&m_calibrationInfo.params1080.padInfo_VGA, 0, sizeof(m_calibrationInfo.params1080.padInfo_VGA));
	nRetVal = XnHostProtocolAlgorithmParams(m_Helper.GetPrivateData(), XN_HOST_PROTOCOL_ALGORITHM_PADDING, &m_calibrationInfo.params1080.padInfo_VGA, sizeof(m_calibrationInfo.params1080.padInfo_VGA), XN_RESOLUTION_VGA, 30);
	XN_IS_STATUS_OK(nRetVal);

	return XN_STATUS_OK;
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetInputFormatCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetInputFormat((XnIODepthFormats)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetRegistrationCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetRegistration((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetHoleFilterCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetHoleFilter((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetWhiteBalanceCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetWhiteBalance((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetGainCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetGain((XnUInt32)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetRegistrationTypeCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetRegistrationType((XnProcessingType)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetGMCModeCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetGMCMode((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetCloseRangeCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetCloseRange((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetCroppingModeCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetCroppingMode((XnCroppingMode)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetGMCDebugCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetGMCDebug((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetWavelengthCorrectionCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetWavelengthCorrection((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetWavelengthCorrectionDebugCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetWavelengthCorrectionDebug((XnBool)nValue);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetAGCBinCallback(XnGeneralProperty* /*pSender*/, const OniGeneralBuffer& gbValue, void* pCookie)
{
	if (gbValue.dataSize != sizeof(XnDepthAGCBin))
	{
		return XN_STATUS_DEVICE_PROPERTY_SIZE_DONT_MATCH;
	}

	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->SetAGCBin((XnDepthAGCBin*)gbValue.data);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::GetAGCBinCallback(const XnGeneralProperty* /*pSender*/, const OniGeneralBuffer& gbValue, void* pCookie)
{
	if (gbValue.dataSize != sizeof(XnDepthAGCBin))
	{
		return XN_STATUS_DEVICE_PROPERTY_SIZE_DONT_MATCH;
	}

	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->GetAGCBin((XnDepthAGCBin*)gbValue.data);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::SetActualReadCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorDepthStream* pThis = (XnSensorDepthStream*)pCookie;
	return pThis->SetActualRead(nValue == TRUE);
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::DecideFirmwareRegistrationCallback(const XnProperty* /*pSender*/, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->DecideFirmwareRegistration((XnBool)pStream->m_DepthRegistration.GetValue(), (XnProcessingType)pStream->m_RegistrationType.GetValue(), pStream->GetResolution());
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::DecidePixelSizeFactorCallback(const XnProperty* /*pSender*/, void* pCookie)
{
	XnSensorDepthStream* pStream = (XnSensorDepthStream*)pCookie;
	return pStream->DecidePixelSizeFactor();
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::ReadAGCBinsFromFile(XnGeneralProperty* pSender, const XnChar* csINIFile, const XnChar* csSection)
{
	XnStatus nRetVal = XN_STATUS_OK;

	for (XnUInt32 nBin = 0; nBin < XN_DEPTH_STREAM_AGC_NUMBER_OF_BINS; ++nBin)
	{
		XnChar csKey[XN_INI_MAX_LEN];
		XnInt32 nValue;

		XnDepthAGCBin bin;
		bin.nBin = (XnUInt16)nBin;

		XnBool bHasMin = FALSE;
		XnBool bHasMax = FALSE;

		sprintf(csKey, "AGCBin%uMinDepth", nBin);

		nRetVal = xnOSReadIntFromINI(csINIFile, csSection, csKey, &nValue);
		if (nRetVal == XN_STATUS_OK)
		{
			bin.nMin = (XnUInt16)nValue;
			bHasMin = TRUE;
		}

		sprintf(csKey, "AGCBin%uMaxDepth", nBin);
		nRetVal = xnOSReadIntFromINI(csINIFile, csSection, csKey, &nValue);
		if (nRetVal == XN_STATUS_OK)
		{
			bin.nMax = (XnUInt16)nValue;
			bHasMax = TRUE;
		}

		if (bHasMax && bHasMin)
		{
			nRetVal = pSender->SetValue(XN_PACK_GENERAL_BUFFER(bin));
			XN_IS_STATUS_OK(nRetVal);
		}
		else if (bHasMin || bHasMax)
		{
			// we have only one
			XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Bin %d should have both min and max values!", nBin);
		}
	}

	return XN_STATUS_OK;
}

XnStatus XN_CALLBACK_TYPE XnSensorDepthStream::GetPixelRegistrationCallback(const XnGeneralProperty* /*pSender*/, const OniGeneralBuffer& gbValue, void* pCookie)
{
	XnSensorDepthStream* pThis = (XnSensorDepthStream*)pCookie;

	if (gbValue.dataSize != sizeof(XnPixelRegistration))
	{
		return XN_STATUS_DEVICE_PROPERTY_SIZE_DONT_MATCH;
	}

	XnPixelRegistration* pArgs = (XnPixelRegistration*)gbValue.data;

	return pThis->GetImageCoordinatesOfDepthPixel(pArgs->nDepthX, pArgs->nDepthY, pArgs->nDepthValue, pArgs->nImageXRes, pArgs->nImageYRes, pArgs->nImageX, pArgs->nImageY);
}

