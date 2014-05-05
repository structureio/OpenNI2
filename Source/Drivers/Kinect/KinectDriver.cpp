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
#include "KinectDriver.h"
#include "KinectDevice.h"
#include <Shlobj.h>
#include "NuiApi.h"
#include "XnLog.h"

using namespace oni::driver;
using namespace kinect_device;
static const char VENDOR_VAL[] = "Microsoft";
static const char NAME_VAL[] = "Kinect";
#define MICROSOFT_VENDOR_ID 0x045e
#define KINECT_FOR_WINDOWS_PRODUCT_ID 0x02bf

KinectDriver::KinectDriver(OniDriverServices* pDriverServices) : DriverBase(pDriverServices)
{
	NuiSetDeviceStatusCallback( &(KinectDriver::StatusProc), this);
}
	
KinectDriver::~KinectDriver()
{
	NuiSetDeviceStatusCallback(NULL, NULL);	
}

OniStatus KinectDriver::initialize(DeviceConnectedCallback connectedCallback, DeviceDisconnectedCallback disconnectedCallback, DeviceStateChangedCallback deviceStateChangedCallback, void* pCookie)
{
	HRESULT hr;
	int iSensorCount = 0;
	INuiSensor * pNuiSensor;
	DriverBase::initialize(connectedCallback, disconnectedCallback, deviceStateChangedCallback, pCookie);
	hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		return ONI_STATUS_OK;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		OniDeviceInfo* pInfo = XN_NEW(OniDeviceInfo);
		BSTR str = pNuiSensor->NuiDeviceConnectionId();
		size_t convertedChars = 0;
		const size_t newsize = ONI_MAX_STR;
		size_t origsize = wcslen(str) + 1;
		wcstombs_s(&convertedChars, pInfo->uri, origsize, str, _TRUNCATE);
		xnOSStrCopy(pInfo->vendor, VENDOR_VAL, ONI_MAX_STR);
		xnOSStrCopy(pInfo->name, NAME_VAL, ONI_MAX_STR);
		m_devices[pInfo] = NULL;
		deviceConnected(pInfo);
		deviceStateChanged(pInfo, hr);
		
		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}
	return ONI_STATUS_OK;
}

DeviceBase* KinectDriver::deviceOpen(const char* uri, const char* /*mode*/)
{
	for (xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*>::Iterator iter = m_devices.Begin(); iter != m_devices.End(); ++iter)
	{
		if (xnOSStrCmp(iter->Key()->uri, uri) == 0)
		{
			// Found
			if (iter->Value() != NULL)
			{
				// already using
				return iter->Value();
			}
			else
			{
				INuiSensor * pNuiSensor;
				HRESULT hr;
				size_t convertedChars = 0;
				wchar_t wcstring[ONI_MAX_STR];
				mbstowcs_s(&convertedChars, wcstring, ONI_MAX_STR, uri, _TRUNCATE);
				// Create the sensor so we can check status, if we can't create it, move on to the next
				hr = NuiCreateSensorById(wcstring, &pNuiSensor);

				if (FAILED(hr))
				{
					return NULL;
				}

				if (NULL != pNuiSensor)
				{
					// Initialize the Kinect and specify that we'll be using color
					hr = pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH); 
					KinectDevice* pDevice = XN_NEW(KinectDevice, pNuiSensor);
					if (pDevice == NULL)
					{
						return NULL;
					}
					iter->Value() = pDevice;
					return pDevice;
				}

				if (NULL == pNuiSensor || FAILED(hr))
				{
					return NULL;
				}
			}			
		}
	}
	return NULL;	
}

void kinect_device::KinectDriver::deviceClose(oni::driver::DeviceBase* pDevice)
{
	for (xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*>::Iterator iter = m_devices.Begin(); iter != m_devices.End(); ++iter)
	{
		if (iter->Value() == pDevice)
		{
			iter->Value() = NULL;
			XN_DELETE(pDevice);
			return;
		}
	}

	// not our device?!
	XN_ASSERT(FALSE);
}

void KinectDriver::shutdown()
{
}

OniStatus KinectDriver::tryDevice(const char* uri)
{
	return ONI_STATUS_OK;	
}

void* KinectDriver::enableFrameSync(StreamBase** pStreams, int streamCount)
{
	return NULL;
}

void KinectDriver::disableFrameSync(void* frameSyncGroup)
{

}

void KinectDriver::StatusUpdate(const OLECHAR* instanceName, bool isConnected)
{
	char str[ONI_MAX_STR];
	size_t convertedChars = 0;
	const size_t newsize = ONI_MAX_STR;
	size_t origsize = wcslen(instanceName) + 1;
	wcstombs_s(&convertedChars, str, origsize, instanceName, _TRUNCATE);
	for (xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*>::Iterator iter = m_devices.Begin(); iter != m_devices.End(); ++iter)
	{
		if (xnOSStrCmp(iter->Key()->uri, str) == 0)
		{
			if (isConnected)
			{
				INuiSensor * pNuiSensor;
				HRESULT hr = NuiCreateSensorById( instanceName, &pNuiSensor);
				if (FAILED(hr))
				{
					return;
				}
				// Get the status of the sensor, and if connected, then we can initialize it
				hr = pNuiSensor->NuiStatus();
				deviceStateChanged(iter->Key(), (int)hr);
			}
			else
			{
				deviceDisconnected(iter->Key());
				KinectDevice* pDevice = (KinectDevice*)iter->Value();
				OniDeviceInfo* pInfo = (OniDeviceInfo*)iter->Key();
				m_devices.Remove(iter);
				if (pDevice != NULL)
					XN_DELETE(pDevice);
				
				XN_DELETE(pInfo);
			}
			return;
		}
	}
	
	if (isConnected)
	{
		INuiSensor * pNuiSensor;
		HRESULT hr = NuiCreateSensorById( instanceName, &pNuiSensor);
		if (FAILED(hr))
		{
			return;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		OniDeviceInfo* pInfo = XN_NEW(OniDeviceInfo);
		int index = pNuiSensor->NuiInstanceIndex();
		strcpy((char*)pInfo->uri, str);
		xnOSStrCopy(pInfo->vendor, VENDOR_VAL, ONI_MAX_STR);
		xnOSStrCopy(pInfo->name, NAME_VAL, ONI_MAX_STR);
		pInfo->usbVendorId = MICROSOFT_VENDOR_ID;
		pInfo->usbProductId = KINECT_FOR_WINDOWS_PRODUCT_ID;
		m_devices[pInfo] = NULL;
		deviceConnected(pInfo);
		deviceStateChanged(pInfo, hr);
	}
}

void CALLBACK KinectDriver::StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName,  void* pUserData )
{      
	((KinectDriver*)pUserData)->StatusUpdate(instanceName,SUCCEEDED( hrStatus ));
}

ONI_EXPORT_DRIVER(kinect_device::KinectDriver)