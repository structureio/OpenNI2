#include "org_openni_NativeMethods.h"

static int register_org_openni_NativeMethods(JNIEnv* env)
{
    static JNINativeMethod methods[] =
    {
		{ "oniFrameRelease", "(J)V", (void*)&Java_org_openni_NativeMethods_oniFrameRelease },
		{ "oniFrameAddRef", "(J)V", (void*)&Java_org_openni_NativeMethods_oniFrameAddRef },
		{ "oniDeviceCreateStream", "(JILorg/openni/VideoStream;)I", (void*)&Java_org_openni_NativeMethods_oniDeviceCreateStream },
		{ "oniStreamDestroy", "(JJ)V", (void*)&Java_org_openni_NativeMethods_oniStreamDestroy },
		{ "oniStreamStart", "(J)I", (void*)&Java_org_openni_NativeMethods_oniStreamStart },
		{ "oniStreamStop", "(J)V", (void*)&Java_org_openni_NativeMethods_oniStreamStop },
		{ "oniStreamReadFrame", "(JLorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniStreamReadFrame },
		{ "getCropping", "(JLorg/openni/OutArg;Lorg/openni/OutArg;Lorg/openni/OutArg;Lorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_getCropping },
		{ "setCropping", "(JIIII)I", (void*)&Java_org_openni_NativeMethods_setCropping },
		{ "isCroppingSupported", "(J)Z", (void*)&Java_org_openni_NativeMethods_isCroppingSupported },
		{ "resetCropping", "(J)I", (void*)&Java_org_openni_NativeMethods_resetCropping },
		{ "getVideoMode", "(JLorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_getVideoMode },
		{ "setVideoMode", "(JIIII)I", (void*)&Java_org_openni_NativeMethods_setVideoMode },
		{ "oniStreamGetSensorInfo", "(J)Lorg/openni/SensorInfo;", (void*)&Java_org_openni_NativeMethods_oniStreamGetSensorInfo },
		{ "hasSensor", "(JI)Z", (void*)&Java_org_openni_NativeMethods_hasSensor },
		{ "oniStreamGetIntProperty", "(JILorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniStreamGetIntProperty },
		{ "oniStreamGetBoolProperty", "(JILorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniStreamGetBoolProperty },
		{ "oniStreamGetFloatProperty", "(JILorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniStreamGetFloatProperty },
		{ "oniStreamSetProperty", "(JII)I", (void*)&Java_org_openni_NativeMethods_oniStreamSetProperty__JII },
		{ "oniStreamSetProperty", "(JIZ)I", (void*)&Java_org_openni_NativeMethods_oniStreamSetProperty__JIZ },
		{ "oniStreamSetProperty", "(JIF)I", (void*)&Java_org_openni_NativeMethods_oniStreamSetProperty__JIF },
		{ "oniStreamIsPropertySupported", "(JI)Z", (void*)&Java_org_openni_NativeMethods_oniStreamIsPropertySupported },
		{ "oniDeviceGetSensorInfo", "(JI)Lorg/openni/SensorInfo;", (void*)&Java_org_openni_NativeMethods_oniDeviceGetSensorInfo },
		{ "oniDeviceEnableDepthColorSync", "(J)I", (void*)&Java_org_openni_NativeMethods_oniDeviceEnableDepthColorSync },
		{ "oniDeviceDisableDepthColorSync", "(J)V", (void*)&Java_org_openni_NativeMethods_oniDeviceDisableDepthColorSync },
		{ "oniDeviceGetDepthColorSyncEnabled", "(J)Z", (void*)&Java_org_openni_NativeMethods_oniDeviceGetDepthColorSyncEnabled },
		{ "seek", "(JJI)I", (void*)&Java_org_openni_NativeMethods_seek },
		{ "isImageRegistrationModeSupported", "(JI)Z", (void*)&Java_org_openni_NativeMethods_isImageRegistrationModeSupported },
		{ "getImageRegistrationMode", "(JLorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_getImageRegistrationMode },
		{ "setImageRegistrationMode", "(JI)I", (void*)&Java_org_openni_NativeMethods_setImageRegistrationMode },
		{ "oniDeviceGetInfo", "(J)Lorg/openni/DeviceInfo;", (void*)&Java_org_openni_NativeMethods_oniDeviceGetInfo },
		{ "oniRecorderStart", "(J)I", (void*)&Java_org_openni_NativeMethods_oniRecorderStart },
		{ "oniRecorderDestroy", "(J)I", (void*)&Java_org_openni_NativeMethods_oniRecorderDestroy },
		{ "oniRecorderStop", "(J)V", (void*)&Java_org_openni_NativeMethods_oniRecorderStop },
		{ "oniRecorderAttachStream", "(JJZ)I", (void*)&Java_org_openni_NativeMethods_oniRecorderAttachStream },
		{ "oniDeviceGetIntProperty", "(JILorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniDeviceGetIntProperty },
		{ "oniDeviceGetBoolProperty", "(JILorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniDeviceGetBoolProperty },
		{ "oniDeviceGetFloatProperty", "(JILorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniDeviceGetFloatProperty },
		{ "oniDeviceSetProperty", "(JII)I", (void*)&Java_org_openni_NativeMethods_oniDeviceSetProperty__JII },
		{ "oniDeviceSetProperty", "(JIZ)I", (void*)&Java_org_openni_NativeMethods_oniDeviceSetProperty__JIZ },
		{ "oniDeviceSetProperty", "(JIF)I", (void*)&Java_org_openni_NativeMethods_oniDeviceSetProperty__JIF },
		{ "oniDeviceIsPropertySupported", "(JI)Z", (void*)&Java_org_openni_NativeMethods_oniDeviceIsPropertySupported },
		{ "oniDeviceIsCommandSupported", "(JI)Z", (void*)&Java_org_openni_NativeMethods_oniDeviceIsCommandSupported },
		{ "oniInitialize", "()I", (void*)&Java_org_openni_NativeMethods_oniInitialize },
		{ "oniShutdown", "()V", (void*)&Java_org_openni_NativeMethods_oniShutdown },
		{ "oniGetVersion", "()Lorg/openni/Version;", (void*)&Java_org_openni_NativeMethods_oniGetVersion },
		{ "oniGetExtendedError", "()Ljava/lang/String;", (void*)&Java_org_openni_NativeMethods_oniGetExtendedError },
		{ "oniGetDeviceList", "(Ljava/util/List;)I", (void*)&Java_org_openni_NativeMethods_oniGetDeviceList },
		{ "oniWaitForAnyStream", "([JLorg/openni/OutArg;I)Z", (void*)&Java_org_openni_NativeMethods_oniWaitForAnyStream },
		{ "oniCoordinateConverterWorldToDepth", "(JFFFLorg/openni/OutArg;Lorg/openni/OutArg;Lorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniCoordinateConverterWorldToDepth },
		{ "oniCoordinateConverterDepthToWorld", "(JFFFLorg/openni/OutArg;Lorg/openni/OutArg;Lorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniCoordinateConverterDepthToWorld },
		{ "oniCoordinateConverterDepthToColor", "(JJIISLorg/openni/OutArg;Lorg/openni/OutArg;)I", (void*)&Java_org_openni_NativeMethods_oniCoordinateConverterDepthToColor },
		{ "oniCreateRecorder", "(Ljava/lang/String;Lorg/openni/Recorder;)I", (void*)&Java_org_openni_NativeMethods_oniCreateRecorder },
		{ "oniDeviceOpen", "(Ljava/lang/String;Lorg/openni/Device;)I", (void*)&Java_org_openni_NativeMethods_oniDeviceOpen__Ljava_lang_String_2Lorg_openni_Device_2 },
		{ "oniDeviceOpen", "(Lorg/openni/Device;)I", (void*)&Java_org_openni_NativeMethods_oniDeviceOpen__Lorg_openni_Device_2 },
		{ "oniDeviceClose", "(J)I", (void*)&Java_org_openni_NativeMethods_oniDeviceClose },
		{ "oniSetLogOutputFolder", "(Ljava/lang/String;)I", (void*)&Java_org_openni_NativeMethods_oniSetLogOutputFolder },
		{ "oniGetLogFileName", "()Ljava/lang/String;", (void*)&Java_org_openni_NativeMethods_oniGetLogFileName },
		{ "oniSetLogMinSeverity", "(I)I", (void*)&Java_org_openni_NativeMethods_oniSetLogMinSeverity },
		{ "oniSetLogConsoleOutput", "(Z)I", (void*)&Java_org_openni_NativeMethods_oniSetLogConsoleOutput },
		{ "oniSetLogFileOutput", "(Z)I", (void*)&Java_org_openni_NativeMethods_oniSetLogFileOutput },
		{ "oniSetLogAndroidOutput", "(Z)I", (void*)&Java_org_openni_NativeMethods_oniSetLogAndroidOutput },

    };
    static int methodsCount = sizeof(methods)/sizeof(methods[0]);
    jclass cls = env->FindClass("org/openni/NativeMethods");
    if (cls == NULL) {
        LOGE("Native registration unable to find class 'org/openni/NativeMethods'");
        return JNI_FALSE;
    }
    if (env->RegisterNatives(cls, methods, methodsCount) < 0) {
        LOGE("RegisterNatives failed for 'org/openni/NativeMethods'");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

#ifdef ANDROID
#include "org_openni_android_OpenNIView.h"

static int register_org_openni_android_OpenNIView(JNIEnv* env)
{
    static JNINativeMethod methods[] =
    {
		{ "nativeCreate", "()J", (void*)&Java_org_openni_android_OpenNIView_nativeCreate },
		{ "nativeDestroy", "(J)V", (void*)&Java_org_openni_android_OpenNIView_nativeDestroy },
		{ "nativeUpdate", "(JLjava/nio/ByteBuffer;IIJ)V", (void*)&Java_org_openni_android_OpenNIView_nativeUpdate },
		{ "nativeClear", "(JLjava/nio/ByteBuffer;)V", (void*)&Java_org_openni_android_OpenNIView_nativeClear },

    };
    static int methodsCount = sizeof(methods)/sizeof(methods[0]);
    jclass cls = env->FindClass("org/openni/android/OpenNIView");
    if (cls == NULL) {
        LOGE("Native registration unable to find class 'org/openni/android/OpenNIView'");
        return JNI_FALSE;
    }
    if (env->RegisterNatives(cls, methods, methodsCount) < 0) {
        LOGE("RegisterNatives failed for 'org/openni/android/OpenNIView'");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}
#endif

static int registerNatives(JNIEnv* env)
{
	if (register_org_openni_NativeMethods(env) != JNI_TRUE) return JNI_FALSE;
#ifdef ANDROID
	if (register_org_openni_android_OpenNIView(env) != JNI_TRUE) return JNI_FALSE;
#endif
	return JNI_TRUE;
}
