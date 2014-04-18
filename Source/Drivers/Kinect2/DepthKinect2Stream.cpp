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
  m_frameReader = NULL;
  m_colorSpaceCoords = new ColorSpacePoint[512*424];

  IDepthFrameSource* frameSource;
  HRESULT hr = pStreamImpl->getKinectSensor()->get_DepthFrameSource(&frameSource);
  if (FAILED(hr)) {
    return;
  }

  hr = frameSource->OpenReader(&m_frameReader);
  frameSource->Release();
  if (FAILED(hr)) {
    return;
  }
}

DepthKinect2Stream::~DepthKinect2Stream()
{
  if (m_frameReader) {
    m_frameReader->Release();
  }
  delete[] m_colorSpaceCoords;
}

void DepthKinect2Stream::frameReady(unsigned long timestamp)
{
  // Get Kinect2 frame
  if (!m_frameReader) {
    return;
  }

  IDepthFrame* frame;
  HRESULT hr = m_frameReader->AcquireLatestFrame(&frame);
  if (FAILED(hr)) {
    return;
  }

  UINT16* source;
  UINT sourceSize;
  hr = frame->AccessUnderlyingBuffer(&sourceSize, &source);
  if (FAILED(hr)) {
    frame->Release();
    return;
  }
  

  // Create OniFrame
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
  pFrame->timestamp = timestamp;

	int numPoints = m_videoMode.resolutionY * m_videoMode.resolutionX;
	if (m_pStreamImpl->getImageRegistrationMode() == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR) {
		copyDepthPixelsWithImageRegistration(source, numPoints, pFrame);
	} else {
		copyDepthPixelsStraight(source, numPoints, pFrame);
	}
  
  
  // Emit OniFrame and clean
  frame->Release();
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

void DepthKinect2Stream::copyDepthPixelsStraight(const USHORT* source, int numPoints, OniFrame* pFrame)
{
  // Copy the depth pixels to OniDriverFrame
  // with applying cropping but NO depth-to-image registration.

	// Note: The local variable assignments and const qualifiers are carefully designed to generate
	// a high performance code with VC 2010. We recommend check the performance when changing the code
	// even if it was a trivial change.

	unsigned short* target = (unsigned short*) pFrame->data;

	const unsigned int width = pFrame->width;
	const unsigned int height = pFrame->height;
	const unsigned int skipWidth = m_videoMode.resolutionX - width;

	// Offset the starting position
	source += pFrame->cropOriginX + pFrame->cropOriginY * m_videoMode.resolutionX;

	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
      *(target++) = FILTER_RELIABLE_DEPTH_VALUE(*source);
      source++;
		}
		source += skipWidth;
	}
}

void DepthKinect2Stream::copyDepthPixelsWithImageRegistration(const USHORT* source, int numPoints, OniFrame* pFrame)
{
  // Copy the depth pixels to OniDriverFrame
  // with applying cropping and depth-to-image registration.

	// Note: The local variable assignments and const qualifiers are carefully designed to generate
	// a high performance code with VC 2010. We recommend check the performance when changing the code
	// even if it was a trivial change.

	unsigned short* const target = (unsigned short*) pFrame->data;
	xnOSMemSet(target, 0, pFrame->dataSize);

  ICoordinateMapper* coordinateMapper = m_pStreamImpl->getCoordinateMapper();
  if (coordinateMapper == NULL) {
    return;
  }

  HRESULT hr = coordinateMapper->MapDepthFrameToColorSpace(numPoints, source, numPoints, m_colorSpaceCoords);
  if (FAILED(hr)) {
    return;
  }

	const unsigned int minX = pFrame->cropOriginX;
	const unsigned int minY = pFrame->cropOriginY;
	const unsigned int width = pFrame->width;
	const unsigned int height = pFrame->height;
	const ColorSpacePoint* mappedCoordsIter = m_colorSpaceCoords;
  const float xFactor = static_cast<float>(m_videoMode.resolutionX)/1920.0f;
  const float yFactor = static_cast<float>(m_videoMode.resolutionY)/1080.0f;

	for (int i = 0; i < numPoints; i++)
	{
    const float fX = mappedCoordsIter->X*xFactor;
    const float fY = mappedCoordsIter->Y*yFactor;
		const unsigned int x = static_cast<unsigned int>(fX + 0.5f) - minX;
		const unsigned int y = static_cast<unsigned int>(fY + 0.5f) - minY;
		if (x < width && y < height) {
			const unsigned short d = FILTER_RELIABLE_DEPTH_VALUE(*source);
			unsigned short* const p = target + x + y * width;
			if (*p == 0 || *p > d) *p = d;
		}
    mappedCoordsIter++;
    source++;
	}
}
