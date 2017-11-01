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
 * The Point3D object encapsulate 3 Dimension point
 **/
public class Point3D<T> {
	/**
	 * The Point3D constructor
	 * 
	 * @param x coordinate of the point
	 * @param y coordinate of the point
	 * @param z coordinate of the point
	 **/
	public Point3D(T x, T y, T z) {
		this.mX = x;
		this.mY = y;
		this.mZ = z;
	}

	/**
	 * Return X coordinate
	 * 
	 * @return X coordinate
	 **/
	public T getX() {
		return mX;
	}

	/**
	 * Return Y coordinate
	 * 
	 * @return Y coordinate
	 **/
	public T getY() {
		return mY;
	}

	/**
	 * Return Z coordinate
	 * 
	 * @return Z coordinate
	 **/
	public T getZ() {
		return mZ;
	}

	private final T mX, mY, mZ;
}
