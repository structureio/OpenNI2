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

import java.util.NoSuchElementException;

/**
 * Provides string names values for all pixel format type codes. <BR>
 * <BR>
 * 
 */
public enum PixelFormat {
	DEPTH_1_MM(100), DEPTH_100_UM(101), SHIFT_9_2(102), SHIFT_9_3(103),

	// Color
	RGB888(200), YUV422(201), GRAY8(202), GRAY16(203), JPEG(204), YUYV(205);
	private final int mValue;

	private PixelFormat(int value) {
		this.mValue = value;
	}

	public int toNative() {
		return this.mValue;
	}

	public static PixelFormat fromNative(int value) {
		for (PixelFormat type : PixelFormat.values()) {
			if (type.mValue == value) return type;
		}

		throw new NoSuchElementException(String.format("Unknown pixel format: %d", value));
	}



}
