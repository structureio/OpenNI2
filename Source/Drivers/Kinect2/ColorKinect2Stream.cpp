#include "ColorKinect2Stream.h"

#include "Kinect2StreamImpl.h"

using namespace oni::driver;
using namespace kinect2_device;
#define DEFAULT_FPS 30

ColorKinect2Stream::ColorKinect2Stream(Kinect2StreamImpl* pStreamImpl)
  : BaseKinect2Stream(pStreamImpl)
{
	m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
	m_videoMode.fps         = DEFAULT_FPS;
	m_videoMode.resolutionX = 960;
	m_videoMode.resolutionY = 540;
}

void ColorKinect2Stream::frameReady(void* data, int width, int height, double timestamp)
{
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
  pFrame->timestamp = static_cast<int>(timestamp);

  RGBQUAD* data_in = reinterpret_cast<RGBQUAD*>(data);
	OniRGB888Pixel* data_out = reinterpret_cast<OniRGB888Pixel*>(pFrame->data);
  const int xStride = width/m_videoMode.resolutionX;
  const int yStride = height/m_videoMode.resolutionY;
  const int frameX = pFrame->cropOriginX * xStride;
  const int frameY = pFrame->cropOriginY * yStride;
  const int frameWidth = pFrame->width * xStride;
  const int frameHeight = pFrame->height * yStride;
  for (int y = frameY; y < frameY + frameHeight; y += yStride) {
    for (int x = frameX; x < frameX + frameWidth; x += xStride) {
      RGBQUAD* iter = data_in + (y*width + x);
			data_out->b = iter->rgbBlue;
			data_out->r = iter->rgbRed;
			data_out->g = iter->rgbGreen;
			data_out++;
    }
  }

	raiseNewFrame(pFrame);
	getServices().releaseFrame(pFrame);
}
