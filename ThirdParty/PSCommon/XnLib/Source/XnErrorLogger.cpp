/*****************************************************************************
*                                                                            *
*  PrimeSense PSCommon Library                                               *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of PSCommon.                                            *
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
#include "XnErrorLogger.h"

namespace xnl
{
#if XN_PLATFORM != XN_PLATFORM_WIN32
	XN_THREAD_STATIC char ErrorLogger::m_errorBuffer[ms_bufferSize] = "";
	XN_THREAD_STATIC int ErrorLogger::m_currentEnd = 0;
#endif

	ErrorLogger& ErrorLogger::GetInstance()
	{
		static ErrorLogger el;
		return el;
	}

	void ErrorLogger::Clear()
	{
		SingleBuffer* pBuffer = getBuffer();
		pBuffer->m_currentEnd = 0;
		pBuffer->m_errorBuffer[0] = '\0';
	}

	void ErrorLogger::Append(const XnChar* cpFormat, ...)
	{
		SingleBuffer* pBuffer = getBuffer();

		if (pBuffer->m_currentEnd >= ms_bufferSize)
			return;

		pBuffer->m_errorBuffer[pBuffer->m_currentEnd++] = '\t';
		unsigned int charsWritten;

		va_list args;
		va_start(args, cpFormat);
		XnStatus status = xnOSStrFormatV(pBuffer->m_errorBuffer+pBuffer->m_currentEnd, ms_bufferSize-pBuffer->m_currentEnd, &charsWritten, cpFormat, args);
		va_end(args);

		if (status == XN_STATUS_OK)
		{
			pBuffer->m_currentEnd += charsWritten;
			pBuffer->m_errorBuffer[pBuffer->m_currentEnd++] = '\n';
			pBuffer->m_errorBuffer[pBuffer->m_currentEnd] = '\0';
		}
		else
		{
			pBuffer->m_currentEnd += charsWritten;
		}
	}

	void ErrorLogger::AppendV(const XnChar* cpFormat, va_list args)
	{
		SingleBuffer* pBuffer = getBuffer();

		if (pBuffer->m_currentEnd >= ms_bufferSize)
			return;

		pBuffer->m_errorBuffer[pBuffer->m_currentEnd++] = '\t';
		unsigned int charsWritten;

		if (xnOSStrFormatV(pBuffer->m_errorBuffer + pBuffer->m_currentEnd, ms_bufferSize - pBuffer->m_currentEnd, &charsWritten, cpFormat, args) == XN_STATUS_OK)
		{
			pBuffer->m_currentEnd += charsWritten;
			pBuffer->m_errorBuffer[pBuffer->m_currentEnd++] = '\n';
			pBuffer->m_errorBuffer[pBuffer->m_currentEnd] = '\0';
		}
		else
		{
			pBuffer->m_currentEnd += charsWritten;
		}
	}

	ErrorLogger::ErrorLogger()
	{
		SingleBuffer* pBuffer = getBuffer();

		xnOSMemSet(pBuffer->m_errorBuffer, 0, ms_bufferSize);
		pBuffer->m_currentEnd = 0;
	}

	ErrorLogger::~ErrorLogger()
	{
#if XN_PLATFORM == XN_PLATFORM_WIN32
		while (!m_buffers.IsEmpty())
		{
			SingleBuffer* pBuffer = m_buffers.Begin()->Value();
			XN_DELETE(pBuffer);
			m_buffers.Remove(m_buffers.Begin());
		}
#endif
	}

#if XN_PLATFORM == XN_PLATFORM_WIN32
	ErrorLogger::SingleBuffer* ErrorLogger::getBuffer()
	{
		m_bufferLock.Lock();

		XN_THREAD_ID threadId;
		xnOSGetCurrentThreadID(&threadId);

		// Check if we already have a buffer for this thread
		xnl::Hash<XN_THREAD_ID, SingleBuffer*>::ConstIterator buffer = m_buffers.Find(threadId);
		if (buffer != m_buffers.End())
		{
			SingleBuffer* pBuffer = buffer->Value();
			m_bufferLock.Unlock();
			return pBuffer;
		}

		// Go over list and remove any buffer for which the thread no longer exists
		xnl::List<XN_THREAD_ID> deadThreads;
		for (xnl::Hash<XN_THREAD_ID, SingleBuffer*>::ConstIterator iter = m_buffers.Begin(); iter != m_buffers.End(); ++iter)
		{
			if (xnOSDoesThreadExistByID(iter->Key()) != TRUE)
			{
				deadThreads.AddLast(iter->Key());
			}
		}

		for (xnl::List<XN_THREAD_ID>::ConstIterator deadIter = deadThreads.Begin(); deadIter != deadThreads.End(); ++deadIter)
		{
			SingleBuffer* pBuffer = m_buffers[*deadIter];
			XN_DELETE(pBuffer);
			m_buffers.Remove(*deadIter);
		}

		// Create a buffer for this thread
		SingleBuffer* pNewBuffer = XN_NEW(SingleBuffer);
		m_buffers[threadId] = pNewBuffer;

		m_bufferLock.Unlock();
		return pNewBuffer;
	}
#else
	ErrorLogger::SingleBuffer* ErrorLogger::getBuffer()
	{
		return this;
	}
#endif
}
