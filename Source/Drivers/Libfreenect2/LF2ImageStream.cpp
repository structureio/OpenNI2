#include "LF2ImageStream.h"
#include <XnOS.h>
#include "LF2Common.h"
#include <XnMath.h>

using namespace LF2;

int
LF2ImageStream::BuildFrame(libfreenect2::Frame* frame_in,OniFrame* frame_out)
{
  frame_out->dataSize = frame_in->width * frame_in->height * sizeof (OniRGB888Pixel);

  frame_out->data = xnOSMalloc (frame_out->dataSize);

  xnOSMemCopy (frame_out->data,frame_in->data,frame_out->dataSize);

  frame_out->frameIndex = frame_in->sequence;
  frame_out->sensorType = ONI_SENSOR_COLOR;

  frame_out->videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
  frame_out->videoMode.resolutionX = frame_in->width;
  frame_out->videoMode.resolutionY = frame_in->height;
  frame_out->videoMode.fps = 30;

  frame_out->width = frame_in->width;
  frame_out->height = frame_in->height;

  frame_out->cropOriginX = frame_out->cropOriginY = 0;
  frame_out->croppingEnabled = FALSE;

  frame_out->sensorType = ONI_SENSOR_COLOR;
  frame_out->stride = frame_in->width*sizeof(OniRGB888Pixel);
  frame_out->timestamp = frame_in->sequence*33369;

  return 1;
}

OniStatus 
LF2ImageStream::getProperty(int propertyId, void* data, int* pDataSize)
{
  OniStatus status = ONI_STATUS_NOT_SUPPORTED;

  switch (propertyId)
    {
    case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
      {
        float* val = (float*)data;
        XnDouble tmp;        
        tmp =  LF2_COLOR_HORIZONTAL_FOV * xnl::Math::DTR;
        *val = (float)tmp;
        status = ONI_STATUS_OK;
        break;
      }
    case ONI_STREAM_PROPERTY_VERTICAL_FOV:
      {
        float* val = (float*)data;
        XnDouble tmp;
        tmp =  LF2_COLOR_VERTICAL_FOV * xnl::Math::DTR;
        *val = (float)tmp;
        status = ONI_STATUS_OK;
        break;
      }		
    default:
      return LF2Stream::getProperty(propertyId, data, pDataSize);
    }

  return status;  
}
