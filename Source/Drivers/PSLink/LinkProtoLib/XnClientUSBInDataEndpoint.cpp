/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#include "XnClientUSBInDataEndpoint.h"
#include "XnLinkProtoLibDefs.h"
#include <XnUSB.h>
#include <XnLog.h>

#define XN_MASK_USB "xnUSB"

namespace xn
{

#if (XN_PLATFORM == XN_PLATFORM_WIN32)
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_BUFFER_NUM_PACKETS_ISO = 80;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_NUM_BUFFERS_ISO = 8;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_TIMEOUT_ISO = 100;

	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_BUFFER_NUM_PACKETS_BULK = 120;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_NUM_BUFFERS_BULK = 8;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_TIMEOUT_BULK = 1000;
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86 || XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_MACOSX || XN_PLATFORM == XN_PLATFORM_ANDROID_ARM || XN_PLATFORM_LINUX_GENERIC )
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_BUFFER_NUM_PACKETS_ISO = 32;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_NUM_BUFFERS_ISO = 16;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_TIMEOUT_ISO = 100;

	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_BUFFER_NUM_PACKETS_BULK = 32;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_NUM_BUFFERS_BULK = 16;
	const XnUInt32 ClientUSBInDataEndpoint::READ_THREAD_TIMEOUT_BULK = 1000;
#else
	#error "Unsupported platform :("
#endif

const XnUInt32 ClientUSBInDataEndpoint::BASE_INPUT_ENDPOINT = XN_EE_DEVICE_IN_DATA_BASE_ENDPOINT;

ClientUSBInDataEndpoint::ClientUSBInDataEndpoint()
{
	m_hUSBDevice = NULL;
	m_nEndpointID = 0;
	m_nMaxPacketSize = 0;
	m_pDataDestination = NULL;
	m_bConnected = FALSE;
	m_hEndpoint = NULL;
}

ClientUSBInDataEndpoint::~ClientUSBInDataEndpoint()
{
	Shutdown();
}

XnStatus ClientUSBInDataEndpoint::Init(XN_USB_DEV_HANDLE hUSBDevice, XnUInt16 nEndpointID)
{
	XN_VALIDATE_INPUT_PTR(hUSBDevice);
	XnStatus nRetVal = XN_STATUS_OK;
	m_hUSBDevice = hUSBDevice;
	m_nEndpointID = BASE_INPUT_ENDPOINT + nEndpointID;
	m_endpointType = XN_USB_EP_ISOCHRONOUS;
	nRetVal = xnUSBOpenEndPoint(m_hUSBDevice, m_nEndpointID, m_endpointType, XN_USB_DIRECTION_IN, &m_hEndpoint);
	if (nRetVal == XN_STATUS_USB_WRONG_ENDPOINT_TYPE)
	{
		m_endpointType = XN_USB_EP_BULK;
		nRetVal = xnUSBOpenEndPoint(m_hUSBDevice, m_nEndpointID, m_endpointType, XN_USB_DIRECTION_IN, &m_hEndpoint);
	}
	XN_IS_STATUS_OK_LOG_ERROR("Open USB endpoint", nRetVal);
	XnUInt32 nTempMaxPacketSize = 0;
	nRetVal = xnUSBGetEndPointMaxPacketSize(m_hEndpoint, &nTempMaxPacketSize);
	XN_IS_STATUS_OK_LOG_ERROR("Get USB endpoint max packet size", nRetVal);
	if (nTempMaxPacketSize > XN_MAX_UINT16)
	{
		xnLogError(XN_MASK_USB, "Max packet size received is larger than max uint16 value?!");
		XN_ASSERT(FALSE);
		return XN_STATUS_ERROR;
	}
	m_nMaxPacketSize = static_cast<XnUInt16>(nTempMaxPacketSize);

	return XN_STATUS_OK;
}

void ClientUSBInDataEndpoint::Shutdown()
{
	Disconnect();
	xnUSBCloseEndPoint(m_hEndpoint);
	m_hEndpoint = NULL;
	m_hUSBDevice = NULL;
}

XnStatus ClientUSBInDataEndpoint::Connect()
{
	XnStatus nRetVal = XN_STATUS_OK;
	Disconnect(); //In case we were connected already

	if (!m_bConnected)
	{
		XnUInt32 nBufferSize = m_nMaxPacketSize * ((m_endpointType == XN_USB_EP_ISOCHRONOUS) ? READ_THREAD_BUFFER_NUM_PACKETS_ISO : READ_THREAD_BUFFER_NUM_PACKETS_BULK);
		XnUInt32 nBuffersCount = (m_endpointType == XN_USB_EP_ISOCHRONOUS) ? READ_THREAD_NUM_BUFFERS_ISO : READ_THREAD_NUM_BUFFERS_BULK;
		XnUInt32 nTimeout = (m_endpointType == XN_USB_EP_ISOCHRONOUS) ? READ_THREAD_TIMEOUT_ISO : READ_THREAD_TIMEOUT_BULK;
		nRetVal = xnUSBInitReadThread(m_hEndpoint, nBufferSize, nBuffersCount, nTimeout, ReadThreadCallback, this);
		XN_IS_STATUS_OK_LOG_ERROR("Init USB Read thread", nRetVal);
		m_bConnected = TRUE;
	}
	return XN_STATUS_OK;
}

void ClientUSBInDataEndpoint::Disconnect()
{
	XnStatus nRetVal = XN_STATUS_OK;
	if (m_bConnected)
	{
		xnLogVerbose(XN_MASK_USB, "Shutting down endpoint 0x%x read thread...", m_nEndpointID);
		nRetVal = xnUSBShutdownReadThread(m_hEndpoint);
		if (nRetVal != XN_STATUS_OK)
		{
			xnLogWarning(XN_MASK_USB, "Failed to shutdown endpoint 0x%x read thread: %s", m_nEndpointID, xnGetStatusString(nRetVal));
			XN_ASSERT(FALSE);
		}
		m_bConnected = FALSE;
	}
}

XnUInt16 ClientUSBInDataEndpoint::GetMaxPacketSize() const
{
	return m_nMaxPacketSize;
}

XnStatus ClientUSBInDataEndpoint::SetDataDestination(IDataDestination* pDataDestination)
{
	XN_VALIDATE_INPUT_PTR(pDataDestination);
	m_pDataDestination = pDataDestination;
	return XN_STATUS_OK;
}

XnBool XN_CALLBACK_TYPE ClientUSBInDataEndpoint::ReadThreadCallback(XnUChar* pBuffer, XnUInt32 nBufferSize, void* pCallbackData)
{
	ClientUSBInDataEndpoint* pThis = reinterpret_cast<ClientUSBInDataEndpoint*>(pCallbackData);
	IDataDestination* pDataDestination = pThis->m_pDataDestination;
	if (pDataDestination != NULL)
	{
		if (nBufferSize == 0)
		{
			//xnLogVerbose(XN_MASK_USB, "USB In Data Endpoint Got 0 length data");
		}
		else
		{
			pDataDestination->IncomingData(pBuffer, nBufferSize);
		}
	}
	
	return TRUE;
}

XnBool ClientUSBInDataEndpoint::IsConnected() const
{
	return m_bConnected;
}

}
