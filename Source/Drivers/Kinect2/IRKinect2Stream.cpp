#include "IRKinect2Stream.h"

#include "Kinect2StreamImpl.h"
#include <Kinect.h>

using namespace oni::driver;
using namespace kinect2_device;
#define DEFAULT_FPS 30

IRKinect2Stream::IRKinect2Stream(Kinect2StreamImpl* pStreamImpl)
  : BaseKinect2Stream(pStreamImpl)
{
	m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_GRAY16;
	m_videoMode.fps = DEFAULT_FPS;
	m_videoMode.resolutionX = 512;
	m_videoMode.resolutionY = 424;
  m_frameReader = NULL;

  IInfraredFrameSource* frameSource;
  HRESULT hr = pStreamImpl->getKinectSensor()->get_InfraredFrameSource(&frameSource);
  if (FAILED(hr)) {
    return;
  }

  hr = frameSource->OpenReader(&m_frameReader);
  frameSource->Release();
  if (FAILED(hr)) {
    return;
  }
}

IRKinect2Stream::~IRKinect2Stream()
{
  if (m_frameReader) {
    m_frameReader->Release();
  }
}

void IRKinect2Stream::frameReady(unsigned long timestamp)
{
  // Get Kinect2 frame
  if (!m_frameReader) {
    return;
  }

  IInfraredFrame* frame;
  HRESULT hr = m_frameReader->AcquireLatestFrame(&frame);
  if (FAILED(hr)) {
    return;
  }

	unsigned short* data_in;
  unsigned int data_in_size;
  hr = frame->AccessUnderlyingBuffer(&data_in_size, &data_in);
  if (FAILED(hr)) {
    frame->Release();
    return;
  }


  // Create OniFrame
	OniFrame* pFrame = getServices().acquireFrame();
	pFrame->videoMode.resolutionX = m_videoMode.resolutionX;
	pFrame->videoMode.resolutionY = m_videoMode.resolutionY;
	pFrame->croppingEnabled = m_cropping.enabled;
	if (m_cropping.enabled)
	{
		pFrame->width = m_cropping.width;
		pFrame->height = m_cropping.height;
		pFrame->cropOriginX = m_cropping.originX; 
		pFrame->cropOriginY = m_cropping.originY;
	}
	else
	{
		pFrame->cropOriginX = 0; 
		pFrame->cropOriginY = 0;
		pFrame->width = m_videoMode.resolutionX;
		pFrame->height = m_videoMode.resolutionY;
	}
	pFrame->dataSize = pFrame->height * pFrame->width * 2;
	pFrame->stride = pFrame->width * 2;
	pFrame->videoMode.pixelFormat = m_videoMode.pixelFormat;
	pFrame->videoMode.fps = m_videoMode.fps;
	pFrame->sensorType = ONI_SENSOR_IR;
	pFrame->frameIndex = m_frameIdx++;
  pFrame->timestamp = timestamp;

	unsigned short* data_out = reinterpret_cast<unsigned short*>(pFrame->data);
	if (!m_cropping.enabled)
	{
    xnOSMemCopy(data_out, data_in, pFrame->dataSize);
	}
	else
	{
		int cropY = m_cropping.originY;
		while (cropY < m_cropping.originY + m_cropping.height)
		{
			int cropX = m_cropping.originX;
			while (cropX < m_cropping.originX + m_cropping.width)
			{
				unsigned short *iter = data_in + (cropY * m_videoMode.resolutionX + cropX++);
				*(data_out++) = (*iter);
			}
			cropY++;
		}
	}


  // Emit OniFrame and clean
  frame->Release();
	raiseNewFrame(pFrame);
	getServices().releaseFrame(pFrame);
}
