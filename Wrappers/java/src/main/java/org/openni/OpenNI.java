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
package org.openni;

import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.concurrent.TimeoutException;

/**
 * The OpenNI class is a static entry point to the library. It is used by every OpenNI 2.0
 * application to initialize the SDK and drivers to enable creation of valid device objects.
 * 
 * It also defines a listener class and events that enable for event driven notification of device
 * connection, device disconnection, and device configuration changes.
 * 
 * In addition, it gives access to SDK version information and provides a function that allows you
 * to wait for data to become available on any one of a list of streams (as opposed to waiting for
 * data on one specific stream with functions provided by the VideoStream class)
 * 
 */
public class OpenNI {
	public final static int TIMEOUT_FOREVER = -1;

	/**
	 * The OpenNI.DeviceConnectedListener interface provides a means of registering for, and
	 * responding to when a device is connected.
	 * 
	 * onDeviceConnected is called whenever a new device is connected to the system (ie this event
	 * would be triggered when a new sensor is manually plugged into the host system running the
	 * application)
	 * 
	 * To use this class, you should write a new class that inherits from it, and override the
	 * onDeviceConnected method. Once you instantiate your class, use the
	 * {@link org.openni.OpenNI#addDeviceConnectedListener(org.openni.OpenNI.DeviceConnectedListener)}
	 * function to add your listener object to OpenNI's list of listeners. Your handler function will
	 * then be called whenever the event occurs. A
	 * {@link org.openni.OpenNI#removeDeviceConnectedListener(org.openni.OpenNI.DeviceConnectedListener)}
	 * function is also provided, if you want to have your class stop listening to these events for
	 * any reason.
	 */
	public interface DeviceConnectedListener {
		/**
		 * Callback function for the onDeviceConnected event. This function will be called whenever this
		 * event occurs. When this happens, a pointer to the {@link DeviceInfo} object for the newly
		 * connected device will be supplied. Note that once a device is removed, if it was opened by a
		 * {@link Device} object, that object can no longer be used to access the device, even if it was
		 * reconnected. Once a device was reconnected, {@link org.openni.Device#open(String)} should be
		 * called again in order to use this device.
		 * 
		 * If you wish to open the new device as it is connected, simply query the provided DeviceInfo
		 * object to obtain the URI of the device, and pass this URI to the
		 * {@link org.openni.Device#open(String)} function.
		 */
		void onDeviceConnected(DeviceInfo info);
	}
	/**
	 * The OpenNI.DeviceDisconnectedListener interface provides a means of registering for, and
	 * responding to when a device is disconnected.
	 * 
	 * onDeviceDisconnected is called when a device is removed from the system. Note that once a
	 * device is removed, if it was opened by a {@link Device} object, that object can no longer be
	 * used to access the device, even if it was reconnected. Once a device was reconnected,
	 * {@link org.openni.Device#open(String)} should be called again in order to use this device.
	 * 
	 * To use this class, you should write a new class that inherits from it, and override the
	 * onDeviceDisconnected method. Once you instantiate your class, use the
	 * {@link org.openni.OpenNI#addDeviceDisconnectedListener(org.openni.OpenNI.DeviceDisconnectedListener)}
	 * function to add your listener object to OpenNI's list of listeners. Your handler function will
	 * then be called whenever the event occurs. A
	 * {@link org.openni.OpenNI#removeDeviceDisconnectedListener(org.openni.OpenNI.DeviceDisconnectedListener)}
	 * function is also provided, if you want to have your class stop listening to these events for
	 * any reason.
	 */
	public interface DeviceDisconnectedListener {
		/**
		 * Callback function for the onDeviceDisconnected event. This function will be called whenever
		 * this event occurs. When this happens, a pointer to the DeviceInfo object for the newly
		 * disconnected device will be supplied. Note that once a device is removed, if it was opened by
		 * a {@link Device} object, that object can no longer be used to access the device, even if it
		 * was reconnected. Once a device was reconnected, {@link org.openni.Device#open(String)} should
		 * be called again in order to use this device.
		 */
		void onDeviceDisconnected(DeviceInfo info);
	}
	/**
	 * The OpenNI::DeviceStateChangedListener interface provides a means of registering for, and
	 * responding to when a device's state is changed.
	 * 
	 * onDeviceStateChanged is triggered whenever the state of a connected device is changed.
	 * 
	 * To use this class, you should write a new class that inherits from it, and override the
	 * onDeviceStateChanged method. Once you instantiate your class, use the
	 * {@link org.openni.OpenNI#addDeviceStateChangedListener(org.openni.OpenNI.DeviceStateChangedListener)}
	 * function to add your listener object to OpenNI's list of listeners. Your handler function will
	 * then be called whenever the event occurs. A
	 * {@link org.openni.OpenNI#removeDeviceStateChangedListener(org.openni.OpenNI.DeviceStateChangedListener)}
	 * function is also provided, if you want to have your class stop listening to these events for
	 * any reason.
	 */
	public interface DeviceStateChangedListener {
		/**
		 * Callback function for the onDeviceStateChanged event. This function will be called whenever
		 * this event occurs. When this happens, a pointer to a DeviceInfo object for the affected
		 * device will be supplied, as well as the new DeviceState value of that device.
		 */
		void onDeviceStateChanged(DeviceInfo info, int deviceState);
	}

	/**
	 * Initialize the library. This will load all available drivers, and see which devices are
	 * available It is forbidden to call any other method in OpenNI before calling initialize().
	 */
	public static void initialize() {
		NativeMethods.checkReturnStatus(NativeMethods.oniInitialize());
		mDeviceConnectedListener = new ArrayList<DeviceConnectedListener>();
		mDeviceDisconnectedListener = new ArrayList<DeviceDisconnectedListener>();
		mDeviceStateChangedListener = new ArrayList<DeviceStateChangedListener>();
	}

	/**
	 * Stop using the library. Unload all drivers, close all streams and devices. Once shutdown() was
	 * called, no other calls to OpenNI is allowed.
	 */
	public static void shutdown() {
		NativeMethods.oniShutdown();
	}

	/**
	 * This function return current OpenNI version
	 * 
	 * @return the version of OpenNI
	 */
	public static Version getVersion() {
		return NativeMethods.oniGetVersion();
	}

	/**
	 * Retrieves the calling thread's last extended error information. The last extended error
	 * information is maintained on a per-thread basis. Multiple threads do not overwrite each others
	 * last extended error information.
	 * 
	 * The extended error information is cleared on every call to an OpenNI method, so you should call
	 * this method immediately after a call to an OpenNI method which have failed.
	 * 
	 * @return OpenNI error String
	 */
	public static String getExtendedError() {
		return NativeMethods.oniGetExtendedError();
	}

	/**
	 * Fills up an array of {@link DeviceInfo} DeviceInfo objects with devices that are available.
	 * 
	 * @return deviceInfoList An array to be filled with devices.
	 */
	public static List<DeviceInfo> enumerateDevices() {
		List<DeviceInfo> devices = new ArrayList<DeviceInfo>();
		NativeMethods.checkReturnStatus(NativeMethods.oniGetDeviceList(devices));
		return devices;
	}

	/**
	 * Wait for a new frame from any of the streams provided. The function blocks until any of the
	 * streams has a new frame available, or the timeout has passed.
	 * 
	 * @param streams An list of streams to wait for.
	 * @param timeout A timeout before returning if no stream has new data. Default value is
	 *        {@link #TIMEOUT_FOREVER}.
	 * @return index of stream which received frame. In case it stop on timeout, function return -1.
	 */
	public static int waitForAnyStream(List<VideoStream> streams, int timeout) throws TimeoutException {
		long[] handles = new long[streams.size()];
		OutArg<Integer> readyId = new OutArg<Integer>();
		Iterator<VideoStream> itr = streams.iterator();
		int i = 0;
		while (itr.hasNext()) {
			handles[i] = itr.next().getHandle();
			i++;
		}
		boolean result = NativeMethods.oniWaitForAnyStream(handles, readyId, timeout);
		if (result)
			return readyId.mValue;
		else
			throw new TimeoutException();
	}

	/**
	 * Add new device connected observer to OpenNI observers list
	 * 
	 * @param deviceListener object which implements DeviceConnectedListener.
	 */
	public static void addDeviceConnectedListener(DeviceConnectedListener deviceListener) {
		mDeviceConnectedListener.add(deviceListener);
	}

	/**
	 * Remove device connected observer to OpenNI observers list
	 * 
	 * @param deviceListener object which implements DeviceConnectedListener.
	 */
	public static void removeDeviceConnectedListener(DeviceConnectedListener deviceListener) {
		mDeviceConnectedListener.remove(deviceListener);
	}

	/**
	 * Add new device connected observer to OpenNI observers list
	 * 
	 * @param deviceListener object which implements DeviceDisconnectedListener.
	 */
	public static void addDeviceDisconnectedListener(DeviceDisconnectedListener deviceListener) {
		mDeviceDisconnectedListener.add(deviceListener);
	}

	/**
	 * Remove device connected observer to OpenNI observers list
	 * 
	 * @param deviceListener object which implements DeviceDisconnectedListener.
	 */
	public static void removeDeviceDisconnectedListener(DeviceDisconnectedListener deviceListener) {
		mDeviceDisconnectedListener.remove(deviceListener);
	}

	/**
	 * Add new device connected observer to OpenNI observers list
	 * 
	 * @param deviceListener object which implements DeviceConnectedListener.
	 */
	public static void addDeviceStateChangedListener(DeviceStateChangedListener deviceListener) {
		mDeviceStateChangedListener.add(deviceListener);
	}

	/**
	 * Remove device state changed observer from OpenNI observers list
	 * 
	 * @param deviceListener object which implements DeviceConnectedListener.
	 */
	public static void removeDeviceStateChangedListener(DeviceStateChangedListener deviceListener) {
		mDeviceStateChangedListener.remove(deviceListener);
	}

	/**
	 * Set minimum severity for log entries
	 * 
	 * @param minSeverity Minimum severity to output
	 */
	public static void setLogMinSeverity(int minSeverity) {
		NativeMethods.checkReturnStatus(NativeMethods.oniSetLogMinSeverity(minSeverity));
	}

	/**
	 * Configures if log entries will be printed to console
	 * 
	 * @param enabled Whether log entries should be printed to console or not.
	 */
	public static void setLogConsoleOutput(boolean enabled) {
		NativeMethods.checkReturnStatus(NativeMethods.oniSetLogConsoleOutput(enabled));
	}

	/**
	 * Configures if log entries will be printed to file
	 * 
	 * @param enabled Whether log entries should be printed to file or not.
	 */
	public static void setLogFileOutput(boolean enabled) {
		NativeMethods.checkReturnStatus(NativeMethods.oniSetLogFileOutput(enabled));
	}

	/**
	 * Set the output folder for log files (if enabled)
	 * 
	 * @param path Path to write log files to.
	 */
	public static void setLogOutputFolder(String path) {
		NativeMethods.checkReturnStatus(NativeMethods.oniSetLogOutputFolder(path));
	}

	/**
	 * Configures if log entries will be printed to file
	 * 
	 * @param enabled Whether log entries should be printed to android log or not.
	 */
	public static void setLogAndroidOutput(boolean enabled) {
		NativeMethods.checkReturnStatus(NativeMethods.oniSetLogAndroidOutput(enabled));
	}

	@SuppressWarnings("unused") /* Called from JNI */
	private static void deviceConnected(DeviceInfo deviceInfo) {
		for (DeviceConnectedListener aDeviceConnectedListener : mDeviceConnectedListener) {
			aDeviceConnectedListener.onDeviceConnected(deviceInfo);
		}
	}

	@SuppressWarnings("unused") /* Called from JNI */
	private static void deviceDisconnected(DeviceInfo deviceInfo) {
		for (DeviceDisconnectedListener aDeviceDisconnectedListener : mDeviceDisconnectedListener) {
			aDeviceDisconnectedListener.onDeviceDisconnected(deviceInfo);
		}
	}

	@SuppressWarnings("unused") /* Called from JNI */
	private static void deviceStateChanged(DeviceInfo deviceInfo, int deviceState) {
		for (DeviceStateChangedListener aDeviceStateChangedListener : mDeviceStateChangedListener) {
			aDeviceStateChangedListener.onDeviceStateChanged(deviceInfo, deviceState);
		}
	}

	static private List<DeviceConnectedListener> mDeviceConnectedListener;
	static private List<DeviceDisconnectedListener> mDeviceDisconnectedListener;
	static private List<DeviceStateChangedListener> mDeviceStateChangedListener;
}
