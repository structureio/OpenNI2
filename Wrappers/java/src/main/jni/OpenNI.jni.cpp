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

#ifndef NULL
#define NULL 0
#endif

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

typedef union {
    JNIEnv* env;
    void* venv;
} UnionJNIEnvToVoid;

extern JavaVM* g_pVM;

extern jclass g_openNIClass;
extern jclass g_videoStreamClass;
extern jclass g_deviceInfoClass;

#include "methods.inl"

JNIEXPORT
jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    LOGD("enter JNI_OnLoad()");

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK)
    {
        LOGE("ERROR: GetEnv failed");
        return -1;
    }
    
    if (!registerNatives(uenv.env))
    {
        LOGE("ERROR: registerNatives failed");
        return -1;
    }

	g_pVM = vm;
	
    // Classes used from C threads (without java in their callstack), won't
    // be found, as there is no ClassLoader in the stack. We find them now,
    // and use them later in callbacks.
    g_videoStreamClass = (jclass)uenv.env->NewGlobalRef(uenv.env->FindClass("org/openni/VideoStream"));
    g_openNIClass = (jclass)uenv.env->NewGlobalRef(uenv.env->FindClass("org/openni/OpenNI"));
    g_deviceInfoClass = (jclass)uenv.env->NewGlobalRef(uenv.env->FindClass("org/openni/DeviceInfo"));

    LOGD("JNI_OnLoad() complete!");
    return JNI_VERSION_1_4;
}

JNIEXPORT
void JNICALL JNI_OnUnload(JavaVM* vm, void * /*reserved*/)
{
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK)
    {
        LOGE("ERROR: GetEnv failed");
    }
    else
    {
		uenv.env->DeleteGlobalRef(g_videoStreamClass);
		uenv.env->DeleteGlobalRef(g_openNIClass);
		uenv.env->DeleteGlobalRef(g_deviceInfoClass);
	}
    
	g_pVM = NULL;
	g_videoStreamClass = NULL;
	g_openNIClass = NULL;
	g_deviceInfoClass = NULL;
}

