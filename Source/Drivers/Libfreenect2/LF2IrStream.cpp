#include "LF2IrStream.h"
#include <XnOS.h>
#include "LF2Common.h"
#include <XnMath.h>

using namespace LF2;

int
LF2IrStream::BuildFrame(libfreenect2::Frame* frame_in,OniFrame* frame_out)
{
  // 1 DepthPixel is 2 byte
  frame_out->dataSize = frame_in->width * frame_in->height * sizeof (ONI_PIXEL_FORMAT_GRAY16);

  frame_out->data = xnOSMalloc (frame_out->dataSize);

  const float* const in_array = (float*) frame_in->data;
  uint16_t* const out_array = (uint16_t*) frame_out->data;
  unsigned int t = 0;
  for (unsigned int y = 0; y < frame_in->height; y++)
	{
      for (unsigned int x = 0; x < frame_in->width; x++)
		{
          out_array[t] = in_array[t];
          ++t;
        }
    }


  frame_out->frameIndex = frame_in->sequence;
  frame_out->sensorType = ONI_SENSOR_IR;

  frame_out->videoMode.pixelFormat = ONI_PIXEL_FORMAT_GRAY16;
  frame_out->videoMode.resolutionX = frame_in->width;
  frame_out->videoMode.resolutionY = frame_in->height;
  frame_out->videoMode.fps = 30;

  frame_out->width = frame_in->width;
  frame_out->height = frame_in->height;

  frame_out->cropOriginX = frame_out->cropOriginY = 0;
  frame_out->croppingEnabled = FALSE;

  frame_out->sensorType = ONI_SENSOR_IR;
  frame_out->stride = frame_in->width*sizeof(ONI_PIXEL_FORMAT_GRAY16);
  frame_out->timestamp = frame_in->sequence*33369;

  return 1;
}

OniStatus 
LF2IrStream::getProperty(int propertyId, void* data, int* pDataSize)
{
  OniStatus status = ONI_STATUS_NOT_SUPPORTED;

  switch (propertyId)
    {
    case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
      {
        float* val = (float*)data;
        XnDouble tmp;
        tmp =  LF2_DEPTH_HORIZONTAL_FOV * xnl::Math::DTR;
        *val = (float)tmp;
        status = ONI_STATUS_OK;
        break;
      }
    case ONI_STREAM_PROPERTY_VERTICAL_FOV:
      {
        float* val = (float*)data;
        XnDouble tmp;
        tmp =  LF2_DEPTH_VERTICAL_FOV * xnl::Math::DTR;
        *val = (float)tmp;
        status = ONI_STATUS_OK;
        break;
      }		
    default:
      return LF2Stream::getProperty(propertyId, data, pDataSize);
    }

  return status;  
}
