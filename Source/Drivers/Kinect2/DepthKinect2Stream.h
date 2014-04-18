#ifndef _DEPTH_KINECT2_STREAM_H_
#define _DEPTH_KINECT2_STREAM_H_

#include "BaseKinect2Stream.h"
#include <Kinect.h>

namespace kinect2_device
{
  class DepthKinect2Stream : public BaseKinect2Stream
  {
    public:
	    DepthKinect2Stream(Kinect2StreamImpl* pStreamImpl);
      virtual ~DepthKinect2Stream();

	    virtual void frameReady(unsigned long timestamp);
	
	    virtual OniStatus getProperty(int propertyId, void* data, int* pDataSize);
	    virtual OniBool isPropertySupported(int propertyId);
	    virtual void notifyAllProperties();

	    virtual OniStatus SetVideoMode(OniVideoMode* pVideoMode);

    private:
	    void copyDepthPixelsStraight(const USHORT* source, int numPoints, OniFrame* pFrame);
	    void copyDepthPixelsWithImageRegistration(const USHORT* source, int numPoints, OniFrame* pFrame);

    private:
      IDepthFrameReader* m_frameReader;
      ColorSpacePoint* m_colorSpaceCoords;
  };
} // namespace kinect2_device

#endif //_DEPTH_KINECT2_STREAM_H_
