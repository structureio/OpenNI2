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
package org.openni.android;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

import org.openni.Device;
import org.openni.DeviceInfo;
import org.openni.OpenNI;

/**
 * Provides facilities needed for using OpenNI in Android applications. This class takes care of:
 * <ul>
 * <li>Loading of OpenNI native libraries.</li>
 * <li>Copying OpenNI configuration and data files that are bundled in the APK as assets.</li>
 * <li>Enumerating the connected OpenNI-compliant devices.</li>
 * <li>Obtaining access permissions for OpenNI-compliant devices.</li>
 * </ul>
 */
public class OpenNIHelper {
	static {
		System.loadLibrary("usb");
		System.loadLibrary("OpenNI2");
		System.loadLibrary("OpenNI2.jni");
	}

	private Context mAndroidContext;
	private static final String OPENNI_ASSETS_DIR = "openni";
	private String mActionUsbPermission;
	private DeviceOpenListener mDeviceOpenListener;
	private String mUri;
	private static final String TAG = "OpenNIHelper";

	/**
	 * Used for receiving the result of {@link OpenNIHelper#requestDeviceOpen(String uri, DeviceOpenListener listener)}. 
	 */
	public static interface DeviceOpenListener {
		/**
		 * Called when permission to access the device is granted.
		 * @param device The device for which permission was granted.
		 */
		public abstract void onDeviceOpened(Device device);

		/**
		 * Called when permission is access the device is denied.
		 * @param uri The device for which permission was denied.
		 */
		public abstract void onDeviceOpenFailed(String uri);
	}

	/**
	 * Constructs an OpenNIHelper object. The constructor also extracts all files saved in {@code assets/openni},
	 * to make them accessible to OpenNI.
	 * @param context a Context object used to access application assets.
	 */
	public OpenNIHelper(Context context) {
		mAndroidContext = context;

		/*
		 * The configuration files are saved as assets. To make them readable by
		 * the OpenNI native library, we need to write them to the application
		 * files directory
		 */
		try {
			for (String fileName : mAndroidContext.getAssets().list(OPENNI_ASSETS_DIR)) {
				extractOpenNIAsset(fileName);
			}
		} catch (IOException e) {
			throw new RuntimeException(e);
		}

		mActionUsbPermission = context.getPackageName() + ".USB_PERMISSION";

		IntentFilter filter = new IntentFilter(mActionUsbPermission);
		mAndroidContext.registerReceiver(mUsbReceiver, filter);
	}

	/**
	 * Requests opening the specified device. If the device is a USB device, this may result in a
	 * system dialog being displayed to the user if permission had not already been granted.
	 * Success or failure is notified via a call the {@link DeviceOpenListener} interface.
	 * If successful, this grants the caller permission to access the device only until the device is
	 * disconnected. 
	 * @param uri device to open
	 * @param listener a listener to be notified about the result.
	 */
	public void requestDeviceOpen(String uri, DeviceOpenListener listener) {
		// TODO: Attach listener to this specific request rather than keep a "global" listener.
		// Theoretically, the client may call this method more than once with different listeners.
		mDeviceOpenListener = listener;
		mUri = uri;

		// check if this is a USB device
		UsbDevice usbDevice = getUsbDevice(uri);

		if (usbDevice == null) {
			// not a USB device, just open it
			openDevice(uri);
		} else {
			// USB device. request permissions for it
			PendingIntent permissionIntent = PendingIntent.getBroadcast(mAndroidContext, 0, new Intent(
					mActionUsbPermission), 0);

			UsbManager manager = (UsbManager) mAndroidContext.getSystemService(Context.USB_SERVICE);

			manager.requestPermission(usbDevice, permissionIntent);
			// flow will continue in the intent
		}
	}

	public UsbDevice getUsbDevice(String uri) {
		// look for this uri in OpenNI to get its device info
		List<DeviceInfo> devices = OpenNI.enumerateDevices();
		Iterator<DeviceInfo> iterator = devices.iterator();
		while (iterator.hasNext()) {
			DeviceInfo deviceInfo = iterator.next();
			if (deviceInfo.getUri().compareTo(uri) == 0) {
				return getUsbDevice(deviceInfo);
			}
		}

		return null;
	}

	public UsbDevice getUsbDevice(DeviceInfo deviceInfo) {
		UsbManager manager = (UsbManager) mAndroidContext.getSystemService(Context.USB_SERVICE);
		HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
		Iterator<UsbDevice> iterator = deviceList.values().iterator();

		// Remove any device that is not known to be OpenNI-compliant from the
		// list of USB devices
		while (iterator.hasNext()) {
			UsbDevice usbDevice = iterator.next();

			if (usbDevice.getVendorId() == deviceInfo.getUsbVendorId() && usbDevice.getProductId() == deviceInfo.getUsbProductId()) {
				return usbDevice;
			}
		}

		return null;
	}

	/**
	 * Releases the resources used by the helper.
	 */
	public void shutdown() {
		mAndroidContext.unregisterReceiver(mUsbReceiver);
	}

	private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();

			if (mActionUsbPermission.equals(action)) {
				synchronized (this) {
					if (mDeviceOpenListener == null) {
						return;
					}

					UsbDevice device = (UsbDevice) intent
							.getParcelableExtra(UsbManager.EXTRA_DEVICE);
					if (device == null) {
						return;
					}

					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
						// permission granted. open the device
						try {
							((UsbManager) mAndroidContext.getSystemService(Context.USB_SERVICE)).openDevice(device);
							openDevice(mUri);
						} catch (Exception ex) {
							Log.e(TAG, "Can't open device though permission was granted: " + ex);
							mDeviceOpenListener.onDeviceOpenFailed(mUri);
						}
					} else {
						Log.e(TAG, "Permission denied for device");
						mDeviceOpenListener.onDeviceOpenFailed(mUri);
					}
				}
			}
		}
	};

	private void openDevice(String uri) {
		Device device = null;
		try {
			device = Device.open(uri);
			mDeviceOpenListener.onDeviceOpened(device);
		} catch (Exception ex) {
			Log.e(TAG, "Failed to open device:" + ex);
			mDeviceOpenListener.onDeviceOpenFailed(uri);
		}
	}

	private void extractOpenNIAsset(String filename) throws IOException {
		InputStream is = mAndroidContext.getAssets().open(OPENNI_ASSETS_DIR + "/" + filename);

		mAndroidContext.deleteFile(filename);
		OutputStream os = mAndroidContext.openFileOutput(filename, Context.MODE_PRIVATE);

		byte[] buffer = new byte[is.available()];
		is.read(buffer);
		is.close();
		os.write(buffer);
		os.close();
	}
}
