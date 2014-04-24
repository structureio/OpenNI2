#include "Kinect2Device.h"
#include "DepthKinect2Stream.h"
#include "ColorKinect2Stream.h"
#include "IRKinect2Stream.h"
#include <Kinect.h>

using namespace kinect2_device;
using namespace oni::driver;
#define DEFAULT_FPS 30

Kinect2Device::Kinect2Device(IKinectSensor* pKinectSensor)
  : m_pDepthStream(NULL),
    m_pColorStream(NULL),
    m_pKinectSensor(pKinectSensor)
{
	m_numSensors = 3;

	m_sensors[0].sensorType = ONI_SENSOR_DEPTH;
	m_sensors[0].numSupportedVideoModes = 1;
	m_sensors[0].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
	m_sensors[0].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
	m_sensors[0].pSupportedVideoModes[0].fps = DEFAULT_FPS;
	m_sensors[0].pSupportedVideoModes[0].resolutionX = 512;
	m_sensors[0].pSupportedVideoModes[0].resolutionY = 424;

	m_sensors[1].sensorType = ONI_SENSOR_COLOR;
	m_sensors[1].numSupportedVideoModes = 2;
	m_sensors[1].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 2);
	m_sensors[1].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_RGB888;
	m_sensors[1].pSupportedVideoModes[0].fps         = DEFAULT_FPS;
	m_sensors[1].pSupportedVideoModes[0].resolutionX = 960;
	m_sensors[1].pSupportedVideoModes[0].resolutionY = 540;

	m_sensors[1].pSupportedVideoModes[1].pixelFormat = ONI_PIXEL_FORMAT_RGB888;
	m_sensors[1].pSupportedVideoModes[1].fps         = DEFAULT_FPS;
	m_sensors[1].pSupportedVideoModes[1].resolutionX = 1920;
	m_sensors[1].pSupportedVideoModes[1].resolutionY = 1080;

	m_sensors[2].sensorType = ONI_SENSOR_IR;
	m_sensors[2].numSupportedVideoModes = 1;
	m_sensors[2].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
	m_sensors[2].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_GRAY16;
	m_sensors[2].pSupportedVideoModes[0].fps = DEFAULT_FPS;
	m_sensors[2].pSupportedVideoModes[0].resolutionX = 512;
	m_sensors[2].pSupportedVideoModes[0].resolutionY = 424;
}

Kinect2Device::~Kinect2Device()
{
	if (m_pDepthStream != NULL) {
		XN_DELETE(m_pDepthStream);
  }
	
  if (m_pColorStream!= NULL) {
		XN_DELETE(m_pColorStream);
  }
	
	if (m_pKinectSensor) {
		m_pKinectSensor->Close();
    m_pKinectSensor->Release();
  }
}

OniStatus Kinect2Device::getSensorInfoList(OniSensorInfo** pSensors, int* numSensors)
{
	*numSensors = m_numSensors;
	*pSensors = m_sensors;
	return ONI_STATUS_OK;
}

StreamBase* Kinect2Device::createStream(OniSensorType sensorType)
{
	BaseKinect2Stream* pImage = NULL;
	
	if (sensorType == ONI_SENSOR_COLOR)
	{
		if (m_pColorStream == NULL)
		{
			m_pColorStream = XN_NEW(Kinect2StreamImpl, m_pKinectSensor, sensorType);
		}	
		pImage = XN_NEW(ColorKinect2Stream, m_pColorStream);
	}
	else if (sensorType == ONI_SENSOR_DEPTH)
	{
		if (m_pDepthStream == NULL)
		{
			m_pDepthStream = XN_NEW(Kinect2StreamImpl, m_pKinectSensor, sensorType);			
		}
		pImage = XN_NEW(DepthKinect2Stream, m_pDepthStream); 
	}
	if (sensorType ==  ONI_SENSOR_IR)
	{
		if (m_pColorStream == NULL)
		{
			m_pColorStream = XN_NEW(Kinect2StreamImpl, m_pKinectSensor, sensorType);
		}		
		pImage = XN_NEW(IRKinect2Stream, m_pColorStream);
	}
	return pImage;
}

void kinect2_device::Kinect2Device::destroyStream(oni::driver::StreamBase* pStream)
{
	XN_DELETE(pStream);
}

OniStatus Kinect2Device::setProperty(int propertyId, const void* data, int dataSize)
{
	switch (propertyId) {
	case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
		if (dataSize == sizeof(OniImageRegistrationMode)) {
      if (m_pDepthStream) {
				OniImageRegistrationMode* pMode = (OniImageRegistrationMode*)data;
				m_pDepthStream->setImageRegistrationMode(*pMode);
				return ONI_STATUS_OK;
      }
      else {
				return ONI_STATUS_ERROR;
			}
		}
    else {
      printf("Unexpected size: %d != %d\n", dataSize, sizeof(OniImageRegistrationMode));
			return ONI_STATUS_ERROR;
		}
	}
	return ONI_STATUS_NOT_IMPLEMENTED;
}

OniStatus Kinect2Device::getProperty(int propertyId, void* data, int* pDataSize)
{
	switch (propertyId) {
	case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
		if (*pDataSize == sizeof(OniImageRegistrationMode)) {
      if (m_pDepthStream) {
				OniImageRegistrationMode* pMode = (OniImageRegistrationMode*)data;
				*pMode = m_pDepthStream->getImageRegistrationMode();
				return ONI_STATUS_OK;
			} else {
				return ONI_STATUS_ERROR;
			}
		}
    else {
      printf("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniImageRegistrationMode));
			return ONI_STATUS_ERROR;
		}
	}
	return ONI_STATUS_NOT_IMPLEMENTED;
}

OniBool Kinect2Device::isPropertySupported(int propertyId)
{
	return (propertyId == ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION);
}

OniBool Kinect2Device::isCommandSupported(int commandId)
{
	return ONI_STATUS_NOT_IMPLEMENTED;
}

OniStatus Kinect2Device::tryManualTrigger()
{
	return ONI_STATUS_NOT_IMPLEMENTED;
}

OniBool Kinect2Device::isImageRegistrationModeSupported(OniImageRegistrationMode mode)
{
	return (mode == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR || mode == ONI_IMAGE_REGISTRATION_OFF);
}
