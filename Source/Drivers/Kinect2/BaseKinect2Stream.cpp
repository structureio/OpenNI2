#include "BaseKinect2Stream.h"
#include "Kinect2StreamImpl.h"
#include "XnMath.h"

using namespace oni::driver;
using namespace kinect2_device;

BaseKinect2Stream::BaseKinect2Stream(Kinect2StreamImpl* pStreamImpl)
  : m_pStreamImpl(pStreamImpl)
{
  m_running = false;
  m_frameIdx = 0;
  m_cropping.enabled = FALSE;
  pStreamImpl->addStream(this);
}

BaseKinect2Stream::~BaseKinect2Stream()
{
  stop();
  m_pStreamImpl->removeStream(this);
}

OniStatus BaseKinect2Stream::start()
{
  OniStatus status = m_pStreamImpl->start();
  if (status == ONI_STATUS_OK)
    m_running = TRUE;
  return status;
}

void BaseKinect2Stream::stop()
{
  m_running = FALSE;
  m_pStreamImpl->stop();
}

OniStatus BaseKinect2Stream::getProperty(int propertyId, void* data, int* pDataSize)
{
  OniStatus status = ONI_STATUS_NOT_SUPPORTED;
  switch (propertyId)
  {
  case ONI_STREAM_PROPERTY_CROPPING:
    if (*pDataSize != sizeof(OniCropping))
    {
      printf("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniCropping));
      status = ONI_STATUS_ERROR;
    }
    else
    {
      status = GetCropping((OniCropping*)data);
    }
    break;
  case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
    {
      float* val = (float*)data;
      XnDouble tmp = m_pStreamImpl->getHorizontalFov()*xnl::Math::DTR;
      *val = (float)tmp;
      status = ONI_STATUS_OK;
      break;
    }
  case ONI_STREAM_PROPERTY_VERTICAL_FOV:
    {
      float* val = (float*)data;
      XnDouble tmp = m_pStreamImpl->getVerticalFov()*xnl::Math::DTR;
      *val = (float)tmp;
      status = ONI_STATUS_OK;
      break;
    }
  case ONI_STREAM_PROPERTY_VIDEO_MODE:
    if (*pDataSize != sizeof(OniVideoMode))
    {
      printf("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniVideoMode));
      status = ONI_STATUS_ERROR;
    }
    else
    {
      status = GetVideoMode((OniVideoMode*)data);
    }
    break;
  default:
    status = ONI_STATUS_NOT_SUPPORTED;
    break;
  }

  return status;
}

OniStatus BaseKinect2Stream::setProperty(int propertyId, const void* data, int dataSize)
{
  OniStatus status = ONI_STATUS_NOT_SUPPORTED;
  if (propertyId == ONI_STREAM_PROPERTY_CROPPING)
  {
    if (dataSize != sizeof(OniCropping))
    {
      printf("Unexpected size: %d != %d\n", dataSize, sizeof(OniCropping));
      status = ONI_STATUS_ERROR;
    }
    status = SetCropping((OniCropping*)data);
  }
  else if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
  {
    if (dataSize != sizeof(OniVideoMode))
    {
      printf("Unexpected size: %d != %d\n", dataSize, sizeof(OniVideoMode));
       status = ONI_STATUS_ERROR;
    }
    status = SetVideoMode((OniVideoMode*)data);
  }
  return status;
}

OniBool BaseKinect2Stream::isPropertySupported(int propertyId)
{
  OniBool status = FALSE;
  switch (propertyId)
  {
  case ONI_STREAM_PROPERTY_CROPPING:
  case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
  case ONI_STREAM_PROPERTY_VERTICAL_FOV:
  case ONI_STREAM_PROPERTY_VIDEO_MODE:
    status = TRUE;
    break;
  default:
    status = FALSE;
    break;
  }
  return status;
}

OniStatus BaseKinect2Stream::SetVideoMode(OniVideoMode* videoMode)
{
  if (!m_pStreamImpl->isRunning())
  {
    m_videoMode = *videoMode;
    m_pStreamImpl->setVideoMode(videoMode);
    return ONI_STATUS_OK;
  }
  return ONI_STATUS_OUT_OF_FLOW;
}

OniStatus BaseKinect2Stream::GetVideoMode(OniVideoMode* pVideoMode)
{
  *pVideoMode = m_videoMode;
  return ONI_STATUS_OK;
}

OniStatus BaseKinect2Stream::SetCropping(OniCropping* cropping)
{
  m_cropping = *cropping;
  return ONI_STATUS_OK;
}

OniStatus BaseKinect2Stream::GetCropping(OniCropping* cropping)
{
  *cropping = m_cropping;
  return ONI_STATUS_OK;
}
