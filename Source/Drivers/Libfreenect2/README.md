Features
========
* Depth,Image,Ir streams of Kinect v2 are available through OpenNI2 interface.
* Free from Kinect SDK, so you can use this even in Linux,Mac OSX, and so on. (I have tested on Ubuntu 14.10.)
* libfreenect2 is now experimental and this implementation is still more experimental.

Requirement
===========
* libfreenect2 (https://github.com/MasWag/libfreenect2/commit/0d78c38c38be35af2b71e063e954203689f1fdbb is required.So don't use master but https://github.com/MasWag/libfreenect2/tree/sequence (branch sequence))
* libfreenect2's patched libusb-1.0 (You may already have libusb, but you need to config to be loaded patched libusb.)
* Powerfull GPU resource
* USB3.0 port

Getting Started
===============
1. You should try example of libfreenect2 first.
2. install libfreenect2
3. configure to be loaded patched libusb
4. write code of OpenNI2
5. build and run!!

Known Issue
===========
* Xtion Pro Live's color stream returns BGR but Kinect v2's one returns RGB.
* Color Record does not work well.

