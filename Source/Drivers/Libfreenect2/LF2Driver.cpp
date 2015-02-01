#include "LF2Driver.h"
#include "LF2Device.h"

using namespace LF2;
using namespace oni::driver;
using namespace libfreenect2;

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

  f2dev = m_f2.openDevice (uri);

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
