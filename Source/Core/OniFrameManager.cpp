#include "OniFrameManager.h"
#include <XnOSCpp.h>
#include <XnLog.h>
#define XN_MASK_ONI_FRAME_MANAGER "MGR"

ONI_NAMESPACE_IMPLEMENTATION_BEGIN

FrameManager::FrameManager()
{
}

FrameManager::~FrameManager()
{
}

OniFrameInternal* FrameManager::acquireFrame()
{
	OniFrameInternal* pFrame = m_frames.Acquire();

	// reset all fields
	pFrame->dataSize = 0;
	pFrame->data = NULL;
	pFrame->sensorType = (OniSensorType)0;
	pFrame->timestamp = 0;
	pFrame->frameIndex = 0;
	pFrame->width = 0;
	pFrame->height = 0;
	pFrame->videoMode.pixelFormat = (OniPixelFormat)0;
	pFrame->videoMode.resolutionX = 0;
	pFrame->videoMode.resolutionY = 0;
	pFrame->videoMode.fps = 0;
	pFrame->croppingEnabled = FALSE;
	pFrame->cropOriginX = 0;
	pFrame->cropOriginY = 0;
	pFrame->stride = 0;
	pFrame->backToPoolFunc = NULL;
	pFrame->backToPoolFuncCookie = NULL;
	pFrame->refCount = 1; // this is the only reference
	pFrame->freeBufferFunc = NULL;
	pFrame->freeBufferFuncCookie = NULL;
	xnLogVerbose(XN_MASK_ONI_FRAME_MANAGER, "Set %d @ %p", pFrame->refCount, (void*)pFrame);

	return pFrame;
}

void FrameManager::addRef(OniFrame* pFrame)
{
	OniFrameInternal* pInternal = (OniFrameInternal*)pFrame;
	m_frames.Lock();
	xnLogVerbose(XN_MASK_ONI_FRAME_MANAGER, "Inc %d @ %p", pInternal->refCount, (void*)pInternal);
	++pInternal->refCount;
	m_frames.Unlock();
}
void FrameManager::release(OniFrame* pFrame)
{
	OniFrameInternal* pInternal = (OniFrameInternal*)pFrame;
	m_frames.Lock();
	xnLogVerbose(XN_MASK_ONI_FRAME_MANAGER, "Dec %d @ %p", pInternal->refCount, (void*)pInternal);
	if (--pInternal->refCount == 0)
	{
		// notify frame is back to pool
        if (pInternal->backToPoolFunc != NULL)
        {
            pInternal->backToPoolFunc(pInternal, pInternal->backToPoolFuncCookie);
        }
        
		// and return frame to pool
		m_frames.Release(pInternal);
	}
	m_frames.Unlock();
}

ONI_NAMESPACE_IMPLEMENTATION_END
