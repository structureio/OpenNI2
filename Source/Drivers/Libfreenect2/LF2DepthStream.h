#ifndef LF2DEPTH_STREAM_H
#define LF2DEPTH_STREAM_H

#include "LF2Stream.h"

namespace LF2 {
  class LF2DepthStream :
    public LF2Stream
  {
  public:
    LF2DepthStream (libfreenect2::Freenect2Device * f2dev) : LF2Stream (f2dev) { 
      m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
      m_videoMode.resolutionX = 512;
      m_videoMode.resolutionY = 424;
      m_videoMode.fps = 30;

      m_frameType = libfreenect2::Frame::Depth;
    }
    
    ~LF2DepthStream ()
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

#endif // LF2DEPTH_STREAM_H
