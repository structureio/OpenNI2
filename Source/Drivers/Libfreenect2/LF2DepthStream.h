#ifndef LF2DEPTH_STREAM_H
#define LF2DEPTH_STREAM_H

#include "LF2Stream.h"

namespace LF2 {
  class LF2DepthStream :
    public LF2Stream
  {
  public:
    LF2DepthStream (libfreenect2::Freenect2Device * f2dev) : LF2Stream (f2dev){}

    ~LF2DepthStream()
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

  protected:
    int
    BuildFrame (libfreenect2::Frame*,OniFrame*);
  };
}

#endif // LF2DEPTH_STREAM_H
