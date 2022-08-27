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
// --------------------------------
// Includes
// --------------------------------
#include "OpenNI.h"
#include "XnLib.h"
#include "Draw.h"
#include "Device.h"
#include "Keyboard.h"
#include "XnMath.h"
#include "Capture.h"
#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
	#include <GLUT/glut.h>
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
	#include <GL/glut.h>
#endif
#include "MouseInput.h"
#include <XnPlatform.h>

#if (XN_PLATFORM == XN_PLATFORM_WIN32)
	#ifdef __INTEL_COMPILER
		#include <ia32intrin.h>
	#else
		#include <intrin.h>
	#endif
#endif

// --------------------------------
// Defines
// --------------------------------
#define MAX_DEPTH XN_MAX_UINT16

#define YUV422_U  0
#define YUV422_Y1 1
#define YUV422_V  2
#define YUV422_Y2 3
#define YUV422_BPP 4
#define YUV_RED   0
#define YUV_GREEN 1
#define YUV_BLUE  2
#define YUV_ALPHA  3
#define YUV_RGBA_BPP 4
#define YUYV_Y1 0
#define YUYV_U  1
#define YUYV_Y2 2
#define YUYV_V  3
#define YUYV_BPP 4
#define YUV_RED   0
#define YUV_GREEN 1
#define YUV_BLUE  2
#define YUV_RGB_BPP 3

// --------------------------------
// Types
// --------------------------------
typedef enum
{
	NOTIFICATION_MESSAGE,
	WARNING_MESSAGE,
	ERROR_MESSAGE,
	FATAL_MESSAGE,
	NUM_DRAW_MESSAGE_TYPES
} DrawMessageType;

typedef struct  
{
	StreamsDrawConfig Streams; 
	bool bShowPointer;
	bool bShowMessage;
	DrawMessageType messageType;
	bool bHelp;
	XnChar strErrorState[256];
	IntRect DepthLocation;
	IntRect ColorLocation;
} DrawConfig;

typedef struct
{
	const char* csName;
	StreamsDrawConfig Config;
} DrawConfigPreset;

typedef struct XnTextureMap
{
	IntPair Size;
	IntPair OrigSize;
	unsigned char* pMap;
	unsigned int nBytesPerPixel;
	GLuint nID;
	GLenum nFormat;
	bool bInitialized;
	IntPair CurSize;
} XnTextureMap;

// --------------------------------
// Global Variables
// --------------------------------
DrawConfig g_DrawConfig;

XnUInt8 PalletIntsR [256] = {0};
XnUInt8 PalletIntsG [256] = {0};
XnUInt8 PalletIntsB [256] = {0};

/* Linear Depth Histogram */
float g_pDepthHist[MAX_DEPTH];

const char* g_DepthDrawColoring[NUM_OF_DEPTH_DRAW_TYPES];
const char* g_ColorDrawColoring[NUM_OF_COLOR_DRAW_TYPES];

typedef struct DrawUserInput
{
	SelectionState State;
	IntRect Rect;
	IntPair Cursor;
} DrawUserInput;

DrawUserInput g_DrawUserInput;

float g_fMaxDepth = 0;

DrawConfigPreset g_Presets[PRESET_COUNT] = 
{
	// NAME,								    Depth_Type,               Transparency  Image_Type			  Arrangement
	{ "Standard Deviation",					{ { STANDARD_DEVIATION,	      1 },        { COLOR_OFF },          OVERLAY } },			// Obsolete
	{ "Depth Histogram",					{ { LINEAR_HISTOGRAM,	      1 },        { COLOR_OFF },          OVERLAY } },
	{ "Psychedelic Depth [Centimeters]",	{ { PSYCHEDELIC,			  1 },        { COLOR_OFF },          OVERLAY } },
	{ "Psychedelic Depth [Millimeters]",	{ { PSYCHEDELIC_SHADES,	      1 },        { COLOR_OFF },          OVERLAY } },
	{ "Rainbow Depth",						{ { CYCLIC_RAINBOW_HISTOGRAM, 1 },        { COLOR_OFF },          OVERLAY } },
	{ "Depth masked Color",					{ { DEPTH_OFF,			      1 },        { DEPTH_MASKED_COLOR }, OVERLAY } },
	{ "Background Removal",					{ { DEPTH_OFF,			      1 },        { DEPTH_MASKED_COLOR }, OVERLAY } },			// Obsolete
	{ "Side by Side",						{ { LINEAR_HISTOGRAM,	      1 },        { COLOR_NORMAL },	      SIDE_BY_SIDE } },
	{ "Depth on Color",						{ { LINEAR_HISTOGRAM,	      1 },        { COLOR_NORMAL },       OVERLAY } },
	{ "Transparent Depth on Color",			{ { LINEAR_HISTOGRAM,         0.6 },      { COLOR_NORMAL },       OVERLAY } },
	{ "Rainbow Depth on Color",				{ { RAINBOW,			      0.6 },      { COLOR_NORMAL },       OVERLAY } },
	{ "Cyclic Rainbow Depth on Color",		{ { CYCLIC_RAINBOW,	          0.6 },      { COLOR_NORMAL },       OVERLAY } },
	{ "Color Only",							{ { DEPTH_OFF,			      1 },        { COLOR_NORMAL },       OVERLAY } },
};

/* Texture maps for depth and color */
XnTextureMap g_texDepth      = {{0}};
XnTextureMap g_texColor      = {{0}};

/* A user message to be displayed. */
char g_csUserMessage[256];

bool g_bFullScreen = true;
bool g_bFirstTimeNonFull = true;
IntPair g_NonFullWinSize = { WIN_SIZE_X, WIN_SIZE_Y };

// --------------------------------
// Textures
// --------------------------------
int GetPowerOfTwo(int num)
{
	int result = 1;

	while (result < num)
		result <<= 1;

	return result;
}

void TextureMapInit(XnTextureMap* pTex, int nSizeX, int nSizeY, unsigned int nBytesPerPixel, int nCurX, int nCurY)
{
	// check if something changed
	if (pTex->bInitialized && pTex->OrigSize.X == nSizeX && pTex->OrigSize.Y == nSizeY)
	{
		if (pTex->CurSize.X != nCurX || pTex->CurSize.Y != nCurY)
		{
			// clear map
			xnOSMemSet(pTex->pMap, 0, pTex->Size.X * pTex->Size.Y * pTex->nBytesPerPixel);

			// update
			pTex->CurSize.X = nCurX;
			pTex->CurSize.Y = nCurY;
			return;
		}
	}

	// free memory if it was allocated
	if (pTex->pMap != NULL)
	{
		delete[] pTex->pMap;
		pTex->pMap = NULL;
	}

	// update it all
	pTex->OrigSize.X = nSizeX;
	pTex->OrigSize.Y = nSizeY;
	pTex->Size.X = GetPowerOfTwo(nSizeX);
	pTex->Size.Y = GetPowerOfTwo(nSizeY);
	pTex->nBytesPerPixel = nBytesPerPixel;
	pTex->CurSize.X = nCurX;
	pTex->CurSize.Y = nCurY;
	pTex->pMap = new unsigned char[pTex->Size.X * pTex->Size.Y * nBytesPerPixel];
	xnOSMemSet(pTex->pMap, 0, pTex->Size.X * pTex->Size.Y * nBytesPerPixel);
	
	if (!pTex->bInitialized)
	{
		glGenTextures(1, &pTex->nID);
		glBindTexture(GL_TEXTURE_2D, pTex->nID);

		switch (pTex->nBytesPerPixel)
		{
		case 3:
			pTex->nFormat = GL_RGB;
			break;
		case 4:
			pTex->nFormat = GL_RGBA;
			break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		pTex->bInitialized = TRUE;
	}
}

inline unsigned char* TextureMapGetLine(XnTextureMap* pTex, unsigned int nLine)
{
	return &pTex->pMap[nLine * pTex->Size.X * pTex->nBytesPerPixel];
}

void TextureMapSetPixel(XnTextureMap* pTex, int x, int y, int red, int green, int blue)
{
	if (x < 0 || y < 0 || x >= (int)pTex->OrigSize.X || y >= (int)pTex->OrigSize.Y)
		return;

	unsigned char* pPixel = TextureMapGetLine(pTex, y) + x * pTex->nBytesPerPixel;
	pPixel[0] = red;
	pPixel[1] = green;
	pPixel[2] = blue;

	if (pTex->nBytesPerPixel > 3)
		pPixel[3] = 255;
}

void TextureMapDrawCursor(XnTextureMap* pTex, IntPair cursor, int red = 255, int green = 0, int blue = 0)
{
	// marked pixel
	TextureMapSetPixel(pTex, cursor.X, cursor.Y, red, green, 0);

	// top left marker
	TextureMapSetPixel(pTex, cursor.X-2, cursor.Y-2, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X-2, cursor.Y-1, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X-1, cursor.Y-2, red, green, blue);

	// top right marker
	TextureMapSetPixel(pTex, cursor.X+2, cursor.Y-2, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X+2, cursor.Y-1, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X+1, cursor.Y-2, red, green, blue);

	// bottom left marker
	TextureMapSetPixel(pTex, cursor.X-2, cursor.Y+2, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X-2, cursor.Y+1, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X-1, cursor.Y+2, red, green, blue);

	// bottom right marker
	TextureMapSetPixel(pTex, cursor.X+2, cursor.Y+2, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X+2, cursor.Y+1, red, green, blue);
	TextureMapSetPixel(pTex, cursor.X+1, cursor.Y+2, red, green, blue);
}

void TextureMapUpdate(XnTextureMap* pTex)
{
	// set current texture object
	glBindTexture(GL_TEXTURE_2D, pTex->nID);

	// set the current image to the texture
	glTexImage2D(GL_TEXTURE_2D, 0, pTex->nFormat, pTex->Size.X, pTex->Size.Y, 0, pTex->nFormat, GL_UNSIGNED_BYTE, pTex->pMap);
}

void TextureMapDraw(XnTextureMap* pTex, IntRect* pLocation)
{
	// set current texture object
	glBindTexture(GL_TEXTURE_2D, pTex->nID);

	// turn on texture mapping
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// set drawing mode to rectangles
	glBegin(GL_QUADS);

	// set the color of the polygon
	glColor4f(1, 1, 1, 1);

	// upper left
	glTexCoord2f(0, 0);
	glVertex2f(pLocation->uLeft, pLocation->uBottom);
	// upper right
	glTexCoord2f((float)pTex->OrigSize.X/(float)pTex->Size.X, 0);
	glVertex2f(pLocation->uRight, pLocation->uBottom);
	// bottom right
	glTexCoord2f((float)pTex->OrigSize.X/(float)pTex->Size.X, (float)pTex->OrigSize.Y/(float)pTex->Size.Y);
	glVertex2f(pLocation->uRight, pLocation->uTop);
	// bottom left
	glTexCoord2f(0, (float)pTex->OrigSize.Y/(float)pTex->Size.Y);
	glVertex2f(pLocation->uLeft, pLocation->uTop);

	glEnd();

	// turn off texture mapping
	glDisable(GL_TEXTURE_2D);

	glDisable(GL_BLEND);
}

// --------------------------------
// Code
// --------------------------------
void CreateRainbowPallet()
{
	unsigned char r, g, b;
	for (int i=0; i<256; i++)
	{
		if (i<=29)
		{
			r = (unsigned char)(129.36-i*4.36);
			g = 0;
			b = (unsigned char)255;
		}
		else if (i<=86)
		{
			r = 0;
			g = (unsigned char)(-133.54+i*4.52);
			b = (unsigned char)255;
		}
		else if (i<=141)
		{
			r = 0;
			g = (unsigned char)255;
			b = (unsigned char)(665.83-i*4.72);
		}
		else if (i<=199)
		{
			r = (unsigned char)(-635.26+i*4.47);
			g = (unsigned char)255;
			b = 0;
		}
		else
		{
			r = (unsigned char)255;
			g = (unsigned char)(1166.81-i*4.57);
			b = 0;
		}

		PalletIntsR[i] = r;
		PalletIntsG[i] = g;
		PalletIntsB[i] = b;
	}
}

void glPrintString(void *font, const char *str)
{
	int i,l = (int)strlen(str);

	for(i=0; i<l; i++)
	{
		glutBitmapCharacter(font,*str++);
	}
}

void drawConfigChanged()
{
	// recalculate registration
	bool bRegistration = 
		(g_DrawConfig.Streams.ScreenArrangement == OVERLAY) && 
		(g_DrawConfig.Streams.Color.Coloring != COLOR_OFF) &&
		(g_DrawConfig.Streams.Depth.Coloring != DEPTH_OFF || g_DrawConfig.Streams.Color.Coloring == DEPTH_MASKED_COLOR);

	changeRegistration(bRegistration);
}

void setPreset(int preset)
{
	g_DrawConfig.Streams = g_Presets[preset].Config;
	drawConfigChanged();
}

const char* getPresetName(int preset)
{
	return g_Presets[preset].csName;
}

void setScreenLayout(int layout)
{
	g_DrawConfig.Streams.ScreenArrangement = (ScreenArrangementType)layout;
	drawConfigChanged();
}

void windowReshaped(int width, int height)
{
	g_NonFullWinSize.X = width;
	g_NonFullWinSize.Y = height;
}

void toggleFullScreen(int)
{
	if (g_bFullScreen)
	{
		if (g_bFirstTimeNonFull)
		{
			g_NonFullWinSize.X = WIN_SIZE_X/2;
			g_NonFullWinSize.Y = WIN_SIZE_Y/2;
			g_bFirstTimeNonFull = false;
		}

		glutReshapeWindow(g_NonFullWinSize.X, g_NonFullWinSize.Y);
		g_bFullScreen = false;
	}
	else
	{
		glutFullScreen();
		g_bFullScreen = true;
	}
}

void displayMessage(const char* csFormat, ...)
{
	g_DrawConfig.messageType = NOTIFICATION_MESSAGE;
	g_DrawConfig.bShowMessage = true;
	va_list args;
	va_start(args, csFormat);
	XnUInt32 nCount;
	xnOSStrFormatV(g_csUserMessage, sizeof(g_csUserMessage), &nCount, csFormat, args);
	va_end(args);
}

void displayError(const char* csFormat, ...)
{
	g_DrawConfig.messageType = ERROR_MESSAGE;
	g_DrawConfig.bShowMessage = true;
	va_list args;
	va_start(args, csFormat);
	XnUInt32 nCount;
	xnOSStrFormatV(g_csUserMessage, sizeof(g_csUserMessage), &nCount, csFormat, args);
	va_end(args);
}

void setErrorState(const char* strFormat, ...)
{
	va_list args;
	XnUInt32 nWritten;
	va_start(args, strFormat);
	xnOSStrFormatV(g_DrawConfig.strErrorState, sizeof(g_DrawConfig.strErrorState), &nWritten, strFormat, args);
	va_end(args);
}

void drawCropStream(openni::VideoStream& stream, IntRect location, IntRect selection, int dividedBy)
{
	if (!stream.isCroppingSupported())
	{
		return;
	}

	openni::VideoMode Mode = stream.getVideoMode();

	// check if entire selection is in location
	if (selection.uLeft >= location.uLeft &&
		selection.uRight <= location.uRight &&
		selection.uBottom >= location.uBottom &&
		selection.uTop <= location.uTop)
	{
		IntRect cropRect;
		cropRect.uBottom = Mode.getResolutionY() * (selection.uBottom - location.uBottom) / (location.uTop - location.uBottom);
		cropRect.uTop    = Mode.getResolutionY() * (selection.uTop - location.uBottom)    / (location.uTop - location.uBottom);
		cropRect.uLeft   = Mode.getResolutionX()  * (selection.uLeft - location.uLeft)     / (location.uRight - location.uLeft);
		cropRect.uRight  = Mode.getResolutionX()  * (selection.uRight - location.uLeft)    / (location.uRight - location.uLeft);

		int originX, originY, width, height;

		originX    = cropRect.uLeft;
		originY    = cropRect.uBottom;
		width  = cropRect.uRight - cropRect.uLeft;
		height = cropRect.uTop   - cropRect.uBottom;

		if ((originX % dividedBy) != 0)
			originX -= (originX % dividedBy);
		if ((width % dividedBy) != 0)
			width += dividedBy - (width % dividedBy);

		setStreamCropping(stream, originX, originY, width, height);
	}
}

void drawSelectionChanged(SelectionState state, IntRect selection)
{
	g_DrawUserInput.State = state;
	g_DrawUserInput.Rect = selection;

	if (state == SELECTION_DONE)
	{
		// Crop depth
		if (getDepthStream().isValid() && isDepthOn() && g_DrawConfig.Streams.Depth.Coloring != DEPTH_OFF)
		{
			drawCropStream(getDepthStream(), g_DrawConfig.DepthLocation, selection, 2);
		}

		// Crop image
		if (getColorStream().isValid() && isColorOn() && g_DrawConfig.Streams.Color.Coloring != COLOR_OFF)
		{
			drawCropStream(getColorStream(), g_DrawConfig.ColorLocation, selection, 4);
		}

		// Crop IR
		if (getIRStream().isValid() && isIROn() && g_DrawConfig.Streams.Color.Coloring != COLOR_OFF)
		{
			drawCropStream(getIRStream(), g_DrawConfig.ColorLocation, selection, 4);
		}
	}
}

void drawCursorMoved(IntPair location)
{
	g_DrawUserInput.Cursor = location;
}

void drawInit()
{
	g_DepthDrawColoring[DEPTH_OFF] = "Off";
	g_DepthDrawColoring[LINEAR_HISTOGRAM] = "Linear Histogram";
	g_DepthDrawColoring[PSYCHEDELIC] = "Psychedelic";
	g_DepthDrawColoring[PSYCHEDELIC_SHADES] = "Psychedelic (Millimeters)";
	g_DepthDrawColoring[RAINBOW] = "Rainbow";
	g_DepthDrawColoring[CYCLIC_RAINBOW] = "Cyclic Rainbow";
	g_DepthDrawColoring[CYCLIC_RAINBOW_HISTOGRAM] = "Cyclic Rainbow Histogram";
	g_DepthDrawColoring[STANDARD_DEVIATION] = "Standard Deviation";

	g_ColorDrawColoring[COLOR_OFF] = "Off";
	g_ColorDrawColoring[COLOR_NORMAL] = "Normal";
	g_ColorDrawColoring[DEPTH_MASKED_COLOR] = "Depth Masked Color";

	CreateRainbowPallet();

	setPreset(7);

	mouseInputRegisterForSelectionRectangle(drawSelectionChanged);
	mouseInputRegisterForCursorMovement(drawCursorMoved);
}

void togglePointerMode(int)
{
	g_DrawConfig.bShowPointer = !g_DrawConfig.bShowPointer;
}

void toggleHelpScreen(int)
{
	g_DrawConfig.bHelp = !g_DrawConfig.bHelp;
}

void calculateHistogram()
{
	xnOSMemSet(g_pDepthHist, 0, MAX_DEPTH*sizeof(float));
	int nNumberOfPoints = 0;

	openni::DepthPixel nValue;

	openni::VideoStream& depthGen = getDepthStream();

	if (!depthGen.isValid() || !getDepthFrame().isValid())
		return;

	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)getDepthFrame().getData();
	const openni::DepthPixel* pDepthEnd = pDepth + (getDepthFrame().getDataSize() / sizeof(openni::DepthPixel));

	while (pDepth != pDepthEnd)
	{
		nValue = *pDepth;

		XN_ASSERT(nValue <= MAX_DEPTH);

		if (nValue != 0)
		{
			g_pDepthHist[nValue]++;
			nNumberOfPoints++;
		}

		pDepth++;
	}

	XnUInt32 nIndex;
	for (nIndex=1; nIndex<MAX_DEPTH; nIndex++)
	{
		g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
	}
	for (nIndex=1; nIndex<MAX_DEPTH; nIndex++)
	{
		if (g_pDepthHist[nIndex] != 0)
		{
			g_pDepthHist[nIndex] = (nNumberOfPoints-g_pDepthHist[nIndex]) / nNumberOfPoints;
		}
	}
}

// --------------------------------
// Drawing
// --------------------------------
#if (XN_PLATFORM == XN_PLATFORM_WIN32)

void YUV422ToRGB888(const XnUInt8* pYUVImage, XnUInt8* pRGBAImage, XnUInt32 nYUVSize, XnUInt32 nRGBSize)
{
	const XnUInt8* pYUVLast = pYUVImage + nYUVSize - 8;
	XnUInt8* pRGBLast = pRGBAImage + nRGBSize - 16;

	const __m128 minus128 = _mm_set_ps1(-128);
	const __m128 plus113983 = _mm_set_ps1(1.13983F);
	const __m128 minus039466 = _mm_set_ps1(-0.39466F);
	const __m128 minus058060 = _mm_set_ps1(-0.58060F);
	const __m128 plus203211 = _mm_set_ps1(2.03211F);
	const __m128 zero = _mm_set_ps1(0);
	const __m128 plus255 = _mm_set_ps1(255);

	// define YUV floats
	__m128 y;
	__m128 u;
	__m128 v;

	__m128 temp;

	// define RGB floats
	__m128 r;
	__m128 g;
	__m128 b;

	// define RGB integers
	__m128i iR;
	__m128i iG;
	__m128i iB;

	XnUInt32* piR = (XnUInt32*)&iR;
	XnUInt32* piG = (XnUInt32*)&iG;
	XnUInt32* piB = (XnUInt32*)&iB;

	while (pYUVImage <= pYUVLast && pRGBAImage <= pRGBLast)
	{
		// process 4 pixels at once (values should be ordered backwards)
		y = _mm_set_ps(pYUVImage[YUV422_Y2 + YUV422_BPP], pYUVImage[YUV422_Y1 + YUV422_BPP], pYUVImage[YUV422_Y2], pYUVImage[YUV422_Y1]);
		u = _mm_set_ps(pYUVImage[YUV422_U + YUV422_BPP],  pYUVImage[YUV422_U + YUV422_BPP],  pYUVImage[YUV422_U],  pYUVImage[YUV422_U]);
		v = _mm_set_ps(pYUVImage[YUV422_V + YUV422_BPP],  pYUVImage[YUV422_V + YUV422_BPP],  pYUVImage[YUV422_V],  pYUVImage[YUV422_V]);

		u = _mm_add_ps(u, minus128); // u -= 128
		v = _mm_add_ps(v, minus128); // v -= 128

		/*

		http://en.wikipedia.org/wiki/YUV

		From YUV to RGB:
		R =     Y + 1.13983 V
		G =     Y - 0.39466 U - 0.58060 V
		B =     Y + 2.03211 U

		*/ 

		temp = _mm_mul_ps(plus113983, v);
		r = _mm_add_ps(y, temp);

		temp = _mm_mul_ps(minus039466, u);
		g = _mm_add_ps(y, temp);
		temp = _mm_mul_ps(minus058060, v);
		g = _mm_add_ps(g, temp);

		temp = _mm_mul_ps(plus203211, u);
		b = _mm_add_ps(y, temp);

		// make sure no value is smaller than 0
		r = _mm_max_ps(r, zero);
		g = _mm_max_ps(g, zero);
		b = _mm_max_ps(b, zero);

		// make sure no value is bigger than 255
		r = _mm_min_ps(r, plus255);
		g = _mm_min_ps(g, plus255);
		b = _mm_min_ps(b, plus255);

		// convert floats to int16 (there is no conversion to uint8, just to int8).
		iR = _mm_cvtps_epi32(r);
		iG = _mm_cvtps_epi32(g);
		iB = _mm_cvtps_epi32(b);

		// extract the 4 pixels RGB values.
		// because we made sure values are between 0 and 255, we can just take the lower byte
		// of each INT16
		pRGBAImage[0] = piR[0];
		pRGBAImage[1] = piG[0];
		pRGBAImage[2] = piB[0];
		pRGBAImage[3] = 255;

		pRGBAImage[4] = piR[1];
		pRGBAImage[5] = piG[1];
		pRGBAImage[6] = piB[1];
		pRGBAImage[7] = 255;

		pRGBAImage[8] = piR[2];
		pRGBAImage[9] = piG[2];
		pRGBAImage[10] = piB[2];
		pRGBAImage[11] = 255;

		pRGBAImage[12] = piR[3];
		pRGBAImage[13] = piG[3];
		pRGBAImage[14] = piB[3];
		pRGBAImage[15] = 255;

		// advance the streams
		pYUVImage += 8;
		pRGBAImage += 16;
	}
}

#else // not Win32

void YUV444ToRGBA(XnUInt8 cY, XnUInt8 cU, XnUInt8 cV,
					XnUInt8& cR, XnUInt8& cG, XnUInt8& cB, XnUInt8& cA)
{
	XnInt32 nC = cY - 16;
	XnInt16 nD = cU - 128;
	XnInt16 nE = cV - 128;

	nC = nC * 298 + 128;

	cR = XN_MIN(XN_MAX((nC            + 409 * nE) >> 8, 0), 255);
	cG = XN_MIN(XN_MAX((nC - 100 * nD - 208 * nE) >> 8, 0), 255);
	cB = XN_MIN(XN_MAX((nC + 516 * nD           ) >> 8, 0), 255);
	cA = 255;
}

void YUV422ToRGB888(const XnUInt8* pYUVImage, XnUInt8* pRGBImage, XnUInt32 nYUVSize, XnUInt32 nRGBSize)
{
	const XnUInt8* pCurrYUV = pYUVImage;
	XnUInt8* pCurrRGB = pRGBImage;
	const XnUInt8* pLastYUV = pYUVImage + nYUVSize - YUV422_BPP;
	XnUInt8* pLastRGB = pRGBImage + nRGBSize - YUV_RGBA_BPP;

	while (pCurrYUV <= pLastYUV && pCurrRGB <= pLastRGB)
	{
		YUV444ToRGBA(pCurrYUV[YUV422_Y1], pCurrYUV[YUV422_U], pCurrYUV[YUV422_V],
						pCurrRGB[YUV_RED], pCurrRGB[YUV_GREEN], pCurrRGB[YUV_BLUE], pCurrRGB[YUV_ALPHA]);
		pCurrRGB += YUV_RGBA_BPP;
		YUV444ToRGBA(pCurrYUV[YUV422_Y2], pCurrYUV[YUV422_U], pCurrYUV[YUV422_V],
						pCurrRGB[YUV_RED], pCurrRGB[YUV_GREEN], pCurrRGB[YUV_BLUE], pCurrRGB[YUV_ALPHA]);
		pCurrRGB += YUV_RGBA_BPP;
		pCurrYUV += YUV422_BPP;
	}
}

#endif

#if (XN_PLATFORM == XN_PLATFORM_WIN32)

void YUYVToRGB888(const XnUInt8* pYUVImage, XnUInt8* pRGBAImage, XnUInt32 nYUVSize, XnUInt32 nRGBSize)
{
	const XnUInt8* pYUVLast = pYUVImage + nYUVSize - 8;
	XnUInt8* pRGBLast = pRGBAImage + nRGBSize - 16;

	const __m128 minus128 = _mm_set_ps1(-128);
	const __m128 plus113983 = _mm_set_ps1(1.13983F);
	const __m128 minus039466 = _mm_set_ps1(-0.39466F);
	const __m128 minus058060 = _mm_set_ps1(-0.58060F);
	const __m128 plus203211 = _mm_set_ps1(2.03211F);
	const __m128 zero = _mm_set_ps1(0);
	const __m128 plus255 = _mm_set_ps1(255);

	// define YUV floats
	__m128 y;
	__m128 u;
	__m128 v;

	__m128 temp;

	// define RGB floats
	__m128 r;
	__m128 g;
	__m128 b;

	// define RGB integers
	__m128i iR;
	__m128i iG;
	__m128i iB;

	XnUInt32* piR = (XnUInt32*)&iR;
	XnUInt32* piG = (XnUInt32*)&iG;
	XnUInt32* piB = (XnUInt32*)&iB;

	while (pYUVImage <= pYUVLast && pRGBAImage <= pRGBLast)
	{
		// process 4 pixels at once (values should be ordered backwards)
		y = _mm_set_ps(pYUVImage[YUYV_Y2 + YUYV_BPP], pYUVImage[YUYV_Y1 + YUYV_BPP], pYUVImage[YUYV_Y2], pYUVImage[YUYV_Y1]);
		u = _mm_set_ps(pYUVImage[YUYV_U + YUYV_BPP],  pYUVImage[YUYV_U + YUYV_BPP],  pYUVImage[YUYV_U],  pYUVImage[YUYV_U]);
		v = _mm_set_ps(pYUVImage[YUYV_V + YUYV_BPP],  pYUVImage[YUYV_V + YUYV_BPP],  pYUVImage[YUYV_V],  pYUVImage[YUYV_V]);

		u = _mm_add_ps(u, minus128); // u -= 128
		v = _mm_add_ps(v, minus128); // v -= 128

		/*

		http://en.wikipedia.org/wiki/YUV

		From YUV to RGB:
		R =     Y + 1.13983 V
		G =     Y - 0.39466 U - 0.58060 V
		B =     Y + 2.03211 U

		*/ 

		temp = _mm_mul_ps(plus113983, v);
		r = _mm_add_ps(y, temp);

		temp = _mm_mul_ps(minus039466, u);
		g = _mm_add_ps(y, temp);
		temp = _mm_mul_ps(minus058060, v);
		g = _mm_add_ps(g, temp);

		temp = _mm_mul_ps(plus203211, u);
		b = _mm_add_ps(y, temp);

		// make sure no value is smaller than 0
		r = _mm_max_ps(r, zero);
		g = _mm_max_ps(g, zero);
		b = _mm_max_ps(b, zero);

		// make sure no value is bigger than 255
		r = _mm_min_ps(r, plus255);
		g = _mm_min_ps(g, plus255);
		b = _mm_min_ps(b, plus255);

		// convert floats to int16 (there is no conversion to uint8, just to int8).
		iR = _mm_cvtps_epi32(r);
		iG = _mm_cvtps_epi32(g);
		iB = _mm_cvtps_epi32(b);

		// extract the 4 pixels RGB values.
		// because we made sure values are between 0 and 255, we can just take the lower byte
		// of each INT16
		pRGBAImage[0] = piR[0];
		pRGBAImage[1] = piG[0];
		pRGBAImage[2] = piB[0];
		pRGBAImage[3] = 255;

		pRGBAImage[4] = piR[1];
		pRGBAImage[5] = piG[1];
		pRGBAImage[6] = piB[1];
		pRGBAImage[7] = 255;

		pRGBAImage[8] = piR[2];
		pRGBAImage[9] = piG[2];
		pRGBAImage[10] = piB[2];
		pRGBAImage[11] = 255;

		pRGBAImage[12] = piR[3];
		pRGBAImage[13] = piG[3];
		pRGBAImage[14] = piB[3];
		pRGBAImage[15] = 255;

		// advance the streams
		pYUVImage += 8;
		pRGBAImage += 16;
	}
}

#else // not Win32

void YUYVToRGB888(const XnUInt8* pYUVImage, XnUInt8* pRGBImage, XnUInt32 nYUVSize, XnUInt32 nRGBSize)
{
	const XnUInt8* pCurrYUV = pYUVImage;
	XnUInt8* pCurrRGB = pRGBImage;
	const XnUInt8* pLastYUV = pYUVImage + nYUVSize - YUYV_BPP;
	XnUInt8* pLastRGB = pRGBImage + nRGBSize - YUV_RGBA_BPP;

	while (pCurrYUV <= pLastYUV && pCurrRGB <= pLastRGB)
	{
		YUV444ToRGBA(pCurrYUV[YUYV_Y1], pCurrYUV[YUYV_U], pCurrYUV[YUYV_V],
			pCurrRGB[YUV_RED], pCurrRGB[YUV_GREEN], pCurrRGB[YUV_BLUE], pCurrRGB[YUV_ALPHA]);
		pCurrRGB += YUV_RGBA_BPP;
		YUV444ToRGBA(pCurrYUV[YUYV_Y2], pCurrYUV[YUYV_U], pCurrYUV[YUYV_V],
			pCurrRGB[YUV_RED], pCurrRGB[YUV_GREEN], pCurrRGB[YUV_BLUE], pCurrRGB[YUV_ALPHA]);
		pCurrRGB += YUV_RGBA_BPP;
		pCurrYUV += YUYV_BPP;
	}
}

#endif

void drawClosedStream(IntRect* pLocation, const char* csStreamName)
{
	char csMessage[512];
	XnUInt32 nWritten;
	xnOSStrFormat(csMessage, sizeof(csMessage), &nWritten, "%s stream is OFF", csStreamName);
	void* pFont = GLUT_BITMAP_TIMES_ROMAN_24;

	int nWidth = glutBitmapLength(pFont, (const unsigned char*)csMessage);
	int nXLocation = (pLocation->uRight + pLocation->uLeft - nWidth) / 2;
	int nYLocation = (pLocation->uTop + pLocation->uBottom) / 2;

	glColor3f(1.0, 0, 0);
	glRasterPos2i(nXLocation, nYLocation);
	glPrintString(pFont, csMessage);
}

void drawColor(IntRect* pLocation, IntPair* pPointer, int pointerRed, int pointerGreen, int pointerBlue)
{
	if (g_DrawConfig.Streams.Color.Coloring == COLOR_OFF)
		return;

	if (!isColorOn() && !isIROn())
	{
		drawClosedStream(pLocation, "Color");
		return;
	}

	openni::VideoFrameRef colorMD;
	int depthWidth = 0, depthHeight = 0, depthFullWidth = 0, depthFullHeight = 0;
	int depthOriginX = 0, depthOriginY = 0;

	if (isColorOn())
	{
		colorMD = getColorFrame();
		if (!colorMD.isValid())
			return;
	}
 	else if (isIROn())
 	{
 		colorMD = getIRFrame();
 	}
	else
		return;

	if (!colorMD.isValid())
		return;

	if (colorMD.getFrameIndex() == 0)
	{
		return;
	}

	openni::VideoFrameRef depthMetaData = getDepthFrame();

	int width = colorMD.getWidth();
	int height = colorMD.getHeight();
	int fullWidth = colorMD.getVideoMode().getResolutionX();
	int fullHeight = colorMD.getVideoMode().getResolutionY();
	int originX = colorMD.getCropOriginX();
	int originY = colorMD.getCropOriginY();

	XnUInt8* pColor = (XnUInt8*)colorMD.getData();
	bool useDepth = false;
	openni::PixelFormat format = colorMD.getVideoMode().getPixelFormat();

	openni::DepthPixel* pDepth = NULL;

	if (depthMetaData.isValid())
	{
		useDepth = true;
		depthWidth = depthMetaData.getWidth();
		depthHeight = depthMetaData.getHeight();
		depthFullWidth = depthMetaData.getVideoMode().getResolutionX();
		depthFullHeight = depthMetaData.getVideoMode().getResolutionY();
		depthOriginX = depthMetaData.getCropOriginX();
		depthOriginY = depthMetaData.getCropOriginY();

		pDepth = (openni::DepthPixel*)depthMetaData.getData();
	}

	for (XnUInt16 nY = 0; nY < height; nY++)
	{
		XnUInt8* pTexture = TextureMapGetLine(&g_texColor, nY + originY) + originX*4;

		if (format == openni::PIXEL_FORMAT_YUV422)
 		{
			YUV422ToRGB888(pColor, pTexture, width*2, g_texColor.Size.X*g_texColor.nBytesPerPixel);
 			pColor += width*2;
 		}
		else if (format == openni::PIXEL_FORMAT_YUYV)
		{
			YUYVToRGB888(pColor, pTexture, width*2, g_texColor.Size.X*g_texColor.nBytesPerPixel);
			pColor += width*2;
		}
 		else
		{
			XnDouble dRealY = (nY + originY) / (XnDouble)fullHeight;
			XnInt32 nDepthY = dRealY * depthFullHeight - depthOriginY;

			for (XnUInt16 nX = 0; nX < width; nX++, pTexture+=4)
			{
				XnInt32 nDepthIndex = 0;

				if (useDepth)
				{
					XnDouble dRealX = (nX + originX) / (XnDouble)fullWidth;

					XnInt32 nDepthX = dRealX * depthFullWidth - depthOriginX;

					if (nDepthX >= depthWidth || nDepthY >= depthHeight || nDepthX < 0 || nDepthY < 0)
					{
						nDepthIndex = -1;
					}
					else
					{
						nDepthIndex = nDepthY*depthWidth + nDepthX;
					}
				}

				switch (format)
 				{
				case openni::PIXEL_FORMAT_RGB888:
					pTexture[0] = pColor[0];
					pTexture[1] = pColor[1];
					pTexture[2] = pColor[2];
					pColor+=3; 
 					break;
				case openni::PIXEL_FORMAT_GRAY8:
 					pTexture[0] = pTexture[1] = pTexture[2] = *pColor;
 					pColor+=1; 
 					break;
				case openni::PIXEL_FORMAT_GRAY16:
 					pTexture[0] = pTexture[1] = pTexture[2] = *((XnUInt16*)pColor) >> 2;
 					pColor+=2; 
 					break;
				default:
					assert(0);
					return;
 				}

				// decide if pixel should be lit or not
				if (g_DrawConfig.Streams.Color.Coloring == DEPTH_MASKED_COLOR &&
					(!depthMetaData.isValid() || nDepthIndex == -1 || pDepth[nDepthIndex] == 0))
				{
					pTexture[3] = 0;
				}
				else
				{
					pTexture[3] = 255;
				}
			}
		}
	}

	if (pPointer != NULL)
	{
		TextureMapDrawCursor(&g_texColor, *pPointer, pointerRed, pointerGreen, pointerBlue);
	}

	TextureMapUpdate(&g_texColor);
	TextureMapDraw(&g_texColor, pLocation);
}

void drawDepth(IntRect* pLocation, IntPair* pPointer)
{
	if (g_DrawConfig.Streams.Depth.Coloring != DEPTH_OFF)
	{
		if (!isDepthOn())
		{
			drawClosedStream(pLocation, "Depth");
			return;
		}

		openni::VideoFrameRef* pDepthMD = &getDepthFrame();

		if (!pDepthMD->isValid())
			return;

		const openni::DepthPixel* pDepth = (openni::DepthPixel*)pDepthMD->getData();
		XN_ASSERT(pDepth);
		
		int width = pDepthMD->getWidth();
		int height = pDepthMD->getHeight();
		int originX = pDepthMD->getCropOriginX();
		int originY = pDepthMD->getCropOriginY();

		if (pDepthMD->getFrameIndex() == 0)
		{
			return;
		}

		// copy depth into texture-map
		for (XnUInt16 nY = originY; nY < height + originY; nY++)
		{
			XnUInt8* pTexture = TextureMapGetLine(&g_texDepth, nY) + originX*4;
			for (XnUInt16 nX = 0; nX < width; nX++, pDepth++, pTexture+=4)
			{
				XnUInt8 nRed = 0;
				XnUInt8 nGreen = 0;
				XnUInt8 nBlue = 0;
				XnUInt8 nAlpha = g_DrawConfig.Streams.Depth.fTransparency*255;

				XnUInt16 nColIndex;

				switch (g_DrawConfig.Streams.Depth.Coloring)
				{
				case LINEAR_HISTOGRAM:
					nRed = nGreen = g_pDepthHist[*pDepth]*255;
					break;
				case PSYCHEDELIC_SHADES:
					nAlpha *= (((XnFloat)(*pDepth % 10) / 20) + 0.5);
				case PSYCHEDELIC:

					switch ((*pDepth/10) % 10)
					{
					case 0:
						nRed = 255;
						break;
					case 1:
						nGreen = 255;
						break;
					case 2:
						nBlue = 255;
						break;
					case 3:
						nRed = 255;
						nGreen = 255;
						break;
					case 4:
						nGreen = 255;
						nBlue = 255;
						break;
					case 5:
						nRed = 255;
						nBlue = 255;
						break;
					case 6:
						nRed = 255;
						nGreen = 255;
						nBlue = 255;
						break;
					case 7:
						nRed = 127;
						nBlue = 255;
						break;
					case 8:
						nRed = 255;
						nBlue = 127;
						break;
					case 9:
						nRed = 127;
						nGreen = 255;
						break;
					}
					break;
				case RAINBOW:
					nColIndex = (XnUInt16)((*pDepth / (g_fMaxDepth / 256)));
					nRed   = PalletIntsR[nColIndex];
					nGreen = PalletIntsG[nColIndex];
					nBlue  = PalletIntsB[nColIndex];
					break;
				case CYCLIC_RAINBOW:
					nColIndex = (*pDepth % 256);
					nRed   = PalletIntsR[nColIndex];
					nGreen = PalletIntsG[nColIndex];
					nBlue  = PalletIntsB[nColIndex];
					break;
				case CYCLIC_RAINBOW_HISTOGRAM:
				{
					float fHist = g_pDepthHist[*pDepth];
					nColIndex = (*pDepth % 256);
					nRed   = PalletIntsR[nColIndex] * fHist;
					nGreen = PalletIntsG[nColIndex] * fHist;
					nBlue  = PalletIntsB[nColIndex] * fHist;
					break;
				}
				default:
					assert(0);
					return;
				}

				pTexture[0] = nRed;
				pTexture[1] = nGreen;
				pTexture[2] = nBlue;

				if (*pDepth == 0)
					pTexture[3] = 0;
				else
					pTexture[3] = nAlpha;
			}
		}

		if (pPointer != NULL)
		{
			TextureMapDrawCursor(&g_texDepth, *pPointer);
		}

		TextureMapUpdate(&g_texDepth);
		TextureMapDraw(&g_texDepth, pLocation);
	}
}

void drawPointerMode(IntPair* pPointer)
{
	char buf[512] = "";
	XnUInt32 chars;
	int nCharWidth = glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, '0');
	int nPointerValue = 0;

	XnDouble dTimestampDivider = 1E6;

	openni::VideoFrameRef* pDepthMD = &getDepthFrame();

	if (pDepthMD->isValid())
	{
		// Print the scale black background
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBegin(GL_QUADS);
		glColor4f(0, 0, 0, 0.7);
		glVertex2i(0, WIN_SIZE_Y); // lower left
		glVertex2i(WIN_SIZE_X, WIN_SIZE_Y);
		glVertex2i(WIN_SIZE_X, WIN_SIZE_Y - 135);
		glVertex2i(0, WIN_SIZE_Y - 135);
		glEnd();

		glDisable(GL_BLEND);

		// set a large point size (for the scale)
		glPointSize(15);

		// Print the scale data
		glBegin(GL_POINTS);
		for (int i=0; i<g_fMaxDepth; i+=1)
		{
			float fNewColor = g_pDepthHist[i];
			if ((fNewColor > 0.004) && (fNewColor < 0.996))
			{
				glColor3f(fNewColor, fNewColor, 0);
				glVertex3f(((i/10)*2), WIN_SIZE_Y - 23, 1);
			}
		}
		glEnd();

		// Print the pointer scale data
		if (pPointer != NULL)
		{
			// make sure pointer in on a depth pixel (take in mind cropping might be in place)
			IntPair pointerInDepth = *pPointer;
			pointerInDepth.X -= pDepthMD->getCropOriginX();
			pointerInDepth.Y -= pDepthMD->getCropOriginY();

			if (pointerInDepth.X < (int)pDepthMD->getWidth() &&
				pointerInDepth.X >= 0 &&
				pointerInDepth.Y < (int)pDepthMD->getHeight() &&
				pointerInDepth.Y >= 0)
			{
				nPointerValue = ((openni::DepthPixel*)(pDepthMD->getData()))[pointerInDepth.Y*pDepthMD->getWidth()+pointerInDepth.X];

				glBegin(GL_POINTS);
				glColor3f(1,0,0);
				glVertex3f(10 + ((nPointerValue/10)*2), WIN_SIZE_Y - 70, 1);
				glEnd();
			}
		}

		// Print the scale texts
		for (int i=0; i<g_fMaxDepth/10; i+=25)
		{
			int xPos = i*2 + 10;

			// draw a small line in this position
			glBegin(GL_LINES);
			glColor3f(0, 1, 0);
			glVertex2i(xPos, WIN_SIZE_Y - 54);
			glVertex2i(xPos, WIN_SIZE_Y - 62);
			glEnd();

			// place a label under, and in the middle of, that line.
			xnOSStrFormat(buf, sizeof(buf), &chars, "%d", i);
			glColor3f(1,0,0);
			glRasterPos2i(xPos - chars*nCharWidth/2, WIN_SIZE_Y - 40);
			glPrintString(GLUT_BITMAP_HELVETICA_18,buf);
		}

		xnOSStrFormat(buf, sizeof(buf), &chars, "%s - Frame %4u, Timestamp %.3f", "Depth"/*getDepthGenerator()->GetInfo().GetInstanceName()*/, pDepthMD->getFrameIndex(), (double)pDepthMD->getTimestamp()/dTimestampDivider);
	}

	openni::VideoFrameRef* pImageMD = &getColorFrame();
	if (pImageMD->isValid())
	{
		if (buf[0] != '\0')
		{
			xnOSStrAppend(buf, " | ", sizeof(buf));
		}

		xnOSStrFormat(buf + strlen(buf), (XnUInt32)(sizeof(buf) - strlen(buf)), &chars, "%s - Frame %4u, Timestamp %.3f", "Color", pImageMD->getFrameIndex(), (double)pImageMD->getTimestamp()/dTimestampDivider);
	}

	openni::VideoFrameRef* pIRMD = &getIRFrame();
	if (pIRMD->isValid())
	{
		if (buf[0] != '\0')
		{
			xnOSStrAppend(buf, " | ", sizeof(buf));
		}

		xnOSStrFormat(buf + strlen(buf), (XnUInt32)(sizeof(buf) - strlen(buf)), &chars, "%s - Frame %4u, Timestamp %.3f", "IR", pIRMD->getFrameIndex(), (double)pIRMD->getTimestamp()/dTimestampDivider);
	}

	int nYLocation = WIN_SIZE_Y - 88;
	glColor3f(1,0,0);
	glRasterPos2i(10,nYLocation);
	glPrintString(GLUT_BITMAP_HELVETICA_18, buf);
	nYLocation -= 26;

	if (pPointer != NULL)
	{
		// Print the pointer text
		XnUInt64 nCutOffMin = 0;
		XnUInt64 nCutOffMax = (pDepthMD != NULL) ? g_fMaxDepth : 0;

		XnChar sPointerValue[100];
		if (nPointerValue != g_fMaxDepth)
		{
			xnOSStrFormat(sPointerValue, sizeof(sPointerValue), &chars, "%.1f", (float)nPointerValue/10);
		}
		else
		{
			xnOSStrFormat(sPointerValue, sizeof(sPointerValue), &chars, "-");
		}

		xnOSStrFormat(buf, sizeof(buf), &chars, "Pointer Value: %s (X:%d Y:%d) Cutoff: %llu-%llu.", 
			sPointerValue, pPointer->X, pPointer->Y, nCutOffMin, nCutOffMax);

		glRasterPos2i(10,nYLocation);
		glPrintString(GLUT_BITMAP_HELVETICA_18, buf);
		nYLocation -= 26;
	}
}

void drawCenteredMessage(void* font, int y, const char* message, float fRed, float fGreen, float fBlue)
{
	const XnUInt32 nMaxLines = 5;
	XnChar buf[512];
	XnChar* aLines[nMaxLines];
	XnUInt32 anLinesWidths[nMaxLines];
	XnUInt32 nLine = 0;
	XnUInt32 nLineLengthChars = 0;
	XnInt32 nLineLengthPixels = 0;
	XnInt32 nMaxLineLength = 0;
	
	aLines[0] = buf;
	
	// parse message to lines
	const char* pChar = message;
	for (;;)
	{
		if (*pChar == '\n' || *pChar == '\0')
		{
			if (nLineLengthChars > 0)
			{
				aLines[nLine][nLineLengthChars++] = '\0';
				aLines[nLine+1] = &aLines[nLine][nLineLengthChars];
				anLinesWidths[nLine] = nLineLengthPixels;
				nLine++;
				if (nLineLengthPixels > nMaxLineLength)
				{
					nMaxLineLength = nLineLengthPixels;
				}
				nLineLengthPixels = 0;
				nLineLengthChars = 0;
			}

			if (nLine >= nMaxLines || *pChar == '\0')
			{
				break;
			}
		}
		else
		{
			aLines[nLine][nLineLengthChars++] = *pChar;
			nLineLengthPixels += glutBitmapWidth(font, *pChar);
		}
		pChar++;
	}
	
	XnUInt32 nHeight = 26;
	int nXLocation = xnl::Math::Max(0, (WIN_SIZE_X - nMaxLineLength) / 2);
	int nYLocation = y;

	// Draw black background
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
	glColor4f(0, 0, 0, 0.6);
	glVertex2i(nXLocation - 5, nYLocation - nHeight - 5);
	glVertex2i(nXLocation + nMaxLineLength + 5, nYLocation - nHeight - 5);
	glVertex2i(nXLocation + nMaxLineLength + 5, nYLocation + nHeight * nLine + 5);
	glVertex2i(nXLocation - 5, nYLocation + nHeight * nLine + 5);
	glEnd();

	glDisable(GL_BLEND);

	// show message
	glColor3f(fRed, fGreen, fBlue);
	for (XnUInt32 i = 0; i < nLine; ++i)
	{
		glRasterPos2i(nXLocation + (nMaxLineLength - anLinesWidths[i])/2, nYLocation + i * nHeight);
		glPrintString(font, aLines[i]);
	}
}

void drawUserMessage()
{
	const float fMessageTypeColors[NUM_DRAW_MESSAGE_TYPES][3] = 
	{
		{ 0, 1, 0 }, /*NOTIFICATION_MESSAGE*/
		{ 1, 1, 0 }, /*WARNING_MESSAGE*/
		{ 1, 0, 0 }, /*ERROR_MESSAGE*/
		{ 1, 0, 0 }, /*FATAL_MESSAGE*/
	};

	if (isInKeyboardInputMode())
	{
		drawCenteredMessage(GLUT_BITMAP_TIMES_ROMAN_24, WIN_SIZE_Y * 4 / 5, getCurrentKeyboardInputMessage(), 0, 1, 0);
	}

	static XnUInt64 nStartShowMessage = 0;

	if (g_DrawConfig.bShowMessage)
	{
		g_DrawConfig.bShowMessage = false;
		xnOSGetTimeStamp(&nStartShowMessage);
	}
	
	XnUInt64 nNow;
	xnOSGetTimeStamp(&nNow);

	if (nNow - nStartShowMessage < 3000)
	{
		drawCenteredMessage(GLUT_BITMAP_TIMES_ROMAN_24, WIN_SIZE_Y * 4 / 5, g_csUserMessage, 
							fMessageTypeColors[g_DrawConfig.messageType][0],
							fMessageTypeColors[g_DrawConfig.messageType][1],
							fMessageTypeColors[g_DrawConfig.messageType][2]);
	}
}

void printRecordingInfo()
{
	char csMessage[256];
	getCaptureMessage(csMessage);

	if (csMessage[0] != 0)
		drawCenteredMessage(GLUT_BITMAP_TIMES_ROMAN_24, 30, csMessage, 1, 0, 0);

	XnUInt32 nWritten;
	xnOSStrFormat(csMessage, sizeof(csMessage), &nWritten, 
		"Image registration is %s. Capture Formats - Depth: %s | Image: %s | IR: %s",
		getDevice().getImageRegistrationMode() == openni::IMAGE_REGISTRATION_OFF ? "off " : "on",
		captureGetDepthFormatName(), 
		captureGetColorFormatName(), 
		captureGetIRFormatName());

	drawCenteredMessage(GLUT_BITMAP_HELVETICA_12, WIN_SIZE_Y - 3, csMessage, 0, 1, 0);
}

void printHelpGroup(int nXLocation, int* pnYLocation, const char* csGroup)
{
	int nYLocation = *pnYLocation;

	int           aSpecialKeys[20];
	unsigned char aKeys[20];
	const char*   aDescs[20];
	int nSpecialCount,nCount;

	getGroupItems(csGroup, aSpecialKeys, aKeys, aDescs, &nSpecialCount, &nCount);

	glColor3f(0, 1, 0);
	glRasterPos2i(nXLocation, nYLocation);
	glPrintString(GLUT_BITMAP_TIMES_ROMAN_24, csGroup);
	nYLocation += 30;

	for (int i = 0; i < (nSpecialCount + nCount); ++i, nYLocation += 22)
	{
		char buf[256];
		XnUInt32 nWritten;
		if(i < nSpecialCount)
		{
			switch (aSpecialKeys[i])
			{
			case GLUT_KEY_LEFT:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "Left"); break;
			case GLUT_KEY_RIGHT:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "Right"); break;
			case GLUT_KEY_UP:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "Up"); break;
			case GLUT_KEY_DOWN:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "Down"); break;
			default:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "[0x%2x]", aSpecialKeys[i]); break;
			}
		}
		else
		{
			int j = i - nSpecialCount;
			switch (aKeys[j])
			{
			case 27:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "Esc"); break;
			case ' ':
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "Space"); break;
			default:
				xnOSStrFormat(buf, sizeof(buf), &nWritten, "%c", aKeys[j]);
				break;
			}
		}

		glColor3f(1, 0, 0);
		glRasterPos2i(nXLocation, nYLocation);
		glPrintString(GLUT_BITMAP_HELVETICA_18, buf);

		glRasterPos2i(nXLocation + 50, nYLocation);
		glPrintString(GLUT_BITMAP_HELVETICA_18, aDescs[i]);
	}

	*pnYLocation = nYLocation + 20;
}

void drawErrorState()
{
	// place a black rect on entire screen
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		
	glBegin(GL_QUADS);
	glColor4f(0, 0, 0, 0.8);
	glVertex2i(0, 0);
	glVertex2i(WIN_SIZE_X, 0);
	glVertex2i(WIN_SIZE_X, WIN_SIZE_Y);
	glVertex2i(0, WIN_SIZE_Y);
	glEnd();
	glDisable(GL_BLEND);

	int nYLocation = WIN_SIZE_Y/2 - 30;

	drawCenteredMessage(GLUT_BITMAP_TIMES_ROMAN_24, nYLocation, "ERROR!", 1, 0, 0);
	nYLocation += 40;
	drawCenteredMessage(GLUT_BITMAP_TIMES_ROMAN_24, nYLocation, g_DrawConfig.strErrorState, 1, 0, 0);
}

void drawHelpScreen()
{
	int nXStartLocation = WIN_SIZE_X/8;
	int nYStartLocation = WIN_SIZE_Y/5;
	int nXEndLocation = WIN_SIZE_X*7/8;
	int nYEndLocation = WIN_SIZE_Y*4/5;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		

	glBegin(GL_QUADS);
	glColor4f(0, 0, 0, 0.8);
	glVertex2i(nXStartLocation, nYStartLocation);
	glVertex2i(nXStartLocation, nYEndLocation);
	glVertex2i(nXEndLocation, nYEndLocation);
	glVertex2i(nXEndLocation, nYStartLocation);
	glEnd();

	glDisable(GL_BLEND);

	// set color to red
	glColor3f(1, 0, 0);

	// leave some margins
	nYStartLocation += 30;
	nXStartLocation += 30;

	// print left pane
	int nXLocation = nXStartLocation;
	int nYLocation = nYStartLocation;
	printHelpGroup(nXLocation, &nYLocation, KEYBOARD_GROUP_PRESETS);
	printHelpGroup(nXLocation, &nYLocation, KEYBOARD_GROUP_DISPLAY);
	printHelpGroup(nXLocation, &nYLocation, KEYBOARD_GROUP_DEVICE);

	// print right pane
	nXLocation = WIN_SIZE_X/2;
	nYLocation = nYStartLocation;
	printHelpGroup(nXLocation, &nYLocation, KEYBOARD_GROUP_PLAYER);
	printHelpGroup(nXLocation, &nYLocation, KEYBOARD_GROUP_CAPTURE);
	printHelpGroup(nXLocation, &nYLocation, KEYBOARD_GROUP_GENERAL);
}

void drawUserInput(bool bCursor)
{
	if (bCursor)
	{
		// draw cursor
		IntPair cursor = g_DrawUserInput.Cursor;
		glPointSize(1);
		glBegin(GL_POINTS);
		glColor3f(1,0,0);
		glVertex2i(cursor.X, cursor.Y);

		// upper left marker
		glVertex2i(cursor.X - 2, cursor.Y - 2);
		glVertex2i(cursor.X - 2, cursor.Y - 1);
		glVertex2i(cursor.X - 1, cursor.Y - 2);

		// bottom left marker
		glVertex2i(cursor.X - 2, cursor.Y + 2);
		glVertex2i(cursor.X - 2, cursor.Y + 1);
		glVertex2i(cursor.X - 1, cursor.Y + 2);

		// upper right marker
		glVertex2i(cursor.X + 2, cursor.Y - 2);
		glVertex2i(cursor.X + 2, cursor.Y - 1);
		glVertex2i(cursor.X + 1, cursor.Y - 2);

		// lower right marker
		glVertex2i(cursor.X + 2, cursor.Y + 2);
		glVertex2i(cursor.X + 2, cursor.Y + 1);
		glVertex2i(cursor.X + 1, cursor.Y + 2);

		glEnd();
	}

	// draw selection frame
	if (g_DrawUserInput.State == SELECTION_ACTIVE)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		

		glBegin(GL_QUADS);
		glColor4f(1, 0, 0, 0.5);
		glVertex2i(g_DrawUserInput.Rect.uLeft, g_DrawUserInput.Rect.uTop); // Upper left
		glVertex2i(g_DrawUserInput.Rect.uRight, g_DrawUserInput.Rect.uTop); // upper right
		glVertex2i(g_DrawUserInput.Rect.uRight, g_DrawUserInput.Rect.uBottom); // lower right
		glVertex2i(g_DrawUserInput.Rect.uLeft, g_DrawUserInput.Rect.uBottom); // lower left
		glEnd();

		glDisable(GL_BLEND);
	}
}

void fixLocation(IntRect* pLocation, int xRes, int yRes)
{
	double resRatio = (double)xRes / yRes;

	double locationRatio = (pLocation->uRight - pLocation->uLeft) / (pLocation->uTop - pLocation->uBottom);

	if (locationRatio > resRatio) 
	{
		// location is wider. use height as reference.
		double width = (pLocation->uTop - pLocation->uBottom) * resRatio;
		pLocation->uRight = (pLocation->uLeft + width);
	}
	else if (locationRatio < resRatio)
	{
		// res is wider. use width as reference.
		double height = (pLocation->uRight - pLocation->uLeft) / resRatio;
		pLocation->uTop = (pLocation->uBottom + height);
	}
}

bool isPointInRect(IntPair point, IntRect* pRect)
{
	return (point.X >= pRect->uLeft && point.X <= pRect->uRight &&
		point.Y >= pRect->uBottom && point.Y <= pRect->uTop);
}

void drawPlaybackSpeed()
{
// 	XnDouble dSpeed = getPlaybackSpeed();
// 	if (dSpeed != 1.0)
// 	{
// 		XnChar strSpeed[30];
// 		int len = sprintf(strSpeed, "x%g", dSpeed);
// 		int width = 0;
// 		for (int i = 0; i < len; ++i)
// 			width += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, strSpeed[i]);
// 
// 		glColor3f(0, 1, 0);
// 		glRasterPos2i(WIN_SIZE_X - width - 3, 30);
// 		glPrintString(GLUT_BITMAP_TIMES_ROMAN_24, strSpeed);
// 	}
}

void drawFrame()
{
	// calculate locations
	g_DrawConfig.DepthLocation.uBottom = 0;
	g_DrawConfig.DepthLocation.uTop = WIN_SIZE_Y - 1;
	g_DrawConfig.DepthLocation.uLeft = 0;
	g_DrawConfig.DepthLocation.uRight = WIN_SIZE_X - 1;

	g_DrawConfig.ColorLocation.uBottom = 0;
	g_DrawConfig.ColorLocation.uTop = WIN_SIZE_Y - 1;
	g_DrawConfig.ColorLocation.uLeft = 0;
	g_DrawConfig.ColorLocation.uRight = WIN_SIZE_X - 1;

	if (g_DrawConfig.Streams.ScreenArrangement == SIDE_BY_SIDE)
	{
		g_DrawConfig.DepthLocation.uTop = WIN_SIZE_Y / 2 - 1;
		g_DrawConfig.DepthLocation.uRight = WIN_SIZE_X / 2 - 1;
		g_DrawConfig.ColorLocation.uTop = WIN_SIZE_Y / 2 - 1;
		g_DrawConfig.ColorLocation.uLeft = WIN_SIZE_X / 2;
	}

	// Texture map init
	openni::VideoFrameRef* pDepthMD = &getDepthFrame();
	if (isDepthOn() && pDepthMD->isValid())
	{
		int maxDepth = 0;
		maxDepth = getDepthStream().getMaxPixelValue();
		g_fMaxDepth = maxDepth;
		
		TextureMapInit(&g_texDepth, pDepthMD->getVideoMode().getResolutionX(), pDepthMD->getVideoMode().getResolutionY(), 4, pDepthMD->getWidth(), pDepthMD->getHeight());
		fixLocation(&g_DrawConfig.DepthLocation, pDepthMD->getVideoMode().getResolutionX(), pDepthMD->getVideoMode().getResolutionY());
	}

	openni::VideoFrameRef* pImageMD = NULL;

	if (isColorOn())
	{
		pImageMD = &getColorFrame();
	}
 	else if (isIROn())
 	{
 		pImageMD = &getIRFrame();
 	}

	if (pImageMD != NULL && pImageMD->isValid())
	{
		TextureMapInit(&g_texColor, pImageMD->getVideoMode().getResolutionX(), pImageMD->getVideoMode().getResolutionY(), 4, pImageMD->getWidth(), pImageMD->getHeight());
		fixLocation(&g_DrawConfig.ColorLocation, pImageMD->getVideoMode().getResolutionX(), pImageMD->getVideoMode().getResolutionY());
	}

	// check if pointer is over a map
	bool bOverDepth = (pDepthMD != NULL && pDepthMD->isValid()) && isPointInRect(g_DrawUserInput.Cursor, &g_DrawConfig.DepthLocation);
	bool bOverImage = (pImageMD != NULL && pImageMD->isValid()) && isPointInRect(g_DrawUserInput.Cursor, &g_DrawConfig.ColorLocation);
	bool bDrawDepthPointer = false;
	bool bDrawImagePointer = false;
	int imagePointerRed = 255;
	int imagePointerGreen = 0;
	int imagePointerBlue = 0;

	IntPair pointerInDepth = {0,0};
	IntPair pointerInColor = {0,0};

	if (bOverImage)
	{
		pointerInColor.X = (double)(g_DrawUserInput.Cursor.X - g_DrawConfig.ColorLocation.uLeft) / (g_DrawConfig.ColorLocation.uRight - g_DrawConfig.ColorLocation.uLeft + 1) * pImageMD->getVideoMode().getResolutionX();
		pointerInColor.Y = (double)(g_DrawUserInput.Cursor.Y - g_DrawConfig.ColorLocation.uBottom) / (g_DrawConfig.ColorLocation.uTop - g_DrawConfig.ColorLocation.uBottom + 1) * pImageMD->getVideoMode().getResolutionY();
		bDrawImagePointer = true;
	}

	if (bOverDepth)
	{
		pointerInDepth.X = (double)(g_DrawUserInput.Cursor.X - g_DrawConfig.DepthLocation.uLeft) / (g_DrawConfig.DepthLocation.uRight - g_DrawConfig.DepthLocation.uLeft + 1) * pDepthMD->getVideoMode().getResolutionX();
		pointerInDepth.Y = (double)(g_DrawUserInput.Cursor.Y - g_DrawConfig.DepthLocation.uBottom) / (g_DrawConfig.DepthLocation.uTop - g_DrawConfig.DepthLocation.uBottom + 1) * pDepthMD->getVideoMode().getResolutionY();
		bDrawDepthPointer = true;

		if (!bOverImage && g_DrawConfig.bShowPointer &&
			pointerInDepth.X >= pDepthMD->getCropOriginX() && pointerInDepth.X < (pDepthMD->getCropOriginX() + pDepthMD->getWidth()) &&
			pointerInDepth.Y >= pDepthMD->getCropOriginY() && pointerInDepth.Y < (pDepthMD->getCropOriginY() + pDepthMD->getHeight()))
		{

			// try to translate depth pixel to image
			openni::DepthPixel* pDepthPixels = (openni::DepthPixel*)pDepthMD->getData();
			openni::DepthPixel pointerDepth = pDepthPixels[(pointerInDepth.Y - pDepthMD->getCropOriginY()) * pDepthMD->getWidth() + (pointerInDepth.X - pDepthMD->getCropOriginX())];
			if (convertDepthPointToColor(pointerInDepth.X, pointerInDepth.Y, pointerDepth, &pointerInColor.X, &pointerInColor.Y))
			{
				bDrawImagePointer = true;
				imagePointerRed = 0;
				imagePointerGreen = 0;
				imagePointerBlue = 255;
			}
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Setup the opengl env for fixed location view
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,WIN_SIZE_X,WIN_SIZE_Y,0,-1.0,1.0);
	glDisable(GL_DEPTH_TEST); 

	if (g_DrawConfig.Streams.Depth.Coloring == CYCLIC_RAINBOW_HISTOGRAM || g_DrawConfig.Streams.Depth.Coloring == LINEAR_HISTOGRAM || g_DrawConfig.bShowPointer)
		calculateHistogram();

	drawColor(&g_DrawConfig.ColorLocation, bDrawImagePointer ? &pointerInColor : NULL, imagePointerRed, imagePointerGreen, imagePointerBlue);

	drawDepth(&g_DrawConfig.DepthLocation, bDrawDepthPointer ? &pointerInDepth : NULL);

	printRecordingInfo();

	if (g_DrawConfig.bShowPointer)
		drawPointerMode(bOverDepth ? &pointerInDepth : NULL);

	drawUserInput(!bOverDepth && !bOverImage);

	drawUserMessage();
	drawPlaybackSpeed();

	if (g_DrawConfig.strErrorState[0] != '\0')
		drawErrorState();

	if (g_DrawConfig.bHelp)
		drawHelpScreen();

	glutSwapBuffers();
}

void setDepthDrawing(int nColoring)
{
	g_DrawConfig.Streams.Depth.Coloring	= (DepthDrawColoringType)nColoring;
}

void setColorDrawing(int nColoring)
{
	g_DrawConfig.Streams.Color.Coloring	= (ColorDrawColoringType)nColoring;
}
