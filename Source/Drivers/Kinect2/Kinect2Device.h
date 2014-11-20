#ifndef _KINECT2_DEVICE_H_
#define _KINECT2_DEVICE_H_

#include "Driver\OniDriverAPI.h"
#include "Kinect2StreamImpl.h"

struct IKinectSensor;

namespace kinect2_device
{
  class Kinect2Device : public oni::driver::DeviceBase
  {
    public:
      Kinect2Device(IKinectSensor* pKinectSensor);
      virtual ~Kinect2Device();

      virtual OniStatus getSensorInfoList(OniSensorInfo** pSensors, int* numSources);

      virtual oni::driver::StreamBase* createStream(OniSensorType streamSource);
      virtual void destroyStream(oni::driver::StreamBase* pStream);

      virtual OniStatus setProperty(int propertyId, const void* data, int dataSize);
      virtual OniStatus getProperty(int propertyId, void* data, int* pDataSize);
      virtual OniBool isPropertySupported(int propertyId);

      virtual OniBool isCommandSupported(int commandId);

      virtual OniStatus tryManualTrigger();

      virtual OniBool isImageRegistrationModeSupported(OniImageRegistrationMode mode);

    private:
      IKinectSensor* m_pKinectSensor;
      Kinect2StreamImpl* m_pDepthStream;
      Kinect2StreamImpl* m_pColorStream;
      Kinect2StreamImpl* m_pIRStream;
      int m_numSensors;
      LONGLONG m_perfCounter;
      OniSensorInfo m_sensors[10];
  };
} // namespace kinect2_device

#endif //_KINECT2_DRIVER_H_
