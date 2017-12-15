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

#ifndef TANGODEPTHMAP_DEPTH_IMAGE_H_
#define TANGODEPTHMAP_DEPTH_IMAGE_H_

#include <tango_client_api.h>
#include <tango-gl/util.h>
#include <thread>
#include <mutex>
#include <vector>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

namespace tango_depth_map {

// DepthImage is a class which projects point cloud on to a color camera's
// image plane.
    class DepthImage {
    public:
        DepthImage();

        ~DepthImage();

        // Invalid the GL data structures.
        void InitializeGL();

        // Update the depth texture with current transformation and current depth map.
        // @param  color_t1_T_depth_t0: The transformation between the color camera
        //    frame on timestamp i (color camera timestamp) and the depth camera
        //    frame on timestamp j (depth camera timestamp).
        // To convert a point in the depth camera frame on timestamp t0 to the color
        // camera frame
        // on timestamp t1, you could do:
        //    color_t1_point = color_t1_T_depth_t0 * depth_t0_point;
        //
        // @param render_point_cloud_buffer: This contains the latest point cloud data
        // that gets projected on to the image plane and fills up the depth_map_buffer
        void UpdateAndUpsampleDepth(const glm::mat4 &color_t1_T_depth_t0,
                                    const TangoPointCloud *render_point_cloud_buffer);

        // Returns the depth texture id.
        GLuint GetTextureId() const { return texture_id_; }

        // Set camera's intrinsics.
        // The intrinsics are used to project the pointcloud to depth image and
        // and undistort the image to the right size.
        void SetCameraIntrinsics(TangoCameraIntrinsics color_intrinsics, TangoCameraIntrinsics depth_intrinsics)
        { rgb_camera_intrinsics_ = color_intrinsics; depth_camera_intrinsics_ = depth_intrinsics; }

        void SetRenderingDistance(int renderingDistance) { mRenderingDistance = renderingDistance; }

        uchar* getFullDepthBuffer() { return full_depth_grayscale_buffer_.data(); }
        uchar* getSmallDepthBuffer() { return small_depth_grayscale_buffer_.data(); }

        cv::Size getFullDepthSize() { return cv::Size(rgb_camera_intrinsics_.width, rgb_camera_intrinsics_.height); }
        cv::Size getSmallDepthSize() { return cv::Size(depth_camera_intrinsics_.width, depth_camera_intrinsics_.height); }

    private:

        // Initialize the OpenGL structures needed for the CPU texture generation.
        // Returns true if the texture was created and false if an existing texture
        // was bound.
        bool CreateOrBindCPUTexture();

        // This function takes care of upsampling depth around a given point by
        // setting the same value in a bounding box. Note:This is a very rudimentary
        // approach to upsampling depth.
        //
        // @param color_value: The value that needs to set for all points in a
        // bounding box.
        //
        // @param pixel_x: The pixel along x axis in the grayscale_buffer around which
        // depth needs to be upsampled.
        //
        // @param pixel_y: The pixel along y axis in the grayscale_buffer around which
        // depth needs to be upsampled.
        //
        // @param grayscale_buffer: The image buffer in which upsampling needs to be
        // done.
        void UpSampleDepthAroundPoint(uint8_t color_value, float depth_value,
                                      int pixel_x, int pixel_y,
                                      std::vector<uint8_t> *grayscale_buffer,
                                      std::vector<float> *depth_map_buffer);

        //Window used for upsampling depth
        int kWindowSize = 4;

        //Distance interval corresponding to a 255 - 0 interval
        int mRenderingDistance;

        // The depth texture id. This is used for other rendering class to
        // render, in this class, we only write value to this texture via
        // CPU or offscreen framebuffer rendering.  This should point either
        // to the cpu_texture_id_ or gpu_texture_id_ value and should not be
        // deleted separately.
        GLuint texture_id_;
        // The backing texture for CPU texture generation.
        GLuint cpu_texture_id_;

        std::vector<float> depth_value_buffer_;

        // Color map buffer is for the texture render purpose, this value is written
        // to the texture id buffer, and display as GL_LUMINANCE value.
        std::vector<uint8_t> full_depth_grayscale_buffer_;

        std::vector<uint8_t> small_depth_grayscale_buffer_;

        // The camera intrinsics of current device.
        TangoCameraIntrinsics rgb_camera_intrinsics_;
        TangoCameraIntrinsics depth_camera_intrinsics_;

        // OpenGL handles for render to texture
        GLuint texture_render_program_;
        GLuint fbo_handle_;
        GLuint vertex_buffer_handle_;
        GLuint vertices_handle_;
        GLuint mvp_handle_;

    };
}  // namespace tango_depth_map

#endif  // TANGODEPTHMAP_DEPTH_IMAGE_H_
