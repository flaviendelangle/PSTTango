//
// Created by Hellsinger on 07/11/2017.
//

#ifndef DEPTHMAP_YUVDRAWABLE_H
#define DEPTHMAP_YUVDRAWABLE_H

#include "tango-gl/drawable_object.h"

namespace dmap {
    class YUVDrawable : public tango_gl::DrawableObject {
    public:
        YUVDrawable();
        void Render(const glm::mat4& projection_mat, const glm::mat4& view_mat) const;
        GLuint GetTextureId() const { return texture_id_; }
        void SetTextureId(GLuint texture_id) { texture_id_ = texture_id; }

    private:
        // This id is populated on construction, and is passed to the tango service.
        GLuint texture_id_;

        GLuint attrib_texture_coords_;
        GLuint uniform_texture_;
        GLuint vertex_buffers_[3];
    };
}  // namespace dmap

#endif //DEPTHMAP_YUVDRAWABLE_H
