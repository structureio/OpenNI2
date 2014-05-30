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
 
//---------------------------------------------------------------------------
// Includes 
//---------------------------------------------------------------------------
#include <XnOS.h>
#include <XnLog.h>
#include <XnJpeg.h>
#include <jerror.h>
#include <jpeglib.h>
#include <setjmp.h>

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#define XN_MASK_JPEG "JPEG"
#define XN_STREAM_STRING_BAD_FORMAT -1

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------
XN_PRAGMA_START_DISABLED_WARNING_SECTION(XN_STRUCT_PADDED_WARNING_ID);
typedef struct XnLibJpegErrorMgr
{
	struct jpeg_error_mgr pub;

	jmp_buf setjmpBuffer;
} XnLibJpegErrorMgr; 
XN_PRAGMA_STOP_DISABLED_WARNING_SECTION;

typedef struct XnStreamCompJPEGContext
{
	jpeg_compress_struct		jCompStruct;
	jpeg_error_mgr				jErrMgr;
	struct jpeg_destination_mgr	jDestMgr;
} XnStreamCompJPEGContext;

typedef struct XnStreamUncompJPEGContext
{
	jpeg_decompress_struct	jDecompStruct;
	XnLibJpegErrorMgr		jErrMgr;
	struct jpeg_source_mgr	jSrcMgr;
} XnStreamUncompJPEGContext;

void XnStreamJPEGDecompSkipFunction(struct jpeg_decompress_struct* pjDecompStruct, long nNumBytes)
{
	// Skip bytes in the internal buffer
	pjDecompStruct->src->next_input_byte += (size_t)nNumBytes;
	pjDecompStruct->src->bytes_in_buffer -= (size_t)nNumBytes;
}

boolean XnStreamJPEGDecompDummyFailFunction(struct jpeg_decompress_struct* /*pjDecompStruct*/)
{
	// If we ever got to the point we need to allocate more memory, something is wrong!
	return (FALSE);
}

void XnStreamJPEGDecompDummyFunction(struct jpeg_decompress_struct* /*pjDecompStruct*/)
{
	// Dummy libjpeg function to wrap internal buffers usage...
}

void XnStreamJPEGDummyErrorExit(j_common_ptr cinfo)
{
	XnLibJpegErrorMgr* errMgr = (XnLibJpegErrorMgr*)cinfo->err; 

	longjmp(errMgr->setjmpBuffer, 1); 
}

void  XnStreamJPEGCompDummyFunction(struct jpeg_compress_struct* /*pjCompStruct*/)
{
	// Dummy libjpeg function to wrap internal buffers usage...
}

boolean XnStreamJPEGCompDummyFailFunction(struct jpeg_compress_struct* /*pjCompStruct*/)
{
	// If we ever got to the point we need to allocate more memory, something is wrong!
	return (FALSE);
}

XnStatus XnStreamFreeCompressImageJ(XnStreamCompJPEGContext* pStreamCompJPEGContext)
{
	// Validate the input/output pointers (to make sure none of them is NULL)
	XN_VALIDATE_INPUT_PTR(pStreamCompJPEGContext);

	jpeg_destroy_compress(&pStreamCompJPEGContext->jCompStruct);

	XN_DELETE(pStreamCompJPEGContext);

	// All is good...
	return (XN_STATUS_OK);
}

XnStatus XnStreamCompressImage8J(XnStreamCompJPEGContext* pStreamCompJPEGContext, const XnUInt8* pInput, XnUInt8* pOutput, XnUInt32* pnOutputSize, const XnUInt32 nXRes, const XnUInt32 nYRes, const XnUInt32 nQuality)
{
	// Local function variables
	XnUInt8* pCurrScanline = (XnUInt8*)pInput;
	XnUInt32 nYIndex = 0;
	jpeg_compress_struct* pjCompStruct = NULL;	

	// Validate the input/output pointers (to make sure none of them is NULL)
	XN_VALIDATE_INPUT_PTR(pStreamCompJPEGContext);
	XN_VALIDATE_INPUT_PTR(pInput);
	XN_VALIDATE_OUTPUT_PTR(pOutput);
	XN_VALIDATE_OUTPUT_PTR(pnOutputSize);

	pjCompStruct = &pStreamCompJPEGContext->jCompStruct;

	pjCompStruct->in_color_space = JCS_GRAYSCALE;
	jpeg_set_defaults(pjCompStruct);
	pjCompStruct->input_components = 1;
	pjCompStruct->num_components = 1;
	pjCompStruct->image_width = nXRes;
	pjCompStruct->image_height = nYRes;
	pjCompStruct->data_precision = 8;
	pjCompStruct->input_gamma = 1.0;

	jpeg_set_quality(pjCompStruct, nQuality, FALSE);

	pjCompStruct->dest->next_output_byte = (JOCTET*)pOutput;
	pjCompStruct->dest->free_in_buffer = *pnOutputSize;

	jpeg_start_compress(pjCompStruct, TRUE);

	for (nYIndex = 0; nYIndex < nYRes; nYIndex++)
	{
		jpeg_write_scanlines(pjCompStruct, &pCurrScanline, 1);

		pCurrScanline += nXRes;
	}

	jpeg_finish_compress(pjCompStruct);

	*pnOutputSize -= (XnUInt32)pjCompStruct->dest->free_in_buffer;

	// All is good...
	return (XN_STATUS_OK);
}

XnStatus XnStreamCompressImage24J(XnStreamCompJPEGContext* pStreamCompJPEGContext, const XnUInt8* pInput, XnUInt8* pOutput, XnUInt32* pnOutputSize, const XnUInt32 nXRes, const XnUInt32 nYRes, const XnUInt32 nQuality)
{
	// Local function variables
	XnUInt8* pCurrScanline = (XnUChar*)pInput;
	XnUInt32 nYIndex = 0;
	XnUInt32 nScanLineSize = 0;
	jpeg_compress_struct* pjCompStruct = NULL;	

	// Validate the input/output pointers (to make sure none of them is NULL)
	XN_VALIDATE_INPUT_PTR(pStreamCompJPEGContext);
	XN_VALIDATE_INPUT_PTR(pInput);
	XN_VALIDATE_OUTPUT_PTR(pOutput);
	XN_VALIDATE_OUTPUT_PTR(pnOutputSize);

	pjCompStruct = &pStreamCompJPEGContext->jCompStruct;

	pjCompStruct->in_color_space = JCS_RGB;
	jpeg_set_defaults(pjCompStruct);
	pjCompStruct->input_components = 3;
	pjCompStruct->num_components = 3;
	pjCompStruct->image_width = nXRes;
	pjCompStruct->image_height = nYRes;
	pjCompStruct->data_precision = 8;
	pjCompStruct->input_gamma = 1.0;

	jpeg_set_quality(pjCompStruct, nQuality, FALSE);

	pjCompStruct->dest->next_output_byte = (JOCTET*)pOutput;
	pjCompStruct->dest->free_in_buffer = *pnOutputSize;

	jpeg_start_compress(pjCompStruct, TRUE);

	nScanLineSize = nXRes * 3;
	for (nYIndex = 0; nYIndex < nYRes; nYIndex++)
	{
		jpeg_write_scanlines(pjCompStruct, &pCurrScanline, 1);

		pCurrScanline += nScanLineSize;
	}

	jpeg_finish_compress(pjCompStruct);

	*pnOutputSize -= (XnUInt32)pjCompStruct->dest->free_in_buffer;

	// All is good...
	return (XN_STATUS_OK);
}

void XnStreamJPEGOutputMessage(j_common_ptr cinfo)
{
	struct jpeg_error_mgr* err = cinfo->err;
	int msg_code = err->msg_code;
	if (msg_code == JWRN_EXTRANEOUS_DATA)
	{
		// NOTE: we are aware this problem occurs. Log a warning every once in a while
		static XnUInt32 nTimes = 0;
		if (++nTimes == 50)
		{
			char buffer[JMSG_LENGTH_MAX];

			/* Create the message */
			(*cinfo->err->format_message) (cinfo, buffer);

			//Temporary disabled this error since it happens all the time and it's a known issue.
			//xnLogWarning(XN_MASK_JPEG, "JPEG: The following warning occurred 50 times: %s", buffer);
			nTimes = 0;
		}
	}
	else
	{
		char buffer[JMSG_LENGTH_MAX];

		/* Create the message */
		(*cinfo->err->format_message) (cinfo, buffer);

		xnLogWarning(XN_MASK_JPEG, "JPEG: %s", buffer);
	}
}

XnStatus XnStreamInitCompressImageJ(XnStreamCompJPEGContext** ppStreamCompJPEGContext)
{
	// Validate the input/output pointers (to make sure none of them is NULL)
 	//XN_VALIDATE_OUTPUT_PTR(*ppStreamCompJPEGContext);

	XnStreamCompJPEGContext* pStreamCompJPEGContext = NULL;
	pStreamCompJPEGContext = XN_NEW(XnStreamCompJPEGContext);

	if (NULL == pStreamCompJPEGContext)
	{
		return XN_STATUS_ERROR;
	}
 
 	pStreamCompJPEGContext->jCompStruct.err = jpeg_std_error(&pStreamCompJPEGContext->jErrMgr);
 
 	jpeg_create_compress(&pStreamCompJPEGContext->jCompStruct);
 
	pStreamCompJPEGContext->jCompStruct.dest = &pStreamCompJPEGContext->jDestMgr;
 	pStreamCompJPEGContext->jCompStruct.dest->init_destination = XnStreamJPEGCompDummyFunction;
 	pStreamCompJPEGContext->jCompStruct.dest->empty_output_buffer = XnStreamJPEGCompDummyFailFunction;
 	pStreamCompJPEGContext->jCompStruct.dest->term_destination = XnStreamJPEGCompDummyFunction;
	*ppStreamCompJPEGContext = pStreamCompJPEGContext;
	// All is good...
	return (XN_STATUS_OK);
}

XnStatus XnStreamInitUncompressImageJ(XnStreamUncompJPEGContext** ppStreamUncompJPEGContext)
{
	// Validate the input/output pointers (to make sure none of them is NULL)
	//XN_VALIDATE_OUTPUT_PTR(*ppStreamUncompJPEGContext);

	XnStreamUncompJPEGContext* pStreamUncompJPEGContext = NULL;
	pStreamUncompJPEGContext = XN_NEW(XnStreamUncompJPEGContext);

	if (NULL == pStreamUncompJPEGContext)
	{
		return XN_STATUS_ERROR;
	}

	pStreamUncompJPEGContext->jDecompStruct.err = jpeg_std_error(&pStreamUncompJPEGContext->jErrMgr.pub);
	pStreamUncompJPEGContext->jErrMgr.pub.output_message = XnStreamJPEGOutputMessage;
 	pStreamUncompJPEGContext->jErrMgr.pub.error_exit = XnStreamJPEGDummyErrorExit;

	jpeg_create_decompress(&pStreamUncompJPEGContext->jDecompStruct);
 
	pStreamUncompJPEGContext->jDecompStruct.src = &pStreamUncompJPEGContext->jSrcMgr;
	pStreamUncompJPEGContext->jDecompStruct.src->init_source = XnStreamJPEGDecompDummyFunction;
	pStreamUncompJPEGContext->jDecompStruct.src->fill_input_buffer = XnStreamJPEGDecompDummyFailFunction;
	pStreamUncompJPEGContext->jDecompStruct.src->skip_input_data = XnStreamJPEGDecompSkipFunction;
	pStreamUncompJPEGContext->jDecompStruct.src->resync_to_restart = jpeg_resync_to_restart;
	pStreamUncompJPEGContext->jDecompStruct.src->term_source = XnStreamJPEGDecompDummyFunction;

	*ppStreamUncompJPEGContext = pStreamUncompJPEGContext;

	// All is good...
	return (XN_STATUS_OK);
}

XnStatus XnStreamFreeUncompressImageJ(XnStreamUncompJPEGContext* pStreamUncompJPEGContext)
{
	// Validate the input/output pointers (to make sure none of them is NULL)
	XN_VALIDATE_INPUT_PTR(pStreamUncompJPEGContext);

	jpeg_destroy_decompress(&pStreamUncompJPEGContext->jDecompStruct);

	XN_DELETE(pStreamUncompJPEGContext);

	// All is good...
	return (XN_STATUS_OK);
}

// to allow the use of setjmp
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
#pragma warning(push)
#pragma warning(disable: 4611)
#endif

XnStatus XnStreamUncompressImageJ(XnStreamUncompJPEGContext* pStreamUncompJPEGContext, const XnUInt8* pInput, const XnUInt32 nInputSize, XnUInt8* pOutput, XnUInt32* pnOutputSize)
{
	// Local function variables
	XnUInt8* pCurrScanline = pOutput;
	XnUInt8* pNextScanline = NULL;
	XnUInt8* pOutputEnd = 0;
	XnUInt32 nScanLineSize = 0;
	XnUInt32 nOutputSize = 0;
	jpeg_decompress_struct* pjDecompStruct = NULL;

	// Validate the input/output pointers (to make sure none of them is NULL)
	XN_VALIDATE_INPUT_PTR(pStreamUncompJPEGContext);
	XN_VALIDATE_INPUT_PTR(pInput);
	XN_VALIDATE_OUTPUT_PTR(pOutput);
	XN_VALIDATE_OUTPUT_PTR(pnOutputSize);

	if (nInputSize == 0)
	{
		xnLogError(XN_MASK_JPEG, "The compressed input buffer is too small to be valid!");
		return (XN_STATUS_INPUT_BUFFER_OVERFLOW);
	}

	pOutputEnd = pOutput + *pnOutputSize;

	pjDecompStruct = &pStreamUncompJPEGContext->jDecompStruct;

	pjDecompStruct->src->bytes_in_buffer = nInputSize;
	pjDecompStruct->src->next_input_byte = pInput;

	if (setjmp(pStreamUncompJPEGContext->jErrMgr.setjmpBuffer))
	{
		//If we get here, the JPEG code has signaled an error.
		XnStreamFreeUncompressImageJ(pStreamUncompJPEGContext);
		XnStreamInitUncompressImageJ(&pStreamUncompJPEGContext);

		*pnOutputSize = 0;
		xnLogError(XN_MASK_JPEG, "Xiron I/O decompression failed!");
		return (XN_STATUS_ERROR);
	} 

	jpeg_read_header(pjDecompStruct, TRUE);

	jpeg_start_decompress(pjDecompStruct);

	nScanLineSize = pjDecompStruct->output_width * pjDecompStruct->num_components;

	nOutputSize = pjDecompStruct->output_height * nScanLineSize;
	if (nOutputSize > *pnOutputSize)
	{
		XnStreamFreeUncompressImageJ(pStreamUncompJPEGContext);
		XnStreamInitUncompressImageJ(&pStreamUncompJPEGContext);

		*pnOutputSize = 0;

		return (XN_STATUS_OUTPUT_BUFFER_OVERFLOW);
	}

	while (pStreamUncompJPEGContext->jDecompStruct.output_scanline < pStreamUncompJPEGContext->jDecompStruct.output_height)
	{
		pNextScanline = pCurrScanline+nScanLineSize;

		if (pNextScanline > pOutputEnd)
		{
			XnStreamFreeUncompressImageJ(pStreamUncompJPEGContext);
			XnStreamInitUncompressImageJ(&pStreamUncompJPEGContext);

			*pnOutputSize = 0;

			return (XN_STATUS_OUTPUT_BUFFER_OVERFLOW);
		}

		jpeg_read_scanlines(pjDecompStruct, &pCurrScanline, 1);
		pCurrScanline = pNextScanline;
	}

	jpeg_finish_decompress(pjDecompStruct);

	*pnOutputSize = nOutputSize;

	// All is good...
	return (XN_STATUS_OK);
}

#if (XN_PLATFORM == XN_PLATFORM_WIN32)
#pragma warning(pop)
#endif
