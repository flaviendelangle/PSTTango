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

package imt.tangodepthmap;

import android.app.Activity;
import android.os.IBinder;
import android.util.Log;

/**
 * Interfaces between C and Java.
 */
public class TangoJNINative {
    static {
        // This project depends on tango_client_api, so we need to make sure we load
        // the correct library first.
        if (TangoInitializationHelper.loadTangoSharedLibrary() ==
                TangoInitializationHelper.ARCH_ERROR) {
            Log.e("TangoJNINative", "ERROR! Unable to load libtango_client_api.so!");
        }
        System.loadLibrary("depth_map");
    }

    /**
     * Interfaces to native OnCreate function.
     *
     * @param callerActivity the caller activity of this function.
     */
    public static native void onCreate(Activity callerActivity);

    /**
     * Called when the Tango service is connected successfully.
     *
     * @param nativeTangoServiceBinder The native binder object.
     */
    public static native void onTangoServiceConnected(IBinder nativeTangoServiceBinder);

    /**
     * Interfaces to native OnPause function.
     */
    public static native void onPause();

    public static native void onGlSurfaceCreated();

    public static native void onGlSurfaceChanged(int width, int height);

    public static native void onGlSurfaceDrawFrame();

    /**
     * Alpha blending between color and depth image.
     * Alpha = 0 : color image, Alpha = 1 : depth map
     */
    public static native void setDepthAlphaValue(float alpha);

    public static native void setRenderingDistanceValue(int renderingDistance);

    public static native void setRecordingMode(boolean isRecording, String path);

    public static native void onDisplayChanged(int displayRotation, int colorCameraRotation);
}
