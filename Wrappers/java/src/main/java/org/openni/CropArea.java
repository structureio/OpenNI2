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
 * The CropArea object encapsulate cropping information data
 **/
public class CropArea {
	/**
	 * The CropArea constructor
	 * 
	 * @param originX X coordinate for the cropping
	 * @param originY Y coordinate for the cropping
	 * @param width width cropping value
	 * @param height height cropping value
	 **/
	public CropArea(int originX, int originY, int width, int height) {
		this.mOriginX = originX;
		this.mOriginY = originY;
		this.mWidth = width;
		this.mHeight = height;
	}

	/**
	 * Return X cropping coordinate
	 * 
	 * @return X cropping coordinate
	 **/
	public int getOriginX() {
		return mOriginX;
	}

	/**
	 * Return Y cropping coordinate
	 * 
	 * @return Y cropping coordinate
	 **/
	public int getOriginY() {
		return mOriginY;
	}

	/**
	 * Return width cropping value
	 * 
	 * @return width cropping value
	 **/
	public int getWidth() {
		return mWidth;
	}

	/**
	 * Return height cropping value
	 * 
	 * @return height cropping value
	 **/
	public int getHeight() {
		return mHeight;
	}

	private final int mOriginX, mOriginY, mWidth, mHeight;
}
