#include "LF2Driver.h"
#include "LF2Device.h"
#include <libfreenect2/depth_packet_processor.h>
#include <libfreenect2/packet_pipeline.h>
#include <XnOS.h>

static const char VENDOR_VAL[] = "Microsoft";
static const char NAME_VAL[] = "Kinect v2";

using namespace LF2;
using namespace oni::driver;
using namespace libfreenect2;

namespace libfreenect2 {
  extern PacketPipeline *createDefaultPacketPipeline();  
}


DeviceBase*
LF2Driver::deviceOpen(const char * uri, const char *)
{
  Freenect2Device * f2dev;  
  LF2Device* pDevice = NULL;

  // if device was already opened for this uri, return the previous one
  if (m_devices.Get (uri,pDevice) == XN_STATUS_OK)
    {
      getServices().errorLoggerAppend("Device is already open.");
      return NULL;
    }

  DepthPacketProcessor::Config config;
  PacketPipeline* packetPipeline = createDefaultPacketPipeline();
  config.MaxDepth = 10000;
  packetPipeline->getDepthPacketProcessor()->setConfiguration(config);
  f2dev = m_f2.openDevice (uri,packetPipeline);

  if (f2dev == 0)
    {
      getServices().errorLoggerAppend("Could not open \"%s\"", uri);
      return NULL;
    }

  pDevice = XN_NEW(LF2Device, f2dev,this);  

  // Add the device and return it.
  m_devices[uri] = pDevice;
  return pDevice;
}

void
LF2Driver::deviceClose(DeviceBase* pDevice)
{
  for (xnl::StringsHash<LF2Device*>::Iterator iter = m_devices.Begin(); iter != m_devices.End(); ++iter)
	{
      if (iter->Value() == pDevice)
		{
          m_devices.Remove(iter);
          XN_DELETE(pDevice);
          return;
		}
	}

  // not our device?!
  XN_ASSERT(FALSE);
}



void
LF2Driver::shutdown ()
{
  // delete devices
  for (xnl::StringsHash<LF2Device*>::Iterator iter = m_devices.Begin(); iter != m_devices.End(); ++iter)
	{
      XN_DELETE(iter->Value ());
    }

  m_devices.Clear ();
}

OniStatus
LF2Driver::initialize (DeviceConnectedCallback connectedCallback, DeviceDisconnectedCallback disconnectedCallback, DeviceStateChangedCallback deviceStateChangedCallback, void* pCookie)
{
  DriverBase::initialize (connectedCallback,disconnectedCallback,deviceStateChangedCallback,pCookie);
  int num_of_device = m_f2.enumerateDevices ();

  if (num_of_device == 0)
    {
      return ONI_STATUS_OK;
    }

  // discover Kinects
  for (int t = 0; t < num_of_device; ++t)
    {
      Freenect2Device* dev = m_f2.openDevice (t);
      OniDeviceInfo* pInfo = XN_NEW (OniDeviceInfo);
      std::string serial = dev->getSerialNumber ();
      
      xnOSStrCopy (pInfo->uri,serial.c_str (),ONI_MAX_STR);      
      xnOSStrCopy (pInfo->vendor,VENDOR_VAL,ONI_MAX_STR);
      xnOSStrCopy (pInfo->name, NAME_VAL,ONI_MAX_STR);
      deviceConnected (pInfo);
      deviceStateChanged (pInfo,false);
      
      dev->close ();      
    }
  return ONI_STATUS_OK;  
}

OniStatus
LF2Driver::tryDevice (const char*)
{
  return ONI_STATUS_OK;
}

ONI_EXPORT_DRIVER(LF2Driver);






