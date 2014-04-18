#include "ColorKinect2Stream.h"

#include "Kinect2StreamImpl.h"
#include <Kinect.h>

using namespace oni::driver;
using namespace kinect2_device;
#define DEFAULT_FPS 30

ColorKinect2Stream::ColorKinect2Stream(Kinect2StreamImpl* pStreamImpl)
  : BaseKinect2Stream(pStreamImpl)
{
	m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
	m_videoMode.fps         = DEFAULT_FPS;
	m_videoMode.resolutionX = 1920;
	m_videoMode.resolutionY = 1080;
  m_frameReader = NULL;
  m_buffer = new RGBQUAD[1920*1080];

  IColorFrameSource* frameSource;
  HRESULT hr = pStreamImpl->getKinectSensor()->get_ColorFrameSource(&frameSource);
  if (FAILED(hr)) {
    return;
  }

  hr = frameSource->OpenReader(&m_frameReader);
  frameSource->Release();
  if (FAILED(hr)) {
    return;
  }
}

ColorKinect2Stream::~ColorKinect2Stream()
{
  if (m_frameReader) {
    m_frameReader->Release();
  }
  delete[] m_buffer;
}

void ColorKinect2Stream::frameReady(unsigned long timestamp)
{
  // Get Kinect2 frame
  if (!m_frameReader) {
    return;
  }

  IColorFrame* frame;
  HRESULT hr = m_frameReader->AcquireLatestFrame(&frame);
  if (FAILED(hr)) {
    return;
  }

  ColorImageFormat imageFormat = ColorImageFormat_None;
  hr = frame->get_RawColorImageFormat(&imageFormat);
  if (FAILED(hr)) {
    return;
  }

  RGBQUAD* data_in;
  if (imageFormat == ColorImageFormat_Bgra) {
    UINT bufferSize;
    hr = frame->AccessRawUnderlyingBuffer(&bufferSize, reinterpret_cast<BYTE**>(&data_in));
  }
  else {
    hr = frame->CopyConvertedFrameDataToArray(m_videoMode.resolutionX*m_videoMode.resolutionY*sizeof(RGBQUAD),
                                              reinterpret_cast<BYTE*>(m_buffer),
                                              ColorImageFormat_Bgra);
	  data_in = m_buffer;
  }
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
	pFrame->dataSize = pFrame->height * pFrame->width * sizeof(OniRGB888Pixel);
	pFrame->stride = pFrame->width * sizeof(OniRGB888Pixel);
	pFrame->videoMode.pixelFormat = m_videoMode.pixelFormat;
	pFrame->videoMode.fps = m_videoMode.fps;
	pFrame->sensorType = ONI_SENSOR_COLOR;
	pFrame->frameIndex = m_frameIdx++;
  pFrame->timestamp = timestamp;

	OniRGB888Pixel* data_out = reinterpret_cast<OniRGB888Pixel*>(pFrame->data);
	if (!m_cropping.enabled)
	{
		RGBQUAD* data_in_end = data_in + (m_videoMode.resolutionY * m_videoMode.resolutionX);
		while (data_in < data_in_end)
		{
			data_out->r = data_in->rgbRed;
			data_out->g = data_in->rgbGreen;
			data_out->b = data_in->rgbBlue;
			++data_in;
			++data_out;
		}
	}
	else
	{
		int cropY = m_cropping.originY;
		while (cropY < m_cropping.originY + m_cropping.height)
		{
		  int cropX = m_cropping.originX;
			while (cropX < m_cropping.originX + m_cropping.width)
			{
				RGBQUAD* iter = data_in + (cropY * m_videoMode.resolutionX + cropX++);
				data_out->b = iter->rgbBlue;
				data_out->r = iter->rgbRed;
				data_out->g = iter->rgbGreen;
				data_out++;
			}
			cropY++;
		}
	}


  // Emit OniFrame and clean
  frame->Release();
	raiseNewFrame(pFrame);
	getServices().releaseFrame(pFrame);
}

OniStatus ColorKinect2Stream::SetVideoMode(OniVideoMode* videoMode)
{
	if (!m_pStreamImpl->isRunning())
	{
    delete[] m_buffer;
    m_buffer = new RGBQUAD[videoMode->resolutionX * videoMode->resolutionY];
	}
	return BaseKinect2Stream::SetVideoMode(videoMode);
}
