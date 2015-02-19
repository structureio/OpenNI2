#ifndef LF2IMAGE_STREAM_H
#define LF2IMAGE_STREAM_H

#include "LF2Stream.h"

namespace LF2 {
  class LF2ImageStream :
    public LF2Stream
  {
  public:
    LF2ImageStream (libfreenect2::Freenect2Device * f2dev) : LF2Stream (f2dev) {
          m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
          m_videoMode.resolutionX = 1920;
          m_videoMode.resolutionY = 1080;
          m_videoMode.fps = 30;

          m_frameType = libfreenect2::Frame::Color;
      }

    ~LF2ImageStream()
    {
      stop ();
    }

    // function for OpenNI2
    OniStatus
    start ()
    {
      m_f2dev->setColorFrameListener (this);
      m_f2dev->start ();
      return ONI_STATUS_OK;
    }

    void
    stop ()
    {
        m_f2dev->setColorFrameListener (NULL);
    }

	OniStatus
    getProperty(int propertyId, void* data, int* pDataSize);

  protected:
    int
    BuildFrame (libfreenect2::Frame*,OniFrame*);
  };
}

#endif // LF2IMAGE_STREAM_H
