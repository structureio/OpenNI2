# OpenNI 2 Kinect v2 Driver #

## Contributors ##

Miguel Angel Vico

---------------------------------------------------------------------------------------------------

## Contact ##

Miguel Angel Vico (mvm9289@gmail.com)

---------------------------------------------------------------------------------------------------

## Summary ##

This OpenNI 2 Driver adds support for the Microsoft Kinect v2 sensor. It allows to fetch data from
either color, depth and infrared streams. It also gives support for DEPTH_TO_COLOR_REGISTRATION.

The videomodes available through this driver are:
* Color: RGB888          1920x1080 30fps
* Color: RGB888          960x540   30fps
* Depth: DepthPixel      512x424   30fps
* IR:    Gray16 (ushort) 512x424   30fps

This driver only works on Windows 8 platforms as it is based on the Microsoft official SDK.

---------------------------------------------------------------------------------------------------

## How to build ##

First of all, since this driver is based on the official Microsoft SDK, it will only compile on
Windows 8. Additionally, even though this driver is shipped with a Visual Studio 2010 vcxproj file,
it is required to use Visual Studio 2012 or higher in order to build it.

In order to build this driver:

1. Place 'Kinect2' folder under 'Source/Drivers' of OpenNI 2 source directory
2. Add 'Kinect2.vcxproj' to the OpenNI 2 Visual Studio solution
3. Build it as any other OpenNI 2 driver

---------------------------------------------------------------------------------------------------

