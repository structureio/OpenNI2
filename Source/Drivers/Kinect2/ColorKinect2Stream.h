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
      virtual void frameReady(void* data, int width, int height, double timestamp);
  };
} // namespace kinect2_device

#endif //_COLOR_KINECT2_STREAM_H_
