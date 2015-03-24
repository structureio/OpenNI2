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
 * The CoordinateConverter class converts points between the different coordinate systems.
 * 
 * <b>Depth and World coordinate systems</b>
 * 
 * OpenNI applications commonly use two different coordinate systems to represent depth. These two
 * systems are referred to as Depth and World representation.
 * 
 * Depth coordinates are the native data representation. In this system, the frame is a map (two
 * dimensional array), and each pixel is assigned a depth value. This depth value represents the
 * distance between the camera plane and whatever object is in the given pixel. The X and Y
 * coordinates are simply the location in the map, where the origin is the top-left corner of the
 * field of view.
 * 
 * World coordinates superimpose a more familiar 3D Cartesian coordinate system on the world, with
 * the camera lens at the origin. In this system, every point is specified by 3 points -- x, y and
 * z. The x axis of this system is along a line that passes through the infrared projector and CMOS
 * imager of the camera. The y axis is parallel to the front face of the camera, and perpendicular
 * to the x axis (it will also be perpendicular to the ground if the camera is upright and level).
 * The z axis runs into the scene, perpendicular to both the x and y axis. From the perspective of
 * the camera, an object moving from left to right is moving along the increasing x axis. An object
 * moving up is moving along the increasing y axis, and an object moving away from the camera is
 * moving along the increasing z axis.
 * 
 * Mathematically, the Depth coordinate system is the projection of the scene on the CMOS. If the
 * sensor's angular field of view and resolution are known, then an angular size can be calculated
 * for each pixel. This is how the conversion algorithms work. The dependence of this calculation on
 * FoV and resolution is the reason that a {@link VideoStream} object must be provided to these
 * functions. The {@link VideoStream} object is used to determine parameters for the specific points
 * to be converted.
 * 
 * Since Depth coordinates are a projective, the apparent size of objects in depth coordinates
 * (measured in pixels) will increase as an object moves closer to the sensor. The size of objects
 * in the World coordinate system is independent of distance from the sensor.
 * 
 * Note that converting from Depth to World coordinates is relatively expensive computationally. It
 * is generally not practical to convert the entire raw depth map to World coordinates. A better
 * approach is to have your computer vision algorithm work in Depth coordinates for as long as
 * possible, and only converting a few specific points to World coordinates right before output.
 * 
 * Note that when converting from Depth to World or vice versa, the Z value remains the same.
 */
public class CoordinateConverter {
	/**
	 * Converts a single point from the World coordinate system to the Depth coordinate system.
	 * 
	 * @param depthStream Reference to an openni::VideoStream that will be used to determine the
	 *        format of the Depth coordinates
	 * @param worldX The X coordinate of the point to be converted, measured in millimeters in World
	 *        coordinates
	 * @param worldY The Y coordinate of the point to be converted, measured in millimeters in World
	 *        coordinates
	 * @param worldZ The Z coordinate of the point to be converted, measured in millimeters in World
	 *        coordinates
	 * @return Coordinate of the output value, and depth measured in the
	 *         {@link PixelFormat} of depthStream
	 */
	public static Point3D<Integer> convertWorldToDepthInt(final VideoStream depthStream,
			float worldX, float worldY, float worldZ) {
		OutArg<Float> x = new OutArg<Float>();
		OutArg<Float> y = new OutArg<Float>();
		OutArg<Float> z = new OutArg<Float>();
		NativeMethods.checkReturnStatus(NativeMethods.oniCoordinateConverterWorldToDepth(
				depthStream.getHandle(), worldX, worldY, worldZ, x, y, z));
		int depthX = x.mValue.intValue();
		int depthY = y.mValue.intValue();
		int depthZ = z.mValue.intValue();
		return new Point3D<Integer>(depthX, depthY, depthZ);
	}

	/**
	 * Converts a single point from the World coordinate system to a floating point representation of
	 * the Depth coordinate system
	 * 
	 * @param depthStream Reference to an openni::VideoStream that will be used to determine the
	 *        format of the Depth coordinates
	 * @param worldX The X coordinate of the point to be converted, measured in millimeters in World
	 *        coordinates
	 * @param worldY The Y coordinate of the point to be converted, measured in millimeters in World
	 *        coordinates
	 * @param worldZ The Z coordinate of the point to be converted, measured in millimeters in World
	 *        coordinates
	 * @return Point3DPoint to a place to store: the X coordinate of the output value, measured in
	 *         pixels with 0.0 at far left of the image
	 *         <p>
	 *         the Y coordinate of the output value, measured in pixels with 0.0 at the top of the
	 *         image
	 *         <p>
	 *         the Z(depth) coordinate of the output value, measured in millimeters with 0.0 at
	 *         the camera lens
	 */
	public static Point3D<Float> convertWorldToDepthFloat(final VideoStream depthStream,
			float worldX, float worldY, float worldZ) {
		OutArg<Float> x = new OutArg<Float>();
		OutArg<Float> y = new OutArg<Float>();
		OutArg<Float> z = new OutArg<Float>();
		NativeMethods.checkReturnStatus(NativeMethods.oniCoordinateConverterWorldToDepth(
				depthStream.getHandle(), worldX, worldY, worldZ, x, y, z));
		float depthX = x.mValue;
		float depthY = y.mValue;
		float depthZ = z.mValue;
		return new Point3D<Float>(depthX, depthY, depthZ);
	}

	/**
	 * Converts a single point from the Depth coordinate system to the World coordinate system.
	 * 
	 * @param depthStream Reference to an {@link VideoStream} that will be used to determine the
	 *        format of the Depth coordinates
	 * @param depthX The X coordinate of the point to be converted, measured in pixels with 0 at the
	 *        far left of the image
	 * @param depthY The Y coordinate of the point to be converted, measured in pixels with 0 at the
	 *        top of the image
	 * @param depthZ the Z(depth) coordinate of the point to be converted, measured in the
	 *        {@link PixelFormat} of depthStream
	 * @return a place to store the X,Y,Z coordinate of the output value, measured
	 *         in millimeters in World coordinates
	 */
	public static Point3D<Float> convertDepthToWorld(final VideoStream depthStream, int depthX,
			int depthY, short depthZ) {
		OutArg<Float> y = new OutArg<Float>();
		OutArg<Float> x = new OutArg<Float>();
		OutArg<Float> z = new OutArg<Float>();
		NativeMethods.checkReturnStatus(NativeMethods.oniCoordinateConverterDepthToWorld(
				depthStream.getHandle(), depthX, depthY, depthZ, x, y, z));
		float worldX = x.mValue;
		float worldY = y.mValue;
		float worldZ = z.mValue;
		return new Point3D<Float>(worldX, worldY, worldZ);
	}

	/**
	 * Converts a single point from a floating point representation of the Depth coordinate system to
	 * the World coordinate system.
	 * 
	 * @param depthStream Reference to an openni::VideoStream that will be used to determine the
	 *        format of the Depth coordinates
	 * @param depthX The X coordinate of the point to be converted, measured in pixels with 0.0 at the
	 *        far left of the image
	 * @param depthY The Y coordinate of the point to be converted, measured in pixels with 0.0 at the
	 *        top of the image
	 * @param depthZ Z(depth) coordinate of the point to be converted, measured in the
	 *        {@link PixelFormat}of depthStream
	 * @return a place to store the X coordinate of the output value, measured in
	 *         millimeters in World coordinates
	 */
	public static Point3D<Float> convertDepthToWorld(final VideoStream depthStream, float depthX,
			float depthY, float depthZ) {
		OutArg<Float> x = new OutArg<Float>();
		OutArg<Float> y = new OutArg<Float>();
		OutArg<Float> z = new OutArg<Float>();
		NativeMethods.checkReturnStatus(NativeMethods.oniCoordinateConverterDepthToWorld(
				depthStream.getHandle(), depthX, depthY, depthZ, x, y, z));
		float worldX = x.mValue;
		float worldY = y.mValue;
		float worldZ = z.mValue;
		return new Point3D<Float>(worldX, worldY, worldZ);
	}

	/**
	 * For a given depth point, provides the coordinates of the corresponding color value. Useful for
	 * superimposing the depth and color images. This operation is the same as turning on
	 * registration, but is performed on a single pixel rather than the whole image.
	 * 
	 * @param depthStream Reference to a openni::VideoStream that produced the depth value
	 * @param colorStream Reference to a openni::VideoStream that we want to find the appropriate
	 *        color pixel in
	 * @param depthX value of the depth point, given in Depth coordinates and measured in pixels
	 * @param depthY value of the depth point, given in Depth coordinates and measured in pixels
	 * @param depthZ value of the depth point, given in the {@link PixelFormat} of depthStream
	 * @return The Point2D with X,Y coordinate of the color pixel that overlaps the given depth pixel,
	 *         measured in pixels
	 */
	public static Point2D<Integer> convertDepthToColor(final VideoStream depthStream,
			final VideoStream colorStream, int depthX, int depthY, short depthZ) {
		OutArg<Integer> x = new OutArg<Integer>();
		OutArg<Integer> y = new OutArg<Integer>();
		NativeMethods.checkReturnStatus(NativeMethods.oniCoordinateConverterDepthToColor(
				depthStream.getHandle(), colorStream.getHandle(), depthX, depthY, depthZ, x, y));
		int colorX = x.mValue;
		int colorY = y.mValue;
		return new Point2D<Integer>(colorX, colorY);
	}
}
