#ifndef LF2IR_STREAM_H
#define LF2IR_STREAM_H

#include "LF2Stream.h"

namespace LF2 {
  class LF2IrStream :
    public LF2Stream
  {
  public:
    LF2IrStream (libfreenect2::Freenect2Device * f2dev) : LF2Stream (f2dev) {}

    ~LF2IrStream()
    {
      stop ();
    }

    // function for OpenNI2
    OniStatus
    start ()
    {
      m_f2dev->setIrAndDepthFrameListener (this);
      return ONI_STATUS_OK;
    }

    void
    stop ();

  protected:
    int
    BuildFrame (libfreenect2::Frame*,OniFrame*);
  };
}

#endif // LF2IR_STREAM_H
