#ifndef _KINECT2_DRIVER_H_
#define _KINECT2_DRIVER_H_

#include "Driver\OniDriverAPI.h"
#include "XnHash.h"

namespace kinect2_device
{
  class Kinect2Driver : public oni::driver::DriverBase 
  {
    public:
	    Kinect2Driver(OniDriverServices* pDriverServices);
	
	    virtual OniStatus initialize(oni::driver::DeviceConnectedCallback connectedCallback,
                                   oni::driver::DeviceDisconnectedCallback disconnectedCallback,
                                   oni::driver::DeviceStateChangedCallback deviceStateChangedCallback,
                                   void* pCookie);
	    virtual ~Kinect2Driver();

	    virtual oni::driver::DeviceBase* deviceOpen(const char* uri, const char* mode);
	    virtual void deviceClose(oni::driver::DeviceBase* pDevice);

	    virtual void shutdown();

	    virtual OniStatus tryDevice(const char* uri);

	    virtual void* enableFrameSync(oni::driver::StreamBase** pStreams, int streamCount);
	    virtual void disableFrameSync(void* frameSyncGroup);

    private:
	    xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*> m_devices;
  };
} // namespace kinect2_device

#endif //_KINECT2_DRIVER_H_
