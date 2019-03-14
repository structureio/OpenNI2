#ifndef _KINECT_DRIVER_H_
#define _KINECT_DRIVER_H_

#include "Driver/OniDriverAPI.h"
#include "XnHash.h"
#include <Shlobj.h>
#include "NuiApi.h"

namespace kinect_device {

class KinectDriver : public oni::driver::DriverBase 
{
public:
	KinectDriver(OniDriverServices* pDriverServices);
	
	virtual OniStatus initialize(oni::driver::DeviceConnectedCallback connectedCallback, oni::driver::DeviceDisconnectedCallback disconnectedCallback, 
												oni::driver::DeviceStateChangedCallback deviceStateChangedCallback, void* pCookie);

	virtual ~KinectDriver();

	virtual oni::driver::DeviceBase* deviceOpen(const char* uri, const char* mode);
	virtual void deviceClose(oni::driver::DeviceBase* pDevice);

	virtual void shutdown();

	virtual OniStatus tryDevice(const char* uri);

	virtual void* enableFrameSync(oni::driver::StreamBase** pStreams, int streamCount);
	virtual void disableFrameSync(void* frameSyncGroup);
	void StatusUpdate(const OLECHAR* instanceName, bool isConnected);
	static void CALLBACK StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName , void* pUserData );
private:
	xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*> m_devices;
};
} // namespace kinect_device
#endif //_KINECT_DRIVER_H_
