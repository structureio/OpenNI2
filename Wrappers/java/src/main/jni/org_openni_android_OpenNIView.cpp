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
#include "org_openni_android_OpenNIView.h"
#include <OpenNI.h>

#if (ONI_PLATFORM == ONI_PLATFORM_ANDROID_ARM)

#define  GL_GLEXT_PROTOTYPES
#include <GLES/gl.h>
#include <GLES/glext.h>

#define DEBUG 1

#if DEBUG && defined(ANDROID)
#include <android/log.h>
#  define  LOGD(x...)  __android_log_print(ANDROID_LOG_INFO,"OpenNI-JNI",x)
#  define  LOGE(x...)  __android_log_print(ANDROID_LOG_ERROR,"OpenNI-JNI",x)
#else
#  define  LOGD(...)  do {} while (0)
#  define  LOGE(...)  do {} while (0)
#endif

using namespace openni;

static void
throwRuntimeException(JNIEnv *env, const char *message)
{
	jclass exClass;
	const char *className = "java/lang/RuntimeException";

	exClass = env->FindClass(className);
	if (!exClass)
		return;

	env->ThrowNew(exClass, message);
}

class OpenNIView
{
public:
	OpenNIView();
	~OpenNIView();
	
	void update(JNIEnv* env, uint8_t* texture, int textureWidth, int textureHeight, VideoFrameRef& frame);

private:
	void calcDepthHist(VideoFrameRef& frame);
	
	int *m_histogram;
	int m_maxGrayVal;
		
	enum { HISTSIZE = 0xFFFF, };
};

OpenNIView::OpenNIView() :
	m_histogram(NULL), m_maxGrayVal(0)
{
}

OpenNIView::~OpenNIView()
{
	free(m_histogram);
}
	
void YUVtoRGB888(XnUInt8 y, XnUInt8 u, XnUInt8 v, XnUInt8& r, XnUInt8& g, XnUInt8& b)
{
	XnInt32 nC = y - 16;
	XnInt16 nD = u - 128;
	XnInt16 nE = v - 128;

	nC = nC * 298 + 128;

	r = (XnUInt8)XN_MIN(XN_MAX((nC            + 409 * nE) >> 8, 0), 255);
	g = (XnUInt8)XN_MIN(XN_MAX((nC - 100 * nD - 208 * nE) >> 8, 0), 255);
	b = (XnUInt8)XN_MIN(XN_MAX((nC + 516 * nD           ) >> 8, 0), 255);
}

void OpenNIView::update(JNIEnv *env, uint8_t* texture, int textureWidth, int textureHeight, VideoFrameRef& frame)
{
	int xres = frame.getVideoMode().getResolutionX();
	int yres = frame.getVideoMode().getResolutionY();

	// if the frame is cropped, zero the texture beforehand, so we can write only to the cropped pixels
	if (frame.getWidth() < xres || frame.getHeight() < yres)
	{
		memset(texture, 0, textureWidth * textureHeight * 4);
	}

	const uint8_t* pFrameData = (const uint8_t*)frame.getData();

	switch (frame.getVideoMode().getPixelFormat())
	{
	case PIXEL_FORMAT_DEPTH_1_MM:
	case PIXEL_FORMAT_DEPTH_100_UM:
		{
			calcDepthHist(frame);
			
			for (int y = 0; y < frame.getHeight(); ++y)
			{
				uint8_t* pTexture = texture + ((frame.getCropOriginY() + y) * textureWidth + frame.getCropOriginX()) * 4;
				const OniDepthPixel* pDepth = (const OniDepthPixel*)(pFrameData + y * frame.getStrideInBytes());
				for (int x = 0; x < frame.getWidth(); ++x, ++pDepth, pTexture += 4)
				{
					int val = m_histogram[*pDepth];
					pTexture[0] = val;
					pTexture[1] = val;
					pTexture[2] = val;
					pTexture[3] = 255;
				}
			}
		}
		break;
		
	case PIXEL_FORMAT_GRAY16:
		{
			const Grayscale16Pixel* pData = (const Grayscale16Pixel*)frame.getData();
			
			// find max val
			for (int i = 0; i < frame.getDataSize()/sizeof(Grayscale16Pixel); ++i, ++pData)
			{
				if (*pData > m_maxGrayVal)
				{
					m_maxGrayVal = *pData;
				}
			}
			
			// and update texture
			for (int y = 0; y < frame.getHeight(); ++y)
			{
				uint8_t* pTexture = texture + ((frame.getCropOriginY() + y) * textureWidth + frame.getCropOriginX()) * 4;
				const Grayscale16Pixel* pData = (const Grayscale16Pixel*)(pFrameData + y * frame.getStrideInBytes());
				for (int x = 0; x < frame.getWidth(); ++x, ++pData, pTexture += 4)
				{
					pTexture[0] = pTexture[1] = pTexture[2] = 255.0 * (*pData) / m_maxGrayVal;
					pTexture[3] = 255;
				}
			}
		}
		break;
		
	case PIXEL_FORMAT_GRAY8:
		{
			for (int y = 0; y < frame.getHeight(); ++y)
			{
				uint8_t* pTexture = texture + ((frame.getCropOriginY() + y) * textureWidth + frame.getCropOriginX()) * 4;
				const uint8_t* pData = pFrameData + y * frame.getStrideInBytes();
				for (int x = 0; x < frame.getWidth(); ++x, ++pData, pTexture += 4)
				{
					pTexture[0] = pTexture[1] = pTexture[2] = *pData;
					pTexture[3] = 255;
				}
			}
		}
		break;
		
	case PIXEL_FORMAT_RGB888:
		{
			for (int y = 0; y < frame.getHeight(); ++y)
			{
				uint8_t* pTexture = texture + ((frame.getCropOriginY() + y) * textureWidth + frame.getCropOriginX()) * 4;
				const RGB888Pixel* pData = (const RGB888Pixel*)(pFrameData + y * frame.getStrideInBytes());
				for (int x = 0; x < frame.getWidth(); ++x, ++pData, pTexture += 4)
				{
					pTexture[0] = pData->r;
					pTexture[1] = pData->g;
					pTexture[2] = pData->b;
					pTexture[3] = 255;
				}
			}
		}
		break;

	case PIXEL_FORMAT_YUV422:
		{
			for (int y = 0; y < frame.getHeight(); ++y)
			{
				uint8_t* pTexture = texture + ((frame.getCropOriginY() + y) * textureWidth + frame.getCropOriginX()) * 4;
				const YUV422DoublePixel* pData = (const YUV422DoublePixel*)(pFrameData + y * frame.getStrideInBytes());
				for (int x = 0; x < frame.getWidth()/2; ++x, ++pData, pTexture += 4)
				{
					YUVtoRGB888(pData->y1, pData->u, pData->v, pTexture[0], pTexture[1], pTexture[2]);
					pTexture[3] = 255;
					pTexture += 4;
					YUVtoRGB888(pData->y2, pData->u, pData->v, pTexture[0], pTexture[1], pTexture[2]);
					pTexture[3] = 255;
					pTexture += 4;
				}
			}
		}
		break;

	case PIXEL_FORMAT_YUYV:
		{
			for (int y = 0; y < frame.getHeight(); ++y)
			{
				uint8_t* pTexture = texture + ((frame.getCropOriginY() + y) * textureWidth + frame.getCropOriginX()) * 4;
				const YUYVDoublePixel* pData = (const YUYVDoublePixel*)(pFrameData + y * frame.getStrideInBytes());
				for (int x = 0; x < frame.getWidth()/2; ++x, ++pData, pTexture += 4)
				{
					YUVtoRGB888(pData->y1, pData->u, pData->v, pTexture[0], pTexture[1], pTexture[2]);
					pTexture[3] = 255;
					pTexture += 4;
					YUVtoRGB888(pData->y2, pData->u, pData->v, pTexture[0], pTexture[1], pTexture[2]);
					pTexture[3] = 255;
					pTexture += 4;
				}
			}
		}
		break;

	default:
		LOGE("Non-supported pixel format: %d", frame.getVideoMode().getPixelFormat());
		throwRuntimeException(env, "Non-supported pixel format!");
	}
}

void OpenNIView::calcDepthHist(VideoFrameRef& frame)
{
	unsigned int value = 0;
	unsigned int index = 0;
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int numberOfPoints = 0;
	const OniDepthPixel* pDepth = (const OniDepthPixel*)frame.getData();

	if (m_histogram == NULL) 
	{
		m_histogram = (int*)malloc(HISTSIZE * sizeof(int));
	}

	// Calculate the accumulative histogram
	memset(m_histogram, 0, HISTSIZE*sizeof(int));
	
	for (int i = 0; i < frame.getDataSize() / sizeof(DepthPixel); ++i, ++pDepth)
	{
		value = *pDepth;

		if (value != 0)
		{
			m_histogram[value]++;
			numberOfPoints++;
		}
	}

	for (index = 1; index < HISTSIZE; index++)
	{
		m_histogram[index] += m_histogram[index - 1];
	}
	
	if (numberOfPoints != 0)
	{
		for (index = 1; index < HISTSIZE; index++)
		{
			m_histogram[index] = (unsigned int)(256 * (1.0f - ((float)m_histogram[index] / numberOfPoints)));
		}
	}
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL Java_org_openni_android_OpenNIView_nativeCreate(JNIEnv *, jclass)
{
	return (jlong)new OpenNIView();
}

JNIEXPORT void JNICALL Java_org_openni_android_OpenNIView_nativeDestroy(JNIEnv *, jclass, jlong nativePtr)
{
	OpenNIView* pView = (OpenNIView*)nativePtr;
	delete pView;
}

JNIEXPORT void JNICALL Java_org_openni_android_OpenNIView_nativeUpdate(JNIEnv* env, jclass, jlong nativePtr, jobject textureBuffer, jint textureWidth, jint textureHeight, jlong frameHandle)
{
	unsigned char* texture = (unsigned char*)env->GetDirectBufferAddress(textureBuffer);
	OpenNIView* pView = (OpenNIView*)nativePtr;
	VideoFrameRef frame;
	frame._setFrame((OniFrame*)frameHandle);
	pView->update(env, texture, textureWidth, textureHeight, frame);
}

JNIEXPORT void JNICALL Java_org_openni_android_OpenNIView_nativeClear(JNIEnv* env, jclass, jlong, jobject textureBuffer)
{
	unsigned char* texture = (unsigned char*)env->GetDirectBufferAddress(textureBuffer);
	jlong size = env->GetDirectBufferCapacity(textureBuffer);
	memset(texture, 0, size);
}

#ifdef __cplusplus
}
#endif

#endif // if ANDROID