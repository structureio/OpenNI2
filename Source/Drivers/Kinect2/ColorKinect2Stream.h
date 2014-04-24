#ifndef _COLOR_KINECT2_STREAM_H_
#define _COLOR_KINECT2_STREAM_H_

#include "BaseKinect2Stream.h"

struct IColorFrameReader;

namespace kinect2_device
{
  class ColorKinect2Stream : public BaseKinect2Stream
  {
    public:
	    ColorKinect2Stream(Kinect2StreamImpl* pStreamImpl);
      virtual ~ColorKinect2Stream();

	    virtual void frameReady(double timestamp);

    private:
      IColorFrameReader* m_frameReader;
      RGBQUAD* m_buffer;
  };
} // namespace kinect2_device

#endif //_COLOR_KINECT2_STREAM_H_
