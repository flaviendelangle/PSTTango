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

#define GLM_FORCE_RADIANS

#include <jni.h>
#include "Depth_map.h"

static dmap::Depth_map app;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL
Java_com_depthmap_TangoJniNative_initialize(
    JNIEnv* env, jobject, jobject activity) {
  return app.TangoInitialize(env, activity);
}

/*JNIEXPORT jint JNICALL
Java_com_depthmap_TangoJniNative_setupConfig(
    JNIEnv*, jobject) {
  return app.TangoSetupConfig();
}*/

/*JNIEXPORT jint JNICALL
Java_com_depthmap_TangoJniNative_connect(
    JNIEnv*, jobject) {
  return app.TangoConnect();
}*/

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_disconnect(
    JNIEnv*, jobject) {
  app.TangoDisconnect();
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_initGlContent(
    JNIEnv*, jobject) {
  app.InitializeGLContent();
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_setupGraphic(
    JNIEnv*, jobject, jint width, jint height) {
  app.SetViewPort(width, height);
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_render(
    JNIEnv*, jobject) {
  app.Render();
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_freeBufferData(
    JNIEnv*, jobject) {
  app.FreeBufferData();
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onTangoServiceConnected(
        JNIEnv* env, jobject, jobject binder) {
    app.OnTangoServiceConnected(env, binder);
}

/*JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onCreate(
        JNIEnv* env, jobject, jobject activity) {
    app.OnCreate(env, activity);
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onPause(
        JNIEnv*, jobject) {
    app.OnPause();
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onGlSurfaceCreated(
        JNIEnv*, jobject) {
    app.OnSurfaceCreated();
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onGlSurfaceChanged(
        JNIEnv*, jobject, jint width, jint height) {
    app.OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onGlSurfaceDrawFrame(
        JNIEnv*, jobject) {
    app.OnDrawFrame();
}*/

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_setYuvMethod(
        JNIEnv*, jobject, jboolean use_yuv_method) {
    if (use_yuv_method) {
        app.SetTextureMethod(dmap::Depth_map::TextureMethod::kYuv);
    } else {
        app.SetTextureMethod(dmap::Depth_map::TextureMethod::kTextureId);
    }
}

JNIEXPORT void JNICALL
Java_com_depthmap_TangoJniNative_onDisplayChanged(
        JNIEnv* , jobject , jint display_rotation) {
    app.OnDisplayChanged(display_rotation);
}

#ifdef __cplusplus
}
#endif
