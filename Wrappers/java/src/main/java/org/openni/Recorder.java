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

/**
 * The Recorder class is used to record streams to an ONI file.
 * 
 * After a recorder is instantiated, it must be initialized with a specific filename where the
 * recording will be stored. The recorder is then attached to one or more streams. Once this is
 * complete, the recorder can be told to start recording. The recorder will store every frame from
 * every stream to the specified file. Later, this file can be used to initialize a file Device, and
 * used to play back the same data that was recorded.
 * 
 * Opening a file device is done by passing its path as the uri to the
 * {@link org.openni.Device#open(String)} method.
 * 
 * {@link PlaybackControl} for options available to play a recorded file.
 * 
 */
public class Recorder {
	/**
	 * Initializes a recorder. You can initialize the recorder only once. Attempts to initialize more
	 * than once will result in an error code being returned.
	 * 
	 * Initialization assigns the recorder to an output file that will be used for recording. Before
	 * use, the {@link #addStream(VideoStream, boolean)} function must also be used to assign input
	 * data to the Recorder.
	 * 
	 * @param fileName The name of a file which will contain the recording.
	 */
	public static Recorder create(String fileName) {
		Recorder recorder = new Recorder();
		NativeMethods.checkReturnStatus(NativeMethods.oniCreateRecorder(fileName, recorder));
		return recorder;
	}

	/**
	 * This function return recorded handle.
	 * 
	 * @return OpenNI recorder handle.
	 */
	public long getHandle() {
		return mRecorderHandle;
	}

	/**
	 * Attaches a stream to the recorder. Note, this won't start recording, you should explicitly
	 * start it using {@link #start()} method. As soon as the recording process has been started, no
	 * more streams can be attached to the recorder.
	 * 
	 * @param stream The stream to be recorded.
	 * @param allowLossyCompression If this value is true, the recorder might use a lossy compression,
	 *        which means that when the recording will be played-back, there might be small
	 *        differences from the original frame. Default value is false.
	 */
	public void addStream(VideoStream stream, boolean allowLossyCompression) {
		NativeMethods.checkReturnStatus(NativeMethods.oniRecorderAttachStream(getHandle(),
				stream.getHandle(), allowLossyCompression));
	}

	/**
	 * Starts recording. Once this method is called, the recorder will take all subsequent frames from
	 * the attached streams and store them in the file. You may not add additional streams once
	 * recording was started.
	 */
	public void start() {
		NativeMethods.checkReturnStatus(NativeMethods.oniRecorderStart(getHandle()));
	}

	/**
	 * Stops recording. You may use {@link #start()} to resume the recording.
	 */
	public void stop() {
		NativeMethods.oniRecorderStop(getHandle());
	}

	/**
	 * Destroys a recorder. This will also stop recording.
	 */
	public void destroy() {
		NativeMethods.checkReturnStatus(NativeMethods.oniRecorderDestroy(getHandle()));
	}

	private long mRecorderHandle;
}
