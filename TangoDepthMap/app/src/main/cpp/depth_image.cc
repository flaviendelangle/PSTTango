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

#include "tango-gl/conversions.h"
#include "tango-gl/camera.h"
#include <fstream>
#include <iostream>

#include "tango_depth_map/depth_image.h"

namespace tango_depth_map {

    DepthImage::DepthImage()
            : texture_id_(0),
              cpu_texture_id_(0),
              depth_value_buffer_(0),
              full_depth_grayscale_buffer_(0),
              small_depth_grayscale_buffer_(0),
              texture_render_program_(0),
              fbo_handle_(0),
              vertex_buffer_handle_(0),
              vertices_handle_(0),
              mvp_handle_(0),
              mRenderingDistance(1000){}

    DepthImage::~DepthImage() {}

    void DepthImage::InitializeGL() {
        texture_id_ = 0;
        cpu_texture_id_ = 0;
        texture_render_program_ = 0;
        fbo_handle_ = 0;
        vertex_buffer_handle_ = 0;
        vertices_handle_ = 0;
        mvp_handle_ = 0;
    }

    bool DepthImage::CreateOrBindCPUTexture() {
        if (cpu_texture_id_) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cpu_texture_id_);
            return false;
        } else {
            glGenTextures(1, &cpu_texture_id_);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cpu_texture_id_);
            glTexImage2D(GL_TEXTURE_2D, 0,  // mip-map level
                         GL_LUMINANCE, rgb_camera_intrinsics_.width,
                         rgb_camera_intrinsics_.height, 0,  // border
                         GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            return true;
        }
    }

// Update function will be called in application's main render loop. This funct-
// ion takes care of projecting raw depth points on the image plane, and render
// the depth image into a texture.
// This function also takes care of swapping the Render buffer and shared buffer
// if there is new point cloud data available.
// color_t1_T__depth_t0: 't1' represents the color camera frame's timestamp,
// 't0'
// represents
// the depth camera timestamp. The transformation is the depth camera's frame on
// timestamp t0 with respect the rgb camera's frame on timestamp t1.
    void DepthImage::UpdateAndUpsampleDepth(
            const glm::mat4 &color_t1_T_depth_t0,
            const TangoPointCloud *render_point_cloud_buffer) {

        int rgb_image_width = rgb_camera_intrinsics_.width;
        int rgb_image_height = rgb_camera_intrinsics_.height;
        int rgb_image_size = rgb_image_width * rgb_image_height;

        depth_value_buffer_.resize(rgb_image_size);
        std::fill(depth_value_buffer_.begin(), depth_value_buffer_.end(), 0);

        full_depth_grayscale_buffer_.resize(rgb_image_size);
        std::fill(full_depth_grayscale_buffer_.begin(), full_depth_grayscale_buffer_.end(), 0);

        int depth_image_width = depth_camera_intrinsics_.width;
        int depth_image_height = depth_camera_intrinsics_.height;
        int depth_image_size = depth_image_height * depth_image_width;

        small_depth_grayscale_buffer_.resize(depth_image_size);
        std::fill(small_depth_grayscale_buffer_.begin(), small_depth_grayscale_buffer_.end(), 0);

        int point_cloud_size = render_point_cloud_buffer->num_points;
        for (int i = 0; i < point_cloud_size; ++i) {
            float x = render_point_cloud_buffer->points[i][0];
            float y = render_point_cloud_buffer->points[i][1];
            float z = render_point_cloud_buffer->points[i][2];

            // depth_t0_point is the point in depth camera frame on timestamp t0.
            // (depth image timestamp).
            glm::vec4 depth_t0_point = glm::vec4(x, y, z, 1.0);

            // color_t1_point is the point in camera frame on timestamp t1.
            // (color image timestamp).
            glm::vec4 color_t1_point = color_t1_T_depth_t0 * depth_t0_point;

            int pixel_x, pixel_y;
            // get the coordinate on image plane.
            pixel_x = static_cast<int>((rgb_camera_intrinsics_.fx) *
                                       (color_t1_point.x / color_t1_point.z) +
                                       rgb_camera_intrinsics_.cx);

            pixel_y = static_cast<int>((rgb_camera_intrinsics_.fy) *
                                       (color_t1_point.y / color_t1_point.z) +
                                       rgb_camera_intrinsics_.cy);

            // Color value is the GL_LUMINANCE value used for displaying the depth
            // image.
            // We can query for depth value in mm from grayscale image buffer by
            // getting a `pixel_value` at (pixel_x,pixel_y) and calculating
            // pixel_value * (UCHAR_MAX / distance)
            uint8_t grayscale_value = UCHAR_MAX - (color_t1_point.z * 1000) * UCHAR_MAX / mRenderingDistance;

            UpSampleDepthAroundPoint(grayscale_value, color_t1_point.z, pixel_x, pixel_y,
                                     &full_depth_grayscale_buffer_, &depth_value_buffer_);

            //DEPTHMAP
            //Same as above but now using depth camera intrinsecs to avoing upsampling
            int depth_x, depth_y;

            depth_x = static_cast<int>((depth_camera_intrinsics_.fx) *
                                       (color_t1_point.x / color_t1_point.z) +
                                       depth_camera_intrinsics_.cx);

            depth_y = static_cast<int>((depth_camera_intrinsics_.fy) *
                                       (color_t1_point.y / color_t1_point.z) +
                                       depth_camera_intrinsics_.cy);

            int pixel_num = depth_x + depth_y * depth_image_width;
            if (pixel_num > 0 && pixel_num < depth_image_size)
                small_depth_grayscale_buffer_[pixel_num] = grayscale_value;
            //DEPTHMAP
        }

        /*int start = clock();
        int end = clock();
        LOGE("Execution time");
        __android_log_print(ANDROID_LOG_INFO, "Zbeul", "%f", (end-start)/double(CLOCKS_PER_SEC)*1000);*/

        this->CreateOrBindCPUTexture();

        if (true) { //Display depthmap calculated from rgb camera intrinsecs
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_image_width, depth_image_height,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE,
                            full_depth_grayscale_buffer_.data());
            tango_gl::util::CheckGlError("DepthImage glTexSubImage2D");
            glBindTexture(GL_TEXTURE_2D, 0);

        } else { //Display depthmap calculated from depth camera intrinsecs
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_image_width, depth_image_height,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE,
                            small_depth_grayscale_buffer_.data());
            tango_gl::util::CheckGlError("DepthImage glTexSubImage2D");
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        texture_id_ = cpu_texture_id_;
    }

    void DepthImage::UpSampleDepthAroundPoint(
            uint8_t grayscale_value, float depth_value, int pixel_x, int pixel_y,
            std::vector<uint8_t> *grayscale_buffer,
            std::vector<float> *depth_map_buffer) {

        int image_width = rgb_camera_intrinsics_.width;
        int image_height = rgb_camera_intrinsics_.height;
        int image_size = image_height * image_width;
        // Set the neighbour pixels to same color.
        for (int a = -kWindowSize; a <= kWindowSize; ++a) {
            for (int b = -kWindowSize; b <= kWindowSize; ++b) {
                if (pixel_x > image_width || pixel_y > image_height || pixel_x < 0 ||
                    pixel_y < 0) {
                    continue;
                }
                int pixel_num = (pixel_x + a) + (pixel_y + b) * image_width;

                if (pixel_num > 0 && pixel_num < image_size) {
                    (*grayscale_buffer)[pixel_num] = grayscale_value;
                    (*depth_map_buffer)[pixel_num] = depth_value;
                }
            }
        }
    }

}  // namespace tango_depth_map
