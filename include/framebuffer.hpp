#pragma once
#include <OpenGL.hpp>
#include <cstdint>

namespace garnish {
class Framebuffer {
   public:
    bool init(int width, int height);
    bool resize(int width, int height);
    void bind();
    void unbind();
    void drawToScreen();
    void cleanup();

   private:
    unsigned int mBufferWidth = 640;
    unsigned int mBufferHeight = 480;
    GLuint mBuffer = 0;
    GLuint mColorTex = 0;
    GLuint mDepthBuffer = 0;
    bool checkComplete() const;
};
}  // namespace garnish