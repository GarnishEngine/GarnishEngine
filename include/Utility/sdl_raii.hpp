#pragma once

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <stb_image.h>

#include <memory>

namespace garnish {

struct SDLWindowDeleter {
    void operator()(SDL_Window* w) const noexcept {
        if (w) SDL_DestroyWindow(w);
    }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

class SDLWindowManager {
   public:
    SDLWindowManager(
        uint32_t initFlags,
        const char* title,
        int width,
        int height,
        SDL_WindowFlags windowFlags
    );
    ~SDLWindowManager() noexcept;
    SDLWindowManager(const SDLWindowManager&) = delete;
    SDLWindowManager& operator=(const SDLWindowManager&) = delete;
    SDLWindowManager(SDLWindowManager&&) = delete;
    SDLWindowManager& operator=(SDLWindowManager&&) = delete;

    [[nodiscard]] SDL_Window* get() const noexcept { return window_.get(); }
    [[nodiscard]] SDL_Window* operator->() const noexcept { return window_.get(); }
    explicit operator bool() const noexcept { return window_ != nullptr; }

   private:
    UniqueSDLWindow window_;
};

class GLContext {
   public:
    GLContext() = default;
    explicit GLContext(SDL_GLContext c) noexcept
        : ctx_(c) {}
    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;
    GLContext(GLContext&&) = delete;
    GLContext& operator=(GLContext&&) = delete;
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

struct STBImageDeleter {
    void operator()(stbi_uc* ptr) const noexcept {
        if (ptr) stbi_image_free(ptr);
    }
};

using UniqueSTBImage = std::unique_ptr<stbi_uc, STBImageDeleter>;

}  // namespace garnish
