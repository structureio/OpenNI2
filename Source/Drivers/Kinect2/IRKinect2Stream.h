#ifndef _IR_KINECT2_STREAM_H_
#define _IR_KINECT2_STREAM_H_

#include "BaseKinect2Stream.h"

struct IInfraredFrameReader;

namespace kinect2_device
{
  class IRKinect2Stream : public BaseKinect2Stream
  {
    public:
      IRKinect2Stream(Kinect2StreamImpl* pStreamImpl);
      virtual void frameReady(void* data, int width, int height, double timestamp);
  };
} // namespace kinect2_device

#endif //_IR_KINECT2_STREAM_H_
