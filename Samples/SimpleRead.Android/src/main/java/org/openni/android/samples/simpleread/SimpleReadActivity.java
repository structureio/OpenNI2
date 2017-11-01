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
package org.openni.android.samples.simpleread;

import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import org.openni.Device;
import org.openni.DeviceInfo;
import org.openni.OpenNI;
import org.openni.SensorType;
import org.openni.VideoFrameRef;
import org.openni.VideoStream;
import org.openni.android.OpenNIHelper;

public class SimpleReadActivity 
		extends Activity 
		implements OpenNIHelper.DeviceOpenListener {
	
	private static final String TAG = "SimplerRead";
	private OpenNIHelper mOpenNIHelper;
	private boolean mDeviceOpenPending = false;
	private Thread mMainLoopThread;
	private boolean mShouldRun = true;
	private Device mDevice;
	private VideoStream mStream;
	private TextView mStatusLine;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		mOpenNIHelper = new OpenNIHelper(this);
		OpenNI.setLogAndroidOutput(true);
		OpenNI.setLogMinSeverity(0);
		OpenNI.initialize();
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_simple_read);
		mStatusLine = (TextView) findViewById(R.id.status_line);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		mOpenNIHelper.shutdown();
		OpenNI.shutdown();
	}

	@Override
	protected void onResume() {
		Log.d(TAG, "onResume");

		super.onResume();

		// onResume() is called after the USB permission dialog is closed, in which case, we don't want
		// to request device permissions again
		if (mDeviceOpenPending) {
			return;
		}

		// Request opening the first OpenNI-compliant device found
		String uri;
		
		List<DeviceInfo> devices = OpenNI.enumerateDevices();
		if (devices.isEmpty()) {
			showAlertAndExit("No OpenNI-compliant device found.");
			return;
		}
		
		uri = devices.get(0).getUri();
		
		mDeviceOpenPending = true;
		mOpenNIHelper.requestDeviceOpen(uri, this);
	}

	private void showAlertAndExit(String message) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(message);
		builder.setNeutralButton("OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				finish();
			}
		});
		builder.show();
	}

	@Override
	public void onDeviceOpened(Device aDevice) {
		Log.d(TAG, "Permission granted for device " + aDevice.getDeviceInfo().getUri());
		
		mDeviceOpenPending = false;

		try {
			mDevice = aDevice;
			mStream = VideoStream.create(mDevice, SensorType.DEPTH);
			mStream.start();
		} catch (RuntimeException e) {
			showAlertAndExit("Failed to open stream: " + e.getMessage());
			return;
		}
		
		mShouldRun = true;
		mMainLoopThread = new Thread() {
			@Override
			public void run() {
				while (mShouldRun) {
					VideoFrameRef frame = null;
					
					try {
						frame = mStream.readFrame();
						// get the middle pixel
						int index = frame.getVideoMode().getResolutionY() / 2 * frame.getVideoMode().getResolutionX() + frame.getVideoMode().getResolutionX() / 2;
						short depthValue = frame.getData().getShort(index * 2);
						updateLabel(String.format("Frame Index: %,d | Timestamp: %.6f seconds | Middle depth value: %,d mm", frame.getFrameIndex(), frame.getTimestamp() / 1e6, depthValue));
					} catch (Exception e) {
						Log.e(TAG, "Failed reading frame: " + e);
					}
				}
			};
		};
		
		mMainLoopThread.setName("SimpleRead MainLoop Thread");
		mMainLoopThread.start();
	}
	
	private void stop() {
		mShouldRun = false;
		
		while (mMainLoopThread != null) {
			try {
				mMainLoopThread.join();
				mMainLoopThread = null;
				break;
			} catch (InterruptedException e) {
			}
		}

		if (mStream != null) {
			mStream.stop();
		}
		
		mStatusLine.setText(R.string.waiting_for_frames);
	}
	
	private void updateLabel(final String message) {
		runOnUiThread(new Runnable() {
			public void run() {
				mStatusLine.setText(message);								
			}
		});
	}

	@Override
	public void onDeviceOpenFailed(String uri) {
		Log.e(TAG, "Failed to open device " + uri);
		mDeviceOpenPending = false;
		showAlertAndExit("Failed to open device");
	}

	@Override
	protected void onPause() {
		Log.d(TAG, "onPause");

		super.onPause();
		
		// onPause() is called just before the USB permission dialog is opened, in which case, we don't
		// want to shutdown OpenNI
		if (mDeviceOpenPending)
			return;

		stop();
		
		if (mStream != null) {
			mStream.destroy();
			mStream = null;
		}
		
		if (mDevice != null) {
			mDevice.close();
			mDevice = null;
		}
	}
}
