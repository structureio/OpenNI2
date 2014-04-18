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
      virtual ~IRKinect2Stream();

	    virtual void frameReady(unsigned long timestamp);

    private:
      IInfraredFrameReader* m_frameReader;
  };
} // namespace kinect2_device

#endif //_IR_KINECT2_STREAM_H_
