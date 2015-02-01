#ifndef LF2STREAM_H
#define LF2STREAM_H

#include "Driver/OniDriverAPI.h"
#include <string>
#include <libfreenect2/frame_listener.hpp>
#include <libfreenect2/libfreenect2.hpp>


namespace LF2 {
  class LF2Stream :
    public oni::driver::StreamBase,
    public libfreenect2::FrameListener
  {
  public:
    LF2Stream (libfreenect2::Freenect2Device * f2dev) : oni::driver::StreamBase ()
    {
      m_f2dev = f2dev;
    }

    // function for libfreenect2
    bool
    onNewFrame(libfreenect2::Frame::Type, libfreenect2::Frame*);

    // function for OpenNI2
    virtual OniStatus
    start () = 0;

    virtual void
    stop () = 0;

  protected:
    virtual int
    BuildFrame (libfreenect2::Frame*,OniFrame* pFrame) = 0;

    libfreenect2::Freenect2Device *m_f2dev;
  };
}

#endif // LF2STREAM_H
