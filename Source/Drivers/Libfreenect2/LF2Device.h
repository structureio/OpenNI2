#ifndef LF2DEVICE_H
#define LF2DEVICE_H

#include <string>
#include "libfreenect2/libfreenect2.hpp"
#include "Driver/OniDriverAPI.h"
#include "LF2Driver.h"


namespace LF2 {
  class LF2Driver;
  class LF2Device : 
    public oni::driver::DeviceBase 
  {
  public :
    LF2Device(libfreenect2::Freenect2Device* f2dev,LF2Driver* driver)
    {
      static const int DEFAULT_FPS = 30;
      m_f2dev = f2dev;
      m_driver = driver;
      // this is actually 3. but now color and ir are not supprted yet.

      m_sensors[0].sensorType = ONI_SENSOR_DEPTH;
      m_sensors[0].numSupportedVideoModes = 1;
      m_sensors[0].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
      m_sensors[0].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
      m_sensors[0].pSupportedVideoModes[0].fps = DEFAULT_FPS;
      m_sensors[0].pSupportedVideoModes[0].resolutionX = 512;
      m_sensors[0].pSupportedVideoModes[0].resolutionY = 424;

      m_sensors[1].sensorType = ONI_SENSOR_COLOR;
      m_sensors[1].numSupportedVideoModes = 2;
      m_sensors[1].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
      m_sensors[1].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_RGB888;
      m_sensors[1].pSupportedVideoModes[0].fps         = DEFAULT_FPS;
      m_sensors[1].pSupportedVideoModes[0].resolutionX = 1920;
      m_sensors[1].pSupportedVideoModes[0].resolutionY = 1080;

      m_sensors[2].sensorType = ONI_SENSOR_IR;
      m_sensors[2].numSupportedVideoModes = 1;
      m_sensors[2].pSupportedVideoModes = XN_NEW_ARR(OniVideoMode, 1);
      m_sensors[2].pSupportedVideoModes[0].pixelFormat = ONI_PIXEL_FORMAT_GRAY16;
      m_sensors[2].pSupportedVideoModes[0].fps = DEFAULT_FPS;
      m_sensors[2].pSupportedVideoModes[0].resolutionX = 512;
      m_sensors[2].pSupportedVideoModes[0].resolutionY = 424;

    }

	OniStatus
    getSensorInfoList(OniSensorInfo** pSensors, int* numSensors)
    {
      *numSensors = m_numSensors;
      *pSensors = m_sensors;
      return ONI_STATUS_OK;
    }

    oni::driver::StreamBase* 
    createStream (OniSensorType);

    void
    destroyStream (oni::driver::StreamBase*);
  protected:
    static const int m_numSensors = 3;
    OniSensorInfo m_sensors[m_numSensors];
    libfreenect2::Freenect2Device* m_f2dev;
    LF2Driver* m_driver;
  };
}

#endif // LF2DEVICE_H




