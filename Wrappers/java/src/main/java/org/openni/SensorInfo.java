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

/**
 * <p>
 * The SensorInfo class encapsulates all info related to a specific sensor in a specific device.<br>
 * A {@link Device} object holds a SensorInfo object for each sensor it contains.<br>
 * A {@link VideoStream} object holds one SensorInfo object, describing the sensor used to produce
 * that stream.
 * </p>
 * 
 * <p>
 * A given SensorInfo object will contain the type of the sensor (Depth, IR or Color), and a list of
 * all video modes that the sensor can support. Each available video mode will have a single
 * VideoMode object that can be queried to get the details of that mode.
 * </p>
 * 
 * <p>
 * SensorInfo objects should be the only source of VideoMode objects for the vast majority of
 * application programs.
 * </p>
 * 
 * <p>
 * Application programs will never directly instantiate objects of type SensorInfo. In fact, no
 * public constructors are provided. SensorInfo objects should be obtained either from a Device or
 * {@link VideoStream}, and in turn be used to provide available video modes for that sensor.
 * </p>
 */
public class SensorInfo {
	/**
	 * Provides the sensor type of the sensor this object is associated with.
	 * 
	 * @return Type of the sensor.
	 */
	public SensorType getSensorType() {
		return mSensorType;
	}

	/**
	 * Provides a list of video modes that this sensor can support. This function is the recommended
	 * method to be used by applications to obtain {@link VideoMode} objects.
	 * 
	 * @return Reference to an array of {@link VideoMode} objects, one for each supported video mode.
	 */
	public final List<VideoMode> getSupportedVideoModes() {
		return mVideoModes;
	}

	private SensorInfo(int sensorType, List<VideoMode> videoModes) {
		this.mSensorType = SensorType.fromNative(sensorType);
		this.mVideoModes = videoModes;
	}

	private final SensorType mSensorType;
	private final List<VideoMode> mVideoModes;
}
