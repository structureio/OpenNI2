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
      m_f2dev = f2dev;
      m_driver = driver;
    }

	OniStatus
    getSensorInfoList(OniSensorInfo**, int*)
    {
      return ONI_STATUS_NOT_IMPLEMENTED;
    }

    oni::driver::StreamBase* 
    createStream (OniSensorType);

    void
    destroyStream (oni::driver::StreamBase*);
  protected:
    libfreenect2::Freenect2Device* m_f2dev;
    LF2Driver* m_driver;
  };
}

#endif // LF2DEVICE_H




