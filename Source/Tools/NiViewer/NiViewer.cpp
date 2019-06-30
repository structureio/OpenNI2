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
// Application Usage:
// 1 - Switch to the depth map view mode.
// 2 - Switch to the psychedelic depth map view mode. In this mode each centimeter will have a difference color.
// 3 - Switch to the psychedelic depth map view mode. In this mode each centimeter will have a difference color and millimeters will have different shades.
// 4 - Switch to the depth map with rainbow colors view mode.
// 5 - Switch to the depth map with RGB registration view mode.
// 6 - Switch to the depth map with RGB registration view mode and a background image.
// 7 - Switch to the side by side of depth and color view mode.
// 8 - Switch to the color map with RGB registration view mode.
// 9 - Switch to the color map with RGB registration view mode. In this mode the depth will be semi transparent.
// 0 - Switch to the color map with RGB registration view mode. In this mode the depth will be semi transparent and use rainbow colors.
// - - Switch to the color map with RGB registration view mode. In this mode the depth will be semi transparent and use depth values color codes.
// = - Switch to the color map only view mode.
// p - Show the laser pointer and the cutoff parameters.
// m - Enable/Disable the mirror mode.
// q - Decrease the minimum depth cutoff by 1.
// Q - Decrease the minimum depth cutoff by 50.
// w - Increase the minimum depth cutoff by 1.
// W - Increase the minimum depth cutoff by 50.
// e - Decrease the maximum depth cutoff by 1.
// E - Decrease the maximum depth cutoff by 50.
// r - Increase the maximum depth cutoff by 1.
// R - Increase the maximum depth cutoff by 50.
// ESC - exit.

// --------------------------------
// Includes
// --------------------------------
// #include <XnCppWrapper.h>
// 
// #if (XN_PLATFORM == XN_PLATFORM_LINUX_X86 || XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_LINUX_AARCH64 || XN_PLATFORM == XN_PLATFORM_LINUX_PPC)
// 	#define UNIX
// 	#define GLX_GLXEXT_LEGACY
// #endif
// 
// #if (XN_PLATFORM == XN_PLATFORM_MACOSX)
// 	#define MACOS
// #endif

// Undeprecate CRT functions
#ifndef _CRT_SECURE_NO_DEPRECATE 
	#define _CRT_SECURE_NO_DEPRECATE 1
#endif


#include "Device.h"
#include "OpenNI.h"
#include "XnLib.h"

#define GLH_EXT_SINGLE_FILE
#if (ONI_PLATFORM == ONI_PLATFORM_WIN32)
#pragma warning(push, 3)
#endif
#include <glh/glh_obs.h>
#include <glh/glh_glut2.h>
#if (ONI_PLATFORM == ONI_PLATFORM_WIN32)
#pragma warning(pop)
#endif

using namespace glh;

// #include <XnLog.h>
#include "Capture.h"
#include "Draw.h"
#include "Keyboard.h"
#include "Menu.h"
#include "MouseInput.h"

#if (ONI_PLATFORM == ONI_PLATFORM_WIN32)
	#include <conio.h>
	#include <direct.h>	
#elif (ONI_PLATFORM == ONI_PLATFORM_LINUX_X86 || ONI_PLATFORM == ONI_PLATFORM_LINUX_ARM || ONI_PLATFORM == ONI_PLATFORM_LINUX_AARCH64 || ONI_PLATFORM == ONI_PLATFORM_LINUX_PPC || ONI_PLATFORM == ONI_PLATFORM_MACOSX)
	#define _getch() getchar()
#endif

// --------------------------------
// Defines
// --------------------------------
#define SAMPLE_XML_PATH "../../../../Data/SamplesConfig.xml"

// --------------------------------
// Types
// --------------------------------
enum {
	ERR_OK,
	ERR_USAGE,
	ERR_DEVICE,
	ERR_UNKNOWN
};

// --------------------------------
// Global Variables
// --------------------------------
/* When true, frames will not be read from the device. */
bool g_bPause = false;
/* When true, only a single frame will be read, and then reading will be paused. */
bool g_bStep = false;
bool g_bShutdown = false;

glut_perspective_reshaper reshaper;
glut_callbacks cb;

IntPair mouseLocation;
IntPair windowSize;

// --------------------------------
// Utilities
// --------------------------------
void motionCallback(int x, int y)
{
	mouseInputMotion(int((double)x/windowSize.X*WIN_SIZE_X), int((double)y/windowSize.Y*WIN_SIZE_Y));
}

void mouseCallback(int button, int state, int x, int y)
{
	mouseInputButton(button, state, int((double)x/windowSize.X*WIN_SIZE_X), int((double)y/windowSize.Y*WIN_SIZE_Y));
}

void keyboardCallback(unsigned char key, int /*x*/, int /*y*/)
{
	if (isCapturing())
	{
		captureStop(0);
	}
	else
	{
		handleKey(key);
	}

	glutPostRedisplay();
}

void keyboardSpecialCallback(int key, int /*x*/, int /*y*/)
{
	if (isCapturing())
	{
		captureStop(0);
	}
	else
	{
		handleSpecialKey(key);
	}

	glutPostRedisplay();
}

void reshapeCallback(int width, int height)
{
	windowSize.X = width;
	windowSize.Y = height;
	windowReshaped(width, height);
}

void idleCallback()
{
	if (g_bShutdown)
	{
		return;
	}

	if (g_bPause != TRUE)
	{
		// read a frame
		readFrame();

		captureRun();
	}

	if (g_bStep == TRUE)
	{
		g_bStep = FALSE;
		g_bPause = TRUE;
	}

	glutPostRedisplay();
}

void step(int)
{
	g_bStep = true;
	g_bPause = false;
}

void seek(int nDiff)
{
	seekFrame(nDiff);

	// now step the last one (that way, if seek is not supported, as in sensor, at least one frame
	// will be read).
	if (g_bPause)
	{
		step(0);
	}
}

void seekToUserInputEnd(bool ok, const char* userInput)
{
	if (ok)
	{
		long seekFrame = atoi(userInput);
		seekFrameAbs(seekFrame);

		// now step the last one (that way, if seek is not supported, as in sensor, at least one frame
		// will be read).
		if (g_bPause)
		{
			step(0);
		}
	}
}

void seekToUserInputBegin(int)
{
	startKeyboardInputMode("Seek to frame: ", true, seekToUserInputEnd);
}

void init_opengl()
{
	glClearStencil(128);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	GLfloat ambient[] = {0.5, 0.5, 0.5, 1};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightf (GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.1f);
}

void closeSample(int errCode)
{
 	captureStop(0);
	g_bShutdown = TRUE;
	closeDevice();

	if (errCode != ERR_OK)
	{
		printf("Press any key to continue . . .\n");
		_getch();
	}

	exit(errCode);
}

void togglePause(int)
{
	g_bPause = !g_bPause;
	setPlaybackSpeed(g_bPause ? -1.0f : 1.0f);
}

void startCapture(int delay)
{
 	if (g_bPause)
 	{
 		displayMessage("Cannot record when paused!");
 	}
 	else
 	{
 		captureStart(delay);
 	}
}

void createKeyboardMap()
{
	startKeyboardMap();
	{
		startKeyboardGroup(KEYBOARD_GROUP_PRESETS);
		{
			registerKey('1', getPresetName(1), setPreset, 1);
			registerKey('2', getPresetName(2), setPreset, 2);
			registerKey('3', getPresetName(3), setPreset, 3);
			registerKey('4', getPresetName(4), setPreset, 4);
			registerKey('5', getPresetName(5), setPreset, 5);
			registerKey('7', getPresetName(7), setPreset, 7);
			registerKey('8', getPresetName(8), setPreset, 8);
			registerKey('9', getPresetName(9), setPreset, 9);
			registerKey('0', getPresetName(10), setPreset, 10);
			registerKey('-', getPresetName(11), setPreset, 11);
			registerKey('=', getPresetName(12), setPreset, 12);
		}
		endKeyboardGroup();
 		startKeyboardGroup(KEYBOARD_GROUP_DEVICE);
 		{
			registerKey('y', "Frame sync on/off", toggleFrameSync, 0);
 			registerKey('m', "Mirror on/off", toggleMirror, 0);
 			registerKey('/', "Reset all croppings", resetAllCropping, 0);

			registerKey('a', "toggle Auto Exposure", toggleImageAutoExposure, 0);
			registerKey('q', "toggle AWB", toggleImageAutoWhiteBalance, 0);
			registerKey('e', "Increase Exposure", changeImageExposure, 1);
			registerKey('E', "Decrease Exposure", changeImageExposure, -1);
			registerKey('g', "Increase Gain", changeImageGain, 10);
			registerKey('G', "Decrease Gain", changeImageGain, -10);

			registerKey('x', "Close Range", toggleCloseRange, 0);
			registerKey('i', "Toggle Image Registration", toggleImageRegistration, 0);
 		}
 		endKeyboardGroup();
 		startKeyboardGroup(KEYBOARD_GROUP_CAPTURE);
 		{
 			registerKey('s', "Start", startCapture, 0);
 			registerKey('d', "Start (5 sec delay)", startCapture, 5);
 			registerKey('x', "Stop", captureStop, 0);
 			registerKey('c', "Capture current frame only", captureSingleFrame, 0);
 		}
		endKeyboardGroup();
		startKeyboardGroup(KEYBOARD_GROUP_DISPLAY);
		{
			registerKey('p', "Pointer Mode On/Off", togglePointerMode, 0);
			registerKey('f', "Full Screen On/Off", toggleFullScreen, 0);
			registerKey('?', "Show/Hide Help screen", toggleHelpScreen, 0);
		}
		endKeyboardGroup();
		startKeyboardGroup(KEYBOARD_GROUP_GENERAL);
		{
			registerKey('?', "Show/Hide help screen", toggleHelpScreen, 0);
			registerKey(27, "Exit", closeSample, ERR_OK);
		}
		endKeyboardGroup();
		startKeyboardGroup(KEYBOARD_GROUP_PLAYER);
		{
			registerKey(' ', "Pause/Resume", togglePause, 0);
			if (getDevice().isFile())
			{
				registerSpecialKey(GLUT_KEY_RIGHT, "Jump 1 frame forward",     seek, 1);
				registerSpecialKey(GLUT_KEY_UP,    "Jump 10 frames forward",   seek, 10);
				registerSpecialKey(GLUT_KEY_LEFT,  "Jump 1 frame backwards",   seek, -1);
				registerSpecialKey(GLUT_KEY_DOWN,  "Jump 10 frames backwards", seek, -10);
				registerKey(':', "Jump to frame", seekToUserInputBegin, 0);

				registerKey('r', "Toggle playback repeat", togglePlaybackRepeat, 0);
				registerKey('[', "Decrease playback speed", changePlaybackSpeed, -1);
				registerKey(']', "Increase playback speed", changePlaybackSpeed, 1);
			}
			registerKey(';', "Read one frame", step, 0);
		}
		endKeyboardGroup();
	}
	endKeyboardMap();
}

void createMenu()
{
	const openni::SensorInfo* pSensorInfo;

	startMenu();
	{
		startSubMenu("View");
		{
			startSubMenu("Presets");
			{
				for (int i = 1; i < PRESET_COUNT; ++i)
				{
					if (i != 6)
						createMenuEntry(getPresetName(i), setPreset, i);
				}
			}
			endSubMenu();
			startSubMenu("Screen Layout");
			{
				createMenuEntry("Side by Side", setScreenLayout, (int)SIDE_BY_SIDE);
				createMenuEntry("Overlay", setScreenLayout, (int)OVERLAY);
			}
			endSubMenu();
			startSubMenu("Depth");
			{
				for (int i = 0; i < NUM_OF_DEPTH_DRAW_TYPES; ++i)
				{
					createMenuEntry(g_DepthDrawColoring[i], setDepthDrawing, i);
				}
			}
			endSubMenu();
			startSubMenu("Color");
			{
				for (int i = 0; i < NUM_OF_COLOR_DRAW_TYPES; ++i)
				{
					createMenuEntry(g_ColorDrawColoring[i], setColorDrawing, i);
				}
			}
			endSubMenu();
			createMenuEntry("Pointer Mode On/Off", togglePointerMode, 0);
			createMenuEntry("Show/Hide Help Screen", toggleHelpScreen, 0);
		}
		endSubMenu();
		startSubMenu("Device");
		{
			startSubMenu("Streams");
			{
				// Depth stream
				pSensorInfo = getDepthSensorInfo();
				if (pSensorInfo != NULL)
				{
					startSubMenu("Depth");
					{
						createMenuEntry("On/Off", toggleDepthState, 0);
						startSubMenu("Video Mode");
						{
							const openni::Array<openni::VideoMode>& supportedModes = pSensorInfo->getSupportedVideoModes();
							for (int i = 0; i < supportedModes.getSize(); ++i)
							{
								const openni::VideoMode* pSupportedMode = &supportedModes[i];
								char name[50];
								XnUInt32 dummy;
								xnOSStrFormat(name, 50, &dummy, "%d x %d @ %d (%s)", pSupportedMode->getResolutionX(), pSupportedMode->getResolutionY(), pSupportedMode->getFps(), getFormatName(pSupportedMode->getPixelFormat()));
								createMenuEntry(name, setDepthVideoMode, i);
							}
						}
						endSubMenu();
						createMenuEntry("Mirror", toggleDepthMirror, 0);
 						createMenuEntry("Reset Cropping", resetDepthCropping, 0);
					}
					endSubMenu();
				}

				// Color stream
				pSensorInfo = getColorSensorInfo();
				if (pSensorInfo != NULL)
				{
					startSubMenu("Color");
					{
						createMenuEntry("On/Off", toggleColorState, 0);
						startSubMenu("Video Mode");
						{
							const openni::Array<openni::VideoMode>& supportedModes = pSensorInfo->getSupportedVideoModes();
							for (int i = 0; i < supportedModes.getSize(); ++i)
							{
								const openni::VideoMode* pSupportedMode = &supportedModes[i];
								char name[50];
								XnUInt32 dummy;
								xnOSStrFormat(name, 50, &dummy, "%d x %d @ %d (%s)", pSupportedMode->getResolutionX(), pSupportedMode->getResolutionY(), pSupportedMode->getFps(), getFormatName(pSupportedMode->getPixelFormat()));
								createMenuEntry(name, setColorVideoMode, i);
							}
						}
						endSubMenu();
						createMenuEntry("Mirror", toggleColorMirror, 0);
 						createMenuEntry("Reset Cropping", resetColorCropping, 0);
					}
					endSubMenu();
				}

				// IR stream
				pSensorInfo = getIRSensorInfo();
				if (pSensorInfo != NULL)
				{
 					startSubMenu("IR");
 					{
						createMenuEntry("On/Off", toggleIRState, 0);
						startSubMenu("Video Mode");
						{
							const openni::Array<openni::VideoMode>& supportedModes = pSensorInfo->getSupportedVideoModes();
							for (int i = 0; i < supportedModes.getSize(); ++i)
							{
								const openni::VideoMode* pSupportedMode = &supportedModes[i];
								char name[50];
								XnUInt32 dummy;
								xnOSStrFormat(name, 50, &dummy, "%d x %d @ %d (%s)", pSupportedMode->getResolutionX(), pSupportedMode->getResolutionY(), pSupportedMode->getFps(), getFormatName(pSupportedMode->getPixelFormat()));
								createMenuEntry(name, setIRVideoMode, i);
							}
						}
						endSubMenu();
						createMenuEntry("Mirror", toggleIRMirror, 0);
 						createMenuEntry("Reset Cropping", resetIRCropping, 0);
 					}
 					endSubMenu();
				}
// 				startSubMenu("Audio");
// 				{
// 					createMenuEntry("On/Off", toggleAudioState, 0);
// 				}
// 				endSubMenu();
// 				startSubMenu("Primary Stream");
// 				{
// 					for (int i = 0; i < g_PrimaryStream.nValuesCount; ++i)
// 					{
// 						createMenuEntry(g_PrimaryStream.pValues[i], changePrimaryStream, i);
// 					}
// 				}
// 				endSubMenu();
			}
			endSubMenu();
			startSubMenu("Registration");
			{
				for (int i = 0; i < g_Registration.nValuesCount; ++i)
				{
					unsigned int nValue = g_Registration.pValues[i];
					createMenuEntry(g_Registration.pValueToName[nValue], changeRegistration, nValue);
				}
			}
			endSubMenu();
			createMenuEntry("Frame Sync", toggleFrameSync, 0);
			createMenuEntry("Mirror All", toggleMirror, 0);
		}
		endSubMenu();
		startSubMenu("Capture");
		{
			startSubMenu("Depth Capturing");
			{
				for (int i = 0; i < g_DepthCapturing.nValuesCount; ++i)
				{
					unsigned int nValue = g_DepthCapturing.pValues[i];
					createMenuEntry(g_DepthCapturing.pValueToName[i], captureSetDepthFormat, nValue);
				}
			}
			endSubMenu();
			startSubMenu("Image Capturing");
			{
				for (int i = 0; i < g_ColorCapturing.nValuesCount; ++i)
				{
					unsigned int nValue = g_ColorCapturing.pValues[i];
					createMenuEntry(g_ColorCapturing.pValueToName[i], captureSetColorFormat, nValue);
				}
			}
			endSubMenu();
			startSubMenu("IR Capturing");
			{
				for (int i = 0; i < g_IRCapturing.nValuesCount; ++i)
				{
					unsigned int nValue = g_IRCapturing.pValues[i];
					createMenuEntry(g_IRCapturing.pValueToName[i], captureSetIRFormat, nValue);
				}
			}
			endSubMenu();
			createMenuEntry("Browse", captureBrowse, 0);
			createMenuEntry("Start", startCapture, 0);
			createMenuEntry("Start (5 sec delay)", startCapture, 5);
			createMenuEntry("Restart", captureRestart, 0);
			createMenuEntry("Stop", captureStop, 0);
		}
		endSubMenu();
		startSubMenu("Player");
		{
			createMenuEntry("Pause/Resume", togglePause, 0);
			if (getDevice().isFile())
			{
				createMenuEntry("Skip 1 frame forward", seek, 1);
				createMenuEntry("Skip 10 frame forward", seek, 10);
				createMenuEntry("Skip 1 frame backwards", seek, -1);
				createMenuEntry("Skip 10 frame backwards", seek, -10);

				createMenuEntry("Toggle playback repeat", togglePlaybackRepeat, 0);

				createMenuEntry("Decrease playback speed", changePlaybackSpeed, -1);
				createMenuEntry("Increase playback speed", changePlaybackSpeed, 1);
			}
			createMenuEntry("Read one frame", step, 0);
		}
		endSubMenu();
		createMenuEntry("Quit", closeSample, ERR_OK);
	}
	endMenu();
}

void onExit()
{
	closeDevice();
	captureStop(0);
}

int changeDirectory(char* arg0)
{
	// get dir name
	XnChar strDirName[XN_FILE_MAX_PATH];
	XnStatus nRetVal = xnOSGetDirName(arg0, strDirName, XN_FILE_MAX_PATH);
	XN_IS_STATUS_OK(nRetVal);

	// now set current directory to it
	nRetVal = xnOSSetCurrentDir(strDirName);
	XN_IS_STATUS_OK(nRetVal);

	return 0;
}

int main(int argc, char **argv)
{
	XnBool bChooseDevice = FALSE;
	const char* uri = NULL;

	DeviceConfig config;
	config.openDepth = SENSOR_TRY;
	config.openColor = SENSOR_TRY;
	config.openIR = SENSOR_TRY;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			printf("Usage: %s [OPTIONS] [URI]\n\n", argv[0]);
			printf("When URI is provided, OpenNI will attempt to open the device with said URI. When not provided, \n");
			printf("any device might be opened.\n\n");
			printf("Options:\n");
			printf("-h, --help\n");
			printf("	Shows this help screen and exits\n");
			printf("-devices\n");
			printf("	Allows to choose the device to open from the list of connected devices\n");
			printf("-depth=<on|off|try>\n");
			printf("-color=<on|off|try>\n");
			printf("-ir=<on|off|try>\n");
			printf("	Toggles each stream state. <off> means the stream will not be opened. <on> means it will be opened, and NiViewer will\n");
			printf("	exit if it fails. <try> means that NiViewer will try to open that stream, but continue if it fails.\n");
			printf("	The default value is <try> for all 3 sensors.\n");
			return 0;
		}
		else if (strcmp(argv[i], "-devices") == 0)
		{
			bChooseDevice = TRUE;
		}
		else if (strcmp(argv[i], "-depth=on") == 0)
		{
			config.openDepth = SENSOR_ON;
		}
		else if (strcmp(argv[i], "-depth=off") == 0)
		{
			config.openDepth = SENSOR_OFF;
		}
		else if (strcmp(argv[i], "-depth=try") == 0)
		{
			config.openDepth = SENSOR_TRY;
		}
		else if (strcmp(argv[i], "-color=on") == 0)
		{
			config.openColor = SENSOR_ON;
		}
		else if (strcmp(argv[i], "-color=off") == 0)
		{
			config.openColor = SENSOR_OFF;
		}
		else if (strcmp(argv[i], "-color=try") == 0)
		{
			config.openColor = SENSOR_TRY;
		}
		else if (strcmp(argv[i], "-ir=on") == 0)
		{
			config.openIR = SENSOR_ON;
		}
		else if (strcmp(argv[i], "-ir=off") == 0)
		{
			config.openIR = SENSOR_OFF;
		}
		else if (strcmp(argv[i], "-ir=try") == 0)
		{
			config.openIR = SENSOR_TRY;
		}
		else if (uri == NULL)
		{
			uri = argv[i];
		}
		else
		{
			printf("unknown argument: %s\n", argv[i]);
			return -1;
		}
	}

	// Xiron Init
	XnStatus rc = XN_STATUS_OK;

	if (bChooseDevice)
	{
		rc = openDeviceFromList(config);
	}
	else
	{
		rc = openDevice(uri, config);
	}

	if (rc != openni::STATUS_OK)
	{
		printf("openDevice failed:\n%s\n", openni::OpenNI::getExtendedError());
		closeSample(ERR_DEVICE);
	}

// 	audioInit();
 	captureInit();

	reshaper.zNear = 1;
	reshaper.zFar = 100;
	glut_add_interactor(&reshaper);

	cb.mouse_function = mouseCallback;
	cb.motion_function = motionCallback;
	cb.passive_motion_function = motionCallback;
	cb.keyboard_function = keyboardCallback;
	cb.special_function  = keyboardSpecialCallback;
	cb.reshape_function = reshapeCallback;
	glut_add_interactor(&cb);

	glutInit(&argc, argv);
	glutInitDisplayString("stencil double rgb");
	glutInitWindowSize(WIN_SIZE_X, WIN_SIZE_Y);
	glutCreateWindow("OpenNI Viewer");
	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	init_opengl();

	glut_helpers_initialize();

	glutIdleFunc(idleCallback);
	glutDisplayFunc(drawFrame);

	drawInit();
	createKeyboardMap();
	createMenu();

	atexit(onExit);
	
	// Per frame code is in drawFrame()
	glutMainLoop();

//	audioShutdown();

	closeSample(ERR_OK);

//	return (ERR_OK);
}
