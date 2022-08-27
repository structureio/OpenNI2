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
#include "OniRecorder.h"

#include "XnLockGuard.h"

// These come from OniFile/Formats:
#include "Xn16zEmbTablesCodec.h"
#include "XnJpegCodec.h"
#include "XnUncompressedCodec.h"

#include "OniContext.h"
#include "OniProperties.h"
#include "PS1080.h"

ONI_NAMESPACE_IMPLEMENTATION_BEGIN

// NOTE: XnLib does not define UINT*_C like macros for some reason...
#define XN_UINT64_C(x) ((x) + (XN_MAX_UINT64 - XN_MAX_UINT64))
#define XN_UINT32_C(x) ((x) + (XN_MAX_UINT32 - XN_MAX_UINT32))

namespace {

enum PropertyType
{
	PROPERTY_TYPE_GENERAL,
	PROPERTY_TYPE_INTEGER,
	PROPERTY_TYPE_REAL
};

const struct PropertyTable {
    XnUInt32     propertyId;
    const char*  propertyName;
	PropertyType propertyType;
} propertyTable[] = {
    { XN_STREAM_PROPERTY_INPUT_FORMAT,				"InputFormat"         , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_CROPPING_MODE,				"CroppingMode"        , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_WHITE_BALANCE_ENABLED,		"WhiteBalancedEnabled", PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_GAIN,						"Gain"                , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_HOLE_FILTER,				"HoleFilter"          , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_REGISTRATION_TYPE,			"RegistrationType"    , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_AGC_BIN,					"AGCBin"              , PROPERTY_TYPE_GENERAL  },
    { XN_STREAM_PROPERTY_CONST_SHIFT,				"ConstShift"          , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_PIXEL_SIZE_FACTOR,			"PixelSizeFactor"     , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_MAX_SHIFT,					"MaxShift"            , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_PARAM_COEFF,				"ParamCoeff"          , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_SHIFT_SCALE,				"ShiftScale"          , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_S2D_TABLE,					"S2D"                 , PROPERTY_TYPE_GENERAL  },
    { XN_STREAM_PROPERTY_D2S_TABLE,					"D2S"                 , PROPERTY_TYPE_GENERAL  },
    { XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE,		"ZPD"                 , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE,		"ZPPS"                , PROPERTY_TYPE_REAL     },
    { XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE,	"LDDIS"               , PROPERTY_TYPE_REAL     },
    { XN_STREAM_PROPERTY_DCMOS_RCMOS_DISTANCE,		"DCRCDIS"             , PROPERTY_TYPE_REAL     },
    { XN_STREAM_PROPERTY_CLOSE_RANGE,				"CloseRange"          , PROPERTY_TYPE_INTEGER  },
    { XN_STREAM_PROPERTY_PIXEL_REGISTRATION,		"PixelRegistration"   , PROPERTY_TYPE_GENERAL  },
};

const XnSizeT propertyTableItemsCount = sizeof(propertyTable) / sizeof(propertyTable[0]);

} // namespace

/**
 * Is used by Recorder to make it possible to undo failed records.
 *
 * The usage pattern is:
 *
 *  void Recorder::YourMethod() {
 *      // Create an undo point.
 *      Memento undoPoint(this);
 *
 *      // Emit records here. If anything goes wrong - just return from this
 *      // method.
 *
 *      // If anything went okay, release the undo point, so that nothing will
 *      // be undone.
 *      undoPoint.Release();
 *  }
 *
 */
class Recorder::Memento
{
public:
    /**
     * Creates a Memento for the current state of the given Recorder.
     */
    Memento(Recorder* pRecorder)
        : m_pRecorder(pRecorder), m_offset(XN_UINT64_C(0))
    {
    	Reuse();
    }

    /**
     * Destroys the Memento and reverts the Recorder back to the remembered state.
     *
     * @note The undo action will not be done if you have released the Memento.
     */
    ~Memento()
    {
        if (m_needRollback)
        {
            Undo();
        }
    }

    /**
     * Releases the Memento. A released memento will not undo.
     */
    void Release()
    {
        m_needRollback = false;
    }

    /**
     * ReUses the Memento. it will function like a brand-new one.
     */
    void Reuse()
    {
        m_needRollback = true;

        XnStatus status = xnOSTellFile64(m_pRecorder->m_file, &m_offset);
        if (status != XN_STATUS_OK)
        {
            m_pRecorder = NULL;
        }
    }

    /**
     * Explicitly undo emits. Reverts the Recorder back to the remembered state.
     */
    void Undo()
    {
        if (m_pRecorder != NULL)
        {
            xnOSSeekFile64(m_pRecorder->m_file, XN_OS_SEEK_SET, m_offset);
        }
    }

    /**
     * An easy way to get the current position of the file. (and save for later use)
     */
    XnUInt64 GetPosition()
    {
        return m_offset;
    }

    /**
     * An easey way to get the current position of the file. (for later use)
     */
    void SetPosition(XnUInt64 pos)
    {
        if (m_pRecorder != NULL)
        {
            xnOSSeekFile64(m_pRecorder->m_file, XN_OS_SEEK_SET, pos);
        }
    }

private:
    Recorder* m_pRecorder;
    XnUInt64  m_offset;
    XnBool    m_needRollback;
};

Recorder::Recorder(FrameManager& frameManager, xnl::ErrorLogger& errorLogger, OniRecorderHandle handle)
        : m_frameManager(frameManager),
          m_errorLogger(errorLogger), 
          m_handle(handle),
          m_maxId(0),
          m_configurationId(0),
		  m_propertyPriority(ms_priorityNormal),
          m_file(XN_INVALID_FILE_HANDLE),
          m_running(FALSE),
          m_started(FALSE),
          m_wasStarted(FALSE)
{
}

Recorder::~Recorder()
{
    stop();
    detachAllStreams();
    send(Message::MESSAGE_TERMINATE);
    xnOSWaitForThreadExit(m_thread, XN_WAIT_INFINITE);
	xnOSCloseThread(&m_thread);
    if (NULL != m_handle)
    {
        m_handle->pRecorder = NULL;
    }
}

OniStatus Recorder::initialize(const char* fileName)
{
    m_fileName = fileName;

    // Probe if we can actually open the file.
    XN_FILE_HANDLE fileHandle = XN_INVALID_FILE_HANDLE;
    XnStatus status = xnOSOpenFile(
            /* file name  = */ fileName, 
            /* open flags = */ XN_OS_FILE_WRITE | XN_OS_FILE_TRUNCATE, 
            /* out handle = */ &fileHandle);
    if (XN_STATUS_OK != status)
    {
        return ONI_STATUS_ERROR;
    }
    xnOSCloseFile(&fileHandle);

    m_assembler.initialize();   
    
    status = xnOSCreateThread(threadMain, this, &m_thread);
    if (XN_STATUS_OK != status)
    {
        return ONI_STATUS_ERROR;
    }
    
    send(Message::MESSAGE_INITIALIZE);
    return ONI_STATUS_OK;
}

OniStatus Recorder::attachStream(VideoStream& stream, OniBool allowLossyCompression)
{
    if (m_wasStarted)
    {
        return ONI_STATUS_ERROR;
    }

    xnl::LockGuard<AttachedStreams> guard(m_streams);
    VideoStream* pStream = &stream;
    if (m_streams.Find(pStream) == m_streams.End())
    {
        if (ONI_STATUS_OK == pStream->addRecorder(*this))
        {
            m_streams[pStream].nodeId                    = ++m_maxId;
            m_streams[pStream].pCodec                    = NULL;
            m_streams[pStream].allowLossyCompression     = allowLossyCompression;
            m_streams[pStream].frameId                   = 0;
            m_streams[pStream].lastOutputTimestamp       = 0;
            m_streams[pStream].lastInputTimestamp        = 0;
            m_streams[pStream].lastNewDataRecordPosition = 0;
            m_streams[pStream].dataIndex.Clear();
            send(Message::MESSAGE_ATTACH, pStream);
            return ONI_STATUS_OK;
        }
    }
    return ONI_STATUS_ERROR;
}

OniStatus Recorder::detachStream(VideoStream& stream)
{
    xnl::LockGuard<AttachedStreams> guard(m_streams);
    VideoStream* pStream = &stream;
    if (m_streams.End() != m_streams.Find(pStream))
    {
        stream.removeRecorder(*this);
        send(Message::MESSAGE_DETACH, pStream);
        return ONI_STATUS_OK;
    }
    return ONI_STATUS_BAD_PARAMETER;
}

OniStatus Recorder::detachAllStreams()
{
    xnl::LockGuard<AttachedStreams> guard(m_streams);
    for (AttachedStreams::Iterator 
                i = m_streams.Begin(),
                e = m_streams.End();
         i != e; ++i)
    {
        detachStream(*i->Key());
    }
    return ONI_STATUS_OK;
}

OniStatus Recorder::start()
{
    m_wasStarted = true;
    send(Message::MESSAGE_START);
    return ONI_STATUS_OK;
}

void Recorder::stop()
{
    m_started = false;
}

OniStatus Recorder::record(VideoStream& stream, OniFrame& aFrame)
{
    if (!m_started)
    {
        return ONI_STATUS_ERROR;
    }
    xnl::LockGuard< AttachedStreams > guard(m_streams);
    VideoStream* pStream = &stream;
    if (m_streams.Find(pStream) == m_streams.End())
    {
        return ONI_STATUS_BAD_PARAMETER;
    }
    OniFrame* pFrame = &aFrame;
    m_frameManager.addRef(pFrame);
    send(Message::MESSAGE_RECORD, pStream, pFrame);
    return ONI_STATUS_OK;
}

OniStatus Recorder::recordStreamProperty(
            VideoStream&     stream,
            int         propertyId,
            const void* pData, 
            int         dataSize)
{
    xnl::LockGuard< AttachedStreams > guard(m_streams);
    VideoStream* pStream = &stream;
    if (m_streams.Find(pStream) == m_streams.End())
    {
        return ONI_STATUS_BAD_PARAMETER;
    }
    // The original pData will not be valid after this function ends.
    // Free this pointer when soon after handling the PropertyMessage!
    void *newPtr = xnOSMalloc(dataSize);
    xnOSMemCopy(newPtr, pData, dataSize);
    send(Message::MESSAGE_RECORDPROPERTY, pStream, newPtr, propertyId, dataSize, m_propertyPriority);
    return ONI_STATUS_OK;
}

XN_THREAD_PROC Recorder::threadMain(XN_THREAD_PARAM pThreadParam)
{
    Recorder* pSelf = reinterpret_cast<Recorder*>(pThreadParam);
    if (NULL != pSelf)
    {
        pSelf->m_running = TRUE;
        while (pSelf->m_running)
        {
            pSelf->messagePump();
        }
    }
    XN_THREAD_PROC_RETURN(XN_STATUS_OK);
}

void Recorder::messagePump()
{
	XnStatus nRetVal = XN_STATUS_OK;
    Message msg = { Message::MESSAGE_NO_OPERATION, 0, NULL, {NULL}, 0, 0 };

	{
		xnl::LockGuard<MessageQueue> guard(m_queue);
		nRetVal = m_queue.Pop(msg);
	}

    if (XN_STATUS_OK == nRetVal)
    {
        switch (msg.type)
        {
            case Message::MESSAGE_INITIALIZE:
                {
                    onInitialize();
                }
                break;
            case Message::MESSAGE_TERMINATE:
                {
                    onTerminate();
                    m_running = FALSE;
                }
                break;
            case Message::MESSAGE_ATTACH:
                {
                    xnl::LockGuard<AttachedStreams> streamsGuard(m_streams);
                    AttachedStreams::Iterator i = m_streams.Find(msg.pStream);
                    if (i != m_streams.End())
                    {
                        onAttach(i->Value().nodeId, msg.pStream);
                    }
                }
                break;
            case Message::MESSAGE_DETACH:
                {
                    xnl::LockGuard<AttachedStreams> streamsGuard(m_streams);
                    AttachedStreams::Iterator i = m_streams.Find(msg.pStream);
                    if (i != m_streams.End())
                    {
                        onDetach(i->Value().nodeId);
                        XN_DELETE(m_streams[msg.pStream].pCodec);
                        m_streams.Remove(msg.pStream);
                    }
                }
                break;
            case Message::MESSAGE_START:
                {
                    xnl::LockGuard<AttachedStreams> streamsGuard(m_streams);
                    for (AttachedStreams::Iterator 
                            i = m_streams.Begin(),
                            e = m_streams.End();
                        i != e; ++i)
                    {
                        onStart(i->Value().nodeId);
                    }
                    m_started = true;
                }
                break;
            case Message::MESSAGE_RECORD:
                {
                    xnl::LockGuard<AttachedStreams> streamsGuard(m_streams);
                    AttachedStreams::Iterator i = m_streams.Find(msg.pStream);
                    if (i != m_streams.End())
                    {
                        XnCodecBase* pCodec = m_streams[msg.pStream].pCodec;
                        XnUInt32 frameId    = ++m_streams[msg.pStream].frameId;
                        XnUInt64 timestamp  = 0;
                        if (frameId > 1)
                        {
                            timestamp = m_streams[msg.pStream].lastOutputTimestamp + (msg.pFrame->timestamp - m_streams[msg.pStream].lastInputTimestamp);
                        }
                        m_streams[msg.pStream].lastInputTimestamp = msg.pFrame->timestamp;
                        m_streams[msg.pStream].lastOutputTimestamp = timestamp;
                        onRecord(i->Value().nodeId, pCodec, msg.pFrame, frameId, timestamp);
                        m_frameManager.release(msg.pFrame);
                    }
                }
                break;
            case Message::MESSAGE_RECORDPROPERTY:
                {
                    xnl::LockGuard<AttachedStreams> streamsGuard(m_streams);
                    AttachedStreams::Iterator i = m_streams.Find(msg.pStream);
                    if (i != m_streams.End())
                    {
                        onRecordProperty(
                            i->Value().nodeId,
                            msg.propertyId,
                            msg.pData,
                            msg.dataSize);
                    }
                    // free the temporary buffer allocated earlier
                    xnOSFree((void*)msg.pData);
                }
                break;
            default:
                ;
        }
    }
}

void Recorder::send(
            Message::Type type, 
            VideoStream*     pStream, 
            const void* pData,
            XnUInt32    propertyId,
            XnSizeT     dataSize,
			int priority)
{
    Message msg = 
    { 
        type,
        pStream != NULL ? m_streams[pStream].nodeId : 0,
        pStream,
        {pData},
        propertyId,
        dataSize
    };
    xnl::LockGuard<MessageQueue> guard(m_queue);
    m_queue.Push(msg, priority);
}

void Recorder::onInitialize()
{
    XnStatus status = xnOSOpenFile(
        /* file name  = */ m_fileName.Data(), 
        /* open flags = */ XN_OS_FILE_WRITE | XN_OS_FILE_TRUNCATE, 
        /* out handle = */ &m_file);

    if (XN_STATUS_OK == status)
    {
        FileHeaderData fileHeader = 
        {
            /* identity     = */ { 'N', 'I', '1', '0' },
            /* version      = */ {   1,   0,  1,   0  },
            /* maxTimeStamp = */ XN_MAX_UINT64,
            /* maxNodeId    = */ m_maxId,
        };
        m_fileHeader = fileHeader;
        xnOSWriteFile(m_file, &m_fileHeader, sizeof(m_fileHeader));
    }
}

// A handy macro, that makes code more readable.
// Usage:
//
//  void Recorder::YourMethod() {
//      Memento undoPoint(this);
//      EMIT(RECORD_END())  /* #1 */
//      EMIT(RECORD_END())  /* #2 */
//      undoPoint.Release();
//  }
//
// This will safely emit RECORD_END into the file, if either of (#1 or #2)
//  expressions fail, the undoPoint will revert the file to the state, prior
//  to YourMethod execution.
//
#define EMIT(expr)                                              \
    if (ONI_STATUS_OK == (m_assembler.emit_##expr))             \
    {                                                           \
        if (ONI_STATUS_OK != m_assembler.serialize(m_file))     \
        {                                                       \
            return;                                             \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        return;                                                 \
    }

#define FIND_ATTACHED_STREAM_INFO(nodeId) \
    AttachedStreamInfo *pInfo = NULL; \
    xnl::LockGuard<AttachedStreams> guard(m_streams); \
    for (AttachedStreams::Iterator  \
                i = m_streams.Begin(), e = m_streams.End(); \
                i != e;  \
                ++i) \
    { \
        if(i->Value().nodeId == nodeId)  \
        { \
            pInfo = &(i->Value()); \
            break; \
        } \
    }

XnUInt64 Recorder::getLastPropertyRecordPos(XnUInt32 nodeId, const char *propName, XnUInt64 newRecordPos)
{
    XnUInt64 pos = 0;
    FIND_ATTACHED_STREAM_INFO(nodeId)
    if (!pInfo) return 0;

    if (pInfo->lastPropertyRecordPosition.Find(propName) != pInfo->lastPropertyRecordPosition.End())
    {
        pos = pInfo->lastPropertyRecordPosition[propName];
    }
    pInfo->lastPropertyRecordPosition[propName] = newRecordPos;
    return pos;
}

void Recorder::onTerminate()
{
    // Truncate the file to it's last offset, so that undone records
    // will not be serialized.
    XnUInt64 truncationOffset = XN_UINT64_C(0);
    if (XN_STATUS_OK == xnOSTellFile64(m_file, &truncationOffset))
    {
        xnOSTruncateFile64(m_file, truncationOffset);
    }

    Memento undoPoint(this);
    EMIT(RECORD_END())
    undoPoint.Release();

    // The file header needs being patched, because its maxNodeId field has become
    // irrelevant by now.
    m_fileHeader.maxNodeId = m_maxId;
    xnOSSeekFile64(m_file, XN_OS_SEEK_SET, XN_UINT64_C(0));
    xnOSWriteFile(m_file, &m_fileHeader, sizeof(m_fileHeader));

    xnOSCloseFile(&m_file);
    m_file = XN_INVALID_FILE_HANDLE;
}

typedef enum XnPixelFormat
{
    XN_PIXEL_FORMAT_RGB24 = 1,
    XN_PIXEL_FORMAT_YUV422 = 2,
    XN_PIXEL_FORMAT_GRAYSCALE_8_BIT = 3,
    XN_PIXEL_FORMAT_GRAYSCALE_16_BIT = 4,
    XN_PIXEL_FORMAT_MJPEG = 5,
} XnPixelFormat;

typedef struct XnSupportedPixelFormats
{
    XnUInt m_bRGB24 : 1;
    XnUInt m_bYUV422 : 1;
    XnUInt m_bGrayscale8Bit : 1;
    XnUInt m_bGrayscale16Bit : 1;
    XnUInt m_bMJPEG : 1;
    XnUInt m_nPadding : 3;
    XnUInt m_nReserved : 24;
} XnSupportedPixelFormats;

XnPixelFormat toXnPixelFormat(OniPixelFormat oniFormat)
{
    switch(oniFormat)
    {
    // Depth
    case ONI_PIXEL_FORMAT_DEPTH_1_MM:		return XN_PIXEL_FORMAT_GRAYSCALE_16_BIT;
    case ONI_PIXEL_FORMAT_DEPTH_100_UM:	return XN_PIXEL_FORMAT_GRAYSCALE_16_BIT;
    case ONI_PIXEL_FORMAT_SHIFT_9_2:		return XN_PIXEL_FORMAT_GRAYSCALE_16_BIT;
    case ONI_PIXEL_FORMAT_SHIFT_9_3:		return XN_PIXEL_FORMAT_GRAYSCALE_16_BIT;
    
    // Color
    case ONI_PIXEL_FORMAT_RGB888:		return XN_PIXEL_FORMAT_RGB24;
    case ONI_PIXEL_FORMAT_YUV422:		return XN_PIXEL_FORMAT_YUV422;
    case ONI_PIXEL_FORMAT_GRAY8:		return XN_PIXEL_FORMAT_GRAYSCALE_8_BIT;
    case ONI_PIXEL_FORMAT_GRAY16:		return XN_PIXEL_FORMAT_GRAYSCALE_16_BIT;
    case ONI_PIXEL_FORMAT_JPEG:		return XN_PIXEL_FORMAT_MJPEG;
    
    default: 
		//not supported by OpenNI 1.x
        return XnPixelFormat(0);
    }
}

void fillXnSupportedPixelFormats(XnSupportedPixelFormats &xnSPF, OniPixelFormat oniFormat)
{
    xnOSMemSet(&xnSPF, 0, sizeof(xnSPF));
    switch(toXnPixelFormat(oniFormat))
    {
    case XN_PIXEL_FORMAT_RGB24:			xnSPF.m_bRGB24		= 1; break;
    case XN_PIXEL_FORMAT_YUV422:		xnSPF.m_bYUV422		= 1; break;
    case XN_PIXEL_FORMAT_GRAYSCALE_8_BIT:	xnSPF.m_bGrayscale8Bit	= 1; break;
    case XN_PIXEL_FORMAT_GRAYSCALE_16_BIT:	xnSPF.m_bGrayscale16Bit	= 1; break;
    case XN_PIXEL_FORMAT_MJPEG:			xnSPF.m_bMJPEG		= 1; break;
    }
}

void Recorder::onAttach(XnUInt32 nodeId, VideoStream* pStream)
{
    if (nodeId == 0 || pStream == NULL) 
    {
        return;
    }
    const OniSensorInfo* pSensorInfo = pStream->getSensorInfo();
    if (pSensorInfo == NULL)
    {
        return;
    }

    // Assume we'll be using uncompressed codec.
    XnUInt32 codecId = ONI_CODEC_UNCOMPRESSED;

    // Applicable for depth streams only.
    int maxDepth = XN_MAX_UINT16;

    OniVideoMode curVideoMode;
    int size = sizeof(OniVideoMode);
    pStream->getProperty(ONI_STREAM_PROPERTY_VIDEO_MODE, &curVideoMode, &size);

    // Guess codec type from video mode format.
    switch (curVideoMode.pixelFormat)
    {
    case ONI_PIXEL_FORMAT_DEPTH_100_UM:
    case ONI_PIXEL_FORMAT_DEPTH_1_MM:
        {
            size = int(sizeof(maxDepth));

            pStream->getProperty(
                    ONI_STREAM_PROPERTY_MAX_VALUE, &maxDepth, &size);

            m_streams[pStream].pCodec = XN_NEW(
                    Xn16zEmbTablesCodec, static_cast<XnUInt16>(maxDepth));

            codecId = ONI_CODEC_16Z_EMB_TABLES;
        }
        break;
    case ONI_PIXEL_FORMAT_RGB888:
        {
            if (m_streams[pStream].allowLossyCompression)
            {
                m_streams[pStream].pCodec = XN_NEW(
                        XnJpegCodec, 
                        /* bRGB = */ TRUE, 
                        curVideoMode.resolutionX,
                        curVideoMode.resolutionY);

                codecId = ONI_CODEC_JPEG;
            }
            else
            {
                m_streams[pStream].pCodec = XN_NEW(XnUncompressedCodec);
            }
        }
        break;
    default:
        m_streams[pStream].pCodec = XN_NEW(XnUncompressedCodec);
        break;
    }

    // If anything went wrong - fall back to uncompressed format. 
    if (XN_STATUS_OK != m_streams[pStream].pCodec->Init())
    {
        XN_DELETE(m_streams[pStream].pCodec);
        m_streams[pStream].pCodec = NULL;
        codecId = ONI_CODEC_UNCOMPRESSED;
    }
    
    Memento undoPoint(this);
    // save the position of this record so we can override it upon detaching
    m_streams[pStream].nodeAddedRecordPosition = undoPoint.GetPosition();

    EMIT(RECORD_NODE_ADDED(
            m_streams[pStream].nodeType = AsNodeType(pSensorInfo->sensorType),
            nodeId,
            m_streams[pStream].codecId = codecId,
            /* numberOfFrames    = */ XN_MAX_UINT32,
            /* minTimeStamp      = */ XN_UINT64_C(0),
            /* maxTimeStamp      = */ XN_MAX_UINT64,
            /* seekTablePosition = */ XN_UINT64_C(0)
        ))
    undoPoint.Reuse();

	// required data size (for cases where data is larger than video mode)
	EMIT(RECORD_INT_PROPERTY(
		nodeId,
		getLastPropertyRecordPos(nodeId, "oniRequiredFrameSize", undoPoint.GetPosition()),
		"oniRequiredFrameSize",
		pStream->getRequiredFrameSize()
		));
	undoPoint.Reuse();

	// isGenerating (needed for OpenNI 1.x playback)
	EMIT(RECORD_INT_PROPERTY(
		nodeId,
		getLastPropertyRecordPos(nodeId, "xnIsGenerating", undoPoint.GetPosition()),
		"xnIsGenerating",
		TRUE
		));
	undoPoint.Reuse();

    // xnDeviceMaxDepth
    if (curVideoMode.pixelFormat == ONI_PIXEL_FORMAT_DEPTH_1_MM ||
        curVideoMode.pixelFormat == ONI_PIXEL_FORMAT_DEPTH_100_UM)
    {
        EMIT(RECORD_INT_PROPERTY(
                nodeId,
                getLastPropertyRecordPos(nodeId, "xnDeviceMaxDepth", undoPoint.GetPosition()),
                "xnDeviceMaxDepth",
                maxDepth
            ))
    }
    undoPoint.Reuse();

    // xnSupportedMapOutputModesCount
    EMIT(RECORD_INT_PROPERTY(
            nodeId,
            getLastPropertyRecordPos(nodeId, "xnSupportedMapOutputModesCount", undoPoint.GetPosition()),
            "xnSupportedMapOutputModesCount",
            pSensorInfo->numSupportedVideoModes
        ))
    undoPoint.Reuse();

    // xnSupportedMapOutputModes
    VideoModeData* pVideoModes = XN_NEW_ARR(
            VideoModeData, pSensorInfo->numSupportedVideoModes);
    for (int i = 0; i < pSensorInfo->numSupportedVideoModes; ++i)
    {
        const OniVideoMode& videoMode = pSensorInfo->pSupportedVideoModes[i];
        pVideoModes[i].width  = videoMode.resolutionX;
        pVideoModes[i].height = videoMode.resolutionY;
        pVideoModes[i].fps    = videoMode.fps;
    }

    EMIT(RECORD_GENERAL_PROPERTY(
            nodeId,
            getLastPropertyRecordPos(nodeId, "xnSupportedMapOutputModes", undoPoint.GetPosition()),
            "xnSupportedMapOutputModes",
            pVideoModes,
            sizeof(*pVideoModes) * pSensorInfo->numSupportedVideoModes
        ))
    undoPoint.Reuse();

    // xnMapOutputMode
    VideoModeData curVMD;
    curVMD.width  = curVideoMode.resolutionX;
    curVMD.height = curVideoMode.resolutionY;
    curVMD.fps    = curVideoMode.fps;
    EMIT(RECORD_GENERAL_PROPERTY(
            nodeId,
            getLastPropertyRecordPos(nodeId, "xnMapOutputMode", undoPoint.GetPosition()),
            "xnMapOutputMode",
            &curVMD,
            sizeof(curVMD)
        ))
    undoPoint.Reuse();

	XnPixelFormat pixelFormat = toXnPixelFormat(curVideoMode.pixelFormat);
	if (pixelFormat != 0)
	{
		// xnSupportedPixelFormats
		XnSupportedPixelFormats supportedPixelFormats;
		fillXnSupportedPixelFormats(supportedPixelFormats, curVideoMode.pixelFormat);
		EMIT(RECORD_GENERAL_PROPERTY(
			nodeId,
			getLastPropertyRecordPos(nodeId, "xnSupportedPixelFormats", undoPoint.GetPosition()),
			"xnSupportedPixelFormats",
			&supportedPixelFormats,
			sizeof(supportedPixelFormats)
			))
			undoPoint.Reuse();

		// xnPixelFormat
		EMIT(RECORD_INT_PROPERTY(
			nodeId,
			getLastPropertyRecordPos(nodeId, "xnPixelFormat", undoPoint.GetPosition()),
			"xnPixelFormat",
			pixelFormat
			))
			undoPoint.Reuse();
	}

	EMIT(RECORD_INT_PROPERTY(
		nodeId,
		getLastPropertyRecordPos(nodeId, "oniPixelFormat", undoPoint.GetPosition()),
		"oniPixelFormat",
		curVideoMode.pixelFormat
		))
		undoPoint.Reuse();

    XN_DELETE_ARR(pVideoModes);

    size = sizeof(XnFloat);
	float vdummy, hdummy;
    if ( pStream->getProperty(ONI_STREAM_PROPERTY_HORIZONTAL_FOV, &hdummy, &size) == ONI_STATUS_OK &&
         pStream->getProperty(ONI_STREAM_PROPERTY_VERTICAL_FOV,   &vdummy, &size) == ONI_STATUS_OK )
    {
		// xnFOV
		struct XnFieldOfView
		{
			/** Horizontal Field Of View, in radians. */
			XnDouble fHFOV;
			/** Vertical Field Of View, in radians. */
			XnDouble fVFOV;
		} fov = {hdummy, vdummy};

        EMIT(RECORD_GENERAL_PROPERTY(
                nodeId,
                getLastPropertyRecordPos(nodeId, "xnFOV", undoPoint.GetPosition()),
                "xnFOV",
                &fov,
                sizeof(fov)
            ))
        undoPoint.Reuse();
    }

	// xnCropping
	struct XnCropping
	{
		/** TRUE if cropping is turned on, FALSE otherwise. */
		XnBool bEnabled;
		/** Offset in the X-axis, in pixels. */
		XnUInt16 nXOffset;
		/** Offset in the Y-axis, in pixels. */
		XnUInt16 nYOffset;
		/** Number of pixels in the X-axis. */
		XnUInt16 nXSize;
		/** Number of pixels in the Y-axis. */
		XnUInt16 nYSize;
	} xncropping = {0};
	OniCropping cropping;
	size = sizeof(OniCropping);
	if (pStream->getProperty(ONI_STREAM_PROPERTY_CROPPING, &cropping, &size) == ONI_STATUS_OK)
	{
		// we support cropping capability
		EMIT(RECORD_INT_PROPERTY(
			nodeId,
			getLastPropertyRecordPos(nodeId, "Cropping", undoPoint.GetPosition()),
			"Cropping",
			TRUE
			));

		undoPoint.Reuse();

		xncropping.bEnabled = cropping.enabled;
		xncropping.nXOffset = (XnUInt16)cropping.originX;
		xncropping.nYOffset = (XnUInt16)cropping.originY;
		xncropping.nXSize = (XnUInt16)cropping.width;
		xncropping.nYSize = (XnUInt16)cropping.height;

		EMIT(RECORD_GENERAL_PROPERTY(
			nodeId,
			getLastPropertyRecordPos(nodeId, "xnCropping", undoPoint.GetPosition()),
			"xnCropping",
			&xncropping,
			sizeof(xncropping)
			))

		undoPoint.Reuse();
	}

	OniBool bMirror = FALSE;
	size = sizeof(bMirror);
	if (pStream->getProperty(ONI_STREAM_PROPERTY_MIRRORING, &bMirror, &size) == ONI_STATUS_OK)
	{
		// we support mirroring capability
		EMIT(RECORD_INT_PROPERTY(
			nodeId,
			getLastPropertyRecordPos(nodeId, "Mirror", undoPoint.GetPosition()),
			"Mirror",
			TRUE
			));

		undoPoint.Reuse();

		// and now tell the mirror state
		EMIT(RECORD_INT_PROPERTY(
			nodeId,
			getLastPropertyRecordPos(nodeId, "xnMirror", undoPoint.GetPosition()),
			"xnMirror",
			bMirror
			))

		undoPoint.Reuse();
	}

	m_propertyPriority = ms_priorityHigh;
    pStream->notifyAllProperties();
	m_propertyPriority = ms_priorityNormal;
    undoPoint.Release();
}

void Recorder::onDetach(XnUInt32 nodeId)
{
    if (nodeId == 0) 
    {
        return;
    }

    FIND_ATTACHED_STREAM_INFO(nodeId)
    if (!pInfo) return;

    Memento undoPoint(this);
    EMIT(RECORD_NODE_REMOVED(
            nodeId,
            pInfo->nodeAddedRecordPosition
        ))
    undoPoint.Release();

    undoPoint.Reuse();
    XnUInt64 nSeekTablePos = undoPoint.GetPosition();
    // write the seek table
    EMIT(RECORD_SEEK_TABLE(
            nodeId,
            pInfo->frameId,
            pInfo->dataIndex
	))

    undoPoint.Release();

    undoPoint.Reuse();
    // Seek to position of node added record
    undoPoint.SetPosition(pInfo->nodeAddedRecordPosition);

    // re-write this record, this time with seek data
    EMIT(RECORD_NODE_ADDED(
            pInfo->nodeType,
            nodeId,
            pInfo->codecId,
            /* numberOfFrames    = */ pInfo->frameId,
            /* minTimeStamp      = */ XN_UINT64_C(0),
            /* maxTimeStamp      = */ pInfo->lastOutputTimestamp,
            /* seekTablePosition = */ nSeekTablePos
        ))
    undoPoint.Undo();
}

void Recorder::onStart(XnUInt32 nodeId)
{
    if (0 == nodeId)
    {
        return;
    }
    Memento undoPoint(this);
    EMIT(RECORD_NODE_STATE_READY(
            nodeId
        ))

    EMIT(RECORD_NODE_DATA_BEGIN(
            nodeId,
            /* framesCount  = */ XN_MAX_UINT32,
            /* maxTimeStamp = */ XN_MAX_UINT64
        ))
    undoPoint.Release();
}

void Recorder::onRecord(XnUInt32 nodeId, XnCodecBase* pCodec, const OniFrame* pFrame, XnUInt32 frameId, XnUInt64 timestamp)
{
    if (0 == nodeId || NULL == pFrame)
    {
        return;
    }

    FIND_ATTACHED_STREAM_INFO(nodeId)
    if (!pInfo) return;

    Memento undoPoint(this);

    if (NULL != pCodec)
    {
        XnUInt32 bufferSize_bytes32 = pFrame->dataSize * 2 + pCodec->GetOverheadSize();
        XnUInt8* buffer             = XN_NEW_ARR(XnUInt8, bufferSize_bytes32);

        XnStatus status = pCodec->Compress(reinterpret_cast<const XnUChar*>(pFrame->data), 
                pFrame->dataSize, buffer, &bufferSize_bytes32);
                XnSizeT  bufferSize_bytes = bufferSize_bytes32;
        if (XN_STATUS_OK == status)
        {
            EMIT(RECORD_NEW_DATA(
                    nodeId,
                    pInfo->lastNewDataRecordPosition,
                    timestamp,
                    frameId,
                    buffer,
                    bufferSize_bytes))
        }
        XN_DELETE_ARR(buffer);
    }
    else
    {
        EMIT(RECORD_NEW_DATA(
                nodeId,
                pInfo->lastNewDataRecordPosition,
                pFrame->timestamp,
                pFrame->frameIndex,
                pFrame->data,
                pFrame->dataSize
            ))
    }
    undoPoint.Release();
    // save this record's position as the last one
    pInfo->lastNewDataRecordPosition = undoPoint.GetPosition();
    
    // write to seek table
    DataIndexEntry dataIndexEntry;
    dataIndexEntry.nTimestamp = timestamp;
    dataIndexEntry.nConfigurationID = m_configurationId;
    dataIndexEntry.nSeekPos = undoPoint.GetPosition();

    pInfo->dataIndex.AddLast(dataIndexEntry);
}

void Recorder::onRecordProperty(
            XnUInt32    nodeId, 
            XnUInt32    propertyId,
            const void* pData,
            XnSizeT     dataSize)
{
    if (0 == nodeId || NULL == pData || 0 == dataSize)
    {
        return;
    }
    Memento undoPoint(this);
    for (XnSizeT i = 0; i < propertyTableItemsCount; ++i)
    {
        if (propertyTable[i].propertyId == propertyId)
        {
			switch (propertyTable[i].propertyType)
			{
			case PROPERTY_TYPE_INTEGER:
				{
					uint64_t value = *(uint64_t*)pData;
					if (dataSize == sizeof(int))
					{
						value = (uint64_t)*(int*)pData;
					}
					EMIT(RECORD_INT_PROPERTY(
						nodeId,
						getLastPropertyRecordPos(nodeId, propertyTable[i].propertyName, undoPoint.GetPosition()),
						propertyTable[i].propertyName,
						value))
				}
				break;
			case PROPERTY_TYPE_REAL:
				{
					double value = *(double*)pData;
					if (dataSize == sizeof(float))
					{
						value = (double)*(float*)pData;
					}
					EMIT(RECORD_REAL_PROPERTY(
						nodeId,
						getLastPropertyRecordPos(nodeId, propertyTable[i].propertyName, undoPoint.GetPosition()),
						propertyTable[i].propertyName,
						value))
				}
				break;
			case PROPERTY_TYPE_GENERAL:
			default:
				{
					EMIT(RECORD_GENERAL_PROPERTY(
						nodeId,
						getLastPropertyRecordPos(nodeId, propertyTable[i].propertyName, undoPoint.GetPosition()),
						propertyTable[i].propertyName,
						pData,
						dataSize))
				}
			}
        }
    }
    undoPoint.Release();

    // update the global configurationId
    ++m_configurationId;
}

ONI_NAMESPACE_IMPLEMENTATION_END
