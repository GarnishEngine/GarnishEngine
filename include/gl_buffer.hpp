#pragma once
#include <iostream>
#include <vector>

#include "OpenGL.hpp"

namespace garnish {
template <typename T, int BUFFER_TYPE>
class GlBuffer {
   public:
    GlBuffer(
        const std::vector<T>& data,
        unsigned int bufferType = GL_STATIC_DRAW
    )
        : handle(0) {
        std::cerr << "init buff" << '\n';

        glGenBuffers(1, &handle);
        Bind();
        glBufferData(
            BUFFER_TYPE,
            data.size() * sizeof(T),
            (void*)data.data(),
            bufferType
        );
    }

    GlBuffer(const GlBuffer& other) = delete;
    GlBuffer(GlBuffer&& other) noexcept = default;
    GlBuffer& operator=(const GlBuffer& other) = delete;
    GlBuffer& operator=(GlBuffer&& other) noexcept = default;

    ~GlBuffer() { glDeleteBuffers(1, &handle); }

    void Bind() { glBindBuffer(BUFFER_TYPE, handle); }

   private:
    unsigned int handle;
};

using VertexBufferObject = GlBuffer<float, GL_ARRAY_BUFFER>;
using ElementBufferObject = GlBuffer<uint32_t, GL_ELEMENT_ARRAY_BUFFER>;
}  // namespace garnish