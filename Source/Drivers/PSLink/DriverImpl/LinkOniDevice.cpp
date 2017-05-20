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
#include "LinkOniDevice.h"
#include "LinkOniDriver.h"

#include "LinkDeviceEnumeration.h"
#include <PS1200Device.h>
#include <XnPsVersion.h>

//---------------------------------------------------------------------------
// LinkOniDevice class
//---------------------------------------------------------------------------
#define XN_MASK_LINK_DEVICE "LinkDevice"
#define CONFIG_DEVICE_SECTION "Device"

#if XN_PLATFORM == XN_PLATFORM_WIN32
	#define XN_DEFAULT_USB_INTERFACE	PS_USB_INTERFACE_ISO_ENDPOINTS;
#elif XN_PLATFORM == XN_PLATFORM_LINUX_X86 || XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_MACOSX || XN_PLATFORM == XN_PLATFORM_ANDROID_ARM || XN_PLATFORM == XN_PLATFORM_LINUX_AARCH64
	#define XN_DEFAULT_USB_INTERFACE	PS_USB_INTERFACE_BULK_ENDPOINTS;
#else
	#error Unsupported platform!
#endif


LinkOniDevice::LinkOniDevice(const char* configFile, const XnChar* uri, oni::driver::DriverServices& driverServices, LinkOniDriver* pDriver) :
	m_configFile(configFile), m_pSensor(NULL), m_driverServices(driverServices), m_pDriver(pDriver)
{
	xnOSMemCopy(&m_info, LinkDeviceEnumeration::GetDeviceInfo(uri), sizeof(m_info));
}

LinkOniDevice::~LinkOniDevice()
{
	// free the allocated arrays
	for(int i=0; i < m_numSensors; ++i)
	{
		XN_DELETE_ARR(m_sensors[i].pSupportedVideoModes);
	}
	
	Destroy();
}

XnStatus LinkOniDevice::readSupportedModesFromStream(XnFwStreamInfo &info, xnl::Array<XnFwStreamVideoMode> &aSupportedModes)
{
	XnUInt16 streamId;
	XnStatus nRetVal = m_pSensor->CreateInputStream(info.type, info.creationInfo, streamId);
	XN_IS_STATUS_OK(nRetVal);

	// TODO: make sure this cast doesn't make us problems
	xn::LinkFrameInputStream *pInputStream = (xn::LinkFrameInputStream *)m_pSensor->GetInputStream(streamId);
	XN_VALIDATE_OUTPUT_PTR(pInputStream);

	aSupportedModes.CopyFrom(pInputStream->GetSupportedVideoModes());

	m_pSensor->DestroyInputStream(streamId);
	return XN_STATUS_OK;
}

XnStatus AddVideoMode(xnl::Array<OniVideoMode>& modes, XnFwStreamVideoMode fwMode, OniPixelFormat pixelFormat)
{
	// make sure it's not in the list already
	for (XnUInt32 i = 0; i < modes.GetSize(); ++i)
	{
		if (modes[i].resolutionX == (int)fwMode.m_nXRes &&
			modes[i].resolutionY == (int)fwMode.m_nYRes &&
			modes[i].fps == (int)fwMode.m_nFPS &&
			modes[i].pixelFormat == pixelFormat)
		{
			return XN_STATUS_OK;
		}
	}

	OniVideoMode mode;
	mode.resolutionX = fwMode.m_nXRes;
	mode.resolutionY = fwMode.m_nYRes;
	mode.fps = fwMode.m_nFPS;
	mode.pixelFormat = pixelFormat;
	return modes.AddLast(mode);
}

XnStatus LinkOniDevice::FillSupportedVideoModes()
{
	xnl::Array<XnFwStreamVideoMode> aSupportedModes;
	xnl::Array<XnFwStreamInfo> aEnumerated;
	xnl::Array<OniVideoMode> aVideoModes;

	int s = -1;

	// Depth
	m_pSensor->EnumerateStreams((XnStreamType)XN_LINK_STREAM_TYPE_SHIFTS, aEnumerated);
	for (int c = 0; c < (int)aEnumerated.GetSize(); ++c)
	{
		XnStatus nRetVal = readSupportedModesFromStream(aEnumerated[c], aSupportedModes);
		XN_IS_STATUS_OK(nRetVal);

		for (XnUInt32 i = 0; i < aSupportedModes.GetSize(); ++i)
		{
			nRetVal = AddVideoMode(aVideoModes, aSupportedModes[i], ONI_PIXEL_FORMAT_DEPTH_1_MM);
			XN_IS_STATUS_OK(nRetVal);
			nRetVal = AddVideoMode(aVideoModes, aSupportedModes[i], ONI_PIXEL_FORMAT_DEPTH_100_UM);
			XN_IS_STATUS_OK(nRetVal);
		}
	}
	++s;
	m_sensors[s].sensorType             = ONI_SENSOR_DEPTH;
	m_sensors[s].pSupportedVideoModes   = XN_NEW_ARR(OniVideoMode, aVideoModes.GetSize());
	XN_VALIDATE_ALLOC_PTR(m_sensors[s].pSupportedVideoModes);
	xnOSMemCopy(m_sensors[s].pSupportedVideoModes, aVideoModes.GetData(), aVideoModes.GetSize() * sizeof(OniVideoMode));
	m_sensors[s].numSupportedVideoModes = aVideoModes.GetSize();

	m_numSensors = s+1;
	aEnumerated.Clear();
	aVideoModes.Clear();

	// IR
	m_pSensor->EnumerateStreams((XnStreamType)XN_LINK_STREAM_TYPE_IR, aEnumerated);
	for (int c = 0; c < (int)aEnumerated.GetSize(); ++c)
	{
		XnStatus nRetVal = readSupportedModesFromStream(aEnumerated[c], aSupportedModes);
		XN_IS_STATUS_OK(nRetVal);

		for (XnUInt32 i = 0; i < aSupportedModes.GetSize(); ++i)
		{
			nRetVal = AddVideoMode(aVideoModes, aSupportedModes[i], ONI_PIXEL_FORMAT_GRAY16);
			XN_IS_STATUS_OK(nRetVal);
		}
	}
	++s;
	m_sensors[s].sensorType             = ONI_SENSOR_IR;
	m_sensors[s].pSupportedVideoModes   = XN_NEW_ARR(OniVideoMode, aVideoModes.GetSize());
	XN_VALIDATE_ALLOC_PTR(m_sensors[s].pSupportedVideoModes);
	xnOSMemCopy(m_sensors[s].pSupportedVideoModes, aVideoModes.GetData(), aVideoModes.GetSize() * sizeof(OniVideoMode));
	m_sensors[s].numSupportedVideoModes = aVideoModes.GetSize();

	m_numSensors = s+1;
	aEnumerated.Clear();
	aVideoModes.Clear();

	return XN_STATUS_OK;
}

XnStatus LinkOniDevice::Init(const char* mode)
{
	XnBool performReset = TRUE;
	XnBool leanInit = FALSE;

	if (mode != NULL)
	{
		for (const char* option = mode; *option != '\0'; ++option)
		{
			switch (*option)
			{
			case 'r':
				performReset = FALSE;
				break;
			case 'l':
				leanInit = TRUE;
				break;
			default:
				m_driverServices.errorLoggerAppend("Invalid mode: %c", *option);
				return XN_STATUS_BAD_PARAM;
			}
		}
	}

	xn::PS1200Device *pPrimeClient = new xn::PS1200Device();
	XN_VALIDATE_ALLOC_PTR(pPrimeClient);

	XnStatus retVal = pPrimeClient->Init(m_info.uri, XN_TRANSPORT_TYPE_USB);
	if (retVal != XN_STATUS_OK)
    {
        xnLogError(XN_MASK_LINK_DEVICE, "Failed to initialize prime client: %s", xnGetStatusString(retVal));
        XN_ASSERT(FALSE);
        XN_DELETE(pPrimeClient);
        return retVal;
    }

    retVal = pPrimeClient->Connect();
    if (retVal != XN_STATUS_OK)
    {
        xnLogError(XN_MASK_LINK_DEVICE, "Failed to connect to device: %s", xnGetStatusString(retVal));
        XN_ASSERT(FALSE);
        XN_DELETE(pPrimeClient);
        return retVal;
    }
    
	if (performReset)
	{
		retVal = pPrimeClient->SoftReset();
		if (retVal != XN_STATUS_OK)
		{
			xnLogError(XN_MASK_LINK_DEVICE, "Failed to reset device: %s", xnGetStatusString(retVal));
			XN_ASSERT(FALSE);
			XN_DELETE(pPrimeClient);
			return retVal;
		}
	}

	m_pSensor = pPrimeClient;

	XnUsbInterfaceType usbType = PS_USB_INTERFACE_DONT_CARE;
	if (performReset)
	{
		usbType = XN_DEFAULT_USB_INTERFACE;
	}

	XnInt32 value32;
	if (XN_STATUS_OK == xnOSReadIntFromINI(m_configFile, CONFIG_DEVICE_SECTION, "UsbInterface", &value32))
	{
		usbType = (XnUsbInterfaceType)value32;
	}

	retVal = setProperty(PS_PROPERTY_USB_INTERFACE, &usbType, sizeof(usbType));
	if (retVal != XN_STATUS_OK)
	{
		XN_DELETE(pPrimeClient);
		return retVal;
	}

	if (XN_STATUS_OK == xnOSReadIntFromINI(m_configFile, CONFIG_DEVICE_SECTION, "FirmwareLog", &value32))
	{
		if (value32 == TRUE)
		{
			retVal = m_pSensor->StartFWLog();
		}

		if (retVal != XN_STATUS_OK)
		{
			XN_DELETE(pPrimeClient);
			return retVal;
		}
	}

	if (!leanInit)
	{
		retVal = FillSupportedVideoModes();
		if (retVal != XN_STATUS_OK)
		{
			xnLogError(XN_MASK_LINK_DEVICE, "Failed to read device video modes: %s", xnGetStatusString(retVal));
			XN_ASSERT(FALSE);
			XN_DELETE(pPrimeClient);
			return retVal;
		}
	}

	return XN_STATUS_OK;
}

void LinkOniDevice::Destroy()
{
	if (m_pSensor == NULL)
	{
		return;
	}

	// TODO can we actually create more than one?
	//m_createdDevices.Remove(pPrimeClient->GetConnectionString());

	m_pSensor->Disconnect();
	m_pSensor->Shutdown();
	XN_DELETE(m_pSensor);
	m_pSensor = NULL;
}

OniStatus LinkOniDevice::getSensorInfoList(OniSensorInfo** pSensors, int* numSensors)
{
	*numSensors = m_numSensors;
	*pSensors   = m_sensors;

	return ONI_STATUS_OK;
}

oni::driver::StreamBase* LinkOniDevice::createStream(OniSensorType sensorType)
{
	LinkOniStream* pStream;

	if (sensorType == ONI_SENSOR_DEPTH)
	{
		pStream = XN_NEW(LinkOniDepthStream, m_configFile, m_pSensor, this);
	}
	else if (sensorType == ONI_SENSOR_IR)
	{
		pStream = XN_NEW(LinkOniIRStream, m_configFile, m_pSensor, this);
	}
	//else if (sensorType == ONI_SENSOR_COLOR)
	//{
	//	pStream = XN_NEW(LinkOniColorStream, &m_sensor, this);
	//}
	else
	{
		m_driverServices.errorLoggerAppend("LinkOniDevice: Can't create a stream of type %d", sensorType);
		return NULL;
	}

	XnStatus nRetVal = pStream->Init();
	if (nRetVal != XN_STATUS_OK)
	{
		m_driverServices.errorLoggerAppend("LinkOniDevice: Can't initialize stream of type %d: %s", sensorType, xnGetStatusString(nRetVal));
		XN_DELETE(pStream);
		return NULL;
	}

	return pStream;
}

void LinkOniDevice::destroyStream(oni::driver::StreamBase* pStream)
{
	XN_DELETE(pStream);
}

OniStatus LinkOniDevice::getProperty(int propertyId, void* data, int* pDataSize)
{
	XnStatus nRetVal = XN_STATUS_OK;

	switch (propertyId)
	{
	case ONI_DEVICE_PROPERTY_FIRMWARE_VERSION:
		{
			XnDetailedVersion versions = m_pSensor->GetFWVersion();
			XnUInt32 nCharsWritten = 0;
			XnStatus rc = xnOSStrFormat((XnChar*)data, *pDataSize, &nCharsWritten, "%d.%d.%d.%d-%s", versions.m_nMajor, versions.m_nMinor, versions.m_nMaintenance, versions.m_nBuild, versions.m_strModifier);
			if (rc != XN_STATUS_OK)
			{
				m_driverServices.errorLoggerAppend("Couldn't get firmware version: %s\n", xnGetStatusString(rc));
				return ONI_STATUS_BAD_PARAMETER;
			}
			*pDataSize = nCharsWritten+1;
			break;
		}

	case ONI_DEVICE_PROPERTY_HARDWARE_VERSION:
		ENSURE_PROP_SIZE_DO(*pDataSize, short)
		{
			m_driverServices.errorLoggerAppend("Unexpected size: %d != %d or %d or %d\n", *pDataSize, sizeof(short), sizeof(int), sizeof(uint64_t));
			XN_ASSERT(FALSE);
			return ONI_STATUS_BAD_PARAMETER;
		}
		ASSIGN_PROP_VALUE_INT(data, *pDataSize, m_pSensor->GetHWVersion());			
		break;
		
	case ONI_DEVICE_PROPERTY_SERIAL_NUMBER:
		{
			const XnChar *serialNumber = m_pSensor->GetSerialNumber();
			
			if (xnOSStrCopy((XnChar*)data, serialNumber, *pDataSize) != XN_STATUS_OK)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", *pDataSize, xnOSStrLen(serialNumber));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}
			break;
		}
	case ONI_DEVICE_PROPERTY_DRIVER_VERSION:
		{
			EXACT_PROP_SIZE_DO(*pDataSize, OniVersion)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniVersion));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}
			OniVersion* version = (OniVersion*)data;
			version->major		 = XN_PS_MAJOR_VERSION;
			version->minor		 = XN_PS_MINOR_VERSION;
			version->maintenance = XN_PS_MAINTENANCE_VERSION;
			version->build		 = XN_PS_BUILD_VERSION;
			break;
		}
/*	case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
		{
			m_pSensor
			if (*pDataSize == sizeof(OniImageRegistrationMode))
			{
				OniImageRegistrationMode* mode = (OniImageRegistrationMode*)data;

				// Find the depth stream in the sensor.
				XnDeviceStream* pDepth = NULL;
				XnStatus xnrc = m_sensor.GetStream(XN_STREAM_NAME_DEPTH, &pDepth);
				if (xnrc != XN_STATUS_OK)
				{
					return ONI_STATUS_BAD_PARAMETER;
				}

				// Set the mode in the depth stream.
				XnUInt64 val;
				xnrc = pDepth->GetProperty(XN_STREAM_PROPERTY_REGISTRATION, &val);
				if (xnrc != XN_STATUS_OK)
				{
					return ONI_STATUS_ERROR;
				}

				// Update the return value.
				*mode = (val == 1) ? ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR : ONI_IMAGE_REGISTRATION_OFF;
			}
			else
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniImageRegistrationMode));
				return ONI_STATUS_ERROR;
			}
		}
		break;*/

	// Internal Link Properties
	case LINK_PROP_FW_VERSION:
		EXACT_PROP_SIZE_DO(*pDataSize, XnDetailedVersion)
		{
			m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", *pDataSize, sizeof(XnDetailedVersion));
			XN_ASSERT(FALSE);
			return ONI_STATUS_BAD_PARAMETER;
		}
		*(XnDetailedVersion*)data = m_pSensor->GetFWVersion();
		break;

	case LINK_PROP_VERSIONS_INFO_COUNT:
		{
			ENSURE_PROP_SIZE(*pDataSize, int);
			xnl::Array<XnComponentVersion> versions;
			nRetVal = m_pSensor->GetComponentsVersions(versions);
			XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

			ASSIGN_PROP_VALUE_INT(data, *pDataSize, versions.GetSize());
			break;
		}

		//case :
		//TODO: Implement Get emitter active
		//break;

	case LINK_PROP_VERSIONS_INFO:
		{
			xnl::Array<XnComponentVersion> components;
			nRetVal = m_pSensor->GetComponentsVersions(components);
			XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

			XnUInt32 nExpectedSize = components.GetSize() * sizeof(XnComponentVersion);
			if (*pDataSize != (int)nExpectedSize)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", *pDataSize, nExpectedSize);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnOSMemCopy(data, components.GetData(), nExpectedSize);
			break;
		}

	case PS_PROPERTY_USB_INTERFACE:
		{
			ENSURE_PROP_SIZE(*pDataSize, XnUInt8);
			ASSIGN_PROP_VALUE_INT(data, *pDataSize, 0);
			XnUInt8 nInterface = 0;
			nRetVal = m_pSensor->GetUsbAltInterface(nInterface);
			XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);
			if (nInterface == 0)
			{
				*(XnUInt8*)data = PS_USB_INTERFACE_ISO_ENDPOINTS;
			}
			else if (nInterface == 1)
			{
				*(XnUInt8*)data = PS_USB_INTERFACE_BULK_ENDPOINTS;
			}
			else
			{
				XN_ASSERT(FALSE);
				return ONI_STATUS_ERROR;
			}
		}
		break;

	case LINK_PROP_BOOT_STATUS:
		{
			EXACT_PROP_SIZE(*pDataSize, XnBootStatus);
			XnBootStatus* pStatus = (XnBootStatus*)data;
			nRetVal = m_pSensor->GetBootStatus(*pStatus);
			XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);
		}
		break;
    case LINK_PROP_ACC_ENABLED:
        {
            ENSURE_PROP_SIZE(*pDataSize, XnBool);

            XnBool bActive;
            nRetVal = m_pSensor->GetAccActive(bActive);
            XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

            ASSIGN_PROP_VALUE_INT(data, *pDataSize, bActive)
        }
        break;

    case LINK_PROP_VDD_ENABLED:
        {
            ENSURE_PROP_SIZE(*pDataSize, XnBool);

            XnBool bActive;
            nRetVal = m_pSensor->GetVDDActive(bActive);
            XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

            ASSIGN_PROP_VALUE_INT(data, *pDataSize, bActive)
        }
        break;

    case LINK_PROP_PERIODIC_BIST_ENABLED:
        {
            ENSURE_PROP_SIZE(*pDataSize, XnBool);

            XnBool bActive;
            nRetVal = m_pSensor->GetPeriodicBistActive(bActive);
            XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

            ASSIGN_PROP_VALUE_INT(data, *pDataSize, bActive)
        }
        break;


	case LINK_PROP_PROJECTOR_POWER:
		{
			ENSURE_PROP_SIZE(*pDataSize, XnUInt16);

			XnUInt16 power;
			nRetVal = m_pSensor->GetProjectorPower(power);
			XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);

			ASSIGN_PROP_VALUE_INT(data, *pDataSize, power)
		}
		break;

	default:
		return ONI_STATUS_BAD_PARAMETER;
	}

	return ONI_STATUS_OK;
}

OniStatus LinkOniDevice::setProperty(int propertyId, const void* data, int dataSize)
{
	XnStatus nRetVal = XN_STATUS_OK;

	switch (propertyId)
	{
/*	case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
		{
			if (dataSize == sizeof(OniImageRegistrationMode))
			{
				OniImageRegistrationMode* mode = (OniImageRegistrationMode*)data;

				// Find the depth stream in the sensor.
				XnDeviceStream* pDepth = NULL;
				XnStatus xnrc = m_sensor.GetStream(XN_STREAM_NAME_DEPTH, &pDepth);
				if (xnrc != XN_STATUS_OK)
				{
					return ONI_STATUS_BAD_PARAMETER;
				}

				// Set the mode in the depth stream.
				XnUInt64 val = (*mode == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR) ? 1 : 0;
				xnrc = pDepth->SetProperty(XN_STREAM_PROPERTY_REGISTRATION, val);
				if (xnrc != XN_STATUS_OK)
				{
					return ONI_STATUS_ERROR;
				}
			}
			else
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(OniImageRegistrationMode));
				return ONI_STATUS_ERROR;
			}
		}
		break;*/


	// Internal Link Properties
	// int props
	case LINK_PROP_PROJECTOR_ACTIVE:
		nRetVal = m_pSensor->SetProjectorActive(*(XnBool*)data);
		XN_IS_STATUS_OK_LOG_ERROR_RET("Set Projector active", nRetVal, ONI_STATUS_ERROR);
		break;

    //controls if the firmware runs all its control loops (BIST)
    case LINK_PROP_ACC_ENABLED:
        nRetVal = m_pSensor->SetAccActive(*(XnBool*)data);
        XN_IS_STATUS_OK_LOG_ERROR_RET("Set Acc active", nRetVal, ONI_STATUS_ERROR);
        break;

        //
    case LINK_PROP_VDD_ENABLED:
        nRetVal = m_pSensor->SetVDDActive(*(XnBool*)data);
        XN_IS_STATUS_OK_LOG_ERROR_RET("Set VDD active", nRetVal, ONI_STATUS_ERROR);
        break;

        //
    case LINK_PROP_PERIODIC_BIST_ENABLED:
        nRetVal = m_pSensor->SetPeriodicBistActive(*(XnBool*)data);
        XN_IS_STATUS_OK_LOG_ERROR_RET("Set PeriodicBist active", nRetVal, ONI_STATUS_ERROR);
        break;

		// string props
	case LINK_PROP_PRESET_FILE:
		nRetVal = m_pSensor->RunPresetFile((XnChar *)data);
		XN_IS_STATUS_OK_LOG_ERROR_RET("RunPresetFile", nRetVal, ONI_STATUS_ERROR);
		break;
	
	case PS_PROPERTY_USB_INTERFACE:
		{
			ENSURE_PROP_SIZE(dataSize, XnUInt8);
			XnUsbInterfaceType type = (XnUsbInterfaceType)*(XnUInt8*)data;
			if (type == PS_USB_INTERFACE_ISO_ENDPOINTS)
			{
				nRetVal = m_pSensor->SetUsbAltInterface(0);
				XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);
			}
			else if (type == PS_USB_INTERFACE_BULK_ENDPOINTS)
			{
				nRetVal = m_pSensor->SetUsbAltInterface(1);
				XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);
			}
			else if (type != PS_USB_INTERFACE_DONT_CARE)
			{
				return ONI_STATUS_BAD_PARAMETER;
			}
		}
		break;

	case LINK_PROP_PROJECTOR_POWER:
		ENSURE_PROP_SIZE(dataSize, XnUInt16);
		nRetVal = m_pSensor->SetProjectorPower(*(XnUInt16*)data);
		XN_IS_STATUS_OK_RET(nRetVal, ONI_STATUS_ERROR);
		break;

	default:
			return ONI_STATUS_BAD_PARAMETER;
	}

	return ONI_STATUS_OK;
}

OniBool LinkOniDevice::isPropertySupported(int propertyId)
{
	switch (propertyId)
	{
	case ONI_DEVICE_PROPERTY_FIRMWARE_VERSION:
	case ONI_DEVICE_PROPERTY_HARDWARE_VERSION:
	case ONI_DEVICE_PROPERTY_SERIAL_NUMBER:
	case ONI_DEVICE_PROPERTY_DRIVER_VERSION:
	//case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:

	// Internal Link Properties
	case LINK_PROP_FW_VERSION:
	case LINK_PROP_VERSIONS_INFO_COUNT:
	case LINK_PROP_VERSIONS_INFO:
	case LINK_PROP_PROJECTOR_ACTIVE:
    case LINK_PROP_ACC_ENABLED:
    case LINK_PROP_VDD_ENABLED:
    case LINK_PROP_PERIODIC_BIST_ENABLED:
	case LINK_PROP_PRESET_FILE:
	case PS_PROPERTY_USB_INTERFACE:
	case LINK_PROP_BOOT_STATUS:
	case LINK_PROP_PROJECTOR_POWER:
		return true;
	default:
		return false;
	}
}

void LinkOniDevice::notifyAllProperties()
{
	XnDetailedVersion version;
	int size = sizeof(version);
	getProperty(LINK_PROP_FW_VERSION, &version, &size);
	raisePropertyChanged(LINK_PROP_FW_VERSION, &version, size);
	
	XnUInt8 altusb;
	size = sizeof(altusb);
	getProperty(PS_PROPERTY_USB_INTERFACE, &altusb, &size);
	raisePropertyChanged(PS_PROPERTY_USB_INTERFACE, &altusb, size);
}

OniStatus LinkOniDevice::invoke(int commandId, void* data, int dataSize)
{
	XnStatus nRetVal = XN_STATUS_OK;

	switch (commandId)
	{
	case PS_COMMAND_AHB_READ:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandAHB)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandAHB));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandAHB* pPropReadAHB = reinterpret_cast<XnCommandAHB*>(data);
			nRetVal = m_pSensor->ReadAHB(pPropReadAHB->address, 
				static_cast<XnUInt8>(pPropReadAHB->offsetInBits), 
				static_cast<XnUInt8>(pPropReadAHB->widthInBits), 
				pPropReadAHB->value);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Read AHB", nRetVal, ONI_STATUS_ERROR);
			break;
		}

	case PS_COMMAND_AHB_WRITE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandAHB)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandAHB));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandAHB* pPropWriteAHB = reinterpret_cast<XnCommandAHB*>(data);
			nRetVal = m_pSensor->WriteAHB(pPropWriteAHB->address, 
				pPropWriteAHB->value, 
				static_cast<XnUInt8>(pPropWriteAHB->offsetInBits), 
				static_cast<XnUInt8>(pPropWriteAHB->widthInBits));
			XN_IS_STATUS_OK_LOG_ERROR_RET("Write AHB", nRetVal, ONI_STATUS_ERROR);
			break;
		}

	case PS_COMMAND_I2C_READ:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandI2C)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandI2C));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandI2C* pPropReadI2C = reinterpret_cast<XnCommandI2C*>(data);
			nRetVal = m_pSensor->ReadI2C(
				static_cast<XnUInt8>(pPropReadI2C->deviceID),  
				static_cast<XnUInt8>(pPropReadI2C->addressSize), 
				pPropReadI2C->address, 
				static_cast<XnUInt8>(pPropReadI2C->valueSize),
				pPropReadI2C->value);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Read I2C", nRetVal, ONI_STATUS_ERROR);
			break;
		}

	case PS_COMMAND_I2C_WRITE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandI2C)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandI2C));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandI2C* pPropWriteI2C = reinterpret_cast<XnCommandI2C*>(data);
			nRetVal = m_pSensor->WriteI2C(static_cast<XnUInt8>(pPropWriteI2C->deviceID), 
				static_cast<XnUInt8>(pPropWriteI2C->addressSize), 
				pPropWriteI2C->address, 
				static_cast<XnUInt8>(pPropWriteI2C->valueSize),
				pPropWriteI2C->value,
				pPropWriteI2C->mask);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Write I2C", nRetVal, ONI_STATUS_ERROR);
			break;
		}

	case PS_COMMAND_SOFT_RESET:
		nRetVal = m_pSensor->SoftReset();
		XN_IS_STATUS_OK_LOG_ERROR_RET("Soft reset", nRetVal, ONI_STATUS_ERROR);
		break;

	case PS_COMMAND_POWER_RESET:
		nRetVal = m_pSensor->HardReset();
		XN_IS_STATUS_OK_LOG_ERROR_RET("Power reset", nRetVal, ONI_STATUS_ERROR);
		break;
    case PS_COMMAND_READ_DEBUG_DATA:
        {
            EXACT_PROP_SIZE_DO(dataSize, XnCommandDebugData)
            {
                m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandDebugData));
                XN_ASSERT(FALSE);
                return ONI_STATUS_BAD_PARAMETER;
            }

            XnCommandDebugData* pArgs = reinterpret_cast<XnCommandDebugData*>(data);
            nRetVal = m_pSensor->ReadDebugData(*pArgs);
            XN_IS_STATUS_OK_LOG_ERROR_RET("Reading Debug Data", nRetVal, ONI_STATUS_ERROR);
        }
        break;
	case PS_COMMAND_BEGIN_FIRMWARE_UPDATE:
		nRetVal = m_pSensor->BeginUploadFileOnControlEP();
		XN_IS_STATUS_OK_LOG_ERROR_RET("Begin update", nRetVal, ONI_STATUS_ERROR);
		break;

	case PS_COMMAND_END_FIRMWARE_UPDATE:
		nRetVal = m_pSensor->EndUploadFileOnControlEP();
		XN_IS_STATUS_OK_LOG_ERROR_RET("End update", nRetVal, ONI_STATUS_ERROR);
		break;

	case PS_COMMAND_UPLOAD_FILE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandUploadFile)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandUploadFile));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandUploadFile* pArgs = reinterpret_cast<XnCommandUploadFile*>(data);
			nRetVal = m_pSensor->UploadFileOnControlEP(pArgs->filePath, pArgs->uploadToFactory);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Upload File", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_DOWNLOAD_FILE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandDownloadFile)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandDownloadFile));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandDownloadFile* pArgs = reinterpret_cast<XnCommandDownloadFile*>(data);
			nRetVal = m_pSensor->DownloadFile(static_cast<XnUInt>(pArgs->zone),  
				pArgs->firmwareFileName, pArgs->targetPath);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Download File", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_GET_FILE_LIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetFileList)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetFileList));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetFileList* pArgs = reinterpret_cast<XnCommandGetFileList*>(data);
			if (pArgs->files == NULL)
			{
				m_driverServices.errorLoggerAppend("Files array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnl::Array<XnFwFileEntry> files;
			nRetVal = m_pSensor->GetFileList(files);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Get file list", nRetVal, ONI_STATUS_ERROR);

			if (pArgs->count < files.GetSize())
			{
				m_driverServices.errorLoggerAppend("Insufficient memory for files list. available: %d, required: %d\n", pArgs->count, files.GetSize());
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnOSMemCopy(pArgs->files, files.GetData(), files.GetSize() * sizeof(XnFwFileEntry));
			pArgs->count = files.GetSize();
		}
		break;

	case PS_COMMAND_FORMAT_ZONE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandFormatZone)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandFormatZone));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			const XnCommandFormatZone* pArgs = reinterpret_cast<const XnCommandFormatZone*>(data);
			nRetVal = m_pSensor->FormatZone(pArgs->zone);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Format Zone", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_DUMP_ENDPOINT:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandDumpEndpoint)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandDumpEndpoint));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			const XnCommandDumpEndpoint* pArgs = reinterpret_cast<const XnCommandDumpEndpoint*>(data);
			XnChar strDumpName[XN_FILE_MAX_PATH] = "";
			xnLinkGetEPDumpName(pArgs->endpoint, strDumpName, sizeof(strDumpName));
			xnDumpSetMaskState(strDumpName, pArgs->enabled);
		}
		break;

	case PS_COMMAND_GET_I2C_DEVICE_LIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetI2CDeviceList)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetI2CDeviceList));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetI2CDeviceList* pArgs = reinterpret_cast<XnCommandGetI2CDeviceList*>(data);
			if (pArgs->devices == NULL)
			{
				m_driverServices.errorLoggerAppend("Devices array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnl::Array<XnLinkI2CDevice> devices;
			nRetVal = m_pSensor->GetSupportedI2CDevices(devices);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Get i2c device list", nRetVal, ONI_STATUS_ERROR);

			if (pArgs->count < devices.GetSize())
			{
				m_driverServices.errorLoggerAppend("Insufficient memory for device list. available: %d, required: %d\n", pArgs->count, devices.GetSize());
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			for (int i = 0; i < (int)devices.GetSize(); ++i)
			{
                pArgs->devices[i].masterId = devices[i].m_nMasterID;
                pArgs->devices[i].slaveId = devices[i].m_nSlaveID;
				pArgs->devices[i].id = devices[i].m_nID;
				xnOSStrCopy(pArgs->devices[i].name, devices[i].m_strName, sizeof(pArgs->devices[i].name));
			}
			pArgs->count = devices.GetSize();
		}
		break;

	case PS_COMMAND_GET_BIST_LIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetBistList)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetBistList));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetBistList* pArgs = reinterpret_cast<XnCommandGetBistList*>(data);
			if (pArgs->tests == NULL)
			{
				m_driverServices.errorLoggerAppend("Bist array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnl::Array<XnBistInfo> tests;
			nRetVal = m_pSensor->GetSupportedBistTests(tests);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Get bist list", nRetVal, ONI_STATUS_ERROR);

			if (pArgs->count < tests.GetSize())
			{
				m_driverServices.errorLoggerAppend("Insufficient memory for tests list. available: %d, required: %d\n", pArgs->count, tests.GetSize());
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			for (int i = 0; i < (int)tests.GetSize(); ++i)
			{
				pArgs->tests[i] = tests[i];
			}
			pArgs->count = tests.GetSize();
		}
		break;

    case PS_COMMAND_GET_TEMP_LIST:
        {
            EXACT_PROP_SIZE_DO(dataSize, XnCommandGetTempList)
            {
                m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetTempList));
                XN_ASSERT(FALSE);
                return ONI_STATUS_BAD_PARAMETER;
            }

            XnCommandGetTempList* pArgs = reinterpret_cast<XnCommandGetTempList*>(data);
            if (pArgs->pTempInfos == NULL)
            {
                m_driverServices.errorLoggerAppend("Temp array must point to valid memory: \n");
                XN_ASSERT(FALSE);
                return ONI_STATUS_BAD_PARAMETER;
            }

            xnl::Array<XnTempInfo> tempInfos;
            nRetVal = m_pSensor->GetSupportedTempList(tempInfos);
            XN_IS_STATUS_OK_LOG_ERROR_RET("Get Temp list", nRetVal, ONI_STATUS_ERROR);

            if (pArgs->count < tempInfos.GetSize())
            {
                m_driverServices.errorLoggerAppend("Insufficient memory for Temperature list. available: %d, required: %d\n", pArgs->pTempInfos, tempInfos.GetSize());
                XN_ASSERT(FALSE);
                return ONI_STATUS_BAD_PARAMETER;
            }

            for (int i = 0; i < (int)tempInfos.GetSize(); ++i)
            {
                pArgs->pTempInfos[i] = tempInfos[i];
            }
            pArgs->count = tempInfos.GetSize();
        }
        break;
    case PS_COMMAND_READ_TEMPERATURE:
        {
            XnCommandTemperatureResponse* pArg;
            EXACT_PROP_SIZE_DO(dataSize,XnCommandTemperatureResponse)
            {
                m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandTemperatureResponse));
                XN_ASSERT(FALSE);
                return ONI_STATUS_BAD_PARAMETER;
            }
            pArg = reinterpret_cast<XnCommandTemperatureResponse*>(data);
            nRetVal = m_pSensor->GetTemperature(*pArg);
            XN_IS_STATUS_OK_LOG_ERROR_RET("Get Temperature", nRetVal, ONI_STATUS_ERROR);
        }
        break;
	case PS_COMMAND_EXECUTE_BIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandExecuteBist)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandExecuteBist));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandExecuteBist* pArgs = reinterpret_cast<XnCommandExecuteBist*>(data);
			if (pArgs->extraData == NULL)
			{
				m_driverServices.errorLoggerAppend("extra data array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			nRetVal = m_pSensor->ExecuteBist(pArgs->id, pArgs->errorCode, pArgs->extraDataSize, pArgs->extraData);
			XN_IS_STATUS_OK_LOG_ERROR_RET("execute bist", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_USB_TEST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandUsbTest)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandUsbTest));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandUsbTest* pArgs = reinterpret_cast<XnCommandUsbTest*>(data);
			if (pArgs->endpoints == NULL)
			{
				m_driverServices.errorLoggerAppend("Endpoints array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			nRetVal = m_pSensor->UsbTest(pArgs->seconds, pArgs->endpointCount, pArgs->endpoints);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Usb test", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_GET_LOG_MASK_LIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetLogMaskList)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetLogMaskList));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetLogMaskList* pArgs = reinterpret_cast<XnCommandGetLogMaskList*>(data);
			if (pArgs->masks == NULL)
			{
				m_driverServices.errorLoggerAppend("Mask array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnl::Array<XnLinkLogFile> masks;
			nRetVal = m_pSensor->GetSupportedLogFiles(masks);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Get log mask list", nRetVal, ONI_STATUS_ERROR);

			if (pArgs->count < masks.GetSize())
			{
				m_driverServices.errorLoggerAppend("Insufficient memory for masks list. available: %d, required: %d\n", pArgs->count, masks.GetSize());
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			for (int i = 0; i < (int)masks.GetSize(); ++i)
			{
				pArgs->masks[i].id = masks[i].m_nID;
				xnOSStrCopy(pArgs->masks[i].name, masks[i].m_strName, sizeof(pArgs->masks[i].name));
			}
			pArgs->count = masks.GetSize();
		}
		break;

	case PS_COMMAND_SET_LOG_MASK_STATE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandSetLogMaskState)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandSetLogMaskState));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandSetLogMaskState* pArgs = reinterpret_cast<XnCommandSetLogMaskState*>(data);
			if (pArgs->enabled)
			{
				nRetVal = m_pSensor->OpenFWLogFile((XnUInt8)pArgs->mask);
			}
			else
			{
				nRetVal = m_pSensor->CloseFWLogFile((XnUInt8)pArgs->mask);
			}
			XN_IS_STATUS_OK_LOG_ERROR_RET("Set log mask state", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_START_LOG:
		{
			nRetVal = m_pSensor->StartFWLog();
			XN_IS_STATUS_OK_LOG_ERROR_RET("Start log", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case PS_COMMAND_STOP_LOG:
		{
			nRetVal = m_pSensor->StopFWLog();
			XN_IS_STATUS_OK_LOG_ERROR_RET("Stop log", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case LINK_COMMAND_GET_FW_STREAM_LIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetFwStreamList)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetLogMaskList));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetFwStreamList* pArgs = reinterpret_cast<XnCommandGetFwStreamList*>(data);
			if (pArgs->streams == NULL)
			{
				m_driverServices.errorLoggerAppend("Streams array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xnl::Array<XnFwStreamInfo> streams;
			nRetVal = m_pSensor->EnumerateStreams(streams);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Get log mask list", nRetVal, ONI_STATUS_ERROR);

			if (pArgs->count < streams.GetSize())
			{
				m_driverServices.errorLoggerAppend("Insufficient memory for stream list. available: %d, required: %d\n", pArgs->count, streams.GetSize());
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			for (int i = 0; i < (int)streams.GetSize(); ++i)
			{
				pArgs->streams[i] = streams[i];
			}
			pArgs->count = streams.GetSize();
		}
		break;

	case LINK_COMMAND_CREATE_FW_STREAM:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandCreateStream)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandCreateStream));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandCreateStream* pArgs = reinterpret_cast<XnCommandCreateStream*>(data);
			XnUInt16 id;
			nRetVal = m_pSensor->CreateInputStream(pArgs->type, pArgs->creationInfo, id);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Create stream", nRetVal, ONI_STATUS_ERROR);
			pArgs->id = id;
		}
		break;

	case LINK_COMMAND_DESTROY_FW_STREAM:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandDestroyStream)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandDestroyStream));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandDestroyStream* pArgs = reinterpret_cast<XnCommandDestroyStream*>(data);
			nRetVal = m_pSensor->DestroyInputStream((XnUInt16)pArgs->id);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Destroy stream", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case LINK_COMMAND_START_FW_STREAM:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandStartStream)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandStartStream));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandStartStream* pArgs = reinterpret_cast<XnCommandStartStream*>(data);
			xn::LinkInputStream* pInputStream = m_pSensor->GetInputStream((XnUInt16)pArgs->id);
			if (pInputStream == NULL)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d wasn't created\n", pArgs->id);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			nRetVal = pInputStream->Start();
			XN_IS_STATUS_OK_LOG_ERROR_RET("Start stream", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case LINK_COMMAND_STOP_FW_STREAM:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandStopStream)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandStopStream));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandStopStream* pArgs = reinterpret_cast<XnCommandStopStream*>(data);
			xn::LinkInputStream* pInputStream = m_pSensor->GetInputStream((XnUInt16)pArgs->id);
			if (pInputStream == NULL)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d wasn't created\n", pArgs->id);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			nRetVal = pInputStream->Stop();
			XN_IS_STATUS_OK_LOG_ERROR_RET("Stop stream", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case LINK_COMMAND_GET_FW_STREAM_VIDEO_MODE_LIST:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetFwStreamVideoModeList)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetFwStreamVideoModeList));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetFwStreamVideoModeList* pArgs = reinterpret_cast<XnCommandGetFwStreamVideoModeList*>(data);
			if (pArgs->videoModes == NULL)
			{
				m_driverServices.errorLoggerAppend("Streams array must point to valid memory: \n");
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xn::LinkInputStream* pInputStream = m_pSensor->GetInputStream((XnUInt16)pArgs->streamId);
			if (pInputStream == NULL)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d wasn't created\n", pArgs->streamId);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			if (pInputStream->GetStreamFragLevel() != XN_LINK_STREAM_FRAG_LEVEL_FRAMES)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d is not a frame stream\n", pArgs->streamId);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xn::LinkFrameInputStream* pFrameInputStream = (xn::LinkFrameInputStream*)pInputStream;
			xnl::Array<XnFwStreamVideoMode> videoModes = pFrameInputStream->GetSupportedVideoModes();
			if (pArgs->count < videoModes.GetSize())
			{
				m_driverServices.errorLoggerAppend("Insufficient memory for stream list. available: %d, required: %d\n", pArgs->count, videoModes.GetSize());
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			for (int i = 0; i < (int)videoModes.GetSize(); ++i)
			{
				pArgs->videoModes[i] = videoModes[i];
			}
			pArgs->count = videoModes.GetSize();
		}
		break;

	case LINK_COMMAND_SET_FW_STREAM_VIDEO_MODE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandSetFwStreamVideoMode)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandSetFwStreamVideoMode));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandSetFwStreamVideoMode* pArgs = reinterpret_cast<XnCommandSetFwStreamVideoMode*>(data);

			xn::LinkInputStream* pInputStream = m_pSensor->GetInputStream((XnUInt16)pArgs->streamId);
			if (pInputStream == NULL)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d wasn't created\n", pArgs->streamId);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			if (pInputStream->GetStreamFragLevel() != XN_LINK_STREAM_FRAG_LEVEL_FRAMES)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d is not a frame stream\n", pArgs->streamId);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xn::LinkFrameInputStream* pFrameInputStream = (xn::LinkFrameInputStream*)pInputStream;
			nRetVal = pFrameInputStream->SetVideoMode(pArgs->videoMode);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Set video mode", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case LINK_COMMAND_GET_FW_STREAM_VIDEO_MODE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandGetFwStreamVideoMode)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandGetFwStreamVideoMode));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandGetFwStreamVideoMode* pArgs = reinterpret_cast<XnCommandGetFwStreamVideoMode*>(data);

			xn::LinkInputStream* pInputStream = m_pSensor->GetInputStream((XnUInt16)pArgs->streamId);
			if (pInputStream == NULL)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d wasn't created\n", pArgs->streamId);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			if (pInputStream->GetStreamFragLevel() != XN_LINK_STREAM_FRAG_LEVEL_FRAMES)
			{
				m_driverServices.errorLoggerAppend("Stream with ID %d is not a frame stream\n", pArgs->streamId);
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			xn::LinkFrameInputStream* pFrameInputStream = (xn::LinkFrameInputStream*)pInputStream;
			pArgs->videoMode = pFrameInputStream->GetVideoMode();
		}
		break;

	case LINK_COMMAND_SET_PROJECTOR_PULSE:
		{
			EXACT_PROP_SIZE_DO(dataSize, XnCommandSetProjectorPulse)
			{
				m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", dataSize, sizeof(XnCommandSetProjectorPulse));
				XN_ASSERT(FALSE);
				return ONI_STATUS_BAD_PARAMETER;
			}

			XnCommandSetProjectorPulse* pArgs = reinterpret_cast<XnCommandSetProjectorPulse*>(data);
			nRetVal = m_pSensor->EnableProjectorPulse((XnFloat)pArgs->delay, (XnFloat)pArgs->width, (XnFloat)pArgs->cycle);
			XN_IS_STATUS_OK_LOG_ERROR_RET("Enable projector pulse", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	case LINK_COMMAND_DISABLE_PROJECTOR_PULSE:
		{
			nRetVal = m_pSensor->DisableProjectorPulse();
			XN_IS_STATUS_OK_LOG_ERROR_RET("Disable projector pulse", nRetVal, ONI_STATUS_ERROR);
		}
		break;

	default:
		return DeviceBase::invoke(commandId, data, dataSize);
	}

	return ONI_STATUS_OK;
}

OniBool LinkOniDevice::isCommandSupported(int commandId)
{
	switch (commandId)
	{
	case PS_COMMAND_AHB_READ:
	case PS_COMMAND_AHB_WRITE:
	case PS_COMMAND_I2C_READ:
	case PS_COMMAND_I2C_WRITE:
	case PS_COMMAND_SOFT_RESET:
	case PS_COMMAND_POWER_RESET:
    case PS_COMMAND_READ_DEBUG_DATA:
	case PS_COMMAND_BEGIN_FIRMWARE_UPDATE:
	case PS_COMMAND_END_FIRMWARE_UPDATE:
	case PS_COMMAND_UPLOAD_FILE:
	case PS_COMMAND_DOWNLOAD_FILE:
	case PS_COMMAND_GET_FILE_LIST:
	case PS_COMMAND_FORMAT_ZONE:
	case PS_COMMAND_DUMP_ENDPOINT:
	case PS_COMMAND_GET_I2C_DEVICE_LIST:
	case PS_COMMAND_GET_BIST_LIST:
    case PS_COMMAND_GET_TEMP_LIST:
    case PS_COMMAND_READ_TEMPERATURE:
	case PS_COMMAND_EXECUTE_BIST:
	case PS_COMMAND_USB_TEST:
	case PS_COMMAND_GET_LOG_MASK_LIST:
	case PS_COMMAND_SET_LOG_MASK_STATE:
	case PS_COMMAND_START_LOG:
	case PS_COMMAND_STOP_LOG:
	case LINK_COMMAND_GET_FW_STREAM_LIST:
	case LINK_COMMAND_CREATE_FW_STREAM:
	case LINK_COMMAND_DESTROY_FW_STREAM:
	case LINK_COMMAND_START_FW_STREAM:
	case LINK_COMMAND_STOP_FW_STREAM:
	case LINK_COMMAND_GET_FW_STREAM_VIDEO_MODE_LIST:
	case LINK_COMMAND_SET_FW_STREAM_VIDEO_MODE:
	case LINK_COMMAND_GET_FW_STREAM_VIDEO_MODE:
		return true;
	default:
		return DeviceBase::isCommandSupported(commandId);
	}
}
