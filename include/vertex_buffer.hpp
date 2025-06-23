#pragma once
#include <OpenGL.hpp>
#include <cstdint>

namespace garnish {
class VertexBuffer {
   public:
    void init();
    void uploadData(OGLMesh vertexData);
    void bind();
    void unbind();
    void draw(GLuint mode, unsigned int start, unsigned int num);
    void cleanup();

   private:
    GLuint mVAO = 0;
    GLuint mVertexVBO = 0;
};
}  // namespace garnish