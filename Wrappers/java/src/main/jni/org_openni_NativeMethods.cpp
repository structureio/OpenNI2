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
#include <jni.h>
#include "OniProperties.h"
#include "OniEnums.h"
#include "OniCAPI.h"
#include "org_openni_NativeMethods.h"
#include <XnPlatform.h>

#ifdef ANDROID
#include <android/log.h>
#endif

#define DEBUG 1

#if DEBUG && defined(ANDROID)
#include <android/log.h>
#  define  LOGD(x...)  __android_log_print(ANDROID_LOG_INFO,"OpenNIJNI",x)
#  define  LOGE(x...)  __android_log_print(ANDROID_LOG_ERROR,"OpenNIJNI",x)
#else
#  define  LOGD(...)
#  define  LOGE(...)
#endif

using namespace openni;
JavaVM* g_pVM = NULL;
jclass g_videoStreamClass;
jclass g_openNIClass;
jclass g_deviceInfoClass;

class JNIEnvSupplier
{
public:
	JNIEnvSupplier() : m_pEnv(NULL), m_bShouldDetach(FALSE)
	{
		if (JNI_EDETACHED == g_pVM->GetEnv((void**)&m_pEnv, JNI_VERSION_1_2))
		{
			g_pVM->AttachCurrentThread((void**)&m_pEnv, NULL);
			m_bShouldDetach = TRUE;
		}
	}

	~JNIEnvSupplier()
	{
		if (m_bShouldDetach)
		{
			g_pVM->DetachCurrentThread();
		}
	}

	JNIEnv* GetEnv() { return m_pEnv; }

private:
	JNIEnv* m_pEnv;
	XnBool m_bShouldDetach;
};


void SetOutArgObjectValue(JNIEnv*env, jobject p, jobject value)
{
	jclass cls = env->GetObjectClass(p);
	jfieldID fieldID = env->GetFieldID(cls, "mValue", "Ljava/lang/Object;");
	env->SetObjectField(p, fieldID, value);
}


void SetOutArgVideoModeValue(JNIEnv*env, jobject p, jobject value)
{
	SetOutArgObjectValue(env, p, value);
}

void SetOutArgDoubleValue(JNIEnv*env, jobject p, double value)
{
	jclass cls = env->FindClass("java/lang/Double");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(D)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}

void SetOutArgLongValue(JNIEnv*env, jobject p, XnUInt64 value)
{
	jclass cls = env->FindClass("java/lang/Long");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(J)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}

void SetOutArgIntValue(JNIEnv*env, jobject p, int value)
{
	jclass cls = env->FindClass("java/lang/Integer");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(I)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}

void SetOutArgShortValue(JNIEnv*env, jobject p, short value)
{
	jclass cls = env->FindClass("java/lang/Short");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(S)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}

void SetOutArgByteValue(JNIEnv*env, jobject p, XnUInt8 value)
{
	jclass cls = env->FindClass("java/lang/Byte");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(B)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}

void SetOutArgBoolValue(JNIEnv*env, jobject p, jboolean value)
{
	jclass cls = env->FindClass("java/lang/Boolean");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(Z)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}

void SetOutArgFloatValue(JNIEnv*env, jobject p, jfloat value)
{
	jclass cls = env->FindClass("java/lang/Float");
	jmethodID ctor = env->GetMethodID(cls, "<init>", "(F)V");
	SetOutArgObjectValue(env, p, env->NewObject(cls, ctor, value));
}


void SetOutArgStringValue(JNIEnv*env, jobject p, const XnChar* value)
{
	SetOutArgObjectValue(env, p, env->NewStringUTF(value));
}

JNIEnv *g_env;
JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniFrameRelease
(JNIEnv *, jclass, jlong frame)
  {
	  oniFrameRelease((OniFrame*)frame);
  }

JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniFrameAddRef
(JNIEnv *, jclass, jlong frame)
  {
	  oniFrameAddRef((OniFrame*)frame);
  }

static void ONI_CALLBACK_TYPE callback(OniStreamHandle streamHandle, void*)
{
	JNIEnvSupplier suplier;
 	jmethodID methodID = suplier.GetEnv()->GetStaticMethodID(g_videoStreamClass, "onFrameReady", "(J)V");
 	jlong handle = (jlong)streamHandle;
 	suplier.GetEnv()->CallStaticVoidMethod(g_videoStreamClass, methodID, handle);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceCreateStream
  (JNIEnv *env, jclass, jlong device, jint sensorType, jobject videoStreamObj)
  {
	  OniStreamHandle* streamHandle;
	  jint status = oniDeviceCreateStream((OniDeviceHandle)device, (OniSensorType)sensorType, (OniStreamHandle*)&streamHandle);
	  if (status == ONI_STATUS_OK)
	  {
		  jclass videoStreamCls = env->FindClass("org/openni/VideoStream");
		  jfieldID fieldID = env->GetFieldID(videoStreamCls, "mStreamHandle", "J");
		  env->SetLongField(videoStreamObj, fieldID, (jlong)streamHandle);
		  OniCallbackHandle handle = 0;
		  status = oniStreamRegisterNewFrameCallback((OniStreamHandle)streamHandle, callback, videoStreamCls, &handle);
		  fieldID = env->GetFieldID(videoStreamCls, "mCallbackHandle", "J");
		  env->SetLongField(videoStreamObj, fieldID, (jlong)handle);
	  }
	  return status;
  }

JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniStreamDestroy
  (JNIEnv *, jclass, jlong streamHandle, jlong callbackHandle)
  {
	  oniStreamUnregisterNewFrameCallback((OniStreamHandle)streamHandle,(OniCallbackHandle)callbackHandle);
	  oniStreamDestroy((OniStreamHandle)streamHandle);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamStart
  (JNIEnv *, jclass, jlong streamHandle)
  {
	  return oniStreamStart((OniStreamHandle)streamHandle);
  }

JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniStreamStop
  (JNIEnv *, jclass, jlong streamHandle)
  { 
	  oniStreamStop((OniStreamHandle)streamHandle);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamReadFrame
  (JNIEnv *env, jclass, jlong streamHandle, jobject outArgObj)
  {
	  OniFrame* pOniFrame;
	  int status = oniStreamReadFrame((OniStreamHandle)streamHandle, &pOniFrame);
	  if (status == STATUS_OK)
	  {
		  jclass videoFrameRefCls = env->FindClass("org/openni/VideoFrameRef");
		  jmethodID videoFrameCtor = env->GetMethodID(videoFrameRefCls, "<init>", "(J)V");
		  jobject videoFrameRefObj = env->NewObject(videoFrameRefCls, videoFrameCtor, (jlong)pOniFrame);

		  jfieldID fieldID = env->GetFieldID(videoFrameRefCls, "mTimestamp", "J");													   
		  env->SetLongField(videoFrameRefObj,fieldID, (jlong)pOniFrame->timestamp);

		  fieldID = env->GetFieldID(videoFrameRefCls, "mIndex", "I");
		  env->SetIntField(videoFrameRefObj,fieldID, pOniFrame->frameIndex);

		  fieldID = env->GetFieldID(videoFrameRefCls, "mWidth", "I");
		  env->SetIntField(videoFrameRefObj,fieldID, pOniFrame->width);

		  fieldID = env->GetFieldID(videoFrameRefCls, "mHeight", "I");
		  env->SetIntField(videoFrameRefObj,fieldID, pOniFrame->height);

		  fieldID = env->GetFieldID(videoFrameRefCls, "mIsCropping", "Z");
		  env->SetBooleanField(videoFrameRefObj,fieldID, (pOniFrame->croppingEnabled == TRUE));


		  fieldID = env->GetFieldID(videoFrameRefCls, "mCropOrigX", "I");
		  env->SetIntField(videoFrameRefObj,fieldID, pOniFrame->cropOriginX);


		  fieldID = env->GetFieldID(videoFrameRefCls, "mCropOrigY", "I");
		  env->SetIntField(videoFrameRefObj,fieldID, pOniFrame->cropOriginY);

		  fieldID = env->GetFieldID(videoFrameRefCls, "mStride", "I");
		  env->SetIntField(videoFrameRefObj,fieldID, pOniFrame->stride);
		  
		  jclass byteOrderCls = env->FindClass("java/nio/ByteOrder");
		  jfieldID littleEndianField = env->GetStaticFieldID(byteOrderCls, "LITTLE_ENDIAN", "Ljava/nio/ByteOrder;");
		  jobject littleEndian = env->GetStaticObjectField(byteOrderCls, littleEndianField);

		  jclass byteBufferCls = env->FindClass("java/nio/ByteBuffer");
		  jmethodID orderMethodId = env->GetMethodID(byteBufferCls, "order", "(Ljava/nio/ByteOrder;)Ljava/nio/ByteBuffer;");

		  jobject buffer = env->NewDirectByteBuffer(pOniFrame->data, pOniFrame->dataSize);
		  env->CallObjectMethod(buffer, orderMethodId, littleEndian);
		  fieldID = env->GetFieldID(videoFrameRefCls, "mData", "Ljava/nio/ByteBuffer;");
		  env->SetObjectField(videoFrameRefObj,fieldID, buffer);

		  jclass sensorTypeCls = env->FindClass("org/openni/SensorType");
		  jmethodID sensorFromNative = env->GetStaticMethodID(sensorTypeCls, "fromNative", "(I)Lorg/openni/SensorType;");
		  jobject sensorTypeObj = env->CallStaticObjectMethod(sensorTypeCls, sensorFromNative, pOniFrame->sensorType);
		  fieldID = env->GetFieldID(videoFrameRefCls, "mSensorType", "Lorg/openni/SensorType;");
		  env->SetObjectField(videoFrameRefObj,fieldID, sensorTypeObj);

		  jclass videoModeCls = env->FindClass("org/openni/VideoMode");
		  jmethodID videoModeCtor = env->GetMethodID(videoModeCls, "<init>", "(IIII)V");
		  jobject videoModeObj = env->NewObject(videoModeCls, videoModeCtor, (jint)pOniFrame->videoMode.resolutionX, 
													(jint)pOniFrame->videoMode.resolutionY,  (jint)pOniFrame->videoMode.fps, (jint)pOniFrame->videoMode.pixelFormat);
		  fieldID = env->GetFieldID(videoFrameRefCls, "mVideoMode", "Lorg/openni/VideoMode;");
		  env->SetObjectField(videoFrameRefObj,fieldID, videoModeObj);
		  SetOutArgObjectValue(env, outArgObj, videoFrameRefObj);
		  
		  // release this frame. The java object is its owner now.
    	  oniFrameRelease(pOniFrame);
	  }
	  	  
	  return status;
  }


JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_getCropping
	(JNIEnv *env, jclass, jlong streamHandle, jobject origXOutArg, jobject origYOutArg, jobject widthOurArg, jobject heightOurArg)
  { 
	  OniCropping cropping;
	  int size = sizeof(cropping);
	  int status = oniStreamGetProperty((OniStreamHandle)streamHandle, STREAM_PROPERTY_CROPPING, &cropping, &size);
	  SetOutArgIntValue(env, origXOutArg, cropping.originX);
	  SetOutArgIntValue(env, origYOutArg, cropping.originY);
	  SetOutArgIntValue(env, widthOurArg, cropping.width);
	  SetOutArgIntValue(env, heightOurArg, cropping.height);
	  return status;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_setCropping
  (JNIEnv *, jclass, jlong streamHandle, jint originX, jint originY, jint width, jint height)
  { 
	  OniCropping cropping;
	  cropping.enabled = true;
	  cropping.originX = originX;
	  cropping.originY = originY;
	  cropping.width = width;
	  cropping.height = height;
	  return oniStreamSetProperty((OniStreamHandle)streamHandle, STREAM_PROPERTY_CROPPING, &cropping, sizeof(cropping));
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_isCroppingSupported
  (JNIEnv *, jclass, jlong streamHandle)
  { 
	  return (oniStreamIsPropertySupported((OniStreamHandle)streamHandle, STREAM_PROPERTY_CROPPING) == TRUE);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_resetCropping
   (JNIEnv *, jclass, jlong streamHandle)
  {
	  OniCropping cropping;
	  cropping.enabled = FALSE;
	  return oniStreamSetProperty((OniStreamHandle)streamHandle, STREAM_PROPERTY_CROPPING, &cropping, sizeof(cropping));
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_getVideoMode
  (JNIEnv *env, jclass, jlong streamHandle, jobject videoModeArgOutObj)
  {
	  jclass videoModeCls = env->FindClass("org/openni/VideoMode");
	  jmethodID videoModeCtor = env->GetMethodID(videoModeCls, "<init>", "(IIII)V");
	  OniVideoMode videoMode;
	  int size = sizeof(OniVideoMode);
	  jint status = oniStreamGetProperty((OniStreamHandle)streamHandle, STREAM_PROPERTY_VIDEO_MODE, &videoMode, &size);
	  
	  jobject videoModeObj = env->NewObject(videoModeCls, videoModeCtor, videoMode.resolutionX, 
		  videoMode.resolutionY,  videoMode.fps, videoMode.pixelFormat);
	  SetOutArgVideoModeValue(env, videoModeArgOutObj, videoModeObj);
	  return status;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_setVideoMode
	(JNIEnv *, jclass, jlong streamHandle, jint resX, jint resY, jint fps, jint pixelFormat)
  {
	  OniVideoMode videoMode;
	  int size = sizeof(OniVideoMode);
	  videoMode.resolutionX = resX; 
	  videoMode.resolutionY = resY;
	  videoMode.fps = fps;
	  videoMode.pixelFormat = (OniPixelFormat)pixelFormat;
	  return oniStreamSetProperty((OniStreamHandle)streamHandle, STREAM_PROPERTY_VIDEO_MODE, &videoMode, size);
  }

JNIEXPORT jobject JNICALL Java_org_openni_NativeMethods_oniStreamGetSensorInfo
  (JNIEnv *env, jclass, jlong streamHandle)
  { 
	  jclass arrayListCls = (*env).FindClass("java/util/ArrayList");
	  jobject vectorObj = (*env).NewObject(arrayListCls, (*env).GetMethodID(arrayListCls, "<init>", "()V"));
	  jclass videoModeCls = (*env).FindClass("org/openni/VideoMode");
	  jmethodID videoModeCtor = env->GetMethodID(videoModeCls, "<init>", "(IIII)V");
	  const OniSensorInfo* sensorInfo = oniStreamGetSensorInfo((OniStreamHandle)streamHandle);
	  int i = 0;
	  while (i < sensorInfo->numSupportedVideoModes)
	  {
		  OniVideoMode& videoMode = sensorInfo->pSupportedVideoModes[i];
		  jobject videoModeObj = env->NewObject(videoModeCls, videoModeCtor, videoMode.resolutionX, 
			  videoMode.resolutionY,  videoMode.fps, (int)videoMode.pixelFormat);

		  (*env).CallBooleanMethod(vectorObj, (*env).GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z"), videoModeObj);
		  i++;			
	  }
	  jclass sensorInfoCls = (*env).FindClass("org/openni/SensorInfo");
	  jobject obj = (*env).NewObject(sensorInfoCls, (*env).GetMethodID(sensorInfoCls, "<init>", "(ILjava/util/List;)V"), sensorInfo->sensorType, vectorObj);
	  return obj; 
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_hasSensor
   (JNIEnv *, jclass, jlong deviceHandle, jint sensorType)
  {
	  const OniSensorInfo* pInfo = oniDeviceGetSensorInfo((OniDeviceHandle)deviceHandle, (OniSensorType)sensorType);

	  if (pInfo == NULL)
	  {
		  return false;
	  }
	  return true;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamGetIntProperty
  (JNIEnv *env, jclass, jlong streamHandle, jint property, jobject argOutObj)
  {
	  int value = 0;
	  int size = sizeof(value);
	  int rc = oniStreamGetProperty((OniStreamHandle)streamHandle, property, &value, &size);
	  SetOutArgIntValue(env, argOutObj, value);
	  return rc;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamGetBoolProperty
  (JNIEnv *env, jclass, jlong streamHandle, jint property, jobject argOutObj)
  { 
	  OniBool value = false;
	  int size = sizeof(value);
	  int rc = oniStreamGetProperty((OniStreamHandle)streamHandle, property, &value, &size);
	  SetOutArgBoolValue(env, argOutObj, value == TRUE);
	  return rc;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamGetFloatProperty
  (JNIEnv *env, jclass, jlong streamHandle, jint property, jobject argOutObj)
  {
	  float value = 0;
	  int size = sizeof(value);
	  int rc = oniStreamGetProperty((OniStreamHandle)streamHandle, property, &value, &size);
	  SetOutArgFloatValue(env, argOutObj, value);
	  return rc;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamSetProperty__JII
  (JNIEnv *, jclass, jlong streamHandle, jint property, jint value)
  { 
	  int size = sizeof(value);
	  return oniStreamSetProperty((OniStreamHandle)streamHandle, property, &value, size);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamSetProperty__JIZ
  (JNIEnv *, jclass, jlong streamHandle, jint property, jboolean value)
  {
	  OniBool val = value?TRUE:FALSE;
	  int size = sizeof(val);
	  return oniStreamSetProperty((OniStreamHandle)streamHandle, property, &val, size);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniStreamSetProperty__JIF
  (JNIEnv *, jclass, jlong streamHandle, jint property, jfloat value)
  { 
	  int size = sizeof(value);
	  return oniStreamSetProperty((OniStreamHandle)streamHandle, property, &value, size);
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_oniStreamIsPropertySupported
  (JNIEnv *, jclass, jlong streamHandle, jint property)
  { 
	  return (oniStreamIsPropertySupported((OniStreamHandle)streamHandle, property) == TRUE);
  }

JNIEXPORT jobject JNICALL Java_org_openni_NativeMethods_oniDeviceGetSensorInfo
  (JNIEnv *env, jclass, jlong deviceHandle, jint sensorType)
  {
	  jclass arrayListCls = (*env).FindClass("java/util/ArrayList");
	  jobject vectorObj = (*env).NewObject(arrayListCls, (*env).GetMethodID(arrayListCls, "<init>", "()V"));
	  jclass videoModeCls = (*env).FindClass("org/openni/VideoMode");
	  jmethodID videoModeCtor = env->GetMethodID(videoModeCls, "<init>", "(IIII)V");
	  const OniSensorInfo* sensorInfo = oniDeviceGetSensorInfo((OniDeviceHandle)deviceHandle, (OniSensorType)sensorType);
	  if (sensorInfo == NULL)
		  return NULL;

	  int i = 0;
	  while (i < sensorInfo->numSupportedVideoModes)
	  {
		  OniVideoMode& videoMode = sensorInfo->pSupportedVideoModes[i];
		  jobject videoModeObj = env->NewObject(videoModeCls, videoModeCtor, videoMode.resolutionX, 
			  videoMode.resolutionY,  videoMode.fps, (int)videoMode.pixelFormat);

		  (*env).CallBooleanMethod(vectorObj, (*env).GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z"), videoModeObj);
		  i++;			
	  }
	  jclass sensorInfoCls = (*env).FindClass("org/openni/SensorInfo");
	  jobject obj = (*env).NewObject(sensorInfoCls, (*env).GetMethodID(sensorInfoCls, "<init>", "(ILjava/util/List;)V"), sensorInfo->sensorType, vectorObj);
	  return obj; 

  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceEnableDepthColorSync
  (JNIEnv *, jclass, jlong deviceHandle)
  { 
	  return oniDeviceEnableDepthColorSync((OniDeviceHandle)deviceHandle);
  }

JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniDeviceDisableDepthColorSync
  (JNIEnv *, jclass, jlong deviceHandle)
  { 
	  return oniDeviceDisableDepthColorSync((OniDeviceHandle)deviceHandle);
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_oniDeviceGetDepthColorSyncEnabled
	(JNIEnv *, jclass, jlong deviceHandle)
{
	return (jboolean)oniDeviceGetDepthColorSyncEnabled((OniDeviceHandle)deviceHandle);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_seek
  (JNIEnv *, jclass, jlong deviceHandle, jlong streamHandle, jint frameIndex)
  {
	  OniSeek seek;
	  seek.frameIndex = frameIndex;
	  seek.stream = (OniStreamHandle)streamHandle;
	  return oniDeviceInvoke((OniDeviceHandle)deviceHandle, DEVICE_COMMAND_SEEK, &seek, sizeof(seek));
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_isImageRegistrationModeSupported
  (JNIEnv *, jclass, jlong deviceHandle, jint mode)
  {
	  return (oniDeviceIsImageRegistrationModeSupported((OniDeviceHandle)deviceHandle, (OniImageRegistrationMode)mode) == TRUE);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_getImageRegistrationMode
   (JNIEnv *env, jclass, jlong deviceHandle, jobject argOutObj)
  {
	  ImageRegistrationMode mode;
	  int size = sizeof(mode);
	  int rc = oniDeviceGetProperty((OniDeviceHandle)deviceHandle, DEVICE_PROPERTY_IMAGE_REGISTRATION, &mode, &size);
	  SetOutArgIntValue(env, argOutObj, mode);
	  return rc;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_setImageRegistrationMode
    (JNIEnv *, jclass, jlong deviceHandle, jint mode)
  {
	  return oniDeviceSetProperty((OniDeviceHandle)deviceHandle, DEVICE_PROPERTY_IMAGE_REGISTRATION, &mode, sizeof(mode));
  }

JNIEXPORT jobject JNICALL Java_org_openni_NativeMethods_oniDeviceGetInfo
  (JNIEnv *env, jclass, jlong deviceHandle)
  {
	  OniDeviceInfo deviceInfo;
	  oniDeviceGetInfo((OniDeviceHandle)deviceHandle, &deviceInfo);
	  jobject nameObj = env->NewStringUTF(deviceInfo.name);
	  jobject uriObj = env->NewStringUTF(deviceInfo.uri);
	  jobject vendorObj = env->NewStringUTF(deviceInfo.vendor);
	  jclass deviceInfoCls = env->FindClass("org/openni/DeviceInfo");
	  return (*env).NewObject(deviceInfoCls, (*env).GetMethodID(deviceInfoCls, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V"), 
		  uriObj, vendorObj, nameObj, deviceInfo.usbVendorId, deviceInfo.usbProductId);
 }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniRecorderStart
  (JNIEnv *, jclass, jlong recorderHandle)
  {
	  return oniRecorderStart((OniRecorderHandle)recorderHandle);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniRecorderDestroy
  (JNIEnv *, jclass, jlong recorderHandle)
  { 
	  return oniRecorderDestroy((OniRecorderHandle*)&recorderHandle);
  }

JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniRecorderStop
  (JNIEnv *, jclass, jlong recorderHandle)
  {
	  oniRecorderStop((OniRecorderHandle)recorderHandle);
  }


JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniRecorderAttachStream
  (JNIEnv *, jclass, jlong recorderHandle, jlong streamHadle, jboolean allowLossy)
  {
	  return oniRecorderAttachStream((OniRecorderHandle)recorderHandle, (OniStreamHandle)streamHadle, allowLossy);
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceGetIntProperty
  (JNIEnv *env , jclass, jlong deviceHandle, jint propery, jobject argOutObj)
{
	int value;
	int size = sizeof(value);
	int rc = oniDeviceGetProperty((OniDeviceHandle)deviceHandle, propery, &value, &size);
	SetOutArgIntValue(env, argOutObj, value);
	return rc;
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceGetBoolProperty
  (JNIEnv *env , jclass, jlong deviceHandle, jint propery, jobject argOutObj)
{
	OniBool value;
	int size = sizeof(value);
	int rc = oniDeviceGetProperty((OniDeviceHandle)deviceHandle, propery, &value, &size);
	SetOutArgBoolValue(env, argOutObj, (value==TRUE)?JNI_TRUE:JNI_FALSE);
	return rc;
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceGetFloatProperty
  (JNIEnv *env , jclass, jlong deviceHandle, jint propery, jobject argOutObj)
{
	float value;
	int size = sizeof(float);
	int rc = oniDeviceGetProperty((OniDeviceHandle)deviceHandle, propery, &value, &size);
	SetOutArgFloatValue(env, argOutObj, value);
	return rc;
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceSetProperty__JII
  (JNIEnv *, jclass, jlong deviceHandle, jint property, jint value)
  {
	  return oniDeviceSetProperty((OniDeviceHandle)deviceHandle, property, &value, sizeof(value));
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceSetProperty__JIZ
  (JNIEnv *, jclass, jlong deviceHandle, jint property, jboolean value)
  {
	  OniBool oniValue = (value == JNI_TRUE)?TRUE:FALSE;
	  return oniDeviceSetProperty((OniDeviceHandle)deviceHandle, property, &oniValue, sizeof(oniValue));
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceSetProperty__JIF
  (JNIEnv *, jclass, jlong deviceHandle, jint property, float value)
  {
	  return oniDeviceSetProperty((OniDeviceHandle)deviceHandle, property, &value, sizeof(value));
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_oniDeviceIsPropertySupported
  (JNIEnv *, jclass, jlong deviceHandle, jint property)
  { 
	  return (oniDeviceIsPropertySupported((OniDeviceHandle)deviceHandle, property) == TRUE);
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_oniDeviceIsCommandSupported
  (JNIEnv *, jclass, jlong deviceHandle, jint command)
 {
	  return (oniDeviceIsCommandSupported((OniDeviceHandle)deviceHandle, command) == TRUE);
 }


static void ONI_CALLBACK_TYPE deviceConnectedCallback(const OniDeviceInfo* pInfo, void*)
{
	JNIEnvSupplier suplier;
	JNIEnv *env = suplier.GetEnv();
	jmethodID methodID = env->GetStaticMethodID(g_openNIClass, "deviceConnected", "(Lorg/openni/DeviceInfo;)V");
	jobject nameObj = env->NewStringUTF(pInfo->name);
	jobject uriObj = env->NewStringUTF(pInfo->uri);
	jobject vendorObj = env->NewStringUTF(pInfo->vendor);
	jobject deviceObj = (*env).NewObject(g_deviceInfoClass, (*env).GetMethodID(g_deviceInfoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V"), 
		uriObj, vendorObj, nameObj, pInfo->usbVendorId, pInfo->usbProductId);
	env->CallStaticVoidMethod(g_openNIClass, methodID, deviceObj);
}

static void ONI_CALLBACK_TYPE deviceDisconnectedCallback(const OniDeviceInfo* pInfo, void*)
{
	JNIEnvSupplier suplier;
	JNIEnv *env = suplier.GetEnv();
	jmethodID methodID = env->GetStaticMethodID(g_openNIClass, "deviceDisconnected", "(Lorg/openni/DeviceInfo;)V");
	jobject nameObj = env->NewStringUTF(pInfo->name);
	jobject uriObj = env->NewStringUTF(pInfo->uri);
	jobject vendorObj = env->NewStringUTF(pInfo->vendor);
	jobject deviceObj = (*env).NewObject(g_deviceInfoClass, (*env).GetMethodID(g_deviceInfoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V"), 
		uriObj, vendorObj, nameObj, pInfo->usbVendorId, pInfo->usbProductId);
	env->CallStaticVoidMethod(g_openNIClass, methodID, deviceObj);
}

static void ONI_CALLBACK_TYPE deviceStateChangedCallback(const OniDeviceInfo* pInfo, OniDeviceState state, void*)
{
	JNIEnvSupplier suplier;
	JNIEnv *env = suplier.GetEnv();
	jmethodID methodID = env->GetStaticMethodID(g_openNIClass, "deviceStateChanged", "(Lorg/openni/DeviceInfo;I)V");
	jobject nameObj = env->NewStringUTF(pInfo->name);
	jobject uriObj = env->NewStringUTF(pInfo->uri);
	jobject vendorObj = env->NewStringUTF(pInfo->vendor);
	jobject deviceObj = (*env).NewObject(g_deviceInfoClass, (*env).GetMethodID(g_deviceInfoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V"), 
		uriObj, vendorObj, nameObj, pInfo->usbVendorId, pInfo->usbProductId);
	env->CallStaticVoidMethod(g_openNIClass, methodID, deviceObj, state);
	
}
static OniCallbackHandle callbackHandle = 0;
JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniInitialize
  (JNIEnv *env, jclass)
  { 
	  int status = oniInitialize(ONI_API_VERSION);
	  if (status == ONI_STATUS_OK)
	  {
		  OniDeviceCallbacks callbacks;
		  callbacks.deviceConnected = deviceConnectedCallback;
		  callbacks.deviceDisconnected = deviceDisconnectedCallback;
		  callbacks.deviceStateChanged = deviceStateChangedCallback;
		  
		  status = oniRegisterDeviceCallbacks(&callbacks, env, &callbackHandle);
	  }
	  return status;
  }

JNIEXPORT void JNICALL Java_org_openni_NativeMethods_oniShutdown
  (JNIEnv *, jclass)
  {
	  if (callbackHandle != 0)
		oniUnregisterDeviceCallbacks(callbackHandle);
	  return oniShutdown();
  }

JNIEXPORT jobject JNICALL Java_org_openni_NativeMethods_oniGetVersion
  (JNIEnv *env , jclass)
  {
	  OniVersion version = oniGetVersion();
	  
	  jclass versionCls = env->FindClass("org/openni/Version");
	  return (*env).NewObject(versionCls, (*env).GetMethodID(versionCls, "<init>", "(IIII)V"), 
		  version.major, version.minor, version.maintenance, version.build);
  }

JNIEXPORT jstring JNICALL Java_org_openni_NativeMethods_oniGetExtendedError
  (JNIEnv *env , jclass)
  { 
	  return  env->NewStringUTF(oniGetExtendedError());
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniGetDeviceList
    (JNIEnv *env, jclass, jobject deviceListObj)
  { 
	  OniDeviceInfo* m_pDeviceInfos;
	  int m_deviceInfoCount;
	  jint status = oniGetDeviceList(&m_pDeviceInfos, &m_deviceInfoCount);
	  if (status == 0)
	  {
		  for (int i = 0; i < m_deviceInfoCount; i++)
		  {
			  jobject nameObj = env->NewStringUTF(m_pDeviceInfos[i].name);
			  jobject uriObj = env->NewStringUTF(m_pDeviceInfos[i].uri);
			  jobject vendorObj = env->NewStringUTF(m_pDeviceInfos[i].vendor);
			  jclass deviceInfoCls = env->FindClass("org/openni/DeviceInfo");
			  jobject deviceInfObj = (*env).NewObject(deviceInfoCls, (*env).GetMethodID(deviceInfoCls, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V"), 
				  uriObj, vendorObj, nameObj, m_pDeviceInfos[i].usbVendorId, m_pDeviceInfos[i].usbProductId);
			  jclass vectorCls = (*env).FindClass("java/util/List");
			  jmethodID methodId = (*env).GetMethodID(vectorCls, "add", "(Ljava/lang/Object;)Z");
			  (*env).CallBooleanMethod(deviceListObj, methodId, deviceInfObj);
		  }
	  }	  
	  return status;
  }

JNIEXPORT jboolean JNICALL Java_org_openni_NativeMethods_oniWaitForAnyStream
	(JNIEnv *env, jclass, jlongArray streamsArray, jobject outArgObj, jint timeout)
  {
	  jlong *streams = env->GetLongArrayElements(streamsArray, JNI_FALSE);
	  int size = env->GetArrayLength(streamsArray);
	  int id = 0;
	  int rc = oniWaitForAnyStream((OniStreamHandle*)streams, size, &id, timeout);
	  env->ReleaseLongArrayElements(streamsArray, streams, JNI_ABORT);
	  SetOutArgIntValue(env, outArgObj, id);
	  return rc == ONI_STATUS_OK;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniCoordinateConverterWorldToDepth
	(JNIEnv *env, jclass, jlong streamHandle, jfloat worldX, jfloat worldY, jfloat worldZ, jobject depthXOutArg, jobject depthYOutArg, jobject depthZOutArg)
  {
	  float x,y,z;
	  int status = oniCoordinateConverterWorldToDepth((OniStreamHandle)streamHandle, worldX, worldY, worldZ, &x, &y, &z);
	  SetOutArgFloatValue(env, depthXOutArg, x);
	  SetOutArgFloatValue(env, depthYOutArg, y);
	  SetOutArgFloatValue(env, depthZOutArg, z);
	  return status;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniCoordinateConverterDepthToWorld
	(JNIEnv *env, jclass, jlong streamHandle, jfloat depthX, jfloat depthY, jfloat depthZ, jobject colorXOutArg, jobject colorYOutArg, jobject colorZOutArg)
  {
	  float x,y,z;
	  int status = oniCoordinateConverterDepthToWorld((OniStreamHandle)streamHandle, depthX, depthY, depthZ, &x, &y, &z);
	  SetOutArgFloatValue(env, colorXOutArg, x);
	  SetOutArgFloatValue(env, colorYOutArg, y);
	  SetOutArgFloatValue(env, colorZOutArg, z);
	  return status;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniCoordinateConverterDepthToColor
	(JNIEnv *env, jclass, jlong depthHandle, jlong colorHandle, jint depthX, jint depthY, jshort depthZ, jobject colorXOutArg, jobject colorYOutArg)  
 { 
	 int x,y;
	 int status = oniCoordinateConverterDepthToColor((OniStreamHandle)depthHandle, (OniStreamHandle)colorHandle, depthX, depthY, depthZ, &x, &y);
	 SetOutArgIntValue(env, colorXOutArg, x);
	 SetOutArgIntValue(env, colorYOutArg, y);
	 return status;
 }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniCreateRecorder
	(JNIEnv *env, jclass, jstring filename, jobject recorder)
  { 
	  OniRecorderHandle handle;
	  jclass recorderCls = env->FindClass("org/openni/Recorder");
	  const char * str = env->GetStringUTFChars(filename, JNI_FALSE);
	  int status = oniCreateRecorder(str, &handle);
	  jfieldID fieldID = env->GetFieldID(recorderCls, "mRecorderHandle", "J");
	  env->SetLongField(recorder, fieldID, (long)handle);
	  return status;
  }

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceOpen__Ljava_lang_String_2Lorg_openni_Device_2
(JNIEnv *env, jclass, jstring uriStrObj, jobject device)
  {
	  OniDeviceHandle handle;
	  const char * str = env->GetStringUTFChars(uriStrObj, JNI_FALSE);
	  int status = oniDeviceOpen(str, &handle);
	  jclass deviceCls = env->FindClass("org/openni/Device");
	  jfieldID fieldID = env->GetFieldID(deviceCls, "mDeviceHandle", "J");
	  env->SetLongField(device, fieldID, (long)handle);
	  return status;
  }
static const char* ANY_DEVICE = NULL;
JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceOpen__Lorg_openni_Device_2
	(JNIEnv *env, jclass, jobject device)
{
	OniDeviceHandle handle;
	int status = oniDeviceOpen(ANY_DEVICE, &handle);
	jclass deviceCls = env->FindClass("org/openni/Device");
	jfieldID fieldID = env->GetFieldID(deviceCls, "mDeviceHandle", "J");
	env->SetLongField(device, fieldID, (long)handle);
	return status;
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniDeviceClose
  (JNIEnv *, jclass, jlong deviceHandle)
{ 
	return oniDeviceClose((OniDeviceHandle)deviceHandle);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniSetLogOutputFolder
	(JNIEnv * env, jclass, jstring path)
{
	const char * str = env->GetStringUTFChars(path, JNI_FALSE);
	return oniSetLogOutputFolder(str);
}

JNIEXPORT jstring JNICALL Java_org_openni_NativeMethods_oniGetLogFileName
	(JNIEnv *env, jclass)
{
	XnChar fileName[XN_FILE_MAX_PATH];
	oniGetLogFileName(fileName, sizeof(fileName));
	return env->NewStringUTF(fileName);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniSetLogMinSeverity
	(JNIEnv *, jclass, jint minSeverity)
{
	return oniSetLogMinSeverity(minSeverity);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniSetLogConsoleOutput
	(JNIEnv *, jclass, jboolean enabled)
{
	return oniSetLogConsoleOutput(enabled);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniSetLogFileOutput
	(JNIEnv *, jclass, jboolean enabled)
{
	return oniSetLogFileOutput(enabled);
}

JNIEXPORT jint JNICALL Java_org_openni_NativeMethods_oniSetLogAndroidOutput
	(JNIEnv *, jclass, jboolean enabled)
{
	(void)(enabled);
#ifdef ANDROID
	return oniSetLogAndroidOutput(enabled);
#else
	return ONI_STATUS_NOT_SUPPORTED;
#endif
}
