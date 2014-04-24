#include "Kinect2StreamImpl.h"
#include "BaseKinect2Stream.h"

#include <Kinect.h>

using namespace oni::driver;
using namespace kinect2_device;
using namespace xnl;

#define DEFAULT_FPS 30

Kinect2StreamImpl::Kinect2StreamImpl(IKinectSensor *pKinectSensor, OniSensorType sensorType, LONGLONG basePerfCounter)
  : m_pKinectSensor(pKinectSensor),
    m_pCoordinateMapper(NULL),
    m_sensorType(sensorType),
    m_imageRegistrationMode(ONI_IMAGE_REGISTRATION_OFF),
    m_running(FALSE),
    m_hStreamHandle(INVALID_HANDLE_VALUE),
    m_hNextFrameEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
    m_perfCounter(basePerfCounter),
    m_perfFreq(1.0)
{
  HRESULT hr = pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
  if (FAILED(hr)) {
    m_pCoordinateMapper = NULL;
  }
  
  LARGE_INTEGER qpf = {0};
  if (QueryPerformanceFrequency(&qpf))
  {
      m_perfFreq = double(qpf.QuadPart)/1000000.0;
  }

	setDefaultVideoMode();
}

Kinect2StreamImpl::~Kinect2StreamImpl()
{
	if (m_running) {
		m_running = FALSE;
		xnOSWaitForThreadExit(m_threadHandle, INFINITE);
		xnOSCloseThread(&m_threadHandle);
	}

	if (m_hNextFrameEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hNextFrameEvent);
  }

  if (m_pCoordinateMapper) {
    m_pCoordinateMapper->Release();
  }
}

void Kinect2StreamImpl::addStream(BaseKinect2Stream* stream)
{
	m_streamList.AddLast(stream);
}

void Kinect2StreamImpl::removeStream(BaseKinect2Stream* stream)
{
	m_streamList.Remove(stream);
}

unsigned int Kinect2StreamImpl::getStreamCount()
{
	return m_streamList.Size();
}

void Kinect2StreamImpl::setVideoMode(OniVideoMode* videoMode)
{
	m_videoMode.fps = videoMode->fps;
	m_videoMode.pixelFormat = videoMode->pixelFormat;
	m_videoMode.resolutionX = videoMode->resolutionX;
	m_videoMode.resolutionY = videoMode->resolutionY;
}

OniStatus Kinect2StreamImpl::start()
{
	if (m_running != TRUE) {
		XnStatus nRetVal = xnOSCreateThread(threadFunc, this, &m_threadHandle);
		if (nRetVal != XN_STATUS_OK) {
			return ONI_STATUS_ERROR;
		}
		return ONI_STATUS_OK;
	}
	else {
		return ONI_STATUS_OK;
	}
}

void Kinect2StreamImpl::stop()
{
	if (m_running == true) {
		List<BaseKinect2Stream*>::Iterator iter = m_streamList.Begin();
		while( iter != m_streamList.End()) {
			if (((BaseKinect2Stream*)(*iter))->isRunning()) {
				return;
      }
			++iter;
		}
		m_running = false;
		xnOSWaitForThreadExit(m_threadHandle, INFINITE);
		xnOSCloseThread(&m_threadHandle);
	}
}

void Kinect2StreamImpl::setSensorType(OniSensorType sensorType)
{ 
	if( m_sensorType != sensorType) {
		m_sensorType = sensorType; 
		setDefaultVideoMode();
	}	
}

static const unsigned int LOOP_TIMEOUT = 10;
void Kinect2StreamImpl::mainLoop()
{
	m_running = TRUE;
	while (m_running) {
		//if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextFrameEvent, LOOP_TIMEOUT) && m_running) {
      LARGE_INTEGER qpc = {0};
      QueryPerformanceCounter(&qpc);
      double timestamp = static_cast<double>(qpc.QuadPart - m_perfCounter)/m_perfFreq;

			List<BaseKinect2Stream*>::ConstIterator iter = m_streamList.Begin();
			while( iter != m_streamList.End()) {
				if (((BaseKinect2Stream*)(*iter))->isRunning()) {
          ((BaseKinect2Stream*)(*iter))->frameReady(timestamp);
        }
				++iter;
			}
      //Sleep(30);
		//}
	}
	return;
}

XnDouble Kinect2StreamImpl::getHorizontalFov()
{
  IFrameDescription* frameDescription = NULL;
  if (m_sensorType == ONI_SENSOR_DEPTH && m_imageRegistrationMode == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR) {
    frameDescription = getFrameDescription(ONI_SENSOR_COLOR);
  }
  else {
    frameDescription = getFrameDescription(m_sensorType);
  }

  if (frameDescription == NULL) {
    return 0;
  }

  float fov;
  HRESULT hr = frameDescription->get_HorizontalFieldOfView(&fov);
  frameDescription->Release();
  if (FAILED(hr)) {
    return 0;
  }
  return fov;
}

XnDouble Kinect2StreamImpl::getVerticalFov()
{
  IFrameDescription* frameDescription = NULL;
  if (m_sensorType == ONI_SENSOR_DEPTH && m_imageRegistrationMode == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR) {
    frameDescription = getFrameDescription(ONI_SENSOR_COLOR);
  }
  else {
    frameDescription = getFrameDescription(m_sensorType);
  }

  if (frameDescription == NULL) {
    return 0;
  }

  float fov;
  HRESULT hr = frameDescription->get_VerticalFieldOfView(&fov);
  frameDescription->Release();
  if (FAILED(hr)) {
    return 0;
  }
  return fov;
}

void Kinect2StreamImpl::setDefaultVideoMode()
{
	switch (m_sensorType)
	{
	case ONI_SENSOR_COLOR:
		m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
		m_videoMode.fps         = DEFAULT_FPS;
		m_videoMode.resolutionX = 960;
		m_videoMode.resolutionY = 540;
		break;

	case ONI_SENSOR_DEPTH:
		m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
		m_videoMode.fps         = DEFAULT_FPS;
		m_videoMode.resolutionX = 512;
		m_videoMode.resolutionY = 424;
		break;

	case ONI_SENSOR_IR:
		m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_GRAY16;
		m_videoMode.fps         = DEFAULT_FPS;
		m_videoMode.resolutionX = 512;
		m_videoMode.resolutionY = 424;
		break;

	default:
		break;
	}
}

IFrameDescription* Kinect2StreamImpl::getFrameDescription(OniSensorType sensorType)
{
  if (!m_pKinectSensor) {
    return NULL;
  }
  
  HRESULT hr;
  IFrameDescription* frameDescription;

  if (sensorType == ONI_SENSOR_COLOR) {
    IColorFrameSource* frameSource;
    hr = m_pKinectSensor->get_ColorFrameSource(&frameSource);
    if (FAILED(hr)) {
      return NULL;
    }
    hr = frameSource->get_FrameDescription(&frameDescription);
    frameSource->Release();
    if (FAILED(hr)) {
      return NULL;
    }
  }
  else if (sensorType = ONI_SENSOR_DEPTH) {
    IDepthFrameSource* frameSource;
    hr = m_pKinectSensor->get_DepthFrameSource(&frameSource);
    if (FAILED(hr)) {
      return NULL;
    }
    hr = frameSource->get_FrameDescription(&frameDescription);
    frameSource->Release();
    if (FAILED(hr)) {
      return NULL;
    }
  }
  else { // ONI_SENSOR_IR
    IInfraredFrameSource* frameSource;
    hr = m_pKinectSensor->get_InfraredFrameSource(&frameSource);
    if (FAILED(hr)) {
      return NULL;
    }
    hr = frameSource->get_FrameDescription(&frameDescription);
    frameSource->Release();
    if (FAILED(hr)) {
      return NULL;
    }
  }

  return frameDescription;
}

XN_THREAD_PROC Kinect2StreamImpl::threadFunc(XN_THREAD_PARAM pThreadParam)
{
	Kinect2StreamImpl* pStream = (Kinect2StreamImpl*)pThreadParam;
	pStream->mainLoop();
	XN_THREAD_PROC_RETURN(XN_STATUS_OK);
}
