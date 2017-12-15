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
#include <tango-gl/conversions.h>
#include <tango_support.h>
#include <chrono>

#include <tango_depth_map/tango_depth_map_app.h>
#include <iterator>

namespace {
// The minimum Tango Core version required from this application.
    constexpr int kTangoCoreMinimumVersion = 9377;

    //COLOR
    void OnFrameAvailableRouter(void *context, TangoCameraId,
                                const TangoImageBuffer *buffer) {
        tango_depth_map::SynchronizationApplication *app =
                static_cast<tango_depth_map::SynchronizationApplication *>(context);
        app->OnFrameAvailable(buffer);
    }


// This function will route callbacks to our application object via the context
// parameter.
// @param context Will be a pointer to a SynchronizationApplication instance  on
// which to call callbacks.
// @param point_cloud The point cloud to pass on.
    void OnPointCloudAvailableRouter(void *context,
                                     const TangoPointCloud *point_cloud) {
        tango_depth_map::SynchronizationApplication *app =
                static_cast<tango_depth_map::SynchronizationApplication *>(context);
        app->OnPointCloudAvailable(point_cloud);
    }

}  // namespace

namespace tango_depth_map {

    void SynchronizationApplication::OnFrameAvailable(
            const TangoImageBuffer *buffer) {
        TangoSupport_updateImageBuffer(image_buffer_manager_, buffer);
    }

    void SynchronizationApplication::OnPointCloudAvailable(
            const TangoPointCloud *point_cloud) {
        // We'll just update the point cloud associated with our depth image.
        TangoSupport_updatePointCloud(point_cloud_manager_, point_cloud);
    }

    SynchronizationApplication::SynchronizationApplication()
            : color_image_(),
              depth_image_(),
              main_scene_(),
              is_service_connected_(false),
              is_gl_initialized_(false),
              color_camera_to_display_rotation_(
                      TangoSupport_Rotation::TANGO_SUPPORT_ROTATION_0) {}

    SynchronizationApplication::~SynchronizationApplication() {
        if (tango_config_) {
            TangoConfig_free(tango_config_);
        }
        TangoSupport_freePointCloudManager(point_cloud_manager_);
        point_cloud_manager_ = nullptr;

        TangoSupport_freeImageBufferManager(image_buffer_manager_);
        image_buffer_manager_ = nullptr;
    }

    void SynchronizationApplication::OnCreate(JNIEnv *env, jobject activity) {
        // Check the installed version of the TangoCore.  If it is too old, then
        // it will not support the most up to date features.
        int version;
        TangoErrorType err = TangoSupport_getTangoVersion(env, activity, &version);
        if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion) {
            LOGE(
                    "SynchronizationApplication::OnCreate, Tango Core version is out of "
                            "date.");
            std::exit(EXIT_SUCCESS);
        }
    }

    void SynchronizationApplication::OnTangoServiceConnected(JNIEnv *env,
                                                             jobject binder) {
        TangoErrorType ret = TangoService_setBinder(env, binder);

        if (ret != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Failed to set Tango service binder with"
                            "error code: %d",
                    ret);
            std::exit(EXIT_SUCCESS);
        }

        // Initialize TangoSupport context.
        TangoSupport_initialize(TangoService_getPoseAtTime,
                                TangoService_getCameraIntrinsics);

        TangoSetupConfig();
        TangoConnectCallbacks();
        TangoConnect();

        is_service_connected_ = true;
    }

    void SynchronizationApplication::TangoSetupConfig() {
        SetDepthAlphaValue(0.0);

        if (tango_config_ != nullptr) {
            return;
        }

        // Here, we'll configure the service to run in the way we'd want. For this
        // application, we'll start from the default configuration
        // (TANGO_CONFIG_DEFAULT). This enables basic motion tracking capabilities.
        tango_config_ = TangoService_getConfig(TANGO_CONFIG_DEFAULT);
        if (tango_config_ == nullptr) {
            std::exit(EXIT_SUCCESS);
        }

        // In addition to motion tracking, however, we want to run with depth so that
        // we can sync Image data with Depth Data. As such, we're going to set an
        // additional flag "config_enable_depth" to true.
        TangoErrorType err =
                TangoConfig_setBool(tango_config_, "config_enable_depth", true);
        if (err != TANGO_SUCCESS) {
            LOGE("Failed to enable depth.");
            std::exit(EXIT_SUCCESS);
        }

        // Need to specify the depth_mode as XYZC.
        err = TangoConfig_setInt32(tango_config_, "config_depth_mode",
                                   TANGO_POINTCLOUD_XYZC);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "Failed to set 'depth_mode' configuration flag with error"
                            " code: %d",
                    err);
            std::exit(EXIT_SUCCESS);
        }

        // We also need to enable the color camera in order to get RGB frame
        // callbacks.
        err = TangoConfig_setBool(tango_config_, "config_enable_color_camera", true);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "Failed to set 'enable_color_camera' configuration flag with error"
                            " code: %d",
                    err);
            std::exit(EXIT_SUCCESS);
        }

        // Note that it's super important for AR applications that we enable low
        // latency imu integration so that we have pose information available as
        // quickly as possible. Without setting this flag, you'll often receive
        // invalid poses when calling GetPoseAtTime for an image.
        err = TangoConfig_setBool(tango_config_,
                                  "config_enable_low_latency_imu_integration", true);
        if (err != TANGO_SUCCESS) {
            LOGE("Failed to enable low latency imu integration.");
            std::exit(EXIT_SUCCESS);
        }
    }

    void SynchronizationApplication::TangoConnectCallbacks() {

        // Use the tango_config to set up the PointCloudManager before we connect
        // the callbacks.
        TangoErrorType err;
        if (point_cloud_manager_ == nullptr) {
            int32_t max_point_cloud_elements;
            err = TangoConfig_getInt32(tango_config_, "max_point_cloud_elements",
                                       &max_point_cloud_elements);
            if (err != TANGO_SUCCESS) {
                LOGE("Failed to query maximum number of point cloud elements.");
                std::exit(EXIT_SUCCESS);
            }

            err = TangoSupport_createPointCloudManager(max_point_cloud_elements,
                                                       &point_cloud_manager_);
            if (err != TANGO_SUCCESS) {
                std::exit(EXIT_SUCCESS);
            }
        }

        //We need to be notified when we receive depth information in order to support measuring
        // 3D points. For both pose and color camera information, we'll be polling.
        // The render loop will drive the rate at which we need color images and all
        // our poses will be driven by timestamps. As such, we'll use GetPoseAtTime.

        err = TangoService_connectOnPointCloudAvailable(OnPointCloudAvailableRouter);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Failed to connect point cloud callback "
                            "with errorcode: %d",
                    err);
            std::exit(EXIT_SUCCESS);
        }

        // Connect color camera texture. The callback is ignored because the
        // color camera is polled.
        err = TangoService_connectOnTextureAvailable(TANGO_CAMERA_COLOR, nullptr,
                                                     nullptr);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Failed to connect texture callback with "
                            "errorcode: %d",
                    err);
            std::exit(EXIT_SUCCESS);
        }
    }

    void SynchronizationApplication::TangoConnect() {
        // Here, we'll connect to the TangoService and set up to run. Note that we're
        // passing in a pointer to ourselves as the context which will be passed back
        // in our callbacks.
        TangoErrorType ret = TangoService_connect(this, tango_config_);
        if (ret != TANGO_SUCCESS) {
            LOGE("SynchronizationApplication: Failed to connect to the Tango service.");
            std::exit(EXIT_SUCCESS);
        }

        // Get the intrinsics for the color camera and pass them on to the depth
        // image. We need these to know how to project the point cloud into the color
        // camera frame.
        TangoCameraIntrinsics color_camera_intrinsics;
        TangoErrorType err = TangoService_getCameraIntrinsics(
                TANGO_CAMERA_COLOR, &color_camera_intrinsics);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Failed to get the intrinsics for the color"
                            "camera.");
            std::exit(EXIT_SUCCESS);
        }

        err = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this,
                                                   OnFrameAvailableRouter);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: OnTangoServiceConnected,"
                            "Error connecting color frame %d",
                    err);
            std::exit(EXIT_SUCCESS);
        }

        if (image_buffer_manager_ == nullptr) {

            err = TangoSupport_createImageBufferManager(
                    TANGO_HAL_PIXEL_FORMAT_YCrCb_420_SP, color_camera_intrinsics.width,
                    color_camera_intrinsics.height, &image_buffer_manager_);
            if (err != TANGO_SUCCESS) {
                LOGE("PointToPointApplication: Failed to create image buffer manager");
                std::exit(EXIT_SUCCESS);
            }
        }

        TangoCameraIntrinsics depth_camera_intrinsics;
        err = TangoService_getCameraIntrinsics(
                TANGO_CAMERA_DEPTH, &depth_camera_intrinsics);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Failed to get the intrinsics for the depth"
                            "camera.");
            std::exit(EXIT_SUCCESS);
        }

        depth_image_.SetCameraIntrinsics(color_camera_intrinsics, depth_camera_intrinsics);
    }

    void SynchronizationApplication::OnPause() {
        is_gl_initialized_ = false;
        is_service_connected_ = false;
        TangoService_disconnect();
    }

    void SynchronizationApplication::OnSurfaceCreated() {
        depth_image_.InitializeGL();
        color_image_.InitializeGL();
        main_scene_.InitializeGL();
        is_gl_initialized_ = true;
    }

    void SynchronizationApplication::OnSurfaceChanged(int width, int height) {
        main_scene_.SetupViewPort(width, height);
    }

    void SynchronizationApplication::OnDrawFrame() {
        // If tracking is lost, further down in this method Scene::Render
        // will not be called. Prevent flickering that would otherwise
        // happen by rendering solid black as a fallback.
        main_scene_.Clear();

        if (!is_service_connected_ || !is_gl_initialized_) {
            return;
        }

        bool new_data = false;
        TangoImageBuffer *imageBuffer;
        if (TangoSupport_getLatestImageBufferAndNewDataFlag(
                image_buffer_manager_, &imageBuffer, &new_data)!=
            TANGO_SUCCESS){
            LOGE("SynchronizationApplication: Failed to get a new image.");
            return;
        }

        color_image_.UpdateColorImage(imageBuffer);

        double color_timestamp = 0.0;
        double depth_timestamp;
        bool new_points = false;
        TangoPointCloud *pointcloud_buffer;

        if (TangoSupport_getLatestPointCloudAndNewDataFlag(
                point_cloud_manager_, &pointcloud_buffer, &new_points) !=
            TANGO_SUCCESS) {
            LOGE("SynchronizationApplication: Failed to get a point cloud.");
            return;
        }

        depth_timestamp = pointcloud_buffer->timestamp;
        // We need to make sure that we update the texture associated with the color
        // image.
        if (TangoService_updateTextureExternalOes(
                TANGO_CAMERA_COLOR, color_image_.GetTextureId(), &color_timestamp) !=
            TANGO_SUCCESS) {
            LOGE("SynchronizationApplication: Failed to get a color image.");
            return;
        }

        // In the following code, we define t0 as the depth timestamp and t1 as the
        // color camera timestamp.

        // Calculate the relative pose between color camera frame at timestamp
        // color_timestamp t1 and depth camera frame at depth_timestamp t0.
        TangoPoseData pose_color_image_t1_T_depth_image_t0;
        if (TangoSupport_calculateRelativePose(
                color_timestamp, TANGO_COORDINATE_FRAME_CAMERA_COLOR, depth_timestamp,
                TANGO_COORDINATE_FRAME_CAMERA_DEPTH,
                &pose_color_image_t1_T_depth_image_t0) != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Could not find a valid relative pose at "
                            "time for color and "
                            " depth cameras.");
            return;
        }

        // The Color Camera frame at timestamp t0 with respect to Depth
        // Camera frame at timestamp t1.
        glm::mat4 color_image_t1_T_depth_image_t0 =
                util::GetMatrixFromPose(&pose_color_image_t1_T_depth_image_t0);

        depth_image_.UpdateAndUpsampleDepth(color_image_t1_T_depth_image_t0,
                                            pointcloud_buffer);

        main_scene_.Render(color_image_.GetTextureId(), depth_image_.GetTextureId(),
                           color_camera_to_display_rotation_);

        RecordManager();
    }

    void SynchronizationApplication::RecordManager(){
        if (_isRecording) {

            cv::Mat full(depth_image_.getFullDepthSize(), CV_8UC1, depth_image_.getFullDepthBuffer());
            cv::Mat small(depth_image_.getSmallDepthSize(), CV_8UC1, depth_image_.getSmallDepthBuffer());

            //_recordingBuffer.push_back(full);
            //_recordingBuffer.push_back(small);

            /*std::time_t result = std::time(nullptr);

            std::string location = "/storage/emulated/0/Android/data/imt.tangodepthmap/files/Download/Recording_4/";

            cv::imwrite(location + "full" + std::asctime(std::localtime(&result)) + ".png", full);
            cv::imwrite(location + "smol" + std::asctime(std::localtime(&result)) + ".png", small);*/
        }
            /*
            if we use bufferization
        else{
            if (!_recordingBuffer.empty()) {
                for (int i = 0; i < _recordingBuffer.size(); i++){
                    cv::imwrite("", _recordingBuffer[i]);
                }
                _recordingBuffer.clear();
            }
        }*/
    }

    void SynchronizationApplication::SetDepthAlphaValue(float alpha) {
        main_scene_.SetDepthAlphaValue(alpha);
    }

    void SynchronizationApplication::SetRenderingDistance(int renderingDistance) {
        depth_image_.SetRenderingDistance(renderingDistance);
    }

    void SynchronizationApplication::SetRecordingMode(bool isRecording, std::string path) {
        _isRecording = isRecording;
        _recordingPath = path;
    }

    void SynchronizationApplication::OnDisplayChanged(int display_rotation,
                                                      int color_camera_rotation) {
        color_camera_to_display_rotation_ =
                tango_gl::util::GetAndroidRotationFromColorCameraToDisplay(
                        display_rotation, color_camera_rotation);
    }

    // RAW TEXT VERSION
    /*void SynchronizationApplication::RecordManager(){
       if (_isRecording) {
           if (!_outputFile.is_open()) {
               _outputFile.open(_path);

               //__android_log_print(ANDROID_LOG_INFO, "Zbeul", "%s", _path.c_str());
               bufferOfBuffer.clear();
           }

           //bufferOfBuffer.push_back(depth_image_.getGrayscaleBuffer());

       } else {

           if (_outputFile.is_open()) {

               for (int i = 0; i < bufferOfBuffer.size(); i++){
                   std::copy(bufferOfBuffer[i].begin(), bufferOfBuffer[i].end(), std::ostream_iterator<uint8_t>(_outputFile, " "));
                   _outputFile << std::endl;
               }
               bufferOfBuffer.clear();
               _outputFile.close();
           }
       }
   }*/

}  // namespace tango_depth_map
