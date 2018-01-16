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

import java.nio.ByteBuffer;

/**
 * The {@link VideoFrameRef} class encapsulates a single video frame - the output of a
 * {@link VideoStream} at a specific time. The data contained will be a single frame of color, IR,
 * or depth video, along with associated meta data.
 * 
 * An object of type {@link VideoFrameRef} does not actually hold the data of the frame, but only a
 * reference to it. OpenNI uses a ref-count to decide when the data buffer can be freed.
 * Once the frame is no longer needed, it can be released by calling the {@link #release()} method.
 * Although the finalization process of the garbage collector also releases the reference, 
 * it is preferable to manually release it by calling this method rather than to 
 * rely on a finalization process which may not run to completion for a long period of time.
 * 
 * The usual way to obtain {@link VideoFrameRef} objects is by a call to
 * {@link org.openni.VideoStream#readFrame()}. Please note that the returned frame
 * holds native memory. Although the finalization process of the garbage collector 
 * also disposes of the same system resources, it is preferable to manually free 
 * the associated resources by calling this method rather than to rely on a finalization 
 * process which may not run to completion for a long period of time.
 * 
 * All data references by a {@link VideoFrameRef} is stored as a primitive array of pixels. Each
 * pixel will be of a type according to the configured pixel format (see {@link VideoMode}).
 */
public class VideoFrameRef {

	/**
	 * Getter function for the array of data pointed to by this object.
	 * 
	 * @return ByteBuffer object to the actual frame data array. Type of data can be determined
	 *         according to the pixel format (can be obtained by calling {@link #getVideoMode()}).
	 */
	public final ByteBuffer getData() {
		return mData;
	}

	/**
	 * Getter function for the sensor type used to produce this frame. Used to determine whether this
	 * is an IR, Color or Depth frame. See the {@link SensorType} enumeration for all possible return
	 * values from this function.
	 * 
	 * @return The type of sensor used to produce this frame.
	 */
	public SensorType getSensorType() {
		return mSensorType;
	}

	/**
	 * Returns a reference to the {@link VideoMode} object assigned to this frame. This object
	 * describes the video mode the sensor was configured to when the frame was produced and can be
	 * used to determine the pixel format and resolution of the data. It will also provide the frame
	 * rate that the sensor was running at when it recorded this frame.
	 * 
	 * @return Reference to the {@link VideoMode} assigned to this frame.
	 */
	public final VideoMode getVideoMode() {
		return mVideoMode;
	}

	/**
	 * Provides a timestamp for the frame. The 'zero' point for this stamp is implementation specific,
	 * but all streams from the same device are guaranteed to use the same zero. This value can
	 * therefore be used to compute time deltas between frames from the same device, regardless of
	 * whether they are from the same stream.
	 * 
	 * @return Timestamp of frame, measured in microseconds from an arbitrary zero
	 */
	public long getTimestamp() {
		return mTimestamp;
	}

	/**
	 * Frames are provided sequential frame ID numbers by the sensor that produced them. If frame
	 * synchronization has been enabled for a device via
	 * {@link org.openni.Device#setDepthColorSyncEnabled(boolean)}, then frame numbers for
	 * corresponding frames of depth and color are guaranteed to match.
	 * 
	 * If frame synchronization is not enabled, then there is no guarantee of matching frame indexes
	 * between {@link VideoStream} "VideoStreams". In the latter case, applications should use
	 * timestamps instead of frame indexes to align frames in time.
	 * 
	 * @return Index number for this frame.
	 */
	public int getFrameIndex() {
		return mIndex;
	}

	/**
	 * Gives the current width of this frame, measured in pixels. If cropping is enabled, this will be
	 * the width of the cropping window. If cropping is not enabled, then this will simply be equal to
	 * the X resolution of the {@link VideoMode} used to produce this frame.
	 * 
	 * @return Width of this frame in pixels.
	 */
	public int getWidth() {
		return mWidth;
	}

	/**
	 * Gives the current height of this frame, measured in pixels. If cropping is enabled, this will
	 * be the length of the cropping window. If cropping is not enabled, then this will simply be
	 * equal to the Y resolution of the {@link VideoMode} used to produce this frame.
	 */
	public int getHeight() {
		return mHeight;
	}

	/**
	 * Indicates whether cropping was enabled when the frame was produced.
	 * 
	 * @return true if cropping is enabled, false otherwise
	 */
	public boolean getCroppingEnabled() {
		return mIsCropping;
	}

	/**
	 * Indicates the X coordinate of the upper left corner of the crop window.
	 * 
	 * @return Distance of crop origin from left side of image, in pixels.
	 */
	public int getCropOriginX() {
		return mCropOrigX;
	}

	/**
	 * Indicates the Y coordinate of the upper left corner of the crop window.
	 * 
	 * @return Distance of crop origin from top of image, in pixels.
	 */
	public int getCropOriginY() {
		return mCropOrigY;
	}

	/**
	 * Gives the length of one row of pixels, measured in bytes. Primarily useful for indexing the
	 * array which contains the data.
	 * 
	 * @return Stride of the array which contains the image for this frame, in bytes
	 */
	public int getStrideInBytes() {
		return mStride;
	}

	/**
	 * Release the reference to the frame. Once this method is called, the object becomes invalid, and
	 * no method should be called other than the assignment operator, or passing this object to a
	 * {@link VideoStream}::readFrame() call.
	 * 
	 * Although the finalization process of the garbage collector also releases the reference, 
	 * it is preferable to manually release it by calling this method rather than to 
	 * rely on a finalization process which may not run to completion for a long period of time.
	 */
	public void release() {
		if (mFrameHandle != 0) {
			NativeMethods.oniFrameRelease(mFrameHandle);
			mFrameHandle = 0;
		}
	}

	public long getHandle() { 
		return mFrameHandle;
	}

	@Override
	protected void finalize() throws Throwable {
		release();
		super.finalize();
	}

	private VideoFrameRef(long handle) { 
		NativeMethods.oniFrameAddRef(handle);
		mFrameHandle = handle;
	}

	private long mFrameHandle;
	private long mTimestamp;
	private VideoMode mVideoMode;
	private ByteBuffer mData;
	private SensorType mSensorType;
	private int mIndex, mWidth, mHeight, mCropOrigX, mCropOrigY, mStride;
	private boolean mIsCropping;
}
