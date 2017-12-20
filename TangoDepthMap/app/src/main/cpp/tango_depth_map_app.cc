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
#include <string>

#include <tango_depth_map/tango_depth_map_app.h>
#include <iterator>

int timestamp = 1;

template<typename T>
std::string to_string(T value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

namespace {
// The minimum Tango Core version required from this application.
    constexpr int kTangoCoreMinimumVersion = 9377;

    //Image buffer callback
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

    // Update the color image buffer
    void SynchronizationApplication::OnFrameAvailable(
            const TangoImageBuffer *buffer) {
        if(TangoSupport_updateImageBuffer(image_buffer_manager_, buffer) != TANGO_SUCCESS)
            LOGE("COULD NOT UPDATE IMG BUFFER");
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
              _isRecording(false),
              _isDetectingFaces(false),
              color_camera_to_display_rotation_(
                      TangoSupport_Rotation::TANGO_SUPPORT_ROTATION_0){}

    SynchronizationApplication::~SynchronizationApplication() {
        if (tango_config_) {
            TangoConfig_free(tango_config_);
        }
        TangoSupport_freePointCloudManager(point_cloud_manager_);
        point_cloud_manager_ = nullptr;

        TangoSupport_freeImageBufferManager(image_buffer_manager_);
        image_buffer_manager_ = nullptr;
    }

    void SynchronizationApplication::OnCreate(JNIEnv *env, jobject activity, JavaVM *javaVM,
                                              jobject context) {

        //Caching the jvm and context to call java functions from cpp later
        _javaVM = javaVM;
        _context = context;

        _classifierLoaded = faceDetector.load("/storage/emulated/0/Movies/lbpcascade_frontalface.xml");
        if(_classifierLoaded) LOGE("FACE DETECTOR LOADED");

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

        //Show color image first
        SetDepthAlphaValue(0.0);
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
        TangoErrorType err = TangoService_connect(this, tango_config_);
        if (err != TANGO_SUCCESS) {
            LOGE("SynchronizationApplication: Failed to connect to the Tango service.");
            std::exit(EXIT_SUCCESS);
        }

        // Get the intrinsics for the color camera and pass them on to the depth
        // image. We need these to know how to project the point cloud into the color
        // camera frame.
        TangoCameraIntrinsics color_camera_intrinsics;
        err = TangoService_getCameraIntrinsics(
                TANGO_CAMERA_COLOR, &color_camera_intrinsics);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: Failed to get the intrinsics for the color"
                            "camera.");
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

        err = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this,
                                                   OnFrameAvailableRouter);
        if (err != TANGO_SUCCESS) {
            LOGE(
                    "SynchronizationApplication: OnTangoServiceConnected,"
                            "Error connecting color frame %d",
                    err);
            std::exit(EXIT_SUCCESS);
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
        _isRecording = false;
        _isDetectingFaces = false;
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

        bool new_img = false;
        TangoImageBuffer *imageBuffer;
        if (TangoSupport_getLatestImageBufferAndNewDataFlag(
                image_buffer_manager_, &imageBuffer, &new_img) !=
            TANGO_SUCCESS) {
            LOGE("SynchronizationApplication: Failed to get a point cloud.");
            return;
        }

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

        color_image_.UpdateColorImage(imageBuffer);

        if (_isDetectingFaces){
            std::vector<cv::Rect> faces;
            //-- Detect faces
            faceDetector.detectMultiScale( color_image_._grayscaleImage, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(200, 200) );

            for( size_t i = 0; i < faces.size(); i++ )
            {
                cv::Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
                cv::ellipse(depth_image_._fullDepthImage, center, cv::Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, 255, 8, 8, 0 );
            }
        }

        // Binds the small and full buffer datas to opengl textures
        // Must be done only after you're done modifying the datas
        depth_image_.RenderingReady();
        main_scene_.Render(color_image_.GetTextureId(), depth_image_.GetTextureId(),
                           color_camera_to_display_rotation_);

        RecordManager();
    }

    void SynchronizationApplication::RecordManager() {
        if (_isRecording) {

            cv::Mat full = depth_image_._fullDepthImage.clone();
            cv::Mat small = depth_image_._smallDepthImage.clone();
            cv::Mat colorImg = color_image_._colorImage.clone();
            cv::Mat grayscaleImg = color_image_._grayscaleImage.clone();

            if (!full.empty()) _imagesBuffer.push_back(full);
            else _imagesBuffer.push_back(cv::Mat(10, 10, CV_8UC1, 0));

            if (!small.empty()) _imagesBuffer.push_back(small);
            else _imagesBuffer.push_back(cv::Mat(10, 10, CV_8UC1, 0));

            if (!colorImg.empty()) _imagesBuffer.push_back(colorImg);
            else _imagesBuffer.push_back(cv::Mat(10, 10, CV_8UC1, 0));

            if (!grayscaleImg.empty()) _imagesBuffer.push_back(grayscaleImg);
            else _imagesBuffer.push_back(cv::Mat(10, 10, CV_8UC1, 0));

        } else {
            if (!_imagesBuffer.empty()) {
                for (int i = 0; i < _imagesBuffer.size(); i += 4) {
                    std::string strTimestamp = formatTimestamp(timestamp);

                    //cv::imwrite(_recordingPath + "fullDepth_" +  strTimestamp + ".png", _imagesBuffer[i]);
                    //cv::imwrite(_recordingPath + "smallDepth_" + strTimestamp + ".png", _imagesBuffer[i + 1]);
                    cv::imwrite(_recordingPath + "colorImg_" +  strTimestamp + ".png", _imagesBuffer[i + 2]);
                    //cv::imwrite(_recordingPath + "grayscaleImg_" +  strTimestamp + ".png", _imagesBuffer[i + 3]);
                    timestamp++;
                }
                _imagesBuffer.clear();

                JNIEnv *env;
                _javaVM->AttachCurrentThread(&env, NULL);

                // access the class imt.tangodepthmap.TangoDepthMapActivity
                jclass cls = env->FindClass("imt/tangodepthmap/TangoDepthMapActivity");
                if (cls == nullptr) LOGE("Class not found");
                else {
                    // access the static method refreshDirectory
                    // (Landroid/content/Context;)V : 1 parameter of type Context, return Void
                    jmethodID methodId = env->GetStaticMethodID(cls, "refreshDirectory",
                                                                "(Landroid/content/Context;)V");
                    if (methodId == nullptr) LOGE("ERROR: method not found !");
                    else {
                        // call the static method with _context as a parameter
                        env->CallStaticVoidMethod(cls, methodId, _context);
                    }
                }
            }
        }
    }

    //Returns a XXXX timestamp
    std::string SynchronizationApplication::formatTimestamp(int timestamp){
        std::string strTimestamp = to_string(timestamp);
        int nbSize = (int)strTimestamp.length();
        std::string zeros(4 - nbSize, '0');
        return zeros + strTimestamp;
    }

    void SynchronizationApplication::SetDepthAlphaValue(float alpha) {
        main_scene_.SetDepthAlphaValue(alpha);
    }

    void SynchronizationApplication::SetRenderingDistance(int renderingDistance) {
        depth_image_.SetRenderingDistance(renderingDistance);
    }

    void SynchronizationApplication::SetRecordingMode(bool isRecording, std::string path) {

        if (isRecording) _recordingPath = path;
        _isRecording = isRecording;
        timestamp = 1;
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
