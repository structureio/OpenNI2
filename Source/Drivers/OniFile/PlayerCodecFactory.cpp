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
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "PlayerCodecFactory.h"
#include "Formats/XnUncompressedCodec.h"
#include "Formats/Xn16zCodec.h"
#include "Formats/Xn16zEmbTablesCodec.h"
#include "Formats/Xn8zCodec.h"
#include "Formats/XnJpegCodec.h"
#include "OniCProperties.h"
//#include <XnLog.h>

namespace oni_file {

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------
XnStatus PlayerCodecFactory::Create(XnCodecID nCodecID, PlayerSource* pSource, XnCodec** ppCodec)
{
	XnCodec* pCodec = NULL;

	switch (nCodecID)
	{
		case XN_CODEC_UNCOMPRESSED:
		{
			XN_VALIDATE_NEW_AND_INIT(pCodec, XnUncompressedCodec);
			break;
		}
		case XN_CODEC_16Z:
		{
			XN_VALIDATE_NEW_AND_INIT(pCodec, Xn16zCodec);
			break;
		}
		case XN_CODEC_16Z_EMB_TABLES:
		{
			// first we need to find max depth
			int nMaxDepth;
			int dataSize = sizeof(nMaxDepth);
			OniStatus rc1 = pSource->GetProperty(ONI_STREAM_PROPERTY_MAX_VALUE, &nMaxDepth, &dataSize);
			if (rc1 != ONI_STATUS_OK)
			{
				return XN_STATUS_ERROR;
			}

			XN_VALIDATE_NEW_AND_INIT(pCodec, Xn16zEmbTablesCodec, (OniDepthPixel)nMaxDepth);
			break;
		}
		case XN_CODEC_8Z:
		{
			XN_VALIDATE_NEW_AND_INIT(pCodec, Xn8zCodec);
			break;
		}
		case XN_CODEC_JPEG:
		{
			// check what is the output format
			OniVideoMode videoMode;
			int dataSize = sizeof(videoMode);
			OniStatus rc1 = pSource->GetProperty(ONI_STREAM_PROPERTY_VIDEO_MODE, &videoMode, &dataSize);
			if (rc1 != ONI_STATUS_OK)
			{
				return XN_STATUS_ERROR;
			}

			XnBool bRGB = FALSE;

			switch (videoMode.pixelFormat)
			{
				case ONI_PIXEL_FORMAT_GRAY8:
				{
					bRGB = FALSE;
					break;
				}
				case ONI_PIXEL_FORMAT_RGB888:
				{
					bRGB = TRUE;
					break;
				}
				default:
				{
					//XN_LOG_WARNING_RETURN(XN_STATUS_ERROR, XN_MASK_DDK, "Codec factory currently supports JPEG codec only for streams of type Gray8 or RGB24!");
					return XN_STATUS_ERROR;
				}
			}

			// take X and Y res
			XnUInt32 nXRes = videoMode.resolutionX;
			XnUInt32 nYRes = videoMode.resolutionY;
			XN_VALIDATE_NEW_AND_INIT(pCodec, XnJpegCodec, bRGB, nXRes, nYRes);
			break;
		}
		default:
		{
			//XN_LOG_WARNING_RETURN(XN_STATUS_ERROR, XN_MASK_DDK, "Codec factory does not support compression type %d", nFormat);
			return XN_STATUS_ERROR;
		}
	}

	*ppCodec = pCodec;
	return (XN_STATUS_OK);
}

void PlayerCodecFactory::Destroy(XnCodec* pCodec)
{
	XN_DELETE(pCodec);
}

} // namespace oni_files_player
