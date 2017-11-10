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

#ifndef TANGODEPTHMAP_UTIL_H_
#define TANGODEPTHMAP_UTIL_H_

#include "tango_client_api.h"  // NOLINT
#include "tango-gl/util.h"

namespace tango_depth_map {
// Utility functioins for Synchronization application.
    namespace util {
// Returns a transformation matrix from a given TangoPoseData structure.
// - pose_data: The original pose is used for the conversion.
        glm::mat4 GetMatrixFromPose(const TangoPoseData *pose_data);
    }  // namespace util
}  // namespace tango_depth_map

#endif  // TANGODEPTHMAP_UTIL_H_
