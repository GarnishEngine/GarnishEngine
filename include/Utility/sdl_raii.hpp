#pragma once

#include <SDL3/SDL_video.h>
#include <memory>

namespace garnish {

struct SDLWindowDeleter {
    void operator()(SDL_Window* w) const noexcept {
        if (w) SDL_DestroyWindow(w);
    }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

class GLContext {
public:
    GLContext() = default;
    explicit GLContext(SDL_GLContext c) noexcept : ctx_(c) {}
    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;
    GLContext(GLContext&& other) noexcept : ctx_(other.ctx_) { other.ctx_ = nullptr; }
    GLContext& operator=(GLContext&& other) noexcept {
        if (this != &other) {
            reset();
            ctx_ = other.ctx_;
            other.ctx_ = nullptr;
        }
        return *this;
    }
    ~GLContext() { reset(); }

    void reset(SDL_GLContext c = nullptr) noexcept {
        if (ctx_) {
            (void)SDL_GL_DestroyContext(ctx_);
        }
        ctx_ = c;
    }

    [[nodiscard]] SDL_GLContext get() const noexcept { return ctx_; }
    explicit operator bool() const noexcept { return ctx_ != nullptr; }

private:
    SDL_GLContext ctx_ = nullptr;
};

} // namespace garnish
