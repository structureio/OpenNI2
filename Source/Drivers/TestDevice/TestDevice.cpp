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
#include "Driver/OniDriverAPI.h"
#include "XnLib.h"
#include "XnHash.h"
#include "XnEvent.h"

#define TEST_RESOLUTION_X 320
#define TEST_RESOLUTION_Y 240

class TestStream : public oni::driver::StreamBase
{
public:
	TestStream() : oni::driver::StreamBase()
	{
		m_osEvent.Create(TRUE);
		m_sendCount = 0;
	}

	~TestStream()
	{
		stop();
	}

	OniStatus start()
	{
		xnOSCreateThread(threadFunc, this, &m_threadHandle);

		return ONI_STATUS_OK;
	}

	void stop()
	{
		m_running = false;
	}

	virtual OniStatus SetVideoMode(OniVideoMode*) = 0;
	virtual OniStatus GetVideoMode(OniVideoMode* pVideoMode) = 0;

	OniStatus getProperty(int propertyId, void* data, int* pDataSize)
	{
		if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
		{
			if (*pDataSize != sizeof(OniVideoMode))
			{
				printf("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniVideoMode));
				return ONI_STATUS_ERROR;
			}
			return GetVideoMode((OniVideoMode*)data);
		}

		return ONI_STATUS_NOT_IMPLEMENTED;
	}

	OniStatus setProperty(int propertyId, const void* data, int dataSize)
	{
		if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
		{
			if (dataSize != sizeof(OniVideoMode))
			{
				printf("Unexpected size: %d != %d\n", dataSize, sizeof(OniVideoMode));
				return ONI_STATUS_ERROR;
			}
			return SetVideoMode((OniVideoMode*)data);
		}
		else if (propertyId == 666)
		{
			if (dataSize != sizeof(int))
			{
				printf("Unexpected size: %d != %d\n", dataSize, sizeof(int));
				return ONI_STATUS_ERROR;
			}

			// Increment the send count.
			m_cs.Lock();
			m_sendCount += *((int*)data);
			m_cs.Unlock();

			// Raise the OS event, to allow thread to start working.
			m_osEvent.Set();
		}

		return ONI_STATUS_NOT_IMPLEMENTED;
	}

	virtual int GetBytesPerPixel() = 0;

protected:

	// Thread
	static XN_THREAD_PROC threadFunc(XN_THREAD_PARAM pThreadParam)
	{
		TestStream* pStream = (TestStream*)pThreadParam;
		pStream->m_running = true;

		while (pStream->m_running)
		{
			pStream->m_osEvent.Wait(XN_WAIT_INFINITE);
			int count = 0;
			do 
			{
				// Get the current count.
				pStream->m_cs.Lock();
				count = pStream->m_sendCount;
				if (pStream->m_sendCount > 0)
				{
					pStream->m_sendCount--;
				}
				pStream->m_cs.Unlock();

				// Send the frame.
				if (count > 0)
				{
					OniFrame* pFrame = pStream->getServices().acquireFrame();
					pStream->BuildFrame(pFrame);
					pStream->raiseNewFrame(pFrame);
					pStream->getServices().releaseFrame(pFrame);
				}

			} while (count > 0);
		}

		XN_THREAD_PROC_RETURN(XN_STATUS_OK);
	}

	virtual int BuildFrame(OniFrame* pFrame) = 0;

	int singleRes(int x, int y) {return y*TEST_RESOLUTION_X+x;}

	bool m_running;
	int m_sendCount;

	XN_THREAD_HANDLE m_threadHandle;

	xnl::CriticalSection m_cs;
	xnl::OSEvent m_osEvent;
};

class TestDepthStream : public TestStream
{
public:

	TestDepthStream() : TestStream()
	{
		m_frameId = 1;
	}

	OniStatus SetVideoMode(OniVideoMode*) {return ONI_STATUS_NOT_IMPLEMENTED;}
	OniStatus GetVideoMode(OniVideoMode* pVideoMode)
	{
		pVideoMode->pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
		pVideoMode->fps = 30;
		pVideoMode->resolutionX = TEST_RESOLUTION_X;
		pVideoMode->resolutionY = TEST_RESOLUTION_Y;
		return ONI_STATUS_OK;
	}

	virtual int GetBytesPerPixel() { return sizeof(OniDepthPixel); }

private:

	virtual int BuildFrame(OniFrame* pFrame)
	{
		pFrame->frameIndex = m_frameId;

		pFrame->videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
		pFrame->videoMode.resolutionX = TEST_RESOLUTION_X;
		pFrame->videoMode.resolutionY = TEST_RESOLUTION_Y;
		pFrame->videoMode.fps = 30;

		pFrame->width = TEST_RESOLUTION_X;
		pFrame->height = TEST_RESOLUTION_Y;

		pFrame->cropOriginX = pFrame->cropOriginY = 0;
		pFrame->croppingEnabled = FALSE;

		pFrame->sensorType = ONI_SENSOR_DEPTH;
		pFrame->stride = TEST_RESOLUTION_X*sizeof(OniDepthPixel);
		pFrame->timestamp = m_frameId*33000;
		m_frameId++;
		return 1;
	}

	int m_frameId;
};

class TestImageStream : public TestStream
{
public:
	TestImageStream() : TestStream()
	{
		m_frameId = 1;
	}

	OniStatus SetVideoMode(OniVideoMode*) {return ONI_STATUS_NOT_IMPLEMENTED;}
	OniStatus GetVideoMode(OniVideoMode* pVideoMode)
	{
		pVideoMode->pixelFormat = ONI_PIXEL_FORMAT_RGB888;
		pVideoMode->fps = 30;
		pVideoMode->resolutionX = TEST_RESOLUTION_X;
		pVideoMode->resolutionY = TEST_RESOLUTION_Y;
		return ONI_STATUS_OK;
	}

	virtual int GetBytesPerPixel() { return sizeof(OniRGB888Pixel); }

private:

	virtual int BuildFrame(OniFrame* pFrame)
	{
		pFrame->frameIndex = m_frameId;

		pFrame->videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
		pFrame->videoMode.resolutionX = TEST_RESOLUTION_X;
		pFrame->videoMode.resolutionY = TEST_RESOLUTION_Y;
		pFrame->videoMode.fps = 30;

		pFrame->width = TEST_RESOLUTION_X;
		pFrame->height = TEST_RESOLUTION_Y;

		pFrame->cropOriginX = pFrame->cropOriginY = 0;
		pFrame->croppingEnabled = FALSE;

		pFrame->sensorType = ONI_SENSOR_COLOR;
		pFrame->stride = TEST_RESOLUTION_X*sizeof(OniRGB888Pixel);
		pFrame->timestamp = m_frameId*33000;
		m_frameId++;
		return 1;
	}

	int m_frameId;
};

class TestDevice : public oni::driver::DeviceBase
{
public:
	TestDevice(OniDeviceInfo* pInfo, oni::driver::DriverServices& driverServices) : m_pInfo(pInfo), m_driverServices(driverServices)
	{
		m_numSensors = 2;

		m_sensors[0].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
		m_sensors[0].sensorType = ONI_SENSOR_DEPTH;
		m_sensors[0].numSupportedVideoModes = 1;
		m_sensors[0].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
		m_sensors[0].pSupportedVideoModes[0].fps = 30;
		m_sensors[0].pSupportedVideoModes[0].resolutionX = TEST_RESOLUTION_X;
		m_sensors[0].pSupportedVideoModes[0].resolutionY = TEST_RESOLUTION_Y;

		m_sensors[1].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
		m_sensors[1].sensorType = ONI_SENSOR_COLOR;
		m_sensors[1].numSupportedVideoModes = 1;
		m_sensors[1].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_RGB888;
		m_sensors[1].pSupportedVideoModes[0].fps = 30;
		m_sensors[1].pSupportedVideoModes[0].resolutionX = TEST_RESOLUTION_X;
		m_sensors[1].pSupportedVideoModes[0].resolutionY = TEST_RESOLUTION_Y;

	}
	OniDeviceInfo* GetInfo()
	{
		return m_pInfo;
	}

	OniStatus getSensorInfoList(OniSensorInfo** pSensors, int* numSensors)
	{
		*numSensors = m_numSensors;
		*pSensors = m_sensors;

		return ONI_STATUS_OK;
	}

	oni::driver::StreamBase* createStream(OniSensorType sensorType)
	{
		if (sensorType == ONI_SENSOR_DEPTH)
		{
			TestDepthStream* pDepth = XN_NEW(TestDepthStream);
			return pDepth;
		}
		if (sensorType == ONI_SENSOR_COLOR)
		{
			TestImageStream* pImage = XN_NEW(TestImageStream);
			return pImage;
		}

		m_driverServices.errorLoggerAppend("TestDevice: Can't create a stream of type %d", sensorType);
		return NULL;
	}

	void destroyStream(oni::driver::StreamBase* pStream)
	{
		XN_DELETE(pStream);
	}

	OniStatus  getProperty(int propertyId, void* data, int* pDataSize)
	{
		OniStatus rc = ONI_STATUS_OK;

		switch (propertyId)
		{
		case ONI_DEVICE_PROPERTY_DRIVER_VERSION:
			{
				if (*pDataSize == sizeof(OniVersion))
				{
					OniVersion* version = (OniVersion*)data;
					version->major = version->minor = version->maintenance = version->build = 2;
				}
				else
				{
					m_driverServices.errorLoggerAppend("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniVersion));
					rc = ONI_STATUS_ERROR;
				}
			}
			break;
		default:
			m_driverServices.errorLoggerAppend("Unknown property: %d\n", propertyId);
			rc = ONI_STATUS_ERROR;
		}
		return rc;
	}
private:
	TestDevice(const TestDevice&);
	void operator=(const TestDevice&);

	OniDeviceInfo* m_pInfo;
	int m_numSensors;
	OniSensorInfo m_sensors[10];
	oni::driver::DriverServices& m_driverServices;
};


class TestDriver : public oni::driver::DriverBase
{
public:
	TestDriver(OniDriverServices* pDriverServices) : DriverBase(pDriverServices)
	{}

	virtual oni::driver::DeviceBase* deviceOpen(const char* uri, const char* /*mode*/)
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

				TestDevice* pDevice = XN_NEW(TestDevice, iter->Key(), getServices());
				iter->Value() = pDevice;
				return pDevice;
			}
		}

		getServices().errorLoggerAppend("Looking for '%s'", uri);
		return NULL;
	}

	virtual void deviceClose(oni::driver::DeviceBase* pDevice)
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

	virtual OniStatus tryDevice(const char* uri)
	{
		if (xnOSStrCmp(uri, "Test"))
		{
			return ONI_STATUS_ERROR;
		}


		OniDeviceInfo* pInfo = XN_NEW(OniDeviceInfo);
		xnOSStrCopy(pInfo->uri, uri, ONI_MAX_STR);
		xnOSStrCopy(pInfo->vendor, "Test", ONI_MAX_STR);
		m_devices[pInfo] = NULL;

		deviceConnected(pInfo);

		return ONI_STATUS_OK;
	}

	void shutdown() {}

protected:

	XN_THREAD_HANDLE m_threadHandle;

	xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*> m_devices;
};

ONI_EXPORT_DRIVER(TestDriver);
