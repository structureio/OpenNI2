#ifndef _BASE_KINECT2_STREAM_H_
#define _BASE_KINECT2_STREAM_H_

#include "Driver\OniDriverAPI.h"

namespace kinect2_device
{
  class Kinect2StreamImpl;

  class BaseKinect2Stream : public oni::driver::StreamBase
  {
    public:
      BaseKinect2Stream(Kinect2StreamImpl* pStreamImpl);
      virtual ~BaseKinect2Stream();

      virtual OniStatus start();
      virtual void stop();

      virtual OniStatus getProperty(int propertyId, void* data, int* pDataSize);
      virtual OniStatus setProperty(int propertyId, const void* data, int dataSize);
      virtual OniBool isPropertySupported(int propertyId);

      virtual OniStatus SetVideoMode(OniVideoMode* pVideoMode);
      virtual OniStatus GetVideoMode(OniVideoMode* pVideoMode);

      virtual OniStatus SetCropping(OniCropping* cropping);
      virtual OniStatus GetCropping(OniCropping* cropping);

      bool isRunning() { return m_running; }

      virtual void frameReady(void* data, int width, int height, double timestamp) = 0;

    protected:
      Kinect2StreamImpl *m_pStreamImpl;
      OniVideoMode m_videoMode;
      OniCropping m_cropping;
      bool m_running;
      int m_frameIdx;
  };
} // namespace kinect2_device

#endif //_BASE_KINECT2_STREAM_H_
