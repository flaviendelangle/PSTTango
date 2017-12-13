#include "tango_depth_map/my_color_image.h"

#include "tango-gl/conversions.h"
#include "tango-gl/camera.h"
#include <fstream>
#include <iostream>

#include "tango_depth_map/depth_image.h"

namespace tango_depth_map {

    MyColorImage::MyColorImage()
            : texture_id_(0),
              cpu_texture_id_(0),
              color_display_buffer_(0),
              texture_render_program_(0),
              fbo_handle_(0),
              vertex_buffer_handle_(0),
              vertices_handle_(0),
              mvp_handle_(0),
              mIsRecording(false) {}

    MyColorImage::~MyColorImage() {}

    void MyColorImage::InitializeGL() {
        texture_id_ = 0;
        cpu_texture_id_ = 0;
        texture_render_program_ = 0;
        fbo_handle_ = 0;
        vertex_buffer_handle_ = 0;
        vertices_handle_ = 0;
        mvp_handle_ = 0;
    }

    bool MyColorImage::CreateOrBindCPUTexture() {
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
    void MyColorImage::UpdateColor(const TangoImageBuffer* buffer) {

        int color_image_width = rgb_camera_intrinsics_.width;
        int color_image_height = rgb_camera_intrinsics_.height;
        int color_image_size = color_image_width * color_image_height + color_image_width * color_image_height / 2;

        color_display_buffer_.resize(color_image_size);
        std::fill(color_display_buffer_.begin(), color_display_buffer_.end(), 0);
        memcpy(&color_display_buffer_[0], buffer->data, color_image_size);

        //OPENCV HERE
        /*cv::Mat _yuv(color_image_height + color_image_height/2, color_image_width, CV_8UC1, buffer->data);
            cv::Mat _bgr(yuv_height_, yuv_width_, CV_8UC3);
            cv::cvtColor(_yuv, _bgr, CV_YUV2BGR_NV21);*/
        //OPENCV HERE

        this->CreateOrBindCPUTexture();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, color_image_width, color_image_height,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        color_display_buffer_.data());
        tango_gl::util::CheckGlError("DepthImage glTexSubImage2D");
        glBindTexture(GL_TEXTURE_2D, 0);

        texture_id_ = cpu_texture_id_;
    }
}  // namespace tango_depth_map
