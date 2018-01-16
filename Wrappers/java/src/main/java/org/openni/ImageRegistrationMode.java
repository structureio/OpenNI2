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
 * Provides string names values for all image registration type codes. <BR>
 * <BR>
 * 
 */
public enum ImageRegistrationMode {
	OFF(0), DEPTH_TO_COLOR(1);

	public int toNative() {
		return this.mValue;
	}

	public static ImageRegistrationMode fromNative(int value) {
		for (ImageRegistrationMode type : ImageRegistrationMode.values()) {
			if (type.mValue == value) return type;
		}

		throw new NoSuchElementException();
	}

	private final int mValue;

	private ImageRegistrationMode(int value) {
		this.mValue = value;
	}
}
