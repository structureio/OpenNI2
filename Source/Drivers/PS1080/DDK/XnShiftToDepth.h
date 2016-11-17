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
#ifndef XNSHIFTTODEPTH_H
#define XNSHIFTTODEPTH_H

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnStreamFormats.h>
#include <XnStatus.h>
#include <XnDDK.h>

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------
typedef struct XnShiftToDepthConfig
{
	/** The zero plane distance in depth units. */
	XnFloat fZeroPlaneDistance;
	/** The zero plane pixel size */
	XnFloat fZeroPlanePixelSize;
	/** The distance between the emitter and the Depth Cmos */
	XnFloat fEmitterDCmosDistance;
	/** The maximum possible shift value from this device. */
	XnUInt32 nDeviceMaxShiftValue;
	/** The maximum possible depth from this device (as opposed to a cut-off). */
	XnUInt32 nDeviceMaxDepthValue;

	XnUInt32 nConstShift;
	XnUInt32 nPixelSizeFactor;
	XnUInt32 nParamCoeff;
	XnUInt32 nShiftScale;

	OniDepthPixel nDepthMinCutOff;
	OniDepthPixel nDepthMaxCutOff;

} XnShiftToDepthConfig;

typedef struct XnShiftToDepthTables
{
	XnBool bIsInitialized;
	/** The shift-to-depth table. */
	OniDepthPixel* pShiftToDepthTable;
	/** The number of entries in the shift-to-depth table. */
	XnUInt32 nShiftsCount;
	/** The depth-to-shift table. */
	XnUInt16* pDepthToShiftTable;
	/** The number of entries in the depth-to-shift table. */
	XnUInt32 nDepthsCount;
} XnShiftToDepthTables;

//---------------------------------------------------------------------------
// Functions Declaration
//---------------------------------------------------------------------------
XnStatus XnShiftToDepthInit(XnShiftToDepthTables* pShiftToDepth, const XnShiftToDepthConfig* pConfig);
XnStatus XnShiftToDepthUpdate(XnShiftToDepthTables* pShiftToDepth, const XnShiftToDepthConfig* pConfig);
XnStatus XnShiftToDepthConvert(XnShiftToDepthTables* pShiftToDepth, XnUInt16* pInput, XnUInt32 nInputSize, OniDepthPixel* pOutput);
XnStatus XnShiftToDepthFree(XnShiftToDepthTables* pShiftToDepth);

#endif // XNSHIFTTODEPTH_H
