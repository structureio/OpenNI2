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
package org.openni.android;

import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.Color;
import android.opengl.GLES10;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

import org.openni.VideoFrameRef;

/**
 * A View that displays OpenNI streams.
 */
public class OpenNIView extends GLSurfaceView {

	// draw area
	protected int mDrawX = 0;
	protected int mDrawY = 0;
	protected int mDrawWidth = 0;
	protected int mDrawHeight = 0;

	protected int mSurfaceWidth = 0;
	protected int mSurfaceHeight = 0;
	
	protected int mTextureWidth = 0;
	protected int mTextureHeight = 0;
	protected ByteBuffer mTexture;
	protected int mTextureId = 0;

	private long mNativePtr = 0;
	
	private int mCurrFrameWidth = 0;
	private int mCurrFrameHeight = 0;
	
	private int mBaseColor = Color.WHITE;

	public OpenNIView(Context context) {
		super(context);
	}

	public OpenNIView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	private void init() {
		if(!isInEditMode()) {
			mNativePtr = nativeCreate();
		}

		setRenderer(new Renderer() {

			@Override
			public void onSurfaceCreated(GL10 gl, EGLConfig c) {
				/* Disable these capabilities. */
				final int gCapbilitiesToDisable[] = {
					GLES10.GL_FOG,
					GLES10.GL_LIGHTING,
					GLES10.GL_CULL_FACE,
					GLES10.GL_ALPHA_TEST,
					GLES10.GL_BLEND,
					GLES10.GL_COLOR_LOGIC_OP,
					GLES10.GL_DITHER,
					GLES10.GL_STENCIL_TEST,
					GLES10.GL_DEPTH_TEST,
					GLES10.GL_COLOR_MATERIAL,
				};

				for (int capability : gCapbilitiesToDisable)
				{
					GLES10.glDisable(capability);
				}

				GLES10.glEnable(GLES10.GL_TEXTURE_2D);

				int ids[] = new int[1];
				GLES10.glGenTextures(1, ids, 0);
				mTextureId = ids[0];
				GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, mTextureId);

				GLES10.glTexParameterf(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_MIN_FILTER, GLES10.GL_LINEAR);
				GLES10.glTexParameterf(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_MAG_FILTER, GLES10.GL_LINEAR);
				GLES10.glShadeModel(GLES10.GL_FLAT);
			}

			@Override
			public void onSurfaceChanged(GL10 gl, int w, int h) {
				synchronized (OpenNIView.this) {
					mSurfaceWidth = w;
					mSurfaceHeight = h;
					calcDrawArea();
				}
			}

			@Override
			public void onDrawFrame(GL10 gl) {
				synchronized (OpenNIView.this) {
					onDrawGL();
				}
			}
		});

		setRenderMode(RENDERMODE_WHEN_DIRTY);
	}

	@Override
	protected void finalize() throws Throwable {
		if (mNativePtr != 0) {
			nativeDestroy(mNativePtr);
			mNativePtr = 0;
		}
		super.finalize();
	}

	public void setBaseColor(int color) {
		mBaseColor = color;
		requestRender();
	}

	public int getBaseColor() {
		return mBaseColor;
	}

	/**
	 * Requests update of the view with an OpenNI frame.
	 * @param frame The frame to be drawn
	 */
	synchronized public void update(VideoFrameRef frame) {
		mCurrFrameWidth = frame.getVideoMode().getResolutionX();
		mCurrFrameHeight = frame.getVideoMode().getResolutionY();
		
		if (mTextureWidth < mCurrFrameWidth || mTextureHeight < mCurrFrameHeight) {
			// need to reallocate texture
			mTextureWidth = getClosestPowerOfTwo(mCurrFrameWidth);
			mTextureHeight = getClosestPowerOfTwo(mCurrFrameHeight);
			mTexture = ByteBuffer.allocateDirect(mTextureWidth * mTextureHeight * 4);
		}
		nativeUpdate(mNativePtr, mTexture, mTextureWidth, mTextureHeight, frame.getHandle());
		calcDrawArea();
		requestRender();
	}
	
	private int getClosestPowerOfTwo(int n)	{
		int m = 2;
		while (m < n)
		{
			m <<= 1;
		}
		return m;
	}

	synchronized public void clear() {
		if (mTexture != null) {
			nativeClear(mNativePtr, mTexture);
			requestRender();
		}
	}

	protected void onDrawGL() {
		if (mTexture == null || mDrawWidth == 0 || mDrawHeight == 0) {
			return;
		}
			
		GLES10.glEnable(GLES10.GL_BLEND);
		GLES10.glBlendFunc(GLES10.GL_SRC_ALPHA, GLES10.GL_ONE_MINUS_SRC_ALPHA);
		int red = Color.red(mBaseColor);
		int green = Color.green(mBaseColor);
		int blue = Color.blue(mBaseColor);
		int alpha = Color.alpha(mBaseColor); 
		GLES10.glColor4f(red/255.f, green/255.f, blue/255.f, alpha/255.f);

		GLES10.glEnable(GLES10.GL_TEXTURE_2D);

		GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, mTextureId);
		int rect[] = {0, mCurrFrameHeight, mCurrFrameWidth, -mCurrFrameHeight};
		GLES11.glTexParameteriv(GLES10.GL_TEXTURE_2D, GLES11Ext.GL_TEXTURE_CROP_RECT_OES, rect, 0);

		GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT);
		GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_RGBA, mTextureWidth, mTextureHeight, 0, GLES10.GL_RGBA,
				GLES10.GL_UNSIGNED_BYTE, mTexture);

		GLES11Ext.glDrawTexiOES(mDrawX, mDrawY, 0, mDrawWidth, mDrawHeight);

		GLES10.glDisable(GLES10.GL_TEXTURE_2D);
	}

	private void calcDrawArea() {
		if (mCurrFrameWidth == 0 || mCurrFrameHeight == 0 || mSurfaceWidth == 0 || mSurfaceHeight == 0) {
			mDrawX = mDrawY = mDrawWidth = mDrawHeight = 0;
			return;
		}

		// start with the entire surface
		mDrawX = 0;
		mDrawY = 0;
		mDrawWidth = mSurfaceWidth;
		mDrawHeight = mSurfaceHeight;

		// if view ratio is larger than frame ratio, make width smaller. Otherwise, make height smaller
		if (mCurrFrameWidth * mDrawHeight > mCurrFrameHeight * mDrawWidth)
		{
			mDrawHeight = mCurrFrameHeight * mDrawWidth / mCurrFrameWidth;
			mDrawY = (mSurfaceHeight - mDrawHeight) / 2;
		}
		else
		{
			mDrawWidth = mCurrFrameWidth * mDrawHeight / mCurrFrameHeight;
			mDrawX = (mSurfaceWidth - mDrawWidth) / 2;
		}
	}
	
	private static native long nativeCreate();
	private static native void nativeDestroy(long nativePtr);
	private static native void nativeUpdate(long nativePtr, ByteBuffer texture, int textureWidth, int textureHeight, long frameRef);
	private static native void nativeClear(long nativePtr, ByteBuffer texture);
}