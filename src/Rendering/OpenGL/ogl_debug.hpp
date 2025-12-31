#pragma once

#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/debug.h>
#include <glbinding/Version.h>
#include <glbinding/glbinding.h>

#include <format>

#include "Utility/log.hpp"

namespace garnish::ogl_debug {

inline void initialize_error_checking() {
#ifndef NDEBUG
    glbinding::aux::enableGetErrorCallback();
#endif
}

inline void log_context_info() {
#ifndef NDEBUG
    const auto version = glbinding::aux::ContextInfo::version();
    const auto vendor = glbinding::aux::ContextInfo::vendor();
    const auto renderer = glbinding::aux::ContextInfo::renderer();

    log_debug(
        std::format(
            "[OpenGL] Version: {}\n"
            "[OpenGL] Vendor: {}\n"
            "[OpenGL] Renderer: {}",
            version.toString(),
            vendor,
            renderer
        )
    );
#endif
}

inline void initialize() {
    initialize_error_checking();
    log_context_info();
}
}  // namespace garnish::ogl_debug
