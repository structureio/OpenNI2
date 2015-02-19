#ifndef LF2IR_STREAM_H
#define LF2IR_STREAM_H

#include "LF2Stream.h"

namespace LF2 {
  class LF2IrStream :
    public LF2Stream
  {
  public:
    LF2IrStream (libfreenect2::Freenect2Device * f2dev) : LF2Stream (f2dev) {
          m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_GRAY16;
          m_videoMode.resolutionX = 512;
          m_videoMode.resolutionY = 424;
          m_videoMode.fps = 30;

          m_frameType = libfreenect2::Frame::Ir;
      }

    ~LF2IrStream()
    {
      stop ();
    }

    // function for OpenNI2
    OniStatus
    start ()
    {
      m_f2dev->setIrAndDepthFrameListener (this);
      m_f2dev->start();
      return ONI_STATUS_OK;
    }

    void
    stop ()
    {
      m_f2dev->setIrAndDepthFrameListener (NULL);
    }

	OniStatus
    getProperty(int propertyId, void* data, int* pDataSize);

  protected:
    int
    BuildFrame (libfreenect2::Frame*,OniFrame*);
  };
}

#endif // LF2IR_STREAM_H
