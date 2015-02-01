#ifndef LF2IMAGE_STREAM_H
#define LF2IMAGE_STREAM_H

#include "LF2Stream.h"

namespace LF2 {
  class LF2ImageStream :
    public LF2Stream
  {
  public:
    LF2ImageStream (libfreenect2::Freenect2Device * f2dev) : LF2Stream (f2dev) {}

    ~LF2ImageStream()
    {
      stop ();
    }

    // function for OpenNI2
    OniStatus
    start ()
    {
      m_f2dev->setColorFrameListener (this);
      return ONI_STATUS_OK;
    }

    void
    stop ();

  protected:
    int
    BuildFrame (libfreenect2::Frame*,OniFrame*);
  };
}

#endif // LF2IMAGE_STREAM_H
