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

#include <opencv2/imgproc.hpp>
#include "tango_depth_map/color_image.h"

namespace tango_depth_map {

    ColorImage::ColorImage() :
            texture_id_(0) {}

    void ColorImage::InitializeGL() {
        glGenTextures(1, &texture_id_);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    }

    void ColorImage::UpdateColorImage(TangoImageBuffer *imageBuffer) {

        if(_colorImage.empty()){
            _colorImage.create(imageBuffer->height, imageBuffer->width, CV_8UC3);
            _colorImage.setTo(cv::Scalar(0, 0, 0));
        }
        if(_grayscaleImage.empty()){
            _grayscaleImage.create(imageBuffer->height, imageBuffer->width, CV_8UC1);
            _grayscaleImage.setTo(0);
        }

        //Format NV21 : 4:2:0 -> Luma in 1/1 image + Chroma in 1/2 image
        cv::Mat _yuv(imageBuffer->height + imageBuffer->height / 2, imageBuffer->width, CV_8UC1,
                     imageBuffer->data);
        _colorImage.setTo(cv::Scalar(0, 0, 0));
        cv::cvtColor(_yuv, _colorImage, CV_YUV2BGR_NV21);
        cv::cvtColor(_colorImage, _grayscaleImage, CV_BGR2GRAY);
    }

    ColorImage::~ColorImage() {
        _colorImage.release();
        _grayscaleImage.release();
    }
}  // namespace tango_depth_map
