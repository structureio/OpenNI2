#include "XnLinkControlEndpoint.h"
#include "XnLinkMsgEncoder.h"
#include "XnLinkMsgParser.h"
#include "IConnectionFactory.h"
#include "XnLinkProtoUtils.h"
#include "XnLinkStatusCodes.h"
#include "XnShiftToDepth.h"
#include "XnLinkProtoLibDefs.h"
#include <XnStatusCodes.h>
#include <XnLog.h>
#include <XnOSCpp.h>

#define MAX_PROP_SIZE 2048

namespace xn
{

const XnUInt16 LinkControlEndpoint::BASE_PACKET_ID = 1; //Packet IDs start from 1
const XnUInt16 LinkControlEndpoint::MAX_RESPONSE_NUM_PACKETS = 8;
const XnChar LinkControlEndpoint::MUTEX_NAME[] = "XnLinkControlEPMutex";
const XnUInt32 LinkControlEndpoint::MUTEX_TIMEOUT = 20000;


LinkControlEndpoint::LinkControlEndpoint()
{
    m_pIncomingRawPacket = NULL;
    m_nMaxOutMsgSize = 0;		
	m_pConnection = NULL;
    m_pIncomingResponse = NULL;
    m_nMaxResponseSize = 0;
	m_bInitialized = FALSE;
	m_bConnected = FALSE;
	m_nPacketID = BASE_PACKET_ID; 
	m_nMaxPacketSize = 0;
	m_hMutex = NULL;
}

LinkControlEndpoint::~LinkControlEndpoint()
{
	Shutdown();
}

XnStatus LinkControlEndpoint::Init(XnUInt32 nMaxOutMsgSize, IConnectionFactory* pConnectionFactory)
{
	XN_VALIDATE_INPUT_PTR(pConnectionFactory);
	XnStatus nRetVal = XN_STATUS_OK;
	if (!m_bInitialized)
	{
		m_nMaxOutMsgSize = nMaxOutMsgSize;
		nRetVal = pConnectionFactory->GetControlConnection(m_pConnection);
		XN_IS_STATUS_OK_LOG_ERROR("Create control connection", nRetVal);
		//TODO: Once we have service discovery, ask the device what's the max packet size.
		//In any way, we DON'T want to ask m_pConnection what its packet size is, cuz that's the low level packet size.

#if XN_PLATFORM == XN_PLATFORM_ANDROID_ARM
	nRetVal = xnOSCreateMutex(&m_hMutex);
	XN_IS_STATUS_OK(nRetVal);
#else
		nRetVal = xnOSCreateNamedMutex(&m_hMutex, MUTEX_NAME);
		XN_IS_STATUS_OK_LOG_ERROR("Create named mutext", nRetVal);
#endif

		/* Initialize supported msg types array with commands that must always be supported */
		nRetVal = m_supportedMsgTypes.SetMinSize(XN_LINK_INTERFACE_PROPS + 1);
		XN_IS_STATUS_OK_LOG_ERROR("Add to supported msg types", nRetVal);
		nRetVal = m_supportedMsgTypes[XN_LINK_INTERFACE_PROPS].Set(XN_LINK_MSG_GET_PROP & 0xFF, TRUE);
		XN_IS_STATUS_OK_LOG_ERROR("Add to supported msg types", nRetVal);
		
		m_bInitialized = TRUE;
	}

	return XN_STATUS_OK;
}

void LinkControlEndpoint::Shutdown()
{
	if (m_pConnection != NULL)
	{
		Disconnect();
		m_pConnection = NULL;
	}

	if (m_hMutex != NULL)
	{
		xnOSCloseMutex(&m_hMutex);
		m_hMutex = NULL;
	}

	m_bInitialized = FALSE;
}

XnStatus LinkControlEndpoint::Connect()
{
	XnStatus nRetVal = XN_STATUS_OK;
	if (!m_bInitialized)
	{
		XN_LOG_ERROR_RETURN(XN_STATUS_NOT_INIT, XN_MASK_LINK, "Not initialized");
	}

	if (!m_bConnected)
	{
		nRetVal = m_pConnection->Connect();
		XN_IS_STATUS_OK_LOG_ERROR("Connect control connection", nRetVal);
		m_nPacketID = BASE_PACKET_ID;

		//First thing we must do - get logical max packet size - sending other commands depends on this.
		nRetVal = GetLogicalMaxPacketSize(m_nMaxPacketSize);
		XN_IS_STATUS_OK_LOG_ERROR("Get logical max packet size", nRetVal);
		
		//Now that we have the device's logical max packet size we can initialize the msg encoder and parser
		nRetVal = m_msgEncoder.Init(m_nMaxOutMsgSize, m_nMaxPacketSize);
		if (nRetVal != XN_STATUS_OK)
		{
			xnLogError(XN_MASK_LINK, "LINK: Failed to init msg encoder: %s", xnGetStatusString(nRetVal));
			XN_ASSERT(FALSE);
			Disconnect();
			return nRetVal;
		}

		nRetVal = m_responseMsgParser.Init();
		if (nRetVal != XN_STATUS_OK)
		{
			xnLogError(XN_MASK_LINK, "LINK: Failed to init msg parser: %s", xnGetStatusString(nRetVal));
			XN_ASSERT(FALSE);
			Disconnect();
			return nRetVal;
		}

		m_pIncomingRawPacket = reinterpret_cast<XnUInt8*>(xnOSMallocAligned(m_nMaxPacketSize, XN_DEFAULT_MEM_ALIGN));
		if (m_pIncomingRawPacket == NULL)
		{
			xnLogError(XN_MASK_LINK, "LINK: Failed to allocate incoming packet");
			XN_ASSERT(FALSE);
			Disconnect();
			return XN_STATUS_ALLOC_FAILED;
		}
        m_nMaxResponseSize = MAX_RESPONSE_NUM_PACKETS * m_nMaxPacketSize;
		m_pIncomingResponse = reinterpret_cast<XnUInt8*>(xnOSMallocAligned(m_nMaxResponseSize, XN_DEFAULT_MEM_ALIGN));
		if (m_pIncomingResponse == NULL)
		{
			xnLogError(XN_MASK_LINK, "LINK: Failed to allocate incoming response");
			XN_ASSERT(FALSE);
			Disconnect();
			return XN_STATUS_ALLOC_FAILED;
		}

		//Now that all our encoding and parsing objects are ready we can get other properties
		nRetVal = GetSupportedMsgTypes(m_supportedMsgTypes);
		XN_IS_STATUS_OK_LOG_ERROR("Get supported msg types", nRetVal);

		m_bConnected = TRUE;
	}

	return XN_STATUS_OK;
}

void LinkControlEndpoint::Disconnect()
{
	//Shutdown everything we initialized in Connect().
	m_msgEncoder.Shutdown();
	m_responseMsgParser.Shutdown();
	XN_ALIGNED_FREE_AND_NULL(m_pIncomingRawPacket);
	m_pIncomingRawPacket = NULL;

	XN_ALIGNED_FREE_AND_NULL(m_pIncomingResponse);
	m_pIncomingResponse = NULL;

	//We don't disconnect the actual connection cuz it is cached and doesn't belong to us.
	m_bConnected = FALSE;
}

XnBool LinkControlEndpoint::IsConnected() const
{
	return m_bConnected;
}

XnStatus LinkControlEndpoint::ExecuteCommand(XnUInt16 nMsgType,
											 XnUInt16 nStreamID, 											 
											 const void* pCmdData, 
											 XnUInt32 nCmdSize, 
											 void* pResponseData, 
											 XnUInt32& nResponseSize,
											 XnBool* pIsLast /*= NULL*/)
{
	XnStatus nRetVal = XN_STATUS_OK;
	xnl::AutoMutexLocker mutexLocker(m_hMutex, MUTEX_TIMEOUT);
	XN_IS_STATUS_OK_LOG_ERROR("Lock mutex", mutexLocker.GetStatus());

	XnBool autoContinue = (pIsLast == NULL);
	XnBool isLast;

	/*XN_LINK_FRAG_SINGLE in this case indicates a single 'block' of data, not necessarily a single
	  packet. */
	nRetVal = ExecuteImpl(nMsgType, nStreamID, pCmdData, nCmdSize, XN_LINK_FRAG_SINGLE, pResponseData, nResponseSize, autoContinue, isLast);
	XN_IS_STATUS_OK_LOG_ERROR("Send Data", nRetVal);

	if (pIsLast != NULL)
	{
		*pIsLast = isLast;
	}

	return XN_STATUS_OK;
}

XnUInt16 LinkControlEndpoint::GetPacketID() const
{
	return m_nPacketID;
}

XN_MUTEX_HANDLE LinkControlEndpoint::GetMutex() const
{
	return m_hMutex;
}

XnBool LinkControlEndpoint::IsMsgTypeSupported(XnUInt16 nMsgType)
{
	XnUInt8 nMsgTypeHi = ((nMsgType >> 8) & 0xFF);
	XnUInt8 nMsgTypeLo = (nMsgType & 0xFF);
	return (nMsgTypeHi < m_supportedMsgTypes.GetSize()) && (m_supportedMsgTypes[nMsgTypeHi].IsSet(nMsgTypeLo));
}

XnStatus LinkControlEndpoint::GetFWVersion(XnLinkDetailedVersion& version)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting FW version...");

	XnLinkDetailedVersion linkVersion;
	XnUInt32 nPropSize = sizeof(linkVersion);
    nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_FW_VERSION, nPropSize, &linkVersion);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get version command", nRetVal);

	if (nPropSize != sizeof(linkVersion))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of FW version property: %u instead of %u", nPropSize, sizeof(linkVersion));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	xnLinkParseDetailedVersion(version, linkVersion);

	xnLogInfo(XN_MASK_LINK, "LINK: FW version is %u.%u.%u.%u-%s", version.m_nMajor, version.m_nMinor, version.m_nMaintenance, version.m_nBuild, version.m_strModifier);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetProtocolVersion(XnLeanVersion& version)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting protocol version...");

	XnLinkLeanVersion linkVersion;
	XnUInt32 nPropSize = sizeof(linkVersion);
	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_PROTOCOL_VERSION, nPropSize, &linkVersion);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get protocol version command", nRetVal);

	if (nPropSize != sizeof(linkVersion))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of protocol version property: %u instead of %u", nPropSize, sizeof(linkVersion));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	xnLinkParseLeanVersion(version, linkVersion);

	xnLogInfo(XN_MASK_LINK, "LINK: Protocol version is %u.%u", version.m_nMajor, version.m_nMinor);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetHardwareVersion(XnUInt32& version)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting hardware version...");

	XnUInt64 linkVersion;
	nRetVal = GetIntProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_HW_VERSION, linkVersion);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get hardware version command", nRetVal);

	version = (XnUInt32)XN_PREPARE_VAR64_IN_BUFFER(linkVersion);

	xnLogInfo(XN_MASK_LINK, "LINK: Hardware version is %llu", version);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetSerialNumber(XnChar* strSerialNumber, XnUInt32 nSize)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting serial number...");

	XnLinkSerialNumber linkSerial;
	XnUInt32 nPropSize = sizeof(linkSerial);
	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_SERIAL_NUMBER, nPropSize, &linkSerial);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get serial version", nRetVal);

	if (nPropSize != sizeof(linkSerial))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of serial version property: %u instead of %u", nPropSize, sizeof(linkSerial));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nRetVal = xnOSStrCopy(strSerialNumber, linkSerial.m_strSerialNumber, nSize);
	XN_IS_STATUS_OK(nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Serial number is %s", strSerialNumber);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetComponentsVersions(xnl::Array<XnComponentVersion>& components)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting components versions...");

	XnLinkComponentVersionsList* pComponentVersionsList = reinterpret_cast<XnLinkComponentVersionsList*>(m_pIncomingResponse);
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_COMPONENT_VERSIONS, nResponseSize, pComponentVersionsList);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get components versions list", nRetVal);

	nRetVal = xnLinkParseComponentVersionsList(components, pComponentVersionsList, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("parse components versions list", nRetVal);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::UploadFile(const XnChar* strFileName, XnBool bOverrideFactorySettings)
{
	XnStatus nRetVal = XN_STATUS_OK;
	XN_FILE_HANDLE hFile = XN_INVALID_FILE_HANDLE;
	XnUInt64 nFileSize = 0;
	XnUInt32 nBytesRead = 0;
	XnUInt32 nCunkSize = m_msgEncoder.GetMaxMsgSize();
	XnUInt8* pChunk = NULL;
	XnUInt64 nBytesToSend = 0;
	XnUInt32 nBytesInChunk = 0;
	XnLinkFragmentation fragmentation = XN_LINK_FRAG_BEGIN;

	xnLogVerbose(XN_MASK_LINK, "LINK: Uploading file %s%s...", strFileName, bOverrideFactorySettings ? "[factory]" : "");

	xnl::AutoMutexLocker mutexLocker(m_hMutex, MUTEX_TIMEOUT);
	XN_IS_STATUS_OK_LOG_ERROR("Lock mutex", mutexLocker.GetStatus());

	nRetVal = xnOSOpenFile(strFileName, XN_OS_FILE_READ, &hFile);
	XN_IS_STATUS_OK_LOG_ERROR("Open file", nRetVal);
	nRetVal = xnOSGetFileSize64(strFileName, &nFileSize);
	XN_IS_STATUS_OK_LOG_ERROR("Get file size", nRetVal);
	nBytesToSend = nFileSize + sizeof(XnLinkUploadFileHeader);
	pChunk = (XnUInt8*)xnOSMallocAligned(nCunkSize, XN_DEFAULT_MEM_ALIGN);
	if (pChunk == NULL)
    {
        xnOSCloseFile(&hFile);
        xnLogError(XN_MASK_LINK, "LINK: Failed to allocate buffer of %u bytes for loading file", nCunkSize);
        XN_ASSERT(FALSE);
        return XN_STATUS_ALLOC_FAILED;
    }

	while (nBytesToSend > 0)
	{
		nBytesInChunk = 0;
		nBytesRead = nCunkSize;

		if (nBytesToSend == nFileSize + sizeof(XnLinkUploadFileHeader))
		{
			// first packet
			XnLinkUploadFileHeader* pUploadFileHeader = (XnLinkUploadFileHeader*)pChunk;
			pUploadFileHeader->m_bOverrideFactorySettings = bOverrideFactorySettings;

			nBytesRead -= sizeof(XnLinkUploadFileHeader);
			nBytesInChunk += sizeof(XnLinkUploadFileHeader);
		}
		
		nRetVal = xnOSReadFile(hFile, pChunk + nBytesInChunk, &nBytesRead);
		if ((nRetVal != XN_STATUS_OK) || (nBytesRead == 0))
		{
			xnOSCloseFile(&hFile);
			xnOSFreeAligned(pChunk);
			xnLogError(XN_MASK_LINK, "LINK: Failed to read from file: %s", 
				(nBytesRead == 0) ? "0 bytes read" : xnGetStatusString(nRetVal));
			XN_ASSERT(FALSE);
			return nRetVal;
		}

		nBytesInChunk += nBytesRead;

		if (nBytesToSend <= nCunkSize)
		{
			//About to send last chunk - add END bit.
			fragmentation = XnLinkFragmentation(fragmentation | XN_LINK_FRAG_END);
		}

		xnLogVerbose(XN_MASK_LINK, "LINK: Sending file chunk...");

		XnUInt32 nResponseSize = m_nMaxResponseSize;
		XnBool isLast;
		nRetVal = ExecuteImpl(XN_LINK_MSG_UPLOAD_FILE, XN_LINK_STREAM_ID_NONE, pChunk, nBytesInChunk, fragmentation, m_pIncomingResponse, nResponseSize, TRUE, isLast);
		if (nRetVal != XN_STATUS_OK)
		{
			xnOSCloseFile(&hFile);
			xnOSFreeAligned(pChunk);
			xnLogError(XN_MASK_LINK, "LINK: Failed to send data: %s", xnGetStatusString(nRetVal));
			XN_ASSERT(FALSE);
			return nRetVal;
		}

		/*If we need to send another chunk, its default fragmentation is MIDDLE (if needed it'll change to 
		  END before sending). */
		fragmentation = XN_LINK_FRAG_MIDDLE;

		nBytesToSend -= nBytesInChunk;
	}

	xnOSCloseFile(&hFile);
	xnOSFreeAligned(pChunk);
	xnLogInfo(XN_MASK_LINK, "LINK: File %s uploaded", strFileName);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetFileList(xnl::Array<XnFwFileEntry>& files)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting file list...");

	files.Clear();

	XnLinkGetFileListResponse* pGetFileListResponse = reinterpret_cast<XnLinkGetFileListResponse*>(m_pIncomingResponse);
	XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_GET_FILE_LIST, XN_LINK_STREAM_ID_NONE, NULL, 0, pGetFileListResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get file list command", nRetVal);

	if (nResponseSize < sizeof(pGetFileListResponse->m_nCount))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of 'get file list' response: %u (should be at least %u)", nResponseSize, sizeof(pGetFileListResponse->m_nCount));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	XnUInt32 nCount = XN_PREPARE_VAR32_IN_BUFFER(pGetFileListResponse->m_nCount);
	if (nResponseSize < sizeof(pGetFileListResponse->m_nCount) + nCount*sizeof(XnLinkFileEntry))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of 'get file list' response: %u (should be at least %u)", nResponseSize, sizeof(pGetFileListResponse->m_nCount) + nCount*sizeof(XnLinkFileEntry));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nRetVal = files.Reserve(nCount);
	XN_IS_STATUS_OK(nRetVal);

	for (XnUInt32 i = 0; i < nCount; ++i)
	{
		XnLinkFileEntry* pLinkEntry = &pGetFileListResponse->m_aFileEntries[i];
		XnFwFileEntry entry;
		xnOSStrCopy(entry.name, pLinkEntry->m_strName, sizeof(entry.name));
		entry.version.major = pLinkEntry->m_nVersion.m_nMajor;
		entry.version.minor = pLinkEntry->m_nVersion.m_nMinor;
		entry.version.maintenance = pLinkEntry->m_nVersion.m_nMaintenance;
		entry.version.build = pLinkEntry->m_nVersion.m_nBuild;
		entry.address = XN_PREPARE_VAR32_IN_BUFFER(pLinkEntry->m_nAddress);
		entry.size = XN_PREPARE_VAR32_IN_BUFFER(pLinkEntry->m_nSize);
		entry.crc = XN_PREPARE_VAR16_IN_BUFFER(pLinkEntry->m_nCRC);
		entry.zone = XN_PREPARE_VAR16_IN_BUFFER(pLinkEntry->m_nZone);
		entry.flags = (XnFwFileFlags)pLinkEntry->m_nFlags;

		nRetVal = files.AddLast(entry);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::DownloadFile(XnUInt16 zone, const XnChar* fwFileName, const XnChar* targetFile)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	xnLogVerbose(XN_MASK_LINK, "LINK: Downloading file %s from zone %u...", fwFileName, zone);

	XnLinkDownloadFileParams downloadFileParams = {0};
	nRetVal = xnOSStrCopy(downloadFileParams.m_strName, fwFileName, sizeof(downloadFileParams.m_strName));
	XN_IS_STATUS_OK_LOG_ERROR("Bad file name", nRetVal);
	downloadFileParams.m_nZone = XN_PREPARE_VAR16_IN_BUFFER(zone);

	XN_FILE_HANDLE hTargetFile;
	nRetVal = xnOSOpenFile(targetFile, XN_OS_FILE_WRITE | XN_OS_FILE_TRUNCATE, &hTargetFile);
	XN_IS_STATUS_OK_LOG_ERROR("Open target file", nRetVal);

	XnUInt64 nStartTime;
	xnOSGetHighResTimeStamp(&nStartTime);
	XnUInt32 nFileSize = 0;

	XnBool isLast = FALSE;
	XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_DOWNLOAD_FILE, 0, &downloadFileParams, sizeof(downloadFileParams), m_pIncomingResponse, nResponseSize, &isLast);
	if (nRetVal != XN_STATUS_OK)
	{
		xnLogWarning(XN_MASK_LINK, "LINK: Failed to execute download file command: %s", xnGetStatusString(nRetVal));
		xnOSCloseFile(&hTargetFile);
		return nRetVal;
	}

	nRetVal = xnOSWriteFile(hTargetFile, m_pIncomingResponse, nResponseSize);
	if (nRetVal != XN_STATUS_OK)
	{
		xnLogWarning(XN_MASK_LINK, "LINK: Failed to write file: %s", xnGetStatusString(nRetVal));
		xnOSCloseFile(&hTargetFile);
		return nRetVal;
	}

	nFileSize += nResponseSize;

	while (!isLast)
	{
		nResponseSize = m_nMaxResponseSize;
		nRetVal = ContinueResponseImpl(XN_LINK_MSG_DOWNLOAD_FILE, 0, m_pIncomingResponse, nResponseSize, isLast);
		if (nRetVal != XN_STATUS_OK)
		{
			xnLogWarning(XN_MASK_LINK, "LINK: Failed to continue download file: %s", xnGetStatusString(nRetVal));
			xnOSCloseFile(&hTargetFile);
			return nRetVal;
		}
		nFileSize += nResponseSize;

		nRetVal = xnOSWriteFile(hTargetFile, m_pIncomingResponse, nResponseSize);
		if (nRetVal != XN_STATUS_OK)
		{
			xnLogWarning(XN_MASK_LINK, "LINK: Failed to write file: %s", xnGetStatusString(nRetVal));
			xnOSCloseFile(&hTargetFile);
			return nRetVal;
		}
	}

	XnUInt64 nEndTime;
	xnOSGetHighResTimeStamp(&nEndTime);

	XnDouble totalTime = (nEndTime - nStartTime)/1e3;

	xnLogVerbose(XN_MASK_LINK, "LINK: Downloaded %u bytes from file %u/%s in %.2f ms (%.2f KB/s)", nFileSize, zone, fwFileName, totalTime, nFileSize / totalTime);

	xnOSCloseFile(&hTargetFile);

	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::GetLogicalMaxPacketSize(XnUInt16& nMaxPacketSize)
{
	XnStatus nRetVal = XN_STATUS_OK;
	XnUInt8 command[64];
	XnUInt8 response[64];
	XnUInt32 nResponseSize = 0;
	XnUInt16 nResponseCode = 0;
	xn::LinkPacketHeader* pCommandHeader = reinterpret_cast<xn::LinkPacketHeader*>(command);
	XnLinkGetPropParams* pGetPropParams = reinterpret_cast<XnLinkGetPropParams*>(command + sizeof(xn::LinkPacketHeader));
	
	XnLinkResponseHeader* pResponseHeader = reinterpret_cast<XnLinkResponseHeader*>(response);
	XnLinkGetPropResponse* pGetPropResponse = reinterpret_cast<XnLinkGetPropResponse*>(response + sizeof(XnLinkResponseHeader));
	XnUInt64* pPropValue = reinterpret_cast<XnUInt64*>(response + sizeof(XnLinkResponseHeader) + sizeof(XnLinkPropValHeader));
	XnUInt64 nTempPropValue = 0;
	
	xnLogVerbose(XN_MASK_LINK, "LINK: Link control endpoint - getting logical max packet size...");

	//We encode this message ourselves, not with msg encoder, because we haven't yet initialized our message encoder
	//(we need the size to initialize it...)
	pCommandHeader->SetMagic();
	pCommandHeader->SetSize(sizeof(xn::LinkPacketHeader) + sizeof(XnLinkGetPropParams));
	pCommandHeader->SetMsgType(XN_LINK_MSG_GET_PROP);
	pCommandHeader->SetFragmentationFlags(XN_LINK_FRAG_SINGLE);
	pCommandHeader->SetStreamID(XN_LINK_STREAM_ID_NONE);
	pCommandHeader->SetPacketID(m_nPacketID);
	pCommandHeader->SetCID(0);
	pGetPropParams->m_nPropID = XN_PREPARE_VAR16_IN_BUFFER(XN_LINK_PROP_ID_CONTROL_MAX_PACKET_SIZE);
	pGetPropParams->m_nPropType = XN_PREPARE_VAR16_IN_BUFFER(XN_LINK_PROP_TYPE_INT);

	nRetVal = m_pConnection->Send(command, pCommandHeader->GetSize());
	XN_IS_STATUS_OK_LOG_ERROR("Get logical control max packet size ", nRetVal);
	
	nResponseSize = sizeof(response);
	nRetVal = m_pConnection->Receive(response, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Receive response for get logical control max packet size command", nRetVal);

	nRetVal = ValidateResponsePacket(reinterpret_cast<xn::LinkPacketHeader*>(&pResponseHeader->m_header), XN_LINK_MSG_GET_PROP, XN_LINK_STREAM_ID_NONE, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Validate response packet for get logical packet size", nRetVal);

	nResponseCode = XN_PREPARE_VAR16_IN_BUFFER(pResponseHeader->m_responseInfo.m_nResponseCode);
	if (pResponseHeader->m_responseInfo.m_nResponseCode != XN_PREPARE_VAR16_IN_BUFFER(XN_LINK_RESPONSE_OK))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got response for get logical control max packet size: '%s' (%u)", xnLinkResponseCodeToStr(nResponseCode));
		XN_ASSERT(FALSE);
		return xnLinkResponseCodeToStatus(nResponseCode);
	}

	if (pGetPropResponse->m_header.m_nPropID != pGetPropParams->m_nPropID)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad prop id in response for get logical control max packet size");
		XN_ASSERT(FALSE);
		return XN_STATUS_ERROR;
	}

	if (pGetPropResponse->m_header.m_nPropType != pGetPropParams->m_nPropType)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad prop type in response for get logical control max packet size");
		XN_ASSERT(FALSE);
		return XN_STATUS_ERROR;
	}

	if (pGetPropResponse->m_header.m_nValueSize != XN_PREPARE_VAR32_IN_BUFFER(sizeof(XnUInt64)))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad value size in response for get logical control max packet size");
		XN_ASSERT(FALSE);
		return XN_STATUS_ERROR;
	}

	nTempPropValue = XN_PREPARE_VAR64_IN_BUFFER(*pPropValue);
	if (nTempPropValue > XN_MAX_UINT16)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad value for logical max packet size");
		XN_ASSERT(FALSE);
		return XN_STATUS_ERROR;
	}

	nMaxPacketSize = static_cast<XnUInt16>(nTempPropValue);
	xnLogVerbose(XN_MASK_LINK, "LINK: Link control endpoint logical max packet size is %u bytes", nMaxPacketSize);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::ExecuteImpl(XnUInt16 nMsgType, 
										   XnUInt16 nStreamID,
										   const void* pData, 
										   XnUInt32 nSize, 
										   XnLinkFragmentation fragmentation, 
										   void* pResponseData, 
										   XnUInt32& nResponseSize,
										   XnBool autoContinue,
										   XnBool& isLast)
{
	XnStatus nRetVal = XN_STATUS_OK;
	XnUInt32 nReceivedResponsePacketSize = 0;
	XnUInt16 nPacketSize = 0;
	XnLinkFragmentation responseFragmentation = XN_LINK_FRAG_MIDDLE;

	//Before we start encoding the command we first make sure it is supported
	if (!IsMsgTypeSupported(nMsgType))
	{
		xnLogWarning(XN_MASK_LINK, "LINK: Msg type 0x%04X is not in supported msg types", nMsgType);
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_CMD_NOT_SUPPORTED;
	}

	/* First step - encode command into separate packets. */
	//Keep only the BEGIN bit of the fragmentation mask for the first packet.
	m_msgEncoder.BeginEncoding(nMsgType, m_nPacketID, nStreamID, XnLinkFragmentation(fragmentation & XN_LINK_FRAG_BEGIN));
	m_msgEncoder.EncodeData(pData, nSize);
	m_msgEncoder.EndEncoding(XnLinkFragmentation(fragmentation & XN_LINK_FRAG_END));

	XnUInt32 nBytesLeftToSend = m_msgEncoder.GetEncodedSize();
	union
	{
		const XnUInt8* pRawCommandPacket;
		XnLinkPacket* pCommandPacket;
	};
	pRawCommandPacket = reinterpret_cast<const XnUInt8*>(m_msgEncoder.GetEncodedData());

	/* Second step - Send each packet and get a response for it. */
	while (nBytesLeftToSend > 0)
	{
		nPacketSize = static_cast<XnUInt16>(XN_MIN(nBytesLeftToSend, m_nMaxPacketSize));
		nRetVal = m_pConnection->Send(pRawCommandPacket, nPacketSize);
		XN_IS_STATUS_OK_LOG_ERROR("Send control packet", nRetVal);
		pRawCommandPacket += nPacketSize;
		nBytesLeftToSend -= nPacketSize;
		nReceivedResponsePacketSize = m_nMaxPacketSize;

		nRetVal = m_pConnection->Receive(m_pIncomingRawPacket, nReceivedResponsePacketSize);
		XN_IS_STATUS_OK_LOG_ERROR("Receive response packet", nRetVal);
		XN_ASSERT(nReceivedResponsePacketSize < XN_MAX_UINT16);
		nRetVal = ValidateResponsePacket(m_pIncomingPacket, nMsgType, nStreamID, nReceivedResponsePacketSize);
		responseFragmentation = m_pIncomingPacket->GetFragmentationFlags();
		XN_IS_STATUS_OK_LOG_ERROR("Parse response packet header", nRetVal);
		nRetVal = m_responseMsgParser.BeginParsing(pResponseData, nResponseSize);
		XN_IS_STATUS_OK_LOG_ERROR("Begin parsing response packet", nRetVal);
		nRetVal = m_responseMsgParser.ParsePacket(*m_pIncomingPacket, m_pIncomingRawPacket + sizeof(XnLinkPacketHeader));
		XN_IS_STATUS_OK_LOG_ERROR("Parse response packet", nRetVal);

		if (nBytesLeftToSend > 0)
		{
			if (responseFragmentation != XN_LINK_FRAG_SINGLE)
			{
				xnLogWarning(XN_MASK_LINK, "LINK: Got unexpected responseFragmentation flag of 0x%X in response when there are still more packets to be sent as part of current command", responseFragmentation);
				XN_ASSERT(FALSE);
				//This is just a warning - we keep going.
			}

			if (m_responseMsgParser.GetParsedSize() > 0)
			{
				xnLogWarning(XN_MASK_LINK, "LINK: Got unexpected response packet size of %u in response when there are still more packets to be sent as part of current command", m_responseMsgParser.GetParsedSize());
				XN_ASSERT(FALSE);
				//This is just a warning - we keep going.
			}
		}

		/* Advance packet ID for next packet*/
		m_nPacketID++;
	}

	XnUInt32 nTotalResponseSize = m_responseMsgParser.GetParsedSize();
	XnUInt8* pResponseBytes = reinterpret_cast<XnUInt8*>(pResponseData);

	/* Third step - If needed, continue receiving the rest of the response, in separate packets. */
	isLast = ((responseFragmentation & XN_LINK_FRAG_END) == XN_LINK_FRAG_END);
	while (autoContinue && !isLast)
	{
		XnUInt32 nPacketResponseSize = nResponseSize - nTotalResponseSize;
		nRetVal = ContinueResponseImpl(nMsgType, nStreamID, pResponseBytes + nTotalResponseSize, nPacketResponseSize, isLast);
		XN_IS_STATUS_OK_LOG_ERROR("Continue response", nRetVal);

		nTotalResponseSize += nPacketResponseSize;
	}

	nResponseSize = nTotalResponseSize;

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::ContinueResponseImpl(XnUInt16 originalMsgType, XnUInt16 streamID, void* pResponseData, XnUInt32& nResponseSize, XnBool& outLastPacket)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	xnLogVerbose(XN_MASK_LINK, "LINK: Asking for additional data for response...");

	XnLinkContinueReponseParams continueResponseParams;
	continueResponseParams.m_nOriginalMsgType = XN_PREPARE_VAR16_IN_BUFFER(originalMsgType);

	//Encode link continue response command with m_msgEncoder.
	m_msgEncoder.BeginEncoding(XN_LINK_MSG_CONTINUE_REPONSE, m_nPacketID, 0);
	m_msgEncoder.EncodeData(&continueResponseParams, sizeof(continueResponseParams));
	m_msgEncoder.EndEncoding();

	//Send Continue Response Command to device
	nRetVal = m_pConnection->Send(m_msgEncoder.GetEncodedData(), m_msgEncoder.GetEncodedSize());
	XN_IS_STATUS_OK_LOG_ERROR("Send Continue Response command", nRetVal);

	//Receive one more response packet
	XnUInt32 nReceivedResponsePacketSize = m_nMaxPacketSize;
	nRetVal = m_pConnection->Receive(m_pIncomingRawPacket, nReceivedResponsePacketSize);
	XN_IS_STATUS_OK_LOG_ERROR("Receive response packet", nRetVal);
	XN_ASSERT(nReceivedResponsePacketSize <= m_nMaxPacketSize);

	//Now expecting continue response message type
	nRetVal = ValidateResponsePacket(m_pIncomingPacket, XN_LINK_MSG_CONTINUE_REPONSE, streamID, nReceivedResponsePacketSize);
	XN_IS_STATUS_OK_LOG_ERROR("Parse response packet header", nRetVal);
	XnLinkFragmentation responseFragmentation = m_pIncomingPacket->GetFragmentationFlags();

	nRetVal = m_responseMsgParser.BeginParsing(pResponseData, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Begin parsing response packet", nRetVal);
	nRetVal = m_responseMsgParser.ParsePacket(*m_pIncomingPacket, m_pIncomingRawPacket + sizeof(XnLinkPacketHeader));
	XN_IS_STATUS_OK_LOG_ERROR("Parse response packet", nRetVal);

	/* Advance packet ID for next packet*/
	m_nPacketID++;

	nResponseSize = m_responseMsgParser.GetParsedSize();
	outLastPacket = ((responseFragmentation & XN_LINK_FRAG_END) == XN_LINK_FRAG_END);

	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::StartStreaming(XnUInt16 nStreamID)
{
	XnStatus nRetVal = XN_STATUS_OK;
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	xnLogVerbose(XN_MASK_LINK, "LINK: Starting streaming for stream %u...", nStreamID);

	nRetVal = ExecuteCommand(XN_LINK_MSG_START_STREAMING, nStreamID, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute start streaming command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u has started streaming.", nStreamID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::StopStreaming(XnUInt16 nStreamID)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Stopping streaming for stream %u...", nStreamID);

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_STOP_STREAMING, nStreamID, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute stop streaming command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u has stopped streaming.", nStreamID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SoftReset()
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Executing soft reset...");

    XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_SOFT_RESET, XN_LINK_STREAM_ID_NONE, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute soft reset", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Soft reset done.");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::HardReset()
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Executing power reset...");

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_HARD_RESET, XN_LINK_STREAM_ID_NONE, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute hard reset", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Power reset done.");

	return XN_STATUS_OK;
}


XnStatus LinkControlEndpoint::GetSupportedI2CDevices(xnl::Array<XnLinkI2CDevice>& supportedDevices)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting I2C devices list...");

	XnLinkSupportedI2CDevices* pSupportedI2CDevices = 
		reinterpret_cast<XnLinkSupportedI2CDevices*>(m_pIncomingResponse);
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_SUPPORTED_I2C_DEVICES, nResponseSize, pSupportedI2CDevices);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get supported I2C devices command", nRetVal);

	nRetVal = xnLinkParseSupportedI2CDevices(pSupportedI2CDevices, nResponseSize, supportedDevices);
	XN_IS_STATUS_OK(nRetVal);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetSupportedLogFiles(xnl::Array<XnLinkLogFile>& supportedFiles)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting supported log files list...");

	XnLinkSupportedLogFiles* pSupportedLogFiles = 
		reinterpret_cast<XnLinkSupportedLogFiles*>(m_pIncomingResponse);
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_SUPPORTED_LOG_FILES, nResponseSize, pSupportedLogFiles);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get supported log files command", nRetVal);

	nRetVal = xnLinkParseSupportedLogFiles(pSupportedLogFiles, nResponseSize, supportedFiles);
	XN_IS_STATUS_OK(nRetVal);

	return XN_STATUS_OK;
}


XnStatus LinkControlEndpoint::GetSupportedBistTests(xnl::Array<XnBistInfo>& supportedTests)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting supported BIST tests list...");

	XnLinkSupportedBistTests* pSupportedBistTests = 
		reinterpret_cast<XnLinkSupportedBistTests*>(m_pIncomingResponse);
    XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_SUPPORTED_BIST_TESTS, nResponseSize, pSupportedBistTests);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get supported bist tests command", nRetVal);

	nRetVal = xnLinkParseSupportedBistTests(pSupportedBistTests, nResponseSize, supportedTests);
	XN_IS_STATUS_OK(nRetVal);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::ExecuteBistTests(XnUInt32 nID, uint32_t& errorCode, uint32_t& extraDataSize, uint8_t* extraData)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Executing BIST %u...", nID);

	XnUInt32 nResponseSize = m_nMaxResponseSize;

	XnLinkExecuteBistParams executeBistParams;
	executeBistParams.m_nID = XN_PREPARE_VAR32_IN_BUFFER(nID);

	nRetVal = ExecuteCommand(XN_LINK_MSG_EXECUTE_BIST_TESTS, 0, &executeBistParams, sizeof(executeBistParams),
		m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute BIST command", nRetVal);

	XnLinkExecuteBistResponse* pExecuteBistResponse = (XnLinkExecuteBistResponse*)m_pIncomingResponse;

	if (nResponseSize < sizeof(pExecuteBistResponse->m_nErrorCode) + sizeof(pExecuteBistResponse->m_nExtraDataSize))
	{
		xnLogError(XN_MASK_LINK, "LINK: Response struct for test is smaller than header (%u instead of %u)", nResponseSize, sizeof(pExecuteBistResponse->m_nErrorCode) + sizeof(pExecuteBistResponse->m_nExtraDataSize));
		return XN_STATUS_OUTPUT_BUFFER_OVERFLOW;
	}

	if (extraDataSize < nResponseSize)
	{
		xnLogError(XN_MASK_LINK, "LINK: Response struct for test is too small (%u instead of %u)", extraDataSize, nResponseSize);
		return XN_STATUS_OUTPUT_BUFFER_OVERFLOW;
	}

	XnUInt32 nExtraDataSize = XN_PREPARE_VAR32_IN_BUFFER(pExecuteBistResponse->m_nExtraDataSize);
	if (nExtraDataSize > nResponseSize - sizeof(pExecuteBistResponse->m_nErrorCode) - sizeof(pExecuteBistResponse->m_nExtraDataSize))
	{
		xnLogError(XN_MASK_LINK, "LINK: Extra data size is invalid (%u. response size: %u)", nExtraDataSize, nResponseSize);
		return XN_STATUS_OUTPUT_BUFFER_OVERFLOW;
	}

	errorCode = XN_PREPARE_VAR32_IN_BUFFER(pExecuteBistResponse->m_nErrorCode);
	extraDataSize = nExtraDataSize;
	xnOSMemCopy(extraData, pExecuteBistResponse->m_ExtraData, nExtraDataSize);

	xnLogInfo(XN_MASK_LINK, "LINK: BIST %u completed with error code %u", nID, errorCode);
	
	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::WriteI2C(XnUInt8 nDeviceID, XnUInt8 nAddressSize, XnUInt32 nAddress, XnUInt8 nValueSize, XnUInt32 nValue, XnUInt32 nMask)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Writing to I2C device %u...", nDeviceID);

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkWriteI2CParams writeI2CParams;

	writeI2CParams.m_nDeviceID = nDeviceID;
	writeI2CParams.m_nAddressSize = nAddressSize;
	writeI2CParams.m_nValueSize = nValueSize;
	writeI2CParams.m_nReserved = 0;
	writeI2CParams.m_nAddress = XN_PREPARE_VAR32_IN_BUFFER(nAddress);
	writeI2CParams.m_nValue = XN_PREPARE_VAR32_IN_BUFFER(nValue);
	writeI2CParams.m_nMask = XN_PREPARE_VAR32_IN_BUFFER(nMask);
	nRetVal = ExecuteCommand(XN_LINK_MSG_WRITE_I2C, XN_LINK_STREAM_ID_NONE, &writeI2CParams, sizeof(writeI2CParams), m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute Write I2C command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: I2C writing completed");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::ReadI2C(XnUInt8 nDeviceID, XnUInt8 nAddressSize, XnUInt32 nAddress, XnUInt8 nValueSize, XnUInt32& nValue)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Reading from I2C device %u...", nDeviceID);

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkReadI2CParams readI2CParams;
	XnLinkReadI2CResponse* pReadI2CResponse = reinterpret_cast<XnLinkReadI2CResponse*>(m_pIncomingResponse);

	readI2CParams.m_nDeviceID = nDeviceID;
	readI2CParams.m_nAddressSize = nAddressSize;
	readI2CParams.m_nValueSize = nValueSize;
	readI2CParams.m_nAddress = XN_PREPARE_VAR32_IN_BUFFER(nAddress);
	nRetVal = ExecuteCommand(XN_LINK_MSG_READ_I2C, XN_LINK_STREAM_ID_NONE, &readI2CParams, sizeof(readI2CParams), pReadI2CResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute Read I2C command", nRetVal);
	if (nResponseSize != sizeof(XnLinkReadI2CResponse))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of readI2C response: %u instead of %u", nResponseSize, sizeof(XnLinkReadI2CResponse));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}
	nValue = XN_PREPARE_VAR32_IN_BUFFER(pReadI2CResponse->m_nValue);

	xnLogInfo(XN_MASK_LINK, "LINK: I2C reading completed");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::WriteAHB(XnUInt32 nAddress, XnUInt32 nValue, XnUInt8 nBitOffset, XnUInt8 nBitWidth)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Writing to AHB register...");

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkWriteAHBParams writeAHBParams;
	writeAHBParams.m_nAddress = XN_PREPARE_VAR32_IN_BUFFER(nAddress);
	writeAHBParams.m_nValue = XN_PREPARE_VAR32_IN_BUFFER(nValue);
	writeAHBParams.m_nBitOffset = nBitOffset;
	writeAHBParams.m_nBitWidth = nBitWidth;
	nRetVal = ExecuteCommand(XN_LINK_MSG_WRITE_AHB, XN_LINK_STREAM_ID_NONE, &writeAHBParams, sizeof(writeAHBParams), m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute Write AHB command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: AHB writing completed");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::ReadAHB(XnUInt32 nAddress, XnUInt8 nBitOffset, XnUInt8 nBitWidth, XnUInt32& nValue)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Reading from AHB register...");

    XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkReadAHBParams readAHBParams;
	XnLinkReadAHBResponse* pReadAHBResponse = reinterpret_cast<XnLinkReadAHBResponse*>(m_pIncomingResponse);
	readAHBParams.m_nAddress = XN_PREPARE_VAR32_IN_BUFFER(nAddress);
	readAHBParams.m_nBitOffset = nBitOffset;
	readAHBParams.m_nBitWidth = nBitWidth;
	nRetVal = ExecuteCommand(XN_LINK_MSG_READ_AHB, XN_LINK_STREAM_ID_NONE, &readAHBParams, sizeof(readAHBParams), pReadAHBResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute read AHB command", nRetVal);
	if (nResponseSize != sizeof(XnLinkReadAHBResponse))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of ReadAHB response: %u instead of %u", nResponseSize, sizeof(XnLinkReadAHBResponse));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}
	nValue = XN_PREPARE_VAR32_IN_BUFFER(pReadAHBResponse->m_nValue);

	xnLogInfo(XN_MASK_LINK, "LINK: AHB reading completed");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetShiftToDepthConfig(XnUInt16 nStreamID, XnShiftToDepthConfig& shiftToDepthConfig)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting shift-to-depth configuration...");

    XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkGetShiftToDepthConfigResponse* pGetS2DConfigResponse = 
		reinterpret_cast<XnLinkGetShiftToDepthConfigResponse*>(m_pIncomingResponse);
	XnLinkShiftToDepthConfig* pLinkS2DConfig = &(pGetS2DConfigResponse->m_config);
	nRetVal = ExecuteCommand(XN_LINK_MSG_GET_S2D_CONFIG, nStreamID, NULL, 0, 
		pGetS2DConfigResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get s2d config command", nRetVal);

	xnLinkParseShiftToDepthConfig(shiftToDepthConfig, *pLinkS2DConfig);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SetVideoMode(XnUInt16 nStreamID, const XnFwStreamVideoMode& videoMode)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Setting video mode for stream %u...", nStreamID);

	XnLinkVideoMode linkVideoMode;
	xnLinkEncodeVideoMode(linkVideoMode, videoMode);
	nRetVal = SetGeneralProperty(nStreamID, XN_LINK_PROP_ID_VIDEO_MODE, sizeof(linkVideoMode), &linkVideoMode);
	XN_IS_STATUS_OK_LOG_ERROR("set map output mode property", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Video mode set for stream %u", nStreamID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetVideoMode(XnUInt16 nStreamID, XnFwStreamVideoMode& videoMode)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting video mode for stream %u...", nStreamID);

    XnLinkVideoMode linkVideoMode;
    XnUInt32 nPropSize = sizeof(linkVideoMode);

    nRetVal = GetGeneralProperty(nStreamID, XN_LINK_PROP_ID_VIDEO_MODE, nPropSize, &linkVideoMode);
    XN_IS_STATUS_OK_LOG_ERROR("Get map output mode property", nRetVal);
    
	if (nPropSize != sizeof(linkVideoMode))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of link map output mode: %u instead of %u", nPropSize, sizeof(linkVideoMode));
		XN_ASSERT(FALSE);
		return XN_STATUS_INVALID_BUFFER_SIZE;
	}

	xnLinkParseVideoMode(videoMode, linkVideoMode);

	XnChar strVideoMode[200];
	xnLinkVideoModeToString(videoMode, strVideoMode, sizeof(strVideoMode));
	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u video mode: %s", nStreamID, strVideoMode);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetSupportedVideoModes(XnUInt16 nStreamID, 
														 xnl::Array<XnFwStreamVideoMode>& supportedVideoModes)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting supported video modes for stream %u...", nStreamID);

    XnUInt8 supportedMapOutputModesBuff[MAX_PROP_SIZE];
    XnUInt32 nPropSize = sizeof(supportedMapOutputModesBuff);
	XnLinkSupportedVideoModes* pLinkSupportedMapOutputModes = reinterpret_cast<XnLinkSupportedVideoModes*>(supportedMapOutputModesBuff);
	XnUInt32 nModes = 0;
	XnUInt32 nExpectedPropSize = 0;

	nRetVal = GetGeneralProperty(nStreamID, XN_LINK_PROP_ID_SUPPORTED_VIDEO_MODES, nPropSize, pLinkSupportedMapOutputModes);
	XN_IS_STATUS_OK_LOG_ERROR("Execute Get Map Output Mode Command", nRetVal);
	nModes = XN_PREPARE_VAR32_IN_BUFFER(pLinkSupportedMapOutputModes->m_nNumModes);
	nExpectedPropSize = (sizeof(pLinkSupportedMapOutputModes->m_nNumModes) + 
        (sizeof(pLinkSupportedMapOutputModes->m_supportedVideoModes[0]) * nModes));
	if (nPropSize != nExpectedPropSize)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of 'supported map output modes' property: %u instead of %u", nPropSize, nExpectedPropSize);
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nRetVal = supportedVideoModes.SetSize(nModes);
	XN_IS_STATUS_OK_LOG_ERROR("Set size of output supported map output modes array", nRetVal);
	for (XnUInt32 i = 0; i < nModes; i++)
	{
		xnLinkParseVideoMode(supportedVideoModes[i], pLinkSupportedMapOutputModes->m_supportedVideoModes[i]);
	}
	
	return XN_STATUS_OK;
}


XnStatus LinkControlEndpoint::EnumerateStreams(xnl::Array<XnFwStreamInfo>& aStreamInfos)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting the list of supported streams...");

	XnLinkEnumerateStreamsResponse* pEnumerateNodesResponse = 
		reinterpret_cast<XnLinkEnumerateStreamsResponse*>(m_pIncomingResponse);
    XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnUInt32 nExpectedResponseSize = 0;
	XnUInt32 nNumNodes = 0;

	nRetVal = ExecuteCommand(XN_LINK_MSG_ENUMERATE_STREAMS, XN_LINK_STREAM_ID_NONE, NULL, 0,
		pEnumerateNodesResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute enumerate nodes command", nRetVal);
	if (nResponseSize < sizeof(pEnumerateNodesResponse->m_nNumStreams))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got insufficient bytes in enumerate nodes response");
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}
	nNumNodes = XN_PREPARE_VAR32_IN_BUFFER(pEnumerateNodesResponse->m_nNumStreams);
	nExpectedResponseSize = (sizeof(pEnumerateNodesResponse->m_nNumStreams) + 
		(nNumNodes * sizeof(pEnumerateNodesResponse->m_streamInfos[0])));
	if (nResponseSize != nExpectedResponseSize)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got incorrect size of enumerate nodes response: expected %u but got %u",
			nExpectedResponseSize, nResponseSize);
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nRetVal = aStreamInfos.SetSize(nNumNodes);
	XN_IS_STATUS_OK_LOG_ERROR("Allocate node infos array", nRetVal);
	for (XnUInt32 i = 0; i < nNumNodes; i++)
	{
		aStreamInfos[i].type = (XnFwStreamType)XN_PREPARE_VAR32_IN_BUFFER(pEnumerateNodesResponse->m_streamInfos[i].m_nStreamType);
		XN_COMPILER_ASSERT(sizeof(aStreamInfos[i].creationInfo) >= sizeof(pEnumerateNodesResponse->m_streamInfos[i].m_strCreationInfo));
		xnOSStrCopy(aStreamInfos[i].creationInfo, 
			pEnumerateNodesResponse->m_streamInfos[i].m_strCreationInfo, 
			sizeof(aStreamInfos[i].creationInfo));
	}

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::CreateInputStream(XnStreamType streamType, const XnChar* strCreationInfo, XnUInt16& nStreamID, XnUInt16& nEndpointID)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Creating stream...");

	XnLinkCreateStreamParams createStreamParams = {0};
	XnLinkCreateStreamResponse* pCreateStreamResponse = reinterpret_cast<XnLinkCreateStreamResponse*>(m_pIncomingResponse);
    XnUInt32 nResponseSize = m_nMaxResponseSize;
	createStreamParams.m_nStreamType = XN_PREPARE_VAR32_IN_BUFFER(streamType);
	xnOSStrCopy(createStreamParams.m_strCreationInfo, strCreationInfo, sizeof(createStreamParams.m_strCreationInfo));
	nRetVal = ExecuteCommand(XN_LINK_MSG_CREATE_STREAM, XN_LINK_STREAM_ID_NONE, &createStreamParams, sizeof(createStreamParams), 
		pCreateStreamResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute create stream command", nRetVal);
	if (nResponseSize != sizeof(*pCreateStreamResponse))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got incorrect size of create nodes response: got %u but expected %u.",
			nResponseSize, sizeof(*pCreateStreamResponse));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nStreamID = XN_PREPARE_VAR16_IN_BUFFER(pCreateStreamResponse->m_nStreamID);
    nEndpointID = XN_PREPARE_VAR16_IN_BUFFER(pCreateStreamResponse->m_nEndpointID);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u created on endpoint %u", nStreamID, nEndpointID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::DestroyInputStream(XnUInt16 nStreamID)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Destroying stream %u...", nStreamID);

    XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_DESTROY_STREAM, nStreamID, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute destroy stream command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u destroyed", nStreamID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SetProperty(XnUInt16 nStreamID, XnLinkPropType propType, XnLinkPropID propID, XnUInt32 nSize, const void* pSource)
{
	XnStatus nRetVal = XN_STATUS_OK;
    XnUInt32 nResponseSize = m_nMaxResponseSize;

	const XnUInt32 nMaxSize = 500;
	XN_ASSERT(nSize < nMaxSize);

	XnUChar message[nMaxSize];

	XnLinkPropVal* pSetPropParams = (XnLinkPropVal*)message;
	pSetPropParams->m_header.m_nPropType = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)propType);
	pSetPropParams->m_header.m_nPropID = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)propID);
	pSetPropParams->m_header.m_nValueSize = XN_PREPARE_VAR32_IN_BUFFER(nSize);
	xnOSMemCopy(pSetPropParams->m_value, pSource, nSize);
	nRetVal = ExecuteCommand(XN_LINK_MSG_SET_PROP, nStreamID, pSetPropParams, sizeof(pSetPropParams->m_header) + nSize,
		m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute set property command", nRetVal);
	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetProperty(XnUInt16 nStreamID, XnLinkPropType propType, XnLinkPropID propID, XnUInt32& nSize, void* pDest)
{
	XnStatus nRetVal = XN_STATUS_OK;
    XnUInt32 nResponseSize = m_nMaxResponseSize;

	XnLinkGetPropParams getPropParams;
	getPropParams.m_nPropType = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)propType);
	getPropParams.m_nPropID = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)propID);
	nRetVal = ExecuteCommand(XN_LINK_MSG_GET_PROP, nStreamID, &getPropParams, sizeof(getPropParams),
		m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get property command", nRetVal);

	XnLinkGetPropResponse* pResponse = (XnLinkGetPropResponse*)m_pIncomingResponse;
	XnUInt32 nValueSize = XN_PREPARE_VAR32_IN_BUFFER(pResponse->m_header.m_nValueSize);

	if (nSize < nValueSize)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got incorrect size for property: got %u but expected a max of %u.",
			nValueSize, nSize);
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	xnOSMemCopy(pDest, pResponse->m_value, nValueSize);
	nSize = nValueSize;

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SetIntProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnUInt64 nValue)
{
	XnUInt64 nProtocolValue = XN_PREPARE_VAR64_IN_BUFFER(nValue);
	return SetProperty(nStreamID, XN_LINK_PROP_TYPE_INT, propID, sizeof(nProtocolValue), &nProtocolValue);
}

XnStatus LinkControlEndpoint::GetIntProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnUInt64& nValue)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	XnUInt64 nProtocolValue;
	XnUInt32 nValueSize = sizeof(nProtocolValue);
	nRetVal = GetProperty(nStreamID, XN_LINK_PROP_TYPE_INT, propID, nValueSize, &nProtocolValue);
	XN_IS_STATUS_OK(nRetVal);

	if (nValueSize != sizeof(nProtocolValue))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got incorrect size for int property: got %u but expected %u.",
			nValueSize, sizeof(nProtocolValue));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nValue = XN_PREPARE_VAR64_IN_BUFFER(nProtocolValue);
	
	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::SetRealProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnDouble dValue)
{
	XnDouble dProtocolValue = XN_PREPARE_VAR_FLOAT_IN_BUFFER(dValue);
	return SetProperty(nStreamID, XN_LINK_PROP_TYPE_REAL, propID, sizeof(dProtocolValue), &dProtocolValue);
}

XnStatus LinkControlEndpoint::GetRealProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnDouble& dValue)
{
	XnStatus nRetVal = XN_STATUS_OK;

	XnDouble dProtocolValue = 0;
	XnUInt32 nValueSize = sizeof(dProtocolValue);
	nRetVal = GetProperty(nStreamID, XN_LINK_PROP_TYPE_REAL, propID, nValueSize, &dProtocolValue);
	XN_IS_STATUS_OK(nRetVal);

	if (nValueSize != sizeof(dProtocolValue))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got incorrect size for int property: got %u but expected %u.",
			nValueSize, sizeof(dProtocolValue));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	dValue = XN_PREPARE_VAR_FLOAT_IN_BUFFER(dProtocolValue);

	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::SetStringProperty(XnUInt16 nStreamID, XnLinkPropID propID, const XnChar* strValue)
{
	return SetProperty(nStreamID, XN_LINK_PROP_TYPE_STRING, propID, xnOSStrLen(strValue)+1, strValue);
}

XnStatus LinkControlEndpoint::GetStringProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnUInt32 nSize, XnChar* strValue)
{
	return GetProperty(nStreamID, XN_LINK_PROP_TYPE_STRING, propID, nSize, strValue);
}

XnStatus LinkControlEndpoint::SetGeneralProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnUInt32 nSize, const void* pSource)
{
	return SetProperty(nStreamID, XN_LINK_PROP_TYPE_GENERAL, propID, nSize, pSource);
}

XnStatus LinkControlEndpoint::GetGeneralProperty(XnUInt16 nStreamID, XnLinkPropID propID, XnUInt32& nSize, void* pDest)
{
	return GetProperty(nStreamID, XN_LINK_PROP_TYPE_GENERAL, propID, nSize, pDest);
}

XnStatus LinkControlEndpoint::GetBitSetProperty(XnUInt16 nStreamID, XnLinkPropID propID, xnl::BitSet& bitSet)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	static const XnUInt32 MAX_SIZE = 512;
	XnChar strBuffer[MAX_SIZE];
	XnUInt32 nSize = MAX_SIZE;
	XnLinkBitSet* pBitSet = (XnLinkBitSet*)strBuffer;

	nRetVal = GetGeneralProperty(nStreamID, propID, nSize, pBitSet);
	XN_IS_STATUS_OK(nRetVal);

	// check header
	if (nSize < sizeof(pBitSet->m_nSize))
	{
		xnLogError(XN_MASK_LINK, "LINK: Bad property value - bit set has no header!");
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	XnUInt32 nBitSetSize = XN_PREPARE_VAR32_IN_BUFFER(pBitSet->m_nSize);

	// check all data is here
	if ((nSize - sizeof(pBitSet->m_nSize)) < nBitSetSize)
	{
		xnLogError(XN_MASK_LINK, "LINK: Bad property value - bit set size should be %u, but got only %u.", nBitSetSize, nSize - sizeof(pBitSet->m_nSize));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	nRetVal = bitSet.SetData(pBitSet->m_aData, nBitSetSize);
	XN_IS_STATUS_OK(nRetVal);
	
	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::GetCameraIntrinsics(XnUInt16 nStreamID, XnLinkCameraIntrinsics& cameraIntrinsics)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting camera intrinsics for stream %u...", nStreamID);

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkCameraIntrinsics* pLinkIntrinsics = reinterpret_cast<XnLinkCameraIntrinsics*>(m_pIncomingResponse);

	nRetVal = ExecuteCommand(XN_LINK_MSG_GET_CAMERA_INTRINSICS, nStreamID, NULL, 0, pLinkIntrinsics, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get FOV command", nRetVal);

	if (nResponseSize != sizeof(XnLinkCameraIntrinsics))
	{
		xnLogError(XN_MASK_LINK, "LINK: Got bad size of get fov response: %u instead of %u", nResponseSize, sizeof(XnLinkCameraIntrinsics));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_RESPONSE_SIZE;
	}

	cameraIntrinsics.m_nOpticalCenterX = XN_PREPARE_VAR16_IN_BUFFER(pLinkIntrinsics->m_nOpticalCenterX);
	cameraIntrinsics.m_nOpticalCenterY = XN_PREPARE_VAR16_IN_BUFFER(pLinkIntrinsics->m_nOpticalCenterY);
	cameraIntrinsics.m_fEffectiveFocalLengthInPixels = XN_PREPARE_VAR_FLOAT_IN_BUFFER(pLinkIntrinsics->m_fEffectiveFocalLengthInPixels);

	return XN_STATUS_OK;		
}

XnStatus LinkControlEndpoint::ValidateResponsePacket(const LinkPacketHeader* pPacketHeader, 
													 XnUInt16 nExpectedMsgType,
													 XnUInt16 nExpectedStreamID,
													 XnUInt32 nBytesToRead)
{
	XnStatus nRetVal = XN_STATUS_OK;
	nRetVal = pPacketHeader->Validate(nBytesToRead);
	XN_IS_STATUS_OK_LOG_ERROR("Validate response packet header", nRetVal);

	if (pPacketHeader->GetMsgType() != nExpectedMsgType)
	{
		xnLogError(XN_MASK_LINK, "LINK: Expected msg type of 0x%X but got 0x%X", nExpectedMsgType, pPacketHeader->GetMsgType());
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_RESPONSE_MSG_TYPE_MISMATCH;
	}

	if (pPacketHeader->GetStreamID() != nExpectedStreamID)
	{
		xnLogError(XN_MASK_LINK, "LINK: Got response packet for stream %u but expected stream %u", pPacketHeader->GetStreamID(), nExpectedStreamID);
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_STREAM_ID;
	}

	if (pPacketHeader->GetPacketID() != m_nPacketID)
	{
		xnLogError(XN_MASK_LINK, "LINK: Expected packet ID of %u in response but got %u on stream %u", 
			m_nPacketID, pPacketHeader->GetPacketID(), pPacketHeader->GetStreamID());
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_PACKETS_LOST;
	}
	
	if (pPacketHeader->GetSize() < sizeof(XnLinkResponseHeader))
	{
		xnLogError(XN_MASK_LINK, "LINK: Response packet size of %u is too small - min response packet size is %u", 
			pPacketHeader->GetSize(), sizeof(XnLinkResponseHeader));
		XN_ASSERT(FALSE);
		return XN_STATUS_LINK_BAD_HEADER_SIZE;
	}

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::BeginUpload()
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Beginning upload session...");

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_BEGIN_UPLOAD, XN_LINK_STREAM_ID_NONE, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute begin upload command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Upload session started");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::EndUpload()
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Ending upload session...");

    XnUInt32 nResponseSize = m_nMaxResponseSize;
	nRetVal = ExecuteCommand(XN_LINK_MSG_END_UPLOAD, XN_LINK_STREAM_ID_NONE, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute end upload command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Upload session ended");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SetCropping(XnUInt16 nStreamID, const OniCropping& cropping)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Setting cropping for stream %u...", nStreamID);

	XnLinkCropping linkCropping;
	linkCropping.m_bEnabled = XnUInt8(cropping.enabled);
	linkCropping.m_nReserved1 = 0;
	linkCropping.m_nReserved2 = 0;
	linkCropping.m_nReserved3 = 0;
	linkCropping.m_nXOffset = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)cropping.originX);
	linkCropping.m_nYOffset = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)cropping.originY);
	linkCropping.m_nXSize   = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)cropping.width);
	linkCropping.m_nYSize   = XN_PREPARE_VAR16_IN_BUFFER((XnUInt16)cropping.height);

	nRetVal = SetGeneralProperty(nStreamID, XN_LINK_PROP_ID_CROPPING, sizeof(linkCropping), &linkCropping);
	XN_IS_STATUS_OK_LOG_ERROR("Set cropping property", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u cropping set", nStreamID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetCropping(XnUInt16 nStreamID, OniCropping& cropping)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting cropping for stream %u...", nStreamID);

	XnLinkCropping linkCropping;
	XnUInt32 nPropSize = sizeof(linkCropping);

	nRetVal = GetGeneralProperty(nStreamID, XN_LINK_PROP_ID_CROPPING, nPropSize, &linkCropping);
	XN_IS_STATUS_OK_LOG_ERROR("Get cropping property", nRetVal);
	if (nPropSize != sizeof(linkCropping))
	{
		xnLogError(XN_MASK_LINK, "LINK: Incorrect size of cropping data: expected %u but got %u", sizeof(linkCropping), nPropSize);
		XN_ASSERT(FALSE);
		return XN_STATUS_INVALID_BUFFER_SIZE;
	}

	cropping.enabled = linkCropping.m_bEnabled;
	cropping.originX = XN_PREPARE_VAR16_IN_BUFFER(linkCropping.m_nXOffset);
	cropping.originY = XN_PREPARE_VAR16_IN_BUFFER(linkCropping.m_nYOffset);
	cropping.width   = XN_PREPARE_VAR16_IN_BUFFER(linkCropping.m_nXSize);
	cropping.height  = XN_PREPARE_VAR16_IN_BUFFER(linkCropping.m_nYSize);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetSupportedMsgTypes(xnl::Array<xnl::BitSet>& supportedMsgTypes)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting supported message types...");

	XnUInt8 supportedMsgTypesBuff[MAX_PROP_SIZE];
	XnUInt32 nBufferSize = sizeof(supportedMsgTypesBuff);
	nRetVal = GetGeneralProperty(XN_LINK_STREAM_ID_NONE, XN_LINK_PROP_ID_SUPPORTED_MSG_TYPES, nBufferSize, supportedMsgTypesBuff);
	XN_IS_STATUS_OK_LOG_ERROR("Get supported msg types property", nRetVal);
	nRetVal = xnLinkParseIDSet(supportedMsgTypes, supportedMsgTypesBuff, nBufferSize);
	XN_IS_STATUS_OK_LOG_ERROR("Parse supported msg types", nRetVal);
	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetSupportedProperties(xnl::Array<xnl::BitSet>& supportedProps)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting supported properties...");

	XnUInt8 supportedPropsBuff[MAX_PROP_SIZE];
	XnUInt32 nBufferSize = sizeof(supportedPropsBuff);
	nRetVal = GetGeneralProperty(XN_LINK_STREAM_ID_NONE, XN_LINK_PROP_ID_SUPPORTED_PROPS, nBufferSize, supportedPropsBuff);
	XN_IS_STATUS_OK_LOG_ERROR("Get supported msg types property", nRetVal);
	nRetVal = xnLinkParseIDSet(supportedProps, supportedPropsBuff, nBufferSize);
	XN_IS_STATUS_OK_LOG_ERROR("Parse supported msg types", nRetVal);
	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetSupportedInterfaces(XnUInt16 nStreamID, xnl::BitSet& supportedInterfaces)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting supported interfaces for stream %u...", nStreamID);

	XnUInt8 supportedInterfacesBuff[MAX_PROP_SIZE];
	XnUInt32 nBufferSize = sizeof(supportedInterfacesBuff);
	nRetVal = GetGeneralProperty(nStreamID, XN_LINK_PROP_ID_STREAM_SUPPORTED_INTERFACES, nBufferSize, supportedInterfacesBuff);
	XN_IS_STATUS_OK_LOG_ERROR("Get supported interfaces", nRetVal);
	nRetVal = xnLinkParseBitSetProp(XN_LINK_PROP_TYPE_GENERAL, supportedInterfacesBuff, nBufferSize, supportedInterfaces);
	XN_IS_STATUS_OK_LOG_ERROR("Parse supported interfaces", nRetVal);
	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SetEmitterActive(XnBool bActive)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	xnLogVerbose(XN_MASK_LINK, "LINK: Turning emitter %s...", bActive ? "on" : "off");

    nRetVal = SetIntProperty(XN_LINK_STREAM_ID_NONE, XN_LINK_PROP_ID_EMITTER_ACTIVE, XnUInt64(bActive));
	XN_IS_STATUS_OK(nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Emitter was turned %s", bActive ? "on" : "off");

	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::GetStreamFragLevel(XnUInt16 nStreamID, XnStreamFragLevel& streamFragLevel)
{
    XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting stream %u fragmentation level...", nStreamID);

    XnUInt64 nTempStreamFragLevel = 0;
    nRetVal = GetIntProperty(nStreamID, XN_LINK_PROP_ID_STREAM_FRAG_LEVEL, nTempStreamFragLevel);
    XN_IS_STATUS_OK_LOG_ERROR("Get int property", nRetVal);
    streamFragLevel = XnStreamFragLevel(nTempStreamFragLevel);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u fragmentation is %s", nStreamID, xnFragmentationFlagsToStr((XnLinkFragmentation)streamFragLevel));

    return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetMirror(XnUInt16 nStreamID, XnBool& bMirror)
{
	XnUInt64 nValue;

	xnLogVerbose(XN_MASK_LINK, "LINK: Checking if stream %u is mirrored...", nStreamID);

	XnStatus nRetVal = GetIntProperty(nStreamID, XN_LINK_PROP_ID_MIRROR, nValue);
	XN_IS_STATUS_OK(nRetVal);
	bMirror = (nValue == TRUE);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u is %smirrored", nStreamID, bMirror ? "" : "not ");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::SetMirror(XnUInt16 nStreamID, XnBool bMirror)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	xnLogVerbose(XN_MASK_LINK, "LINK: Turning stream %u mirror %s...", nStreamID, bMirror ? "on" : "off");

	nRetVal = SetIntProperty(nStreamID, XN_LINK_PROP_ID_MIRROR, static_cast<XnUInt64>(bMirror));
	XN_IS_STATUS_OK(nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Stream %u mirror was turned %s", nStreamID, bMirror ? "on" : "off");

	return (XN_STATUS_OK);
}

XnStatus LinkControlEndpoint::FormatZone(XnUInt8 nZone)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Formatting zone...", nZone);

	XnUInt32 nResponseSize = m_nMaxResponseSize;
	XnLinkFormatZoneParams formatZoneParams;
	formatZoneParams.m_nZone = nZone;
	nRetVal = ExecuteCommand(XN_LINK_MSG_FORMAT_ZONE, XN_LINK_STREAM_ID_NONE, &formatZoneParams, sizeof(formatZoneParams), m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute Format Zone command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: Zone %u formatted", nZone);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::StartUsbTest()
{
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	xnLogVerbose(XN_MASK_LINK, "LINK: Starting USB test...");

	XnStatus nRetVal = ExecuteCommand(XN_LINK_MSG_START_USB_TEST, 0, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute start usb test command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: USB Test started");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::StopUsbTest()
{
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	xnLogVerbose(XN_MASK_LINK, "LINK: Stopping USB test...");

	XnStatus nRetVal = ExecuteCommand(XN_LINK_MSG_STOP_USB_TEST, 0, NULL, 0, m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK_LOG_ERROR("Execute stop usb test command", nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: USB Test stopped");

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::GetBootStatus(XnBootStatus& bootStatus)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Getting boot status...");

	XnLinkBootStatus* pBootStatus = 
		reinterpret_cast<XnLinkBootStatus*>(m_pIncomingResponse);
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = GetGeneralProperty(XN_LINK_PROP_ID_NONE, XN_LINK_PROP_ID_BOOT_STATUS, nResponseSize, pBootStatus);
	XN_IS_STATUS_OK_LOG_ERROR("Execute get boot status command", nRetVal);

	xnLinkParseBootStatus(bootStatus, *pBootStatus);
	
	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::OpenFWLogFile(XnUInt8 logID, XnUInt16 nLogStreamID)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Enabling FW log file %u...", logID);

	XnLinkLogOpenCloseParams params;
	params.m_nID = logID;
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = ExecuteCommand(XN_LINK_MSG_START_LOG_FILE, nLogStreamID, &params, sizeof(params), m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK(nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: FW log file %u enabled", logID);

	return XN_STATUS_OK;
}

XnStatus LinkControlEndpoint::CloseFWLogFile(XnUInt8 logID, XnUInt16 nLogStreamID)
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnLogVerbose(XN_MASK_LINK, "LINK: Disabling FW log file %u...", logID);

	XnLinkLogOpenCloseParams params;
	params.m_nID = logID;
	XnUInt32 nResponseSize = m_nMaxResponseSize;

	nRetVal = ExecuteCommand(XN_LINK_MSG_STOP_LOG_FILE, nLogStreamID, &params, sizeof(params), m_pIncomingResponse, nResponseSize);
	XN_IS_STATUS_OK(nRetVal);

	xnLogInfo(XN_MASK_LINK, "LINK: FW log file %u disabled", logID);

	return XN_STATUS_OK;
}

}
