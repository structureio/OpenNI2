#include "LF2DepthStream.h"
#include <XnOS.h>


using namespace LF2;

int
LF2DepthStream::BuildFrame(libfreenect2::Frame* frame_in,OniFrame* frame_out)
{
  // 1 DepthPixel is 2 byte
  frame_out->dataSize = frame_in->width * frame_in->height * 2;

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
  frame_out->timestamp = frame_in->sequence*33369;

  return 1;
}

OniStatus 
LF2DepthStream::getProperty(int propertyId, void* data, int* pDataSize)
{
  OniStatus status = ONI_STATUS_NOT_SUPPORTED;

  switch (propertyId)
    {
	case ONI_STREAM_PROPERTY_MAX_VALUE:
      if (*pDataSize != sizeof(int))
		{
          return ONI_STATUS_BAD_PARAMETER;
		}

      *(int*)data = 10000;
      return ONI_STATUS_OK;
	case ONI_STREAM_PROPERTY_MIN_VALUE:
      if (*pDataSize != sizeof(int))
		{
          return ONI_STATUS_BAD_PARAMETER;
		}

      *(int*)data = 0;
      return ONI_STATUS_OK;
    default:
      return LF2Stream::getProperty(propertyId, data, pDataSize);
    }

  return status;  
}
