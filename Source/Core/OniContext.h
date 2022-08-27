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
#include "OniSyncedStreamsFrameHolder.h"
#include "OniDeviceDriver.h"
#include "OniRecorder.h"
#include "OniFrameManager.h"

#include "XnList.h"
#include "XnHash.h"
#include "XnEvent.h"
#include "OniDriverHandler.h"
#include "OniCommon.h"

struct _OniDevice
{
	oni::implementation::Device* pDevice;
};
struct _OniStream
{
	oni::implementation::VideoStream* pStream;
};
struct _OniFrameSync
{
	oni::implementation::SyncedStreamsFrameHolder* pSyncedStreamsFrameHolder;
	oni::implementation::DeviceDriver* pDeviceDriver;
	void* pFrameSyncHandle;
};
struct _OniRecorder
{
    oni::implementation::Recorder* pRecorder;
};

ONI_NAMESPACE_IMPLEMENTATION_BEGIN

class Context
{
public:
	Context();
	~Context();

	OniStatus initialize();
	void shutdown();

	OniStatus registerDeviceConnectedCallback(OniDeviceInfoCallback handler, void* pCookie, OniCallbackHandle& handle);
	void unregisterDeviceConnectedCallback(OniCallbackHandle handle);
	OniStatus registerDeviceDisconnectedCallback(OniDeviceInfoCallback handler, void* pCookie, OniCallbackHandle& handle);
	void unregisterDeviceDisconnectedCallback(OniCallbackHandle handle);
	OniStatus registerDeviceStateChangedCallback(OniDeviceStateCallback handler, void* pCookie, OniCallbackHandle& handle);
	void unregisterDeviceStateChangedCallback(OniCallbackHandle handle);

	OniStatus getDeviceList(OniDeviceInfo** pDevices, int* pDeviceCount);
	OniStatus releaseDeviceList(OniDeviceInfo* pDevices);

	OniStatus deviceOpen(const char* uri, const char* mode, OniDeviceHandle* pDevice);
	OniStatus deviceClose(OniDeviceHandle device);

	const OniSensorInfo* getSensorInfo(OniDeviceHandle device, OniSensorType sensorType);

	OniStatus createStream(OniDeviceHandle device, OniSensorType sensorType, OniStreamHandle* pStream);
	OniStatus streamDestroy(OniStreamHandle stream);

	const OniSensorInfo* getSensorInfo(OniStreamHandle stream);

	OniStatus readFrame(OniStreamHandle stream, OniFrame** pFrame);

	void frameRelease(OniFrame* pFrame);
	void frameAddRef(OniFrame* pFrame);

	OniStatus waitForStreams(OniStreamHandle* pStreams, int streamCount, int* pStreamIndex, int timeout);

	OniStatus enableFrameSync(OniStreamHandle* pStreams, int numStreams, OniFrameSyncHandle* pFrameSyncHandle);
	OniStatus enableFrameSyncEx(VideoStream** pStreams, int numStreams, DeviceDriver* pDriver, OniFrameSyncHandle* pFrameSyncHandle);
	void disableFrameSync(OniFrameSyncHandle frameSyncHandle);

	void clearErrorLogger();
	const char* getExtendedError();

	void addToLogger(const XnChar* cpFormat, ...);

    OniStatus recorderOpen(const char* fileName, OniRecorderHandle* pRecorder);
    OniStatus recorderClose(OniRecorderHandle* pRecorder);
    OniStatus recorderClose(Recorder* pRecorder);

	static OniBool s_valid;
protected:
	OniStatus streamDestroy(VideoStream* pStream);
	static void ONI_CALLBACK_TYPE deviceDriver_DeviceConnected(Device* pDevice, void* pCookie);
	static void ONI_CALLBACK_TYPE deviceDriver_DeviceDisconnected(Device* pDevice, void* pCookie);
	static void ONI_CALLBACK_TYPE deviceDriver_DeviceStateChanged(Device* pDevice, OniDeviceState deviceState, void* pCookie);

private:
	Context(const Context& other);
	Context& operator=(const Context&other);

	XnStatus loadLibraries(const char* directoryName);
	void onNewFrame();
	XN_EVENT_HANDLE getThreadEvent();
	void releaseThreadEvent();
	static void XN_CALLBACK_TYPE newFrameCallback(void* pCookie);

	FrameManager m_frameManager;

	xnl::ErrorLogger& m_errorLogger;

	xnl::Event1Arg<const OniDeviceInfo*> m_deviceConnectedEvent;
	xnl::Event1Arg<const OniDeviceInfo*> m_deviceDisconnectedEvent;
	xnl::Event2Args<const OniDeviceInfo*, OniDeviceState> m_deviceStateChangedEvent;

	xnl::List<oni::implementation::DeviceDriver*> m_deviceDrivers;
	xnl::List<oni::implementation::Device*> m_devices;
	xnl::List<oni::implementation::VideoStream*> m_streams;
    xnl::List<oni::implementation::Recorder*> m_recorders;

	xnl::Hash<XN_THREAD_ID, XN_EVENT_HANDLE> m_waitingThreads;
	xnl::Hash<XN_THREAD_ID, XN_EVENT_HANDLE> m_knownThreads;

	xnl::CriticalSection m_cs;

	char m_overrideDevice[XN_FILE_MAX_PATH];

	int m_initializationCounter;
};

ONI_NAMESPACE_IMPLEMENTATION_END
