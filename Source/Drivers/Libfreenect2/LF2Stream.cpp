#include "LF2Stream.h"

using namespace LF2;
using namespace oni::driver;
using namespace libfreenect2;

bool
LF2Stream::onNewFrame (Frame::Type type,Frame* frame)
{
  OniFrame* pFrame = getServices ().acquireFrame ();
  BuildFrame (frame,pFrame);
  raiseNewFrame (pFrame);
  getServices ().releaseFrame (pFrame);
  return true;
}

