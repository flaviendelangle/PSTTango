//
// Created by Hellsinger on 13/12/2017.
//

#ifndef TANGODEPTHMAP_MY_COLOR_IMAGE_H
#define TANGODEPTHMAP_MY_COLOR_IMAGE_H

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
    class MyColorImage {
    public:
        MyColorImage();
        ~MyColorImage();

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
        void UpdateColor(const TangoImageBuffer* buffer);

        // Returns the depth texture id.
        GLuint GetTextureId() const { return texture_id_; }

        // Set camera's intrinsics.
        // The intrinsics are used to project the pointcloud to depth image and
        // and undistort the image to the right size.
        void SetCameraIntrinsics(TangoCameraIntrinsics intrinsics) { rgb_camera_intrinsics_ = intrinsics; }

        void SetRecordingMode(bool isRecording, std::string path) { mIsRecording = isRecording; recPath = path; }

    private:

        // Initialize the OpenGL structures needed for the CPU texture generation.
        // Returns true if the texture was created and false if an existing texture
        // was bound.
        bool CreateOrBindCPUTexture();

        bool mIsRecording;

        std::string recPath;

        // The depth texture id. This is used for other rendering class to
        // render, in this class, we only write value to this texture via
        // CPU or offscreen framebuffer rendering.  This should point either
        // to the cpu_texture_id_ or gpu_texture_id_ value and should not be
        // deleted separately.
        GLuint texture_id_;
        // The backing texture for CPU texture generation.
        GLuint cpu_texture_id_;

        // Color map buffer is for the texture render purpose, this value is written
        // to the texture id buffer, and display as GL_LUMINANCE value.
        std::vector<uint8_t> color_display_buffer_;

        // The camera intrinsics of current device. Note that the color camera and
        // depth camera are the same hardware on the device.
        TangoCameraIntrinsics rgb_camera_intrinsics_;

        // OpenGL handles for render to texture
        GLuint texture_render_program_;
        GLuint fbo_handle_;
        GLuint vertex_buffer_handle_;
        GLuint vertices_handle_;
        GLuint mvp_handle_;

    };
}  // namespace tango_depth_map

#endif //TANGODEPTHMAP_MY_COLOR_IMAGE_H
