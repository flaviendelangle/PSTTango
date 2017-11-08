/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.depthmap;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.hardware.display.DisplayManager;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.RequiresApi;
import android.view.Display;
import android.view.View;
import android.widget.ToggleButton;

/**
 * Main activity shows video overlay scene.
 */
public class DepthMap extends Activity {
    private GLSurfaceView mSurfaceView;
    private ToggleButton mYuvRenderSwitcher;
    private boolean mIsConnected = false;

    private static final int MY_CAMERA_REQUEST_CODE = 100;

    private ServiceConnection mTangoServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            TangoJniNative.onTangoServiceConnected(binder);
            setDisplayRotation();
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            // Handle this if you need to gracefully shutdown/retry
            // in the event that Tango itself crashes/gets upgraded while running.
        }
    };

    @SuppressLint("NewApi")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.CAMERA}, MY_CAMERA_REQUEST_CODE);
        }

        //TangoJniNative.onCreate(this);

        // Register for display orientation change updates.
        DisplayManager displayManager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
        if (displayManager != null) {
            displayManager.registerDisplayListener(new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {}

                @Override
                public void onDisplayChanged(int displayId) {
                    synchronized (this) {
                        setDisplayRotation();
                    }
                }

                @Override
                public void onDisplayRemoved(int displayId) {}
            }, null);
        }



        // Configure OpenGL renderer
        mSurfaceView = (GLSurfaceView) findViewById(R.id.surfaceview);
        mSurfaceView.setEGLContextClientVersion(2);
        mSurfaceView.setRenderer(new DepthMapRenderer());

        mYuvRenderSwitcher = (ToggleButton) findViewById(R.id.yuv_switcher);
		
		TangoJniNative.initialize(this);
	}

    @Override
    protected void onResume() {
        super.onResume();
        mSurfaceView.onResume();
        TangoInitializationHelper.bindTangoService(this, mTangoServiceConnection);
        
		//TangoJniNative.setupConfig();
		if (!mIsConnected) {
		  //TangoJniNative.connect();
		  mIsConnected = true;
		}
		TangoJniNative.setYuvMethod(mYuvRenderSwitcher.isChecked());
    }

    @Override
    protected void onPause() {
        super.onPause();
        mSurfaceView.onPause();
        /*TangoJniNative.onPause();
        unbindService(mTangoServiceConnection);*/
		if (mIsConnected) {
		  TangoJniNative.disconnect();
		  mIsConnected = false;
		}
		TangoJniNative.freeBufferData();
    }

    /**
     * The render mode toggle button was pressed.
     */
    public void renderModeClicked(View view) {
        TangoJniNative.setYuvMethod(mYuvRenderSwitcher.isChecked());
    }

    /**
     *  Pass device rotation to native layer. This parameter is important for Tango to render video
     *  overlay in the correct device orientation.
     */
    private void setDisplayRotation() {
        Display display = getWindowManager().getDefaultDisplay();
        TangoJniNative.onDisplayChanged(display.getRotation());
    }
}
