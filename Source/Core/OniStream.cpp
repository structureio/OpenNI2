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
#include "OniStream.h"
#include "OniDevice.h"
#include "OniProperties.h"
#include "Driver/OniDriverTypes.h"
#include "OniRecorder.h"
#include "XnLockGuard.h"

#include <math.h>

#define STREAM_DESTROY_THREAD_TIMEOUT			2000

ONI_NAMESPACE_IMPLEMENTATION_BEGIN

VideoStream::VideoStream(Sensor* pSensor, const OniSensorInfo* pSensorInfo, Device& device, const DriverHandler& libraryHandler, FrameManager& frameManager, xnl::ErrorLogger& errorLogger) :
	m_errorLogger(errorLogger),
	m_pSensorInfo(NULL),
	m_running(true),
	m_device(device),
	m_driverHandler(libraryHandler),
	m_frameManager(frameManager),
	m_pSensor(pSensor),
	m_hNewFrameEvent(NULL),
	m_started(FALSE)
{
	xnOSCreateEvent(&m_newFrameInternalEvent, false);
	xnOSCreateEvent(&m_newFrameInternalEventForFrameHolder, false);
	xnOSCreateThread(newFrameThread, this, &m_newFrameThread);

	m_pSensorInfo = XN_NEW(OniSensorInfo);
	m_pSensorInfo->sensorType = pSensorInfo->sensorType;
	m_pSensorInfo->numSupportedVideoModes = pSensorInfo->numSupportedVideoModes;
	m_pSensorInfo->pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, m_pSensorInfo->numSupportedVideoModes);
	xnOSMemCopy(m_pSensorInfo->pSupportedVideoModes, pSensorInfo->pSupportedVideoModes, sizeof(OniVideoMode)*m_pSensorInfo->numSupportedVideoModes);

	m_pSensor->newFrameEvent().Register(stream_NewFrame, this, m_hNewFrameEvent);

    m_driverHandler.streamSetPropertyChangedCallback(m_pSensor->streamHandle(), stream_PropertyChanged, this);

	refreshWorldConversionCache();
}

// Stream
VideoStream::~VideoStream()
{
	// Make sure stream is stopped.
	stop();

	if (m_hNewFrameEvent != NULL)
	{
		m_pSensor->newFrameEvent().Unregister(m_hNewFrameEvent);
		m_hNewFrameEvent = NULL;
	}

	m_device.clearStream(this);

    // Detach all recorders from this stream.
    xnl::LockGuard< Recorders > guard(m_recorders);
    while (m_recorders.Begin() != m_recorders.End())
    {
        // NOTE: DetachStream has a side effect of modifying m_recorders.
        m_recorders.Begin()->Value()->detachStream(*this);
    }

	// Try to close the thread properly, and forcibly terminate it if failed/timedout.
	m_running = false;
	xnOSSetEvent(m_newFrameInternalEvent);
	xnOSSetEvent(m_newFrameInternalEventForFrameHolder);
	XnStatus rc = xnOSWaitForThreadExit(m_newFrameThread, STREAM_DESTROY_THREAD_TIMEOUT);
	if (rc != XN_STATUS_OK)
	{
		xnOSTerminateThread(&m_newFrameThread);
	}
	else
	{
		xnOSCloseThread(&m_newFrameThread);
	}

	m_pFrameHolder->setStreamEnabled(this, FALSE);

	if (m_device.getHandle() != NULL)
	{
		xnl::AutoCSLocker lock(m_pSensor->m_refCountCS);
		XN_ASSERT(m_pSensor->m_streamCount >= 1);
		if (--m_pSensor->m_streamCount == 0)
		{
			m_driverHandler.deviceDestroyStream(m_device.getHandle(), m_pSensor->streamHandle());
		}
	}

	xnOSCloseEvent(&m_newFrameInternalEvent);
	xnOSCloseEvent(&m_newFrameInternalEventForFrameHolder);

	XN_DELETE_ARR(m_pSensorInfo->pSupportedVideoModes);
	XN_DELETE(m_pSensorInfo);
}

OniStatus VideoStream::start()
{
	if (!m_started)
	{
		m_pFrameHolder->clear();

		xnl::AutoCSLocker lock(m_pSensor->m_refCountCS);
		if (m_pSensor->m_startedStreamCount == 0)
		{
			int requiredFrameSize = getRequiredFrameSize();
			m_pSensor->setRequiredFrameSize(requiredFrameSize);

			OniStatus rc = m_driverHandler.streamStart(m_pSensor->streamHandle());
			if (rc != ONI_STATUS_OK)
			{
				return rc;
			}

			m_device.refreshDepthColorSyncState();
		}

		++m_pSensor->m_startedStreamCount;

		m_pFrameHolder->setStreamEnabled(this, m_started);
		m_started = TRUE;
	}

	return ONI_STATUS_OK;
}

void VideoStream::stop()
{
	if (!m_started)
	{
		return;
	}

	m_started = FALSE;

	m_device.refreshDepthColorSyncState();

	m_pFrameHolder->setStreamEnabled(this, m_started);

	{
		xnl::AutoCSLocker lock(m_pSensor->m_refCountCS);
		XN_ASSERT(m_pSensor->m_startedStreamCount != 0);
		if (--m_pSensor->m_startedStreamCount == 0)
		{
			m_driverHandler.streamStop(m_pSensor->streamHandle());
		}
	}

	m_pFrameHolder->clear();
}
	
OniBool VideoStream::isStarted() 
{ 
	return m_started; 
}

OniFrame* VideoStream::peekFrame()
{
	return m_pFrameHolder->peekFrame(this);
}

void VideoStream::lockFrame()
{
	m_pFrameHolder->lock();
}

void VideoStream::unlockFrame()
{
	m_pFrameHolder->unlock();
}

OniStatus VideoStream::setProperty(int propertyId, const void* data, int dataSize)
{
	xnl::AutoCSLocker lock(m_pSensor->m_refCountCS);
	// if this stream is open, and not just by me (multiple depth streams for example), don't allow any changes
	int myOpenRefCount = m_started ? 1 : 0;
	if (m_pSensor->m_startedStreamCount > myOpenRefCount)
	{
		m_errorLogger.Append("This stream is open by other components. Configuration cannot be changed.");
		return ONI_STATUS_OUT_OF_FLOW;
	}

	OniStatus rc = m_driverHandler.streamSetProperty(m_pSensor->streamHandle(), propertyId, data, dataSize);
	if (rc != ONI_STATUS_OK)
	{
		m_errorLogger.Append("Stream setProperty(%d) failed\n", propertyId);
		return rc;
	}

	if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
	{
		refreshWorldConversionCache();
	}

	return ONI_STATUS_OK;
}
OniStatus VideoStream::getProperty(int propertyId, void* data, int* pDataSize)
{
	OniStatus rc = m_driverHandler.streamGetProperty(m_pSensor->streamHandle(), propertyId, data, pDataSize);
	if (rc != ONI_STATUS_OK)
	{
		m_errorLogger.Append("Stream getProperty(%d) failed\n", propertyId);
	}
	return rc;
}
OniBool VideoStream::isPropertySupported(int propertyId)
{
	return m_driverHandler.streamIsPropertySupported(m_pSensor->streamHandle(), propertyId);
}
void VideoStream::notifyAllProperties()
{
	m_driverHandler.streamNotifyAllProperties(m_pSensor->streamHandle());
}

OniStatus VideoStream::invoke(int commandId, void* data, int dataSize)
{
	return m_driverHandler.streamInvoke(m_pSensor->streamHandle(), commandId, data, dataSize);
}
OniBool VideoStream::isCommandSupported(int commandId)
{
	return m_driverHandler.streamIsCommandSupported(m_pSensor->streamHandle(), commandId);
}

OniStatus VideoStream::readFrame(OniFrame** pFrame)
{
	return m_pFrameHolder->readFrame(this, pFrame);
}

OniStatus VideoStream::registerNewFrameCallback(OniGeneralCallback handler, void* pCookie, XnCallbackHandle* pHandle)
{
	return OniStatusFromXnStatus(m_newFrameEvent.Register(handler, pCookie, *pHandle));
}

void VideoStream::unregisterNewFrameCallback(XnCallbackHandle handle)
{
	m_newFrameEvent.Unregister(handle);
}

OniStatus VideoStream::setFrameBufferAllocator(OniFrameAllocBufferCallback alloc, OniFrameFreeBufferCallback free, void* pCookie)
{
	return m_pSensor->setFrameBufferAllocator(alloc, free, pCookie);
}

const OniSensorInfo* VideoStream::getSensorInfo() const
{
	return m_pSensorInfo;
}

void VideoStream::newFrameThreadMainloop()
{
	XnStatus rc = XN_STATUS_OK;
	// Wait on frame
	while (m_running)
	{
		rc = xnOSWaitEvent(m_newFrameInternalEvent, XN_WAIT_INFINITE);
		if ((rc == XN_STATUS_OK) && m_running)
		{
			m_newFrameEvent.Raise();
			// HACK: To avoid starvation of other threads.
			xnOSSleep(1);
		}
	}
}

OniStatus VideoStream::addRecorder(Recorder& aRecorder)
{
    xnl::LockGuard<Recorders> guard(m_recorders);
    m_recorders[&aRecorder] = &aRecorder;
    return ONI_STATUS_OK;
}

OniStatus VideoStream::removeRecorder(Recorder& aRecorder)
{
    xnl::LockGuard<Recorders> guard(m_recorders);
    m_recorders.Remove(&aRecorder);
    return ONI_STATUS_OK;
}

XN_THREAD_PROC VideoStream::newFrameThread(XN_THREAD_PARAM pThreadParam)
{
	oni::implementation::VideoStream* pStream = (oni::implementation::VideoStream*)pThreadParam;
	pStream->newFrameThreadMainloop();

	XN_THREAD_PROC_RETURN(XN_STATUS_OK);
}

void ONI_CALLBACK_TYPE VideoStream::stream_NewFrame(OniFrame* pFrame, void* pCookie)
{
    // Validate parameters.
    if (NULL == pCookie || NULL == pFrame)
    {
        return;
    }

    VideoStream* pStream = (VideoStream*)pCookie;
	// ignore frames if not started (this can happen if multiple streams exist for the same sensor).
	if (!pStream->m_started)
		return;

	// Record the frame.
	// NOTE: record operation must go before ProcessNewFrame, because
	// m_pFrameHolder might block. We're recording every single frame, no
	// matter what. Or else we might loose frames or have other odd side
	// effects.

    {   
		// NOTE: scoped for the guard.
        xnl::LockGuard<Recorders> guard(pStream->m_recorders);
        for (Recorders::Iterator 
                i = pStream->m_recorders.Begin(), 
                e = pStream->m_recorders.End();
            i != e; ++i)
        {
            i->Key()->record(*pStream, *pFrame);
        }
    }

    // Process the frame.
    pStream->m_pFrameHolder->processNewFrame(pStream, pFrame);
}

void VideoStream::raiseNewFrameEvent()
{
	xnOSSetEvent(m_newFrameInternalEvent);
	xnOSSetEvent(m_newFrameInternalEventForFrameHolder);
	m_newFrameCallback(m_newFrameCookie);
}

XnStatus VideoStream::waitForNewFrameEvent()
{
	return xnOSWaitEvent(m_newFrameInternalEventForFrameHolder, XN_WAIT_INFINITE);
}

Device& VideoStream::getDevice()
{
	return m_device;
}

void* VideoStream::getHandle() const 
{
	return m_pSensor->streamHandle();
}

void VideoStream::setFrameHolder(FrameHolder* pFrameHolder)
{
	m_pFrameHolder = pFrameHolder;
}

FrameHolder* VideoStream::getFrameHolder()
{
	return m_pFrameHolder;
}

void ONI_CALLBACK_TYPE VideoStream::stream_PropertyChanged(void* /*streamHandle*/, int propertyId, const void* data, int dataSize, void* pCookie)
{
	VideoStream* pStream = (VideoStream*)pCookie;
    if (NULL == pStream)
    {
        return;
    }
    xnl::LockGuard< VideoStream::Recorders > guard(pStream->m_recorders);
    for (VideoStream::Recorders::Iterator
            i = pStream->m_recorders.Begin(),
            e = pStream->m_recorders.End();
        i != e; ++i)
    {
        i->Value()->recordStreamProperty(*pStream, propertyId, data, dataSize);
    }
}

OniStatus VideoStream::convertDepthToWorldCoordinates(float depthX, float depthY, float depthZ, float* pWorldX, float* pWorldY, float* pWorldZ)
{
	if (m_pSensorInfo->sensorType != ONI_SENSOR_DEPTH)
	{
		m_errorLogger.Append("convertDepthToWorldCoordinates: Stream is not from DEPTH\n");
		return ONI_STATUS_NOT_SUPPORTED;
	}

	float normalizedX = depthX / m_worldConvertCache.resolutionX - .5f;
	float normalizedY = .5f - depthY / m_worldConvertCache.resolutionY;

	OniVideoMode videoMode;
	int size = sizeof(videoMode);
	getProperty(ONI_STREAM_PROPERTY_VIDEO_MODE, &videoMode, &size);

	float const convertToMillimeters = (videoMode.pixelFormat == ONI_PIXEL_FORMAT_DEPTH_100_UM) ? 10.f : 1.f;
	*pWorldX = (normalizedX * depthZ * m_worldConvertCache.xzFactor) / convertToMillimeters;
	*pWorldY = (normalizedY * depthZ * m_worldConvertCache.yzFactor) / convertToMillimeters;
	*pWorldZ = depthZ / convertToMillimeters;

	return ONI_STATUS_OK;
}

OniStatus VideoStream::convertWorldToDepthCoordinates(float worldX, float worldY, float worldZ, float* pDepthX, float* pDepthY, float* pDepthZ)
{
	if (m_pSensorInfo->sensorType != ONI_SENSOR_DEPTH)
	{
		m_errorLogger.Append("convertWorldToDepthCoordinates: Stream is not from DEPTH\n");
		return ONI_STATUS_NOT_SUPPORTED;
	}

	*pDepthX = m_worldConvertCache.coeffX * worldX / worldZ + m_worldConvertCache.halfResX;
	*pDepthY = m_worldConvertCache.halfResY - m_worldConvertCache.coeffY * worldY / worldZ;
	*pDepthZ = worldZ;
	return ONI_STATUS_OK;
}

void VideoStream::refreshWorldConversionCache()
{
	if (m_pSensorInfo->sensorType != ONI_SENSOR_DEPTH)
	{
		return;
	}

	OniVideoMode videoMode;
	int size = sizeof(videoMode);
	getProperty(ONI_STREAM_PROPERTY_VIDEO_MODE, &videoMode, &size);

	size = sizeof(float);
	float horizontalFov;
	float verticalFov;
	getProperty(ONI_STREAM_PROPERTY_HORIZONTAL_FOV, &horizontalFov, &size);
	getProperty(ONI_STREAM_PROPERTY_VERTICAL_FOV, &verticalFov, &size);

	m_worldConvertCache.xzFactor = (XnFloat)tan(horizontalFov / 2) * 2;
	m_worldConvertCache.yzFactor = (XnFloat)tan(verticalFov / 2) * 2;
	m_worldConvertCache.resolutionX = videoMode.resolutionX;
	m_worldConvertCache.resolutionY = videoMode.resolutionY;
	m_worldConvertCache.halfResX = m_worldConvertCache.resolutionX / 2;
	m_worldConvertCache.halfResY = m_worldConvertCache.resolutionY / 2;
	m_worldConvertCache.coeffX = m_worldConvertCache.resolutionX / m_worldConvertCache.xzFactor;
	m_worldConvertCache.coeffY = m_worldConvertCache.resolutionY / m_worldConvertCache.yzFactor;
}

OniStatus VideoStream::convertDepthToColorCoordinates(VideoStream* colorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY)
{
	if (m_pSensorInfo->sensorType != ONI_SENSOR_DEPTH || colorStream->m_pSensorInfo->sensorType != ONI_SENSOR_COLOR)
	{
		m_errorLogger.Append("convertDepthToColorCoordinates: Streams are from the wrong sensors (should be DEPTH and COLOR)\n");
		return ONI_STATUS_NOT_SUPPORTED;
	}

	if (&m_device != &colorStream->m_device)
	{
		m_errorLogger.Append("convertDepthToColorCoordinates: Streams are not from the same device\n");
		return ONI_STATUS_NOT_SUPPORTED;
	}
	return m_driverHandler.convertDepthPointToColor(m_pSensor->streamHandle(), colorStream->m_pSensor->streamHandle(), depthX, depthY, depthZ, pColorX, pColorY);
}

int VideoStream::getRequiredFrameSize()
{
	return m_driverHandler.streamGetRequiredFrameSize(m_pSensor->streamHandle());
}

ONI_NAMESPACE_IMPLEMENTATION_END
