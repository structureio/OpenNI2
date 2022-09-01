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
#ifndef _XN_DEVICESENSOR_H_
#define _XN_DEVICESENSOR_H_

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnPlatform.h>
#include <XnDevice.h>
#include <XnDDK.h>
#include "XnDeviceSensorIO.h"
#include <XnFPSCalculator.h>
#include <XnLog.h>
#include <XnScheduler.h>
#include <Core/XnBuffer.h>
#include <DDK/XnFrameBufferManager.h>
#include "XnSensorFPS.h"
#include "XnFirmwareInfo.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define XN_DEVICE_SENSOR_THREAD_KILL_TIMEOUT 5000

#define XN_DEVICE_SENSOR_DEPTH_CMOS_XRES 1280
#define XN_DEVICE_SENSOR_DEPTH_CMOS_YRES 1024

#define XN_DEVICE_SENSOR_VGA_DEPTH_XRES 640
#define XN_DEVICE_SENSOR_VGA_DEPTH_YRES 480

#define XN_DEVICE_SENSOR_MIN_DEPTH 0
#define XN_DEVICE_SENSOR_MAX_DEPTH_1_MM 10000
#define XN_DEVICE_SENSOR_MAX_DEPTH_100_UM 65534
#define XN_DEVICE_SENSOR_NO_DEPTH_VALUE 0
#define XN_DEVICE_SENSOR_MAX_SHIFT_VALUE 2048/*336*/

#define XN_DEVICE_SENSOR_BOARDID_SEP ":"
#define XN_DEVICE_SENSOR_DEFAULT_ID "*"

#define XN_DEVICE_SENSOR_INI_FILE_EXT ".ini"

#define XN_SENSOR_PROTOCOL_SENSOR_ID_LENGTH	16

#define XN_SENSOR_TIMESTAMP_SANITY_DIFF 10 // in ms

#define XN_MASK_DEVICE_SENSOR           "DEV"
#define XN_MASK_DEVICE_IO               "DIO"
#define XN_MASK_SENSOR_PROTOCOL         "SPX"
#define XN_MASK_SENSOR_PROTOCOL_IMAGE   XN_MASK_SENSOR_PROTOCOL "SPI"
#define XN_MASK_SENSOR_PROTOCOL_DEPTH   XN_MASK_SENSOR_PROTOCOL "SPD"
#define XN_MASK_SENSOR_PROTOCOL_AUDIO   XN_MASK_SENSOR_PROTOCOL "SPA"
#define XN_MASK_SENSOR_READ             "SRX"
#define XN_MASK_SENSOR_READ_IMAGE       XN_MASK_SENSOR_READ "SRI"
#define XN_MASK_SENSOR_READ_DEPTH       XN_MASK_SENSOR_READ "SRD"
#define XN_MASK_SENSOR_READ_AUDIO       XN_MASK_SENSOR_READ "SRA"
#define XN_DUMP_AUDIO_IN                "AudioIn"
#define XN_DUMP_IMAGE_IN                "ImageIn"
#define XN_DUMP_DEPTH_IN                "DepthIn"
#define XN_DUMP_MINI_PACKETS            "MiniPackets"
#define XN_DUMP_TIMESTAMPS              "SensorTimestamps"
#define XN_DUMP_BANDWIDTH               "SensorBandwidth"
#define XN_DUMP_BAD_IMAGE               "BadImage"
#define XN_DUMP_FRAME_SYNC              "FrameSync"
#define XN_DUMP_SENSOR_LOG              "SensorLog"

//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
class XnSensorFirmware;
struct XnDevicePrivateData;
class XnSensorFixedParams;
class XnSensorFPS;
class XnCmosInfo;

//---------------------------------------------------------------------------
// Structures & Enums
//---------------------------------------------------------------------------

typedef struct XnSensorObjects
{
	XnSensorObjects(XnSensorFirmware* pFirmware, XnDevicePrivateData* pDevicePrivateData, XnSensorFPS* pFPS, XnCmosInfo* pCmosInfo) :
		pFirmware(pFirmware),
		pDevicePrivateData(pDevicePrivateData),
		pFPS(pFPS),
		pCmosInfo(pCmosInfo)
	{}

	XnSensorFirmware* pFirmware;
	XnDevicePrivateData* pDevicePrivateData;
	XnSensorFPS* pFPS;
	XnCmosInfo* pCmosInfo;
} XnSensorObjects;

typedef struct XnHWInfo
{
	XnHWVer	  nHWVer;
} XnHWInfo;

typedef struct XnChipInfo
{
	XnChipVer	nChipVer;
} XnChipInfo;

typedef enum {
	RGBREG_NONE = 0,
	RGBREG_FIX_IMAGE = 1,
	RGBREG_FIX_DEPTH = 2
} XnDeviceSensorRGBRegType;

typedef struct
{
	XN_THREAD_HANDLE	hThread;
	XnBool				bKillThread;
	XnBool				bThreadAlive;
} XnDeviceSensorThreadContext;

typedef struct XnRegistrationFunctionCoefficients
{
	XnDouble dA;
	XnDouble dB;
	XnDouble dC;
	XnDouble dD;
	XnDouble dE;
	XnDouble dF;
} XnRegistrationFunctionCoefficients;

typedef struct
{
	/* Is this the first time timestamp is calculated. */
	XnBool bFirst;
	/* The device TS which we use as reference for calculation. */
	XnUInt32 nReferenceTS;
	/* The time corresponding to the TS in nReferenceTS. */
	XnUInt64 nTotalTicksAtReferenceTS;
	/* The last device TS received. */
	XnUInt32 nLastDeviceTS;
	/* The last result time calculated. */
	XnUInt64 nLastResultTime;
	/* Stream name - for debug purposes. */
	const XnChar* csStreamName;
} XnTimeStampData;

typedef struct XnDeviceSensorGMCPoint
{
	XnUInt16 m_X;
	XnUInt16 m_Y;
	XnUInt16 m_DX;
	XnInt16 m_DY;
	XnUInt16 m_Score;
} XnDeviceSensorGMCPoint;

typedef struct XnCmosBlankingCoefficients
{
	XnFloat fA;
	XnFloat fB;
} XnCmosBlankingCoefficients;

typedef struct XnCmosBlankingInformation
{
	XnCmosBlankingCoefficients Coefficients[2];
} XnCmosBlankingInformation;

typedef struct XnDeviceInformation
{
	XnChar strDeviceName[128];
	XnChar strVendorData[128];
} XnDeviceInformation;

typedef XnStatus (XN_CALLBACK_TYPE* NewAudioDataCallback)(void* pCookie);

struct XnSpecificUsbDevice; // Forward Declaration
class XnSensor; // Forward Declaration

typedef struct XnDeviceAudioBuffer
{
	XN_CRITICAL_SECTION_HANDLE hLock;
	/** A single (big) buffer for audio. */
	XnUInt8* pAudioBuffer;
	/** An array of pointers into the audio buffer. */
	XnUInt64* pAudioPacketsTimestamps;
	/** The index of the next packet that should be written. */
	volatile XnUInt32 nAudioWriteIndex;
	/** The index of the next packet that can be read. */
	volatile XnUInt32 nAudioReadIndex;
	/** Size of the audio buffer, in packets. */
	XnUInt32 nAudioBufferNumOfPackets;
	/** Size of the audio buffer, in bytes. */
	XnUInt32 nAudioBufferSize;
	XnUInt32 nAudioPacketSize;
	/** A callback for new data */
	NewAudioDataCallback pAudioCallback;
	void* pAudioCallbackCookie;
} XnDeviceAudioBuffer;

typedef struct XnDevicePrivateData
{
	XnVersions Version;

	XN_SENSOR_HANDLE	SensorHandle;
	XnFirmwareInfo		FWInfo;
	XnHWInfo			HWInfo;
	XnChipInfo			ChipInfo;

	XnSpecificUsbDevice* pSpecificDepthUsb;
	XnSpecificUsbDevice* pSpecificImageUsb;
	XnSpecificUsbDevice* pSpecificMiscUsb;

	XnFloat fDeviceFrequency;

	/** Keeps the global reference TS (the one marking time-zero). */
	XnUInt32 nGlobalReferenceTS;
	/** Keeps the OS time of global reference TS. */
	XnUInt64 nGlobalReferenceOSTime;

	/** A general critical section used to synch end-points threads. */
	XN_CRITICAL_SECTION_HANDLE hEndPointsCS;

	/** Used to dump timestamps data. */
	XnDumpFile* TimestampsDump;
	/** Used to dump bandwidth data. */
	XnDumpFile* BandwidthDump;
	/** Used to dump MiniPackets data. */
	XnDumpFile* MiniPacketsDump;

	XnSensor* pSensor;

	XN_MUTEX_HANDLE hExecuteMutex;

	XnDeviceSensorThreadContext		LogThread;
	/** GMC Mode. */
	XnUInt32 nGMCMode;
	XnBool bWavelengthCorrectionEnabled;
	XnBool bWavelengthCorrectionDebugEnabled;

} XnDevicePrivateData;

#pragma pack (push, 1)

typedef struct XnFixedParams
{
	// Misc
	XnInt32 nSerialNumber;
	XnInt32 nWatchDogTimeout;

	// Flash
	XnInt32 nFlashType;
	XnInt32 nFlashSize;
	XnInt32 nFlashBurstEnable;
	XnInt32 nFmifReadBurstCycles;
	XnInt32 nFmifReadAccessCycles;
	XnInt32 nFmifReadRecoverCycles;
	XnInt32 nFmifWriteAccessCycles;
	XnInt32 nFmifWriteRecoverCycles;
	XnInt32 nFmifWriteAssertionCycles;
	
	// Audio
	XnInt32 nI2SLogicClockPolarity;

	// Depth
	XnInt32 nDepthCiuHorizontalSyncPolarity;
	XnInt32 nDepthCiuVerticalSyncPolarity;
	XnInt32 nDepthCmosType;
	XnInt32 nDepthCmosI2CAddress;
	XnInt32 nDepthCmosI2CBus;

	// Image
	XnInt32 nImageCiuHorizontalSyncPolarity;
	XnInt32 nImageCiuVerticalSyncPolarity;
	XnInt32 nImageCmosType;
	XnInt32 nImageCmosI2CAddress;
	XnInt32 nImageCmosI2CBus;

	// Geometry
	XnInt32 nIrCmosCloseToProjector;
	XnFloat fDCmosEmitterDistance;
	XnFloat fDCmosRCmosDistance;
	XnFloat fReferenceDistance;
	XnFloat fReferencePixelSize;

	// Clocks
	XnInt32 nPllValue;
	XnInt32 nSystemClockDivider;
	XnInt32 nRCmosClockDivider;
	XnInt32 nDCmosClockDivider;
	XnInt32 nAdcClocDivider;
	XnInt32 nI2CStandardSpeedHCount;
	XnInt32 nI2CStandardSpeedLCount;
	
	XnInt32 nI2CHoldFixDelay;

	XnInt32 nSensorType;
	XnInt32 nDebugMode;
	XnInt32 nUseExtPhy;
	XnInt32 bProjectorProtectionEnabled;
	XnInt32 nProjectorDACOutputVoltage;
	XnInt32 nProjectorDACOutputVoltage2;
	XnInt32 nTecEmitterDelay;
} XnFixedParams;

typedef struct XnFixedParamsV26
{
	// Misc
	XnInt32 nSerialNumber;
	XnInt32 nWatchDogTimeout;

	// Flash
	XnInt32 nFlashType;
	XnInt32 nFlashSize;
	XnInt32 nFlashBurstEnable;
	XnInt32 nFmifReadBurstCycles;
	XnInt32 nFmifReadAccessCycles;
	XnInt32 nFmifReadRecoverCycles;
	XnInt32 nFmifWriteAccessCycles;
	XnInt32 nFmifWriteRecoverCycles;
	XnInt32 nFmifWriteAssertionCycles;

	// Audio
	XnInt32 nI2SLogicClockPolarity;

	// Depth
	XnInt32 nDepthCiuHorizontalSyncPolarity;
	XnInt32 nDepthCiuVerticalSyncPolarity;
	XnInt32 nDepthCmosType;
	XnInt32 nDepthCmosI2CAddress;
	XnInt32 nDepthCmosI2CBus;

	// Image
	XnInt32 nImageCiuHorizontalSyncPolarity;
	XnInt32 nImageCiuVerticalSyncPolarity;
	XnInt32 nImageCmosType;
	XnInt32 nImageCmosI2CAddress;
	XnInt32 nImageCmosI2CBus;

	// Geometry
	XnInt32 nIrCmosCloseToProjector;
	XnFloat fDCmosEmitterDistance;
	XnFloat fDCmosRCmosDistance;
	XnFloat fReferenceDistance;
	XnFloat fReferencePixelSize;

	// Clocks
	XnInt32 nPllValue;
	XnInt32 nSystemClockDivider;
	XnInt32 nRCmosClockDivider;
	XnInt32 nDCmosClockDivider;
	XnInt32 nAdcClocDivider;
	XnInt32 nI2CStandardSpeedHCount;
	XnInt32 nI2CStandardSpeedLCount;

	XnInt32 nI2CHoldFixDelay;

	XnInt32 nSensorType;
	XnInt32 nDebugMode;
	XnInt32 nTecEmitterDelay;
	XnInt32 nUseExtPhy;
} XnFixedParamsV26;

typedef struct XnFixedParamsV20
{
	// Misc
	XnInt32 nSerialNumber;
	XnInt32 nWatchDogTimeout;

	// Flash
	XnInt32 nFlashType;
	XnInt32 nFlashSize;
	XnInt32 nFlashBurstEnable;
	XnInt32 nFmifReadBurstCycles;
	XnInt32 nFmifReadAccessCycles;
	XnInt32 nFmifReadRecoverCycles;
	XnInt32 nFmifWriteAccessCycles;
	XnInt32 nFmifWriteRecoverCycles;
	XnInt32 nFmifWriteAssertionCycles;

	// Audio
	XnInt32 nI2SLogicClockPolarity;

	// Depth
	XnInt32 nDepthCiuHorizontalSyncPolarity;
	XnInt32 nDepthCiuVerticalSyncPolarity;
	XnInt32 nDepthCmosType;
	XnInt32 nDepthCmosI2CAddress;
	XnInt32 nDepthCmosI2CBus;

	// Image
	XnInt32 nImageCiuHorizontalSyncPolarity;
	XnInt32 nImageCiuVerticalSyncPolarity;
	XnInt32 nImageCmosType;
	XnInt32 nImageCmosI2CAddress;
	XnInt32 nImageCmosI2CBus;

	// Geometry
	XnInt32 nIrCmosCloseToProjector;
	XnFloat fDCmosEmitterDistance;
	XnFloat fDCmosRCmosDistance;
	XnFloat fReferenceDistance;
	XnFloat fReferencePixelSize;

	// Clocks
	XnInt32 nPllValue;
	XnInt32 nSystemClockDivider;
	XnInt32 nRCmosClockDivider;
	XnInt32 nDCmosClockDivider;
	XnInt32 nAdcClocDivider;
	XnInt32 nI2CStandardSpeedHCount;
	XnInt32 nI2CStandardSpeedLCount;

	XnInt32 nI2CHoldFixDelay;

	XnInt32 nSensorType;
	XnInt32 nDebugMode;
	XnInt32 nTecEmitterDelay;
} XnFixedParamsV20;

typedef struct XnRegistrationInformation1000
{
	XnRegistrationFunctionCoefficients FuncX;
	XnRegistrationFunctionCoefficients FuncY;
	XnDouble dBeta;
} XnRegistrationInformation1000;

typedef struct XnRegistrationInformation1080
{
	XnInt32 nRGS_DX_CENTER;
	XnInt32 nRGS_AX;
	XnInt32 nRGS_BX;
	XnInt32 nRGS_CX;
	XnInt32 nRGS_DX;
	XnInt32 nRGS_DX_START;
	XnInt32 nRGS_AY;
	XnInt32 nRGS_BY;
	XnInt32 nRGS_CY;
	XnInt32 nRGS_DY;
	XnInt32 nRGS_DY_START;
	XnInt32 nRGS_DX_BETA_START;
	XnInt32 nRGS_DY_BETA_START;
	XnInt32 nRGS_ROLLOUT_BLANK;
	XnInt32 nRGS_ROLLOUT_SIZE;
	XnInt32 nRGS_DX_BETA_INC;
	XnInt32 nRGS_DY_BETA_INC;
	XnInt32 nRGS_DXDX_START;
	XnInt32 nRGS_DXDY_START;
	XnInt32 nRGS_DYDX_START;
	XnInt32 nRGS_DYDY_START;
	XnInt32 nRGS_DXDXDX_START;
	XnInt32 nRGS_DYDXDX_START;
	XnInt32 nRGS_DXDXDY_START;
	XnInt32 nRGS_DYDXDY_START;
	XnInt32 nBACK_COMP1;
	XnInt32 nRGS_DYDYDX_START;
	XnInt32 nBACK_COMP2;
	XnInt32 nRGS_DYDYDY_START;
} XnRegistrationInformation1080;

typedef struct XnRegistrationPaddingInformation
{
	XnUInt16 nStartLines;
	XnUInt16 nEndLines;
	XnUInt16 nCroppingLines;
} XnRegistrationPaddingInformation;

typedef struct XnDepthInformation
{
	XnUInt16 nConstShift;
} XnDepthInformation;

typedef struct XnFrequencyInformation
{
	XnFloat fDeviceFrequency;
} XnFrequencyInformation;

typedef struct XnAudioSharedBuffer
{
	XnUInt32 nPacketCount;
	XnUInt32 nPacketSize;
	XnUInt32 nWritePacketIndex;
} XnAudioSharedBuffer;

#pragma pack (pop)

XnStatus XnDeviceSensorSetParam(XnDevicePrivateData* pDevicePrivateData, const XnChar* cpParamName, const XnInt32 nValue);

#endif 	
