#include "LF2DepthStream.h"

using namespace LF2;

int
LF2DepthStream::BuildFrame(libfreenect2::Frame* frame_in,OniFrame* frame_out)
{
  frame_out->dataSize = frame_in->width * frame_in->height * frame_in->bytes_per_pixel;

  memcpy (frame_out->data,frame_in->data,frame_out->dataSize);

  frame_out->frameIndex = frame_in->sequence;
  frame_out->sensorType = ONI_SENSOR_DEPTH;

  frame_out->videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
  frame_out->videoMode.resolutionX = frame_in->width;
  frame_out->videoMode.resolutionY = frame_in->height;
  frame_out->videoMode.fps = 30;

  frame_out->width = frame_in->width;
  frame_out->height = frame_in->height;

  frame_out->cropOriginX = frame_out->cropOriginY = 0;
  frame_out->croppingEnabled = FALSE;

  frame_out->sensorType = ONI_SENSOR_DEPTH;
  frame_out->stride = frame_in->width*sizeof(OniDepthPixel);
  frame_out->timestamp = frame_in->sequence*33000;

  return 1;
}

