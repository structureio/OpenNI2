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
#include "OniDevice.h"
#include "OniStream.h"
#include "OniContext.h"
#include "OniSensor.h"
#include <OniProperties.h>
#include <XnLog.h>

#define XN_MASK_ONI_DEVICE "OniDevice"

ONI_NAMESPACE_IMPLEMENTATION_BEGIN

Device::Device(DeviceDriver* pDeviceDriver, const DriverHandler& driverHandler, FrameManager& frameManager, const OniDeviceInfo* pDeviceInfo, xnl::ErrorLogger& errorLogger) : 
	m_driverHandler(driverHandler),
	m_frameManager(frameManager),
	m_errorLogger(errorLogger),
	m_active(false),
	m_openCount(0),
	m_deviceHandle(NULL),
	m_pDeviceDriver(pDeviceDriver),
	m_depthColorSyncHandle(NULL),
	m_pContext(NULL),
	m_syncEnabled(FALSE)
{
	m_pInfo = XN_NEW(OniDeviceInfo);
	xnOSMemCopy(m_pInfo, pDeviceInfo, sizeof(OniDeviceInfo));
	xnOSMemSet(&m_sensors, 0, sizeof(m_sensors));
}
Device::~Device()
{
	while (m_openCount > 0)
	{
		close();
	}

	XN_DELETE(m_pInfo);
	m_pInfo = NULL;
}

OniStatus Device::open(const char* mode)
{
	if (m_openCount == 0)
	{
		m_deviceHandle = m_driverHandler.deviceOpen(m_pInfo->uri, mode);
		if (m_deviceHandle == NULL)
		{
			return ONI_STATUS_ERROR;
		}
	}
	m_openCount++;

	return ONI_STATUS_OK;
}

OniStatus Device::close()
{
	--m_openCount;

	if (m_openCount == 0)
	{
		for (int i = 0; i < MAX_SENSORS_PER_DEVICE; ++i)
		{
			if (m_sensors[i] != NULL)
			{
				XN_DELETE(m_sensors[i]);
				m_sensors[i] = NULL;
			}
		}

		if (m_deviceHandle != NULL)
		{
			m_driverHandler.deviceClose(m_deviceHandle);
		}
		m_deviceHandle = NULL;
	}

	return ONI_STATUS_OK;
}

const OniDeviceInfo* Device::getInfo() const {return m_pInfo;}

OniStatus Device::getSensorInfoList(OniSensorInfo** pSensorInfos, int& numSensors)
{
	return m_driverHandler.deviceGetSensorInfoList(m_deviceHandle, pSensorInfos, &numSensors);
}

VideoStream* Device::createStream(OniSensorType sensorType)
{
	OniSensorInfo* pSensorInfos;
	OniSensorInfo* pSensorInfo = NULL;

	int numSensors = 0;
	getSensorInfoList(&pSensorInfos, numSensors);
	for (int i = 0; i < numSensors; ++i)
	{
		if (pSensorInfos[i].sensorType == sensorType)
		{
			pSensorInfo = &pSensorInfos[i];
			break;
		}
	}

	if (pSensorInfo == NULL)
	{
		m_errorLogger.Append("Device: Can't find this source %d", sensorType);
		return NULL;
	}

	// make sure our sensor array is big enough
	if ((int)sensorType >= MAX_SENSORS_PER_DEVICE)
	{
		xnLogError(XN_MASK_ONI_DEVICE, "Internal error!");
		m_errorLogger.Append("Device: Can't find this source %d", sensorType);
		XN_ASSERT(FALSE);
		return NULL;
	}

	xnl::AutoCSLocker lock(m_cs);
	if (m_sensors[sensorType] == NULL)
	{
		m_sensors[sensorType] = XN_NEW(Sensor, m_errorLogger, m_frameManager, m_driverHandler);
		if (m_sensors[sensorType] == NULL)
		{
			XN_ASSERT(FALSE);
			return NULL;
		}
	}

	{
		// check if stream already exists. Do this in a lock to make it thread-safe
		xnl::AutoCSLocker sensor_lock(m_sensors[sensorType]->m_refCountCS);
		if (m_sensors[sensorType]->m_streamCount == 0)
		{
			void* streamHandle = m_driverHandler.deviceCreateStream(m_deviceHandle, sensorType);
			if (streamHandle == NULL)
			{
				m_errorLogger.Append("Stream: couldn't create using source %d", sensorType);
				return NULL;
			}

			m_sensors[sensorType]->setDriverStream(streamHandle);
		}

		++m_sensors[sensorType]->m_streamCount;
	}

	VideoStream* pStream = XN_NEW(VideoStream, m_sensors[sensorType], pSensorInfo, *this, m_driverHandler, m_frameManager, m_errorLogger);
	m_streams.AddLast(pStream);

	if ((sensorType == ONI_SENSOR_DEPTH || sensorType == ONI_SENSOR_COLOR) &&
		m_depthColorSyncHandle != NULL && m_pContext != NULL)
	{
		refreshDepthColorSyncState();
	}
	return pStream;
}

OniStatus oni::implementation::Device::setProperty(int propertyId, const void* data, int dataSize)
{
	OniStatus rc = m_driverHandler.deviceSetProperty(m_deviceHandle, propertyId, data, dataSize);
	if (rc != ONI_STATUS_OK)
	{
		m_errorLogger.Append("Device.setProperty(%x) failed\n", propertyId);
	}
	return rc;
}
OniStatus oni::implementation::Device::getProperty(int propertyId, void* data, int* pDataSize)
{
	OniStatus rc = m_driverHandler.deviceGetProperty(m_deviceHandle, propertyId, data, pDataSize);
	if (rc != ONI_STATUS_OK)
	{
		m_errorLogger.Append("Device.getProperty(%x) failed\n", propertyId);
	}
	return rc;
}
OniBool oni::implementation::Device::isPropertySupported(int propertyId)
{
	return m_driverHandler.deviceIsPropertySupported(m_deviceHandle, propertyId);
}
void Device::notifyAllProperties()
{
	m_driverHandler.deviceNotifyAllProperties(m_deviceHandle);
}
OniStatus Device::invoke(int commandId, void* data, int dataSize)
{
	if (commandId == ONI_DEVICE_COMMAND_SEEK)
	{
		if (dataSize != sizeof(OniSeek))
		{
			return ONI_STATUS_BAD_PARAMETER;
		}

		// Change seek's stream handle.
		Device::Seek seek;
		OniSeek* pSeek = (OniSeek*)data;
		seek.frameId = pSeek->frameIndex;
		seek.pStream = ((_OniStream*)pSeek->stream)->pStream->getHandle();

		// Update data to point to new structure.
		data = &seek;
		dataSize = sizeof(seek);
	}

	return m_driverHandler.deviceInvoke(m_deviceHandle, commandId, data, dataSize);
}
OniBool Device::isCommandSupported(int commandId)
{
	return m_driverHandler.deviceIsCommandSupported(m_deviceHandle, commandId);
}
OniStatus Device::tryManualTrigger()
{
	return m_driverHandler.tryManualTrigger(m_deviceHandle);
}

void Device::refreshDepthColorSyncState()
{
	if (!m_syncEnabled)
		return;
	Context* pTmpContext = m_pContext;
	disableDepthColorSync();
	enableDepthColorSync(pTmpContext);
}

void Device::clearStream(VideoStream* pStream)
{
	xnl::AutoCSLocker lock(m_cs);
	m_streams.Remove(pStream);

	if ((pStream->getSensorInfo()->sensorType == ONI_SENSOR_DEPTH ||
		pStream->getSensorInfo()->sensorType == ONI_SENSOR_COLOR) &&
		m_depthColorSyncHandle != NULL && m_pContext != NULL)
	{
		refreshDepthColorSyncState();
	}
}

OniStatus Device::enableDepthColorSync(Context* pContext)
{
	m_pContext = pContext;
	m_syncEnabled = TRUE;
	xnl::Array<VideoStream*> streamArray(m_streams.Size());
	streamArray.SetSize(m_streams.Size());

	int streamsUsed = 0;
	for (xnl::List<VideoStream*>::Iterator iter = m_streams.Begin(); iter != m_streams.End(); ++iter)
	{
		if (((*iter)->getSensorInfo()->sensorType == ONI_SENSOR_DEPTH || (*iter)->getSensorInfo()->sensorType == ONI_SENSOR_COLOR) &&
			(*iter)->isStarted())
		{
			streamArray[streamsUsed++] = *iter;
		}
	}

	if (streamsUsed == 0)
	{
		return ONI_STATUS_OK;
	}
	return m_pContext->enableFrameSyncEx(streamArray.GetData(), streamsUsed, m_pDeviceDriver, &m_depthColorSyncHandle);
}
void Device::disableDepthColorSync()
{
	if (m_pContext == NULL || m_depthColorSyncHandle == NULL || !m_syncEnabled)
		return;
	m_pContext->disableFrameSync(m_depthColorSyncHandle);
	m_depthColorSyncHandle = NULL;
	m_pContext = NULL;
	m_syncEnabled = FALSE;
}

OniBool Device::isDepthColorSyncEnabled()
{
	return m_syncEnabled;
}

void ONI_CALLBACK_TYPE Device::stream_PropertyChanged(void* /*deviceHandle*/, int /*propertyId*/, const void* /*data*/, int /*dataSize*/, void* pCookie)
{
	Device* pStream = (Device*)pCookie;
	XN_REFERENCE_VARIABLE(pStream);
}

OniBool Device::isImageRegistrationModeSupported(OniImageRegistrationMode mode)
{
	return m_driverHandler.deviceIsImageRegistrationModeSupported(m_deviceHandle, mode);
}

ONI_NAMESPACE_IMPLEMENTATION_END
