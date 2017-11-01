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
 * The PlaybackControl class provides access to a series of specific to playing back a recording
 * from a file device.
 * 
 * When playing a stream back from a recording instead of playing from a live device, it is possible
 * to vary playback speed, change the current time location (ie fast forward / rewind / seek),
 * specify whether the playback should be repeated at the end of the recording, and query the total
 * size of the recording.
 * 
 * Since none of these functions make sense in the context of a physical device, they are split out
 * into a separate playback control class. To use, simply create your file device, create a
 * PlaybackControl, and then attach the PlaybackControl to the file device.
 */
public class PlaybackControl {

	public PlaybackControl(Device device) {
		this.mDevice = device;
	}

	/**
	 * Getter function for the current playback speed of this device.
	 * 
	 * This value is expressed as a multiple of the speed the original recording was taken at. For
	 * example, if the original recording was at 30fps, and playback speed is set to 0.5, then the
	 * recording will play at 15fps. If playback speed is set to 2.0, then the recording would
	 * playback at 60fps.
	 * 
	 * In addition, there are two "special" values. A playback speed of 0.0 indicates that the
	 * playback should occur as fast as the system is capable of returning frames. This is most useful
	 * when testing algorithms on large data sets, as it enables playback to be done at a much higher
	 * rate than would otherwise be possible.
	 * 
	 * A value of -1 indicates that speed is "manual". In this mode, new frames will only become
	 * available when an application manually reads them. If used in a polling loop, this setting also
	 * enables systems to read and process frames limited only by available processing speeds.
	 * 
	 */
	public float getSpeed() {
		OutArg<Float> val = new OutArg<Float>();
		NativeMethods.checkReturnStatus(NativeMethods.oniDeviceGetFloatProperty(mDevice.getHandle(),
				NativeMethods.DEVICE_PROPERTY_PLAYBACK_SPEED, val));
		return val.mValue;
	}

	/**
	 * Setter function for the playback speed of the device. For a full explanation of what this
	 * value means @see PlaybackControl::getSpeed().
	 * 
	 * @param speed Desired new value of playback speed, as ratio of original recording.
	 */
	public void setSpeed(float speed) {
		NativeMethods.checkReturnStatus(NativeMethods.oniDeviceSetProperty(mDevice.getHandle(),
				NativeMethods.DEVICE_PROPERTY_PLAYBACK_SPEED, speed));
	}

	/**
	 * Gets the current repeat setting of the file device.
	 * 
	 * @return true if repeat is enabled, false if not enabled.
	 */
	public boolean getRepeatEnabled() {
		OutArg<Boolean> val = new OutArg<Boolean>();
		NativeMethods.checkReturnStatus(NativeMethods.oniDeviceGetBoolProperty(mDevice.getHandle(),
				NativeMethods.DEVICE_PROPERTY_PLAYBACK_REPEAT_ENABLED, val));
		return val.mValue;
	}

	/**
	 * Changes the current repeat mode of the device. If repeat mode is turned on, then the recording
	 * will begin playback again at the beginning after the last frame is read. If turned off, no more
	 * frames will become available after last frame is read.
	 * 
	 * @param repeat New value for repeat -- true to enable, false to disable
	 */
	public void setRepeatEnabled(boolean repeat) {
		NativeMethods.checkReturnStatus(NativeMethods.oniDeviceSetProperty(mDevice.getHandle(),
				NativeMethods.DEVICE_PROPERTY_PLAYBACK_REPEAT_ENABLED, repeat));
	}

	/**
	 * Seeks within a VideoStream to a given FrameID. Note that when this function is called on one
	 * stream, all other streams will also be changed to the corresponding place in the recording. The
	 * FrameIDs of different streams may not match, since FrameIDs may differ for streams that are not
	 * synchronized, but the recording will set all streams to the same moment in time.
	 * 
	 * @param stream Stream for which the frameIndex value is valid.
	 * @param frameIndex Frame index to move playback to
	 */
	public void seek(VideoStream stream, int frameIndex) {
		NativeMethods.checkReturnStatus(NativeMethods.seek(mDevice.getHandle(), stream.getHandle(),
				frameIndex));
	}

	/**
	 * Provides the a count of frames that this recording contains for a given stream. This is useful
	 * both to determine the length of the recording, and to ensure that a valid Frame Index is set
	 * when using the {@link #seek(VideoStream stream, int frameIndex)} function.
	 * 
	 * @param stream The video stream to count frames for
	 * @return Number of frames in provided {@link VideoStream}, or 0 if the stream is not part of the
	 *         recording
	 */
	public int getNumberOfFrames(final VideoStream stream) {
		OutArg<Integer> val = new OutArg<Integer>();
		NativeMethods.checkReturnStatus(NativeMethods.oniStreamGetIntProperty(stream.getHandle(),
				NativeMethods.STREAM_PROPERTY_NUMBER_OF_FRAMES, val));
		return val.mValue;
	}

	private final Device mDevice;
}
