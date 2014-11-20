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
    m_perfCounter(basePerfCounter),
    m_perfFreq(1.0)
{
  HRESULT hr = pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
  if (FAILED(hr)) {
    m_pCoordinateMapper = NULL;
  }

  m_pFrameBuffer.color = NULL;
  m_pFrameBuffer.depth = NULL;
  m_pFrameBuffer.infrared = NULL;
  createFrameBuffer();

  m_pFrameReader.color = NULL;
  m_pFrameReader.depth = NULL;
  m_pFrameReader.infrared = NULL;
  openFrameReader();

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

  closeFrameReader();
  destroyFrameBuffer();

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
  if (m_sensorType != sensorType) {
    closeFrameReader();
    destroyFrameBuffer();

    m_sensorType = sensorType;

    createFrameBuffer();
    openFrameReader();

    setDefaultVideoMode();
  }
}

void Kinect2StreamImpl::mainLoop()
{
  m_running = TRUE;
  while (m_running) {
    int width, height;
    void* data = populateFrameBuffer(width, height);

    LARGE_INTEGER qpc = {0};
    QueryPerformanceCounter(&qpc);
    double timestamp = static_cast<double>(qpc.QuadPart - m_perfCounter)/m_perfFreq;

    List<BaseKinect2Stream*>::ConstIterator iter = m_streamList.Begin();
    while( iter != m_streamList.End()) {
      if (((BaseKinect2Stream*)(*iter))->isRunning()) {
        ((BaseKinect2Stream*)(*iter))->frameReady(data, width, height, timestamp);
      }
      ++iter;
    }
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

  IFrameDescription* frameDescription = NULL;
  if (sensorType == ONI_SENSOR_COLOR) {
    IColorFrameSource* frameSource = NULL;
    HRESULT hr = m_pKinectSensor->get_ColorFrameSource(&frameSource);
    if (SUCCEEDED(hr)) {
      hr = frameSource->get_FrameDescription(&frameDescription);
      if (FAILED(hr) && frameDescription) {
        frameDescription->Release();
        frameDescription = NULL;
      }
    }
    if (frameSource) {
      frameSource->Release();
    }
  }
  else if (sensorType == ONI_SENSOR_DEPTH) {
    IDepthFrameSource* frameSource = NULL;
    HRESULT hr = m_pKinectSensor->get_DepthFrameSource(&frameSource);
    if (SUCCEEDED(hr)) {
      hr = frameSource->get_FrameDescription(&frameDescription);
      if (FAILED(hr) && frameDescription) {
        frameDescription->Release();
        frameDescription = NULL;
      }
    }
    if (frameSource) {
      frameSource->Release();
    }
  }
  else { // ONI_SENSOR_IR
    IInfraredFrameSource* frameSource = NULL;
    HRESULT hr = m_pKinectSensor->get_InfraredFrameSource(&frameSource);
    if (SUCCEEDED(hr)) {
      hr = frameSource->get_FrameDescription(&frameDescription);
      if (FAILED(hr) && frameDescription) {
        frameDescription->Release();
        frameDescription = NULL;
      }
    }
    if (frameSource) {
      frameSource->Release();
    }
  }

  return frameDescription;
}

void Kinect2StreamImpl::createFrameBuffer()
{
  if (m_sensorType == ONI_SENSOR_COLOR && !m_pFrameBuffer.color) {
    m_pFrameBuffer.color = new RGBQUAD[1920*1080];
  }
  else if (m_sensorType == ONI_SENSOR_DEPTH && !m_pFrameBuffer.depth) {
    m_pFrameBuffer.depth = new UINT16[512*424];
  }
  else if (!m_pFrameBuffer.infrared) { // ONI_SENSOR_IR
    m_pFrameBuffer.infrared = new UINT16[512*424];
  }
}

void Kinect2StreamImpl::destroyFrameBuffer()
{
  if (m_sensorType == ONI_SENSOR_COLOR && m_pFrameBuffer.color) {
    delete[] m_pFrameBuffer.color;
    m_pFrameBuffer.color = NULL;
  }
  else if (m_sensorType == ONI_SENSOR_DEPTH && m_pFrameBuffer.depth) {
    delete[] m_pFrameBuffer.depth;
    m_pFrameBuffer.depth = NULL;
  }
  else if (m_pFrameBuffer.infrared) { // ONI_SENSOR_IR
    delete[] m_pFrameBuffer.infrared;
    m_pFrameBuffer.infrared = NULL;
  }
}

void Kinect2StreamImpl::openFrameReader()
{
  if (!m_pKinectSensor) {
    return;
  }

  if (m_sensorType == ONI_SENSOR_COLOR && !m_pFrameReader.color) {
    IColorFrameSource* frameSource = NULL;
    HRESULT hr = m_pKinectSensor->get_ColorFrameSource(&frameSource);
    if (SUCCEEDED(hr)) {
      hr = frameSource->OpenReader(&m_pFrameReader.color);
      if (FAILED(hr) && m_pFrameReader.color) {
        m_pFrameReader.color->Release();
        m_pFrameReader.color = NULL;
      }
    }
    if (frameSource) {
      frameSource->Release();
    }
  }
  else if (m_sensorType == ONI_SENSOR_DEPTH && !m_pFrameReader.depth) {
    IDepthFrameSource* frameSource = NULL;
    HRESULT hr = m_pKinectSensor->get_DepthFrameSource(&frameSource);
    if (SUCCEEDED(hr)) {
      hr = frameSource->OpenReader(&m_pFrameReader.depth);
      if (FAILED(hr) && m_pFrameReader.depth) {
        m_pFrameReader.depth->Release();
        m_pFrameReader.depth = NULL;
      }
    }
    if (frameSource) {
      frameSource->Release();
    }
  }
  else if(!m_pFrameReader.infrared) { // ONI_SENSOR_IR
    IInfraredFrameSource* frameSource = NULL;
    HRESULT hr = m_pKinectSensor->get_InfraredFrameSource(&frameSource);
    if (SUCCEEDED(hr)) {
      hr = frameSource->OpenReader(&m_pFrameReader.infrared);
      if (FAILED(hr) && m_pFrameReader.infrared) {
        m_pFrameReader.infrared->Release();
        m_pFrameReader.infrared = NULL;
      }
    }
    if (frameSource) {
      frameSource->Release();
    }
  }
}

void Kinect2StreamImpl::closeFrameReader()
{
  if (m_sensorType == ONI_SENSOR_COLOR && m_pFrameReader.color) {
    m_pFrameReader.color->Release();
    m_pFrameReader.color = NULL;
  }
  else if (m_sensorType == ONI_SENSOR_DEPTH && m_pFrameReader.depth) {
    m_pFrameReader.depth->Release();
    m_pFrameReader.depth = NULL;
  }
  else if (m_pFrameReader.infrared) { // ONI_SENSOR_IR
    m_pFrameReader.infrared->Release();
    m_pFrameReader.infrared = NULL;
  }
}

void* Kinect2StreamImpl::populateFrameBuffer(int& buffWidth, int& buffHeight)
{
  buffWidth = 0;
  buffHeight = 0;

  if (m_sensorType == ONI_SENSOR_COLOR) {
    if (m_pFrameReader.color && m_pFrameBuffer.color) {
      buffWidth = 1920;
      buffHeight = 1080;

      IColorFrame* frame = NULL;
      HRESULT hr = m_pFrameReader.color->AcquireLatestFrame(&frame);
      if (SUCCEEDED(hr)) {
        ColorImageFormat imageFormat = ColorImageFormat_None;
        hr = frame->get_RawColorImageFormat(&imageFormat);
        if (SUCCEEDED(hr)) {
          if (imageFormat == ColorImageFormat_Bgra) {
            RGBQUAD* data;
            UINT bufferSize;
            frame->AccessRawUnderlyingBuffer(&bufferSize, reinterpret_cast<BYTE**>(&data));
            memcpy(m_pFrameBuffer.color, data, 1920*1080*sizeof(RGBQUAD));
          }
          else {
            frame->CopyConvertedFrameDataToArray(1920*1080*sizeof(RGBQUAD), reinterpret_cast<BYTE*>(m_pFrameBuffer.color), ColorImageFormat_Bgra);
          }
        }
      }
      if (frame) {
        frame->Release();
      }

      return reinterpret_cast<void*>(m_pFrameBuffer.color);
    }
  }
  else if (m_sensorType == ONI_SENSOR_DEPTH) {
    if (m_pFrameReader.depth && m_pFrameBuffer.depth) {
      buffWidth = 512;
      buffHeight = 424;

      IDepthFrame* frame = NULL;
      HRESULT hr = m_pFrameReader.depth->AcquireLatestFrame(&frame);
      if (SUCCEEDED(hr)) {
        UINT16* data;
        UINT bufferSize;
        frame->AccessUnderlyingBuffer(&bufferSize, &data);
        memcpy(m_pFrameBuffer.depth, data, 512*424*sizeof(UINT16));
      }
      if (frame) {
        frame->Release();
      }

      return reinterpret_cast<void*>(m_pFrameBuffer.depth);
    }
  }
  else { // ONI_SENSOR_IR
    if (m_pFrameReader.infrared && m_pFrameBuffer.infrared) {
      buffWidth = 512;
      buffHeight = 424;

      IInfraredFrame* frame = NULL;
      HRESULT hr = m_pFrameReader.infrared->AcquireLatestFrame(&frame);
      if (SUCCEEDED(hr)) {
        UINT16* data;
        UINT bufferSize;
        frame->AccessUnderlyingBuffer(&bufferSize, &data);
        memcpy(m_pFrameBuffer.infrared, data, 512*424*sizeof(UINT16));
      }
      if (frame) {
        frame->Release();
      }

      return reinterpret_cast<void*>(m_pFrameBuffer.infrared);
    }
  }

  return NULL;
}

XN_THREAD_PROC Kinect2StreamImpl::threadFunc(XN_THREAD_PARAM pThreadParam)
{
  Kinect2StreamImpl* pStream = (Kinect2StreamImpl*)pThreadParam;
  pStream->mainLoop();
  XN_THREAD_PROC_RETURN(XN_STATUS_OK);
}
