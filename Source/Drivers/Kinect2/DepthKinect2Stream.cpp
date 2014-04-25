#include "DepthKinect2Stream.h"

#include "Kinect2StreamImpl.h"

using namespace oni::driver;
using namespace kinect2_device;
#define DEFAULT_FPS 30

#define DEVICE_MAX_DEPTH_VAL 10000
#define FILTER_RELIABLE_DEPTH_VALUE(VALUE) (((VALUE) < DEVICE_MAX_DEPTH_VAL) ? (VALUE) : 0)

DepthKinect2Stream::DepthKinect2Stream(Kinect2StreamImpl* pStreamImpl)
  : BaseKinect2Stream(pStreamImpl)
{
	m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
	m_videoMode.fps = DEFAULT_FPS;
	m_videoMode.resolutionX = 512;
	m_videoMode.resolutionY = 424;
  m_colorSpaceCoords = new ColorSpacePoint[512*424];
}

DepthKinect2Stream::~DepthKinect2Stream()
{
  delete[] m_colorSpaceCoords;
}

void DepthKinect2Stream::frameReady(void* data, int width, int height, double timestamp)
{
	OniFrame* pFrame = getServices().acquireFrame();
	pFrame->videoMode.resolutionY = m_videoMode.resolutionY;
	pFrame->videoMode.resolutionX = m_videoMode.resolutionX;
	pFrame->croppingEnabled = m_cropping.enabled;
	if (m_cropping.enabled)
	{
		pFrame->width = m_cropping.width;
		pFrame->height = m_cropping.height;
		pFrame->cropOriginX = m_cropping.originX;
		pFrame->cropOriginY = m_cropping.originY;
	}
  else {
		pFrame->cropOriginX = 0; 
		pFrame->cropOriginY = 0;
		pFrame->width = m_videoMode.resolutionX;
		pFrame->height = m_videoMode.resolutionY;
	}
	pFrame->dataSize = pFrame->height * pFrame->width * 2;
	pFrame->stride = pFrame->width * 2;
	pFrame->videoMode.pixelFormat = m_videoMode.pixelFormat;
	pFrame->videoMode.fps = m_videoMode.fps;
	pFrame->sensorType = ONI_SENSOR_DEPTH;
	pFrame->frameIndex = m_frameIdx++;
  pFrame->timestamp = static_cast<int>(timestamp);

  UINT16* data_in = reinterpret_cast<UINT16*>(data);
	if (m_pStreamImpl->getImageRegistrationMode() == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR) {
		copyDepthPixelsWithImageRegistration(data_in, width, height, pFrame);
	} else {
		copyDepthPixelsStraight(data_in, width, height, pFrame);
	}

	raiseNewFrame(pFrame);
	getServices().releaseFrame(pFrame);
}

OniStatus DepthKinect2Stream::getProperty(int propertyId, void* data, int* pDataSize)
{
	OniStatus status = ONI_STATUS_NOT_SUPPORTED;
	switch (propertyId)
	{
	case ONI_STREAM_PROPERTY_MAX_VALUE:
		{
			XnInt * val = (XnInt *)data;
			*val = DEVICE_MAX_DEPTH_VAL;
			status = ONI_STATUS_OK;
			break;
		}
	case ONI_STREAM_PROPERTY_MIRRORING:
		{
			XnBool * val = (XnBool *)data;
			*val = TRUE;
			status = ONI_STATUS_OK;
			break;
		}
	default:
		status = BaseKinect2Stream::getProperty(propertyId, data, pDataSize);
		break;
	}

	return status;
}

OniBool DepthKinect2Stream::isPropertySupported(int propertyId)
{
	OniBool status = FALSE;
	switch (propertyId)
	{
	case ONI_STREAM_PROPERTY_MAX_VALUE:
	case ONI_STREAM_PROPERTY_MIRRORING:
		status = TRUE;
	default:
		status = BaseKinect2Stream::isPropertySupported(propertyId);
		break;
	}
	return status;
}

void DepthKinect2Stream::notifyAllProperties()
{
	XnInt nInt;
	int size = sizeof(nInt);
	getProperty(ONI_STREAM_PROPERTY_MAX_VALUE, &nInt, &size);
	raisePropertyChanged(ONI_STREAM_PROPERTY_MAX_VALUE, &nInt, size);

	BaseKinect2Stream::notifyAllProperties();
}

OniStatus DepthKinect2Stream::SetVideoMode(OniVideoMode* videoMode)
{
	if (!m_pStreamImpl->isRunning())
	{
    delete[] m_colorSpaceCoords;
    m_colorSpaceCoords = new ColorSpacePoint[videoMode->resolutionX * videoMode->resolutionY];
	}
	return BaseKinect2Stream::SetVideoMode(videoMode);
}

void DepthKinect2Stream::copyDepthPixelsStraight(const UINT16* data_in, int width, int height, OniFrame* pFrame)
{
  // Copy the depth pixels to OniDriverFrame
  // with applying cropping but NO depth-to-image registration.

  const int xStride = width/m_videoMode.resolutionX;
  const int yStride = height/m_videoMode.resolutionY;
  const int frameX = pFrame->cropOriginX * xStride;
  const int frameY = pFrame->cropOriginY * yStride;
  const int frameWidth = pFrame->width * xStride;
  const int frameHeight = pFrame->height * yStride;
  
	unsigned short* data_out = (unsigned short*) pFrame->data;
  for (int y = frameY; y < frameY + frameHeight; y += yStride) {
    for (int x = frameX; x < frameX + frameWidth; x += xStride) {
      unsigned short* iter = const_cast<unsigned short*>(data_in + (y*width + x));
      *data_out = FILTER_RELIABLE_DEPTH_VALUE(*iter);
			data_out++;
    }
  }
}

void DepthKinect2Stream::copyDepthPixelsWithImageRegistration(const UINT16* data_in, int width, int height, OniFrame* pFrame)
{
  // Copy the depth pixels to OniDriverFrame
  // with applying cropping and depth-to-image registration.

  const int xStride = width/m_videoMode.resolutionX;
  const int yStride = height/m_videoMode.resolutionY;
	const int minX = pFrame->cropOriginX;
	const int minY = pFrame->cropOriginY;
  const int sizeX = pFrame->width;
  const int sizeY = pFrame->height;
  const int frameX = pFrame->cropOriginX * xStride;
  const int frameY = pFrame->cropOriginY * yStride;
  const int frameWidth = pFrame->width * xStride;
  const int frameHeight = pFrame->height * yStride;
  const int numPoints = m_videoMode.resolutionX * m_videoMode.resolutionY;
  const float xFactor = static_cast<float>(m_videoMode.resolutionX)/1920.0f;
  const float yFactor = static_cast<float>(m_videoMode.resolutionY)/1080.0f;
  const int skipWidth = m_videoMode.resolutionX - sizeX;

  ICoordinateMapper* coordinateMapper = m_pStreamImpl->getCoordinateMapper();
  if (coordinateMapper == NULL) {
    return;
  }

  HRESULT hr = coordinateMapper->MapDepthFrameToColorSpace(width*height, data_in, numPoints, m_colorSpaceCoords);
  if (FAILED(hr)) {
    return;
  }
  
	unsigned short* data_out = (unsigned short*) pFrame->data;
	xnOSMemSet(data_out, 0, pFrame->dataSize);

	const ColorSpacePoint* mappedCoordsIter = m_colorSpaceCoords;
  mappedCoordsIter += minX + minY*m_videoMode.resolutionX;

  for (int y = frameY; y < frameY + frameHeight; y += yStride) {
    for (int x = frameX; x < frameX + frameWidth; x += xStride) {
      const float fX = mappedCoordsIter->X*xFactor;
      const float fY = mappedCoordsIter->Y*yFactor;
		  const int cx = static_cast<int>(fX + 0.5f) - minX;
		  const int cy = static_cast<int>(fY + 0.5f) - minY;

		  if (cx >= 0 && cy >= 0 && cx < sizeX && cy < sizeY) {
        unsigned short* iter = const_cast<unsigned short*>(data_in + (y*width + x));
			  const unsigned short d = FILTER_RELIABLE_DEPTH_VALUE(*iter);
			  unsigned short* const p = data_out + cx + cy * sizeX;
			  if (*p == 0 || *p > d) *p = d;
		  }
      mappedCoordsIter++;
    }
		mappedCoordsIter += skipWidth;
  }
}
