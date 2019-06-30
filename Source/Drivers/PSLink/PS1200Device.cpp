#include "PS1200Device.h"
#include "XnClientUSBConnectionFactory.h"
#include "XnSocketConnectionFactory.h"
#include <PSLink.h>
#include <XnLog.h>

#define XN_MASK_PS1200_DEVICE "PS1200Device"

namespace xn
{

const XnUInt32 PS1200Device::WAIT_FOR_FREE_BUFFER_TIMEOUT_MS = XN_WAIT_INFINITE;
const XnUInt32 PS1200Device::NUM_OUTPUT_CONNECTIONS = 0;
const XnUInt32 PS1200Device::NUM_INPUT_CONNECTIONS = 3;
const XnUInt32 PS1200Device::PRE_CONTROL_RECEIVE_SLEEP = 0;

PS1200Device::PS1200Device()
{
	m_hInputInterruptCallback = NULL;
	m_bInitialized = FALSE;
}

PS1200Device::~PS1200Device()
{
	Shutdown();
}

XnStatus PS1200Device::Init(const XnChar* strConnString, XnTransportType transportType)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (transportType != XN_TRANSPORT_TYPE_USB)
	{
		xnLogError(XN_MASK_LINK, "Transport type not supported: %d", transportType);
		XN_ASSERT(FALSE);
		return XN_STATUS_BAD_PARAM;
	}

	nRetVal = PrimeClient::Init(strConnString, XN_TRANSPORT_TYPE_USB);
	XN_IS_STATUS_OK_LOG_ERROR("Init EE Device", nRetVal);

#if (XN_PLATFORM == XN_PLATFORM_WIN32)
	// On all platforms other than Windows, prefer BULK
	nRetVal = SetUsbAltInterface(0);
	XN_IS_STATUS_OK_LOG_ERROR("Switch to ISO", nRetVal);
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86 || XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_LINUX_AARCH64 || XN_PLATFORM == XN_PLATFORM_LINUX_PPC || XN_PLATFORM == XN_PLATFORM_MACOSX || XN_PLATFORM == XN_PLATFORM_ANDROID_ARM)
	// On all platforms other than Windows, prefer BULK
	nRetVal = SetUsbAltInterface(1);
	XN_IS_STATUS_OK_LOG_ERROR("Switch to BULK", nRetVal);
#else
	#error "Unsupported platform"
#endif

	m_bInitialized = TRUE;
	return XN_STATUS_OK;	
}

void PS1200Device::Shutdown()
{
	PrimeClient::Shutdown();
	m_bInitialized = FALSE;
}

XnBool PS1200Device::IsInitialized() const
{
	return m_bInitialized;
}

IConnectionFactory* PS1200Device::CreateConnectionFactory(XnTransportType transportType)
{
	if (transportType != XN_TRANSPORT_TYPE_USB)
	{
		XN_ASSERT(FALSE);
		return NULL;
	}

	return XN_NEW(ClientUSBConnectionFactory, 
			        NUM_INPUT_CONNECTIONS, 
					NUM_OUTPUT_CONNECTIONS, 
					PRE_CONTROL_RECEIVE_SLEEP);
}

ClientUSBConnectionFactory* PS1200Device::GetConnectionFactory()
{
	return (ClientUSBConnectionFactory*)m_pConnectionFactory;
}

const ClientUSBConnectionFactory* PS1200Device::GetConnectionFactory() const
{
	return (const ClientUSBConnectionFactory*)m_pConnectionFactory;
}

XnStatus PS1200Device::SetUsbAltInterface(XnUInt8 altInterface)
{
	return GetConnectionFactory()->SetUsbAltInterface(altInterface);
}

XnStatus PS1200Device::GetUsbAltInterface(XnUInt8& altInterface) const
{
	return GetConnectionFactory()->GetUsbAltInterface(&altInterface);
}

class UsbEndpointTester : public IDataDestination
{
public:
	void Reset()
	{
		m_nCounter = 0;
		m_nTotalBytes = 0;
		m_nLostPackets = 0;
	}

	virtual XnStatus IncomingData(const void* pData, XnUInt32 nSize)
	{
		m_nTotalBytes += nSize;

		const XnUInt8* pCurData = (const XnUInt8*)pData;
		const XnUInt8* pEndData = pCurData + nSize;

		while (pCurData < pEndData)
		{
			// first word is a counter
			const XnUInt32* pDWords = (const XnUInt32*)pCurData;
			XnUInt32 nPacketSize = pDWords[0];
			XnUInt32 nCounter = pDWords[1];

			XnUInt32 nLostPackets = (nCounter - m_nCounter - 1);

			m_nLostPackets += nLostPackets;
			m_nCounter = nCounter;
			pCurData += nPacketSize;
		}

		return XN_STATUS_OK;
	}

	virtual void HandleDisconnection()
	{
		xnLogWarning(XN_MASK_PS1200_DEVICE, "Endpoint disconnected during USB test!");
	}

	XnUInt32 m_nEP;
	XnUInt32 m_nTotalBytes;
	XnUInt32 m_nLostPackets;

private:
	int m_nCounter;
};

XnStatus PS1200Device::UsbTest(XnUInt32 nSeconds, XnUInt32& endpointsCount, XnUsbTestEndpointResult* endpoints)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	xn::ClientUSBConnectionFactory* pConnFactory = GetConnectionFactory();

	if (m_linkInputStreamsMgr.HasStreams())
	{
		xnLogWarning(XN_MASK_PS1200_DEVICE, "Can't start USB test when other streams exists!");
		return XN_STATUS_ERROR;
	}

	XnUInt16 nNumEndpoints = pConnFactory->GetNumInputDataConnections();
	if (nNumEndpoints > endpointsCount)
	{
		xnLogWarning(XN_MASK_PS1200_DEVICE, "Endpoints array is too small");
		return XN_STATUS_BAD_PARAM;
	}

	xn::IAsyncInputConnection* aEndpoints[20];
	UsbEndpointTester aTesters[20];

	for (int i = 0; i < nNumEndpoints; ++i)
	{
		nRetVal = pConnFactory->CreateInputDataConnection((XnUInt16)i, aEndpoints[i]);
		if (nRetVal != XN_STATUS_OK)
		{
			for (int j = 0; j < i; ++j)
			{
				XN_DELETE(aEndpoints[j]);
			}
			return nRetVal;
		}

		aTesters[i].Reset();
		aTesters[i].m_nEP = i;
		aEndpoints[i]->SetDataDestination(&aTesters[i]);
		aEndpoints[i]->Connect();
	}

	nRetVal = m_linkControlEndpoint.StartUsbTest();
	if (nRetVal != XN_STATUS_OK)
	{
		for (int i = 0; i < nNumEndpoints; ++i)
		{
			XN_DELETE(aEndpoints[i]);
		}
		return nRetVal;
	}

	// let the test run
	xnOSSleep(nSeconds*1000);

	nRetVal = m_linkControlEndpoint.StopUsbTest();
	if (nRetVal != XN_STATUS_OK)
	{
		xnLogWarning(XN_MASK_PS1200_DEVICE, "Failed to stop USB test!");
		XN_ASSERT(FALSE);
	}

	for (int i = 0; i < nNumEndpoints; ++i)
	{
		XN_DELETE(aEndpoints[i]);
		
		endpoints[i].averageBytesPerSecond = aTesters[i].m_nTotalBytes / (XnDouble)nSeconds;
		endpoints[i].lostPackets = aTesters[i].m_nLostPackets;
	}
	endpointsCount = nNumEndpoints;

	return (XN_STATUS_OK);
}

}
