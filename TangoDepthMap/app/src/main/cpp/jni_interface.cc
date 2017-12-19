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

#include <jni.h>

#include "tango_depth_map/tango_depth_map_app.h"

static tango_depth_map::SynchronizationApplication app;

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onCreate(
        JNIEnv *env, jobject, jobject activity) {

    JavaVM* javaVM;
    env->GetJavaVM(&javaVM);

    app.OnCreate(env, activity, javaVM, env->NewGlobalRef(activity));
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onTangoServiceConnected(
        JNIEnv *env, jobject, jobject iBinder) {
    app.OnTangoServiceConnected(env, iBinder);
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onPause(
        JNIEnv *, jobject) {
    app.OnPause();
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onGlSurfaceCreated(
        JNIEnv *, jobject) {
    app.OnSurfaceCreated();
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onGlSurfaceChanged(
        JNIEnv *, jobject, jint width, jint height) {

    app.OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onGlSurfaceDrawFrame(
        JNIEnv *, jobject) {
    app.OnDrawFrame();
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_setDepthAlphaValue(
        JNIEnv *, jobject, jfloat alpha) {
    app.SetDepthAlphaValue(alpha);
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_setRenderingDistanceValue(
        JNIEnv *, jobject ,jint renderingDistance) {
    app.SetRenderingDistance(renderingDistance);
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_setRecordingMode(
        JNIEnv *env, jobject ,jboolean isRecording, jstring path) {

    const char *pathString = env->GetStringUTFChars(path, JNI_FALSE);
    std::string cppPath = std::string(pathString);
    env->ReleaseStringUTFChars(path, pathString);

    app.SetRecordingMode(isRecording, cppPath);
}

JNIEXPORT void JNICALL
Java_imt_tangodepthmap_TangoJNINative_onDisplayChanged(
        JNIEnv *, jobject, jint display_rotation, jint color_camera_rotation) {
    app.OnDisplayChanged(display_rotation, color_camera_rotation);
}

#ifdef __cplusplus
}
#endif