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
#include "OniContext.h"
#include "OniVersion.h"
#include "OniProperties.h"
#include "OniInternal.h"
#include <XnLog.h>

oni::implementation::Context g_Context;

ONI_C_API OniStatus oniInitialize(int /*apiVersion*/)
{
	g_Context.clearErrorLogger();
	OniStatus rc = g_Context.initialize();
	return rc;
}

ONI_C_API void oniShutdown()
{
	g_Context.clearErrorLogger();
	g_Context.shutdown();
}

ONI_C_API OniStatus oniGetDeviceList(OniDeviceInfo** pDevices, int* pDeviceCount)
{
	g_Context.clearErrorLogger();
	return g_Context.getDeviceList(pDevices, pDeviceCount);
}
ONI_C_API OniStatus oniReleaseDeviceList(OniDeviceInfo* pDevices)
{
	g_Context.clearErrorLogger();
	return g_Context.releaseDeviceList(pDevices);
}

struct DeviceHandles
{
	OniCallbackHandle deviceConnectedHandle;
	OniCallbackHandle deviceDisconnectedHandle;
	OniCallbackHandle deviceStateChangedHandle;
	void* pCookie;
};
ONI_C_API OniStatus oniRegisterDeviceCallbacks(OniDeviceCallbacks* pCallbacks, void* pCookie, OniCallbackHandle* pHandle)
{
	g_Context.clearErrorLogger();
	DeviceHandles* pDeviceHandles = XN_NEW(DeviceHandles);
	XN_VALIDATE_PTR(pDeviceHandles, ONI_STATUS_ERROR);

	pDeviceHandles->deviceConnectedHandle = NULL;
	pDeviceHandles->deviceDisconnectedHandle = NULL;
	pDeviceHandles->deviceStateChangedHandle = NULL;
	pDeviceHandles->pCookie = pCookie;

	g_Context.registerDeviceConnectedCallback(pCallbacks->deviceConnected, pCookie, pDeviceHandles->deviceConnectedHandle);
	g_Context.registerDeviceDisconnectedCallback(pCallbacks->deviceDisconnected, pCookie, pDeviceHandles->deviceDisconnectedHandle);
	g_Context.registerDeviceStateChangedCallback(pCallbacks->deviceStateChanged, pCookie, pDeviceHandles->deviceStateChangedHandle);
	*pHandle = (OniCallbackHandle)pDeviceHandles;

	return ONI_STATUS_OK;
}
ONI_C_API void oniUnregisterDeviceCallbacks(OniCallbackHandle handle)
{
	g_Context.clearErrorLogger();
	DeviceHandles* pDevicesHandles = (DeviceHandles*)handle;
	if (pDevicesHandles == NULL)
	{
		return;
	}

	g_Context.unregisterDeviceConnectedCallback(pDevicesHandles->deviceConnectedHandle);
	g_Context.unregisterDeviceDisconnectedCallback(pDevicesHandles->deviceDisconnectedHandle);
	g_Context.unregisterDeviceStateChangedCallback(pDevicesHandles->deviceStateChangedHandle);

	XN_DELETE(pDevicesHandles);
}

ONI_C_API OniStatus oniWaitForAnyStream(OniStreamHandle* pStreams, int streamCount, int* pStreamIndex, int timeout)
{
	g_Context.clearErrorLogger();
	return g_Context.waitForStreams(pStreams, streamCount, pStreamIndex, timeout);
}

ONI_C_API const char* oniGetExtendedError()
{
	return g_Context.getExtendedError();
}

ONI_C_API OniVersion oniGetVersion()
{
	g_Context.clearErrorLogger();
	OniVersion version;

	version.major = ONI_VERSION_MAJOR;
	version.minor = ONI_VERSION_MINOR;
	version.maintenance = ONI_VERSION_MAINTENANCE;
	version.build = ONI_VERSION_BUILD;

	return version;
}

ONI_C_API int oniFormatBytesPerPixel(OniPixelFormat format)
{
	g_Context.clearErrorLogger();
	switch (format)
	{
	case ONI_PIXEL_FORMAT_GRAY8:
		return 1;
	case ONI_PIXEL_FORMAT_DEPTH_1_MM:
	case ONI_PIXEL_FORMAT_DEPTH_100_UM:
	case ONI_PIXEL_FORMAT_SHIFT_9_2:
	case ONI_PIXEL_FORMAT_SHIFT_9_3:
	case ONI_PIXEL_FORMAT_GRAY16:
		return 2;
	case ONI_PIXEL_FORMAT_RGB888:
		return 3;
	case ONI_PIXEL_FORMAT_YUV422:
	case ONI_PIXEL_FORMAT_YUYV:
		return 2;
	case ONI_PIXEL_FORMAT_JPEG:
		return 1;
	default:
		XN_ASSERT(FALSE);
		return 0;
	}
}

/////////

ONI_C_API OniStatus oniDeviceOpen(const char* uri, OniDeviceHandle* pDevice)
{
	return oniDeviceOpenEx(uri, NULL, pDevice);
}
ONI_C_API OniStatus oniDeviceOpenEx(const char* uri, const char* mode, OniDeviceHandle* pDevice)
{
	g_Context.clearErrorLogger();
	return g_Context.deviceOpen(uri, mode, pDevice);
}
ONI_C_API OniStatus oniDeviceClose(OniDeviceHandle device)
{
	g_Context.clearErrorLogger();
	if (!oni::implementation::Context::s_valid)
	{
		return ONI_STATUS_ERROR;
	}
	return g_Context.deviceClose(device);
}

ONI_C_API OniStatus oniDeviceGetInfo(OniDeviceHandle device, OniDeviceInfo* pInfo)
{
	g_Context.clearErrorLogger();
	const OniDeviceInfo* pDeviceInfo = device->pDevice->getInfo();
	xnOSMemCopy(pInfo, pDeviceInfo, sizeof(OniDeviceInfo));

	return ONI_STATUS_OK;
}

ONI_C_API const OniSensorInfo* oniDeviceGetSensorInfo(OniDeviceHandle device, OniSensorType sensorType)
{
	g_Context.clearErrorLogger();
	return g_Context.getSensorInfo(device, sensorType);
}

ONI_C_API OniStatus oniDeviceCreateStream(OniDeviceHandle device, OniSensorType sensorType, OniStreamHandle* pStream)
{
	g_Context.clearErrorLogger();
	return g_Context.createStream(device, sensorType, pStream);
}

ONI_C_API OniStatus oniDeviceEnableDepthColorSync(OniDeviceHandle device)
{
	g_Context.clearErrorLogger();
	return device->pDevice->enableDepthColorSync(&g_Context);
}
ONI_C_API void oniDeviceDisableDepthColorSync(OniDeviceHandle device)
{
	g_Context.clearErrorLogger();
	device->pDevice->disableDepthColorSync();
}
ONI_C_API OniBool oniDeviceGetDepthColorSyncEnabled(OniDeviceHandle device)
{
	g_Context.clearErrorLogger();
	return device->pDevice->isDepthColorSyncEnabled();
}

ONI_C_API OniStatus oniDeviceSetProperty(OniDeviceHandle device, int propertyId, const void* data, int dataSize)
{
	g_Context.clearErrorLogger();
	return device->pDevice->setProperty(propertyId, data, dataSize);
}
ONI_C_API OniStatus oniDeviceGetProperty(OniDeviceHandle device, int propertyId, void* data, int* pDataSize)
{
	g_Context.clearErrorLogger();
	return device->pDevice->getProperty(propertyId, data, pDataSize);
}
ONI_C_API OniBool oniDeviceIsPropertySupported(OniDeviceHandle device, int propertyId)
{
	g_Context.clearErrorLogger();
	return device->pDevice->isPropertySupported(propertyId);
}
ONI_C_API OniStatus oniDeviceInvoke(OniDeviceHandle device, int commandId, void* data, int dataSize)
{
	g_Context.clearErrorLogger();
	return device->pDevice->invoke(commandId, data, dataSize);
}
ONI_C_API OniBool oniDeviceIsCommandSupported(OniDeviceHandle device, int commandId)
{
	g_Context.clearErrorLogger();
	return device->pDevice->isCommandSupported(commandId);
}
ONI_C_API OniBool oniDeviceIsImageRegistrationModeSupported(OniDeviceHandle device, OniImageRegistrationMode mode)
{
	g_Context.clearErrorLogger();
	return device->pDevice->isImageRegistrationModeSupported(mode);
}

/////////

ONI_C_API void oniStreamDestroy(OniStreamHandle stream)
{
	g_Context.clearErrorLogger();
	if (!oni::implementation::Context::s_valid)
	{
		return;
	}
	g_Context.streamDestroy(stream);
}

ONI_C_API const OniSensorInfo* oniStreamGetSensorInfo(OniStreamHandle stream)
{
	g_Context.clearErrorLogger();
	return g_Context.getSensorInfo(stream);
}

ONI_C_API OniStatus oniStreamStart(OniStreamHandle stream)
{
	g_Context.clearErrorLogger();
	return stream->pStream->start();
}
ONI_C_API void oniStreamStop(OniStreamHandle stream)
{
	g_Context.clearErrorLogger();
	if (stream == NULL || !oni::implementation::Context::s_valid)
	{
		return;
	}
	stream->pStream->stop();
}

ONI_C_API OniStatus oniStreamPeekFrame(OniStreamHandle stream, OniProcessFrameCallback handler, void* pCookie)
{
    return g_Context.peekFrame(stream, handler, pCookie);
}

ONI_C_API OniStatus oniStreamReadFrame(OniStreamHandle stream, OniFrame** pFrame)
{
	g_Context.clearErrorLogger();
	return g_Context.readFrame(stream, pFrame);
}

struct OniNewFrameCookie
{
	OniStreamHandle streamHandle;
	OniNewFrameCallback handler;
	void* pCookie;
	XnCallbackHandle handle;
};

void ONI_CALLBACK_TYPE OniNewFrameTranslationHandler(void* pCookie)
{
	OniNewFrameCookie* pNewFrameCookie = (OniNewFrameCookie*)pCookie;

	(*pNewFrameCookie->handler)(pNewFrameCookie->streamHandle, pNewFrameCookie->pCookie);
}

ONI_C_API OniStatus oniStreamRegisterNewFrameCallback(OniStreamHandle stream, OniNewFrameCallback handler, void* pCookie, OniCallbackHandle* pHandle)
{
	g_Context.clearErrorLogger();

	if (*pHandle != NULL)
	{
		// Already registered to something
		g_Context.addToLogger("Can't register same listener instance to multiple events");
		return ONI_STATUS_ERROR;
	}

	OniNewFrameCookie* pNewFrameCookie = XN_NEW(OniNewFrameCookie);
	XN_VALIDATE_PTR(pNewFrameCookie, ONI_STATUS_ERROR);

	pNewFrameCookie->streamHandle = stream;
	pNewFrameCookie->handler = handler;
	pNewFrameCookie->pCookie = pCookie;

	*pHandle = (OniCallbackHandle)pNewFrameCookie;
	return stream->pStream->registerNewFrameCallback(OniNewFrameTranslationHandler, pNewFrameCookie, &(pNewFrameCookie->handle));
}
ONI_C_API void oniStreamUnregisterNewFrameCallback(OniStreamHandle stream, OniCallbackHandle handle)
{
	g_Context.clearErrorLogger();
	OniNewFrameCookie* pNewFrameCookie = (OniNewFrameCookie*)handle;

	if (pNewFrameCookie == NULL)
	{
		return;
	}

	if (oni::implementation::Context::s_valid)
	{
		stream->pStream->unregisterNewFrameCallback(pNewFrameCookie->handle);
	}
	XN_DELETE(pNewFrameCookie);
}

ONI_C_API OniStatus oniStreamSetProperty(OniStreamHandle stream, int propertyId, const void* data, int dataSize)
{
	g_Context.clearErrorLogger();
	return stream->pStream->setProperty(propertyId, data, dataSize);
}

ONI_C_API OniStatus oniStreamGetProperty(OniStreamHandle stream, int propertyId, void* data, int* pDataSize)
{
	g_Context.clearErrorLogger();
	return stream->pStream->getProperty(propertyId, data, pDataSize);
}
ONI_C_API OniBool oniStreamIsPropertySupported(OniStreamHandle stream, int propertyId)
{
	g_Context.clearErrorLogger();
	return stream->pStream->isPropertySupported(propertyId);
}

ONI_C_API OniStatus oniStreamInvoke(OniStreamHandle stream, int commandId, void* data, int dataSize)
{
	g_Context.clearErrorLogger();
	return stream->pStream->invoke(commandId, data, dataSize);
}
ONI_C_API OniBool oniStreamIsCommandSupported(OniStreamHandle stream, int commandId)
{
	g_Context.clearErrorLogger();
	return stream->pStream->isCommandSupported(commandId);
}

ONI_C_API OniStatus oniStreamSetFrameBuffersAllocator(OniStreamHandle stream, OniFrameAllocBufferCallback alloc, OniFrameFreeBufferCallback free, void* pCookie)
{
	g_Context.clearErrorLogger();
	return stream->pStream->setFrameBufferAllocator(alloc, free, pCookie);	
}

////
ONI_C_API void oniFrameRelease(OniFrame* pFrame)
{
	g_Context.clearErrorLogger();
	if (!oni::implementation::Context::s_valid)
	{
		return;
	}
	g_Context.frameRelease(pFrame);
}
ONI_C_API void oniFrameAddRef(OniFrame* pFrame)
{
	g_Context.clearErrorLogger();
	g_Context.frameAddRef(pFrame);
}

//////////////////////////////////////////////////////////////////////////
// Recorder
//////////////////////////////////////////////////////////////////////////

ONI_C_API OniStatus oniCreateRecorder(
        const char* fileName, 
        OniRecorderHandle* pRecorder)
{
	g_Context.clearErrorLogger();
    return g_Context.recorderOpen(fileName, pRecorder);
}

ONI_C_API OniStatus oniRecorderAttachStream(
        OniRecorderHandle   recorder, 
        OniStreamHandle     stream, 
        OniBool                allowLossyCompression)
{
	g_Context.clearErrorLogger();

	// Validate parameters.
    if (NULL == recorder || NULL == recorder->pRecorder ||
        NULL == stream   || NULL == stream->pStream)
    {
        return ONI_STATUS_BAD_PARAMETER;
    }
    // Attach a stream to the recorder.
    return recorder->pRecorder->attachStream(
            *stream->pStream, allowLossyCompression);
}

ONI_C_API OniStatus oniRecorderStart(OniRecorderHandle recorder)
{
	g_Context.clearErrorLogger();
    // Validate parameters.
    if (NULL == recorder || NULL == recorder->pRecorder)
    {
        return ONI_STATUS_BAD_PARAMETER;
    }
    return recorder->pRecorder->start();
}

ONI_C_API void oniRecorderStop(OniRecorderHandle recorder)
{
	g_Context.clearErrorLogger();
    // Validate parameters.
    if (NULL == recorder || NULL == recorder->pRecorder)
    {
        return;
    }
    recorder->pRecorder->stop();
}

ONI_C_API OniStatus oniRecorderDestroy(OniRecorderHandle* pRecorder)
{
	g_Context.clearErrorLogger();
    return g_Context.recorderClose(pRecorder);
}

ONI_C_API void oniWriteLogEntry(const char* mask, int severity, const char* message)
{
	xnLogWrite(mask, (XnLogSeverity)severity, "External", 0, message);
}

ONI_C_API OniStatus oniSetLogOutputFolder(const char* strOutputFolder)
{
	XnStatus rc = xnLogSetOutputFolder((XnChar*)strOutputFolder);

	if (rc != XN_STATUS_OK)
		return ONI_STATUS_ERROR;
	
	return ONI_STATUS_OK;
}

ONI_C_API OniStatus oniGetLogFileName(char* strFileName, int nBufferSize)
{
	XnStatus rc = xnLogGetFileName((XnChar*)strFileName, (XnUInt32)nBufferSize);

	if (rc != XN_STATUS_OK)
		return ONI_STATUS_ERROR;

	return ONI_STATUS_OK;
}

ONI_C_API OniStatus oniSetLogMinSeverity(int nMinSeverity)
{
	XnStatus rc = xnLogSetMaskMinSeverity(XN_LOG_MASK_ALL, (XnLogSeverity)nMinSeverity);

	if (rc != XN_STATUS_OK)
		return ONI_STATUS_ERROR;

	return ONI_STATUS_OK;
}

ONI_C_API OniStatus oniSetLogConsoleOutput(OniBool bConsoleOutput)
{
	XnStatus rc = xnLogSetConsoleOutput(bConsoleOutput);

	if (rc != XN_STATUS_OK)
		return ONI_STATUS_ERROR;

	return ONI_STATUS_OK;
}

ONI_C_API OniStatus oniSetLogFileOutput(OniBool bFileOutput)
{
	XnStatus rc = xnLogSetFileOutput(bFileOutput);

	if (rc != XN_STATUS_OK)
		return ONI_STATUS_ERROR;

	return ONI_STATUS_OK;
}

#if ONI_PLATFORM == ONI_PLATFORM_ANDROID_ARM
ONI_C_API OniStatus oniSetLogAndroidOutput(OniBool bAndroidOutput)
{
	XnStatus rc = xnLogSetAndroidOutput((XnBool)bAndroidOutput);

	if (rc != XN_STATUS_OK)
		return ONI_STATUS_ERROR;

	return ONI_STATUS_OK;
}
#endif
ONI_C_API OniStatus oniCoordinateConverterDepthToWorld(OniStreamHandle depthStream, float depthX, float depthY, float depthZ, float* pWorldX, float* pWorldY, float* pWorldZ)
{
	g_Context.clearErrorLogger();
	return depthStream->pStream->convertDepthToWorldCoordinates(depthX, depthY, depthZ, pWorldX, pWorldY, pWorldZ);
}

ONI_C_API OniStatus oniCoordinateConverterWorldToDepth(OniStreamHandle depthStream, float worldX, float worldY, float worldZ, float* pDepthX, float* pDepthY, float* pDepthZ)
{
	g_Context.clearErrorLogger();
	return depthStream->pStream->convertWorldToDepthCoordinates(worldX, worldY, worldZ, pDepthX, pDepthY, pDepthZ);
}

ONI_C_API OniStatus oniCoordinateConverterDepthToColor(OniStreamHandle depthStream, OniStreamHandle colorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY)
{
	g_Context.clearErrorLogger();
	return depthStream->pStream->convertDepthToColorCoordinates(colorStream->pStream, depthX, depthY, depthZ, pColorX, pColorY);
}

XN_API_EXPORT_INIT()