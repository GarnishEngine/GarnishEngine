#pragma once

#include <imgui.h>

namespace garnish {

class ImGuiContextRAII {
   public:
    ImGuiContextRAII()
        : ctx_(create_context()) {}

    ~ImGuiContextRAII() noexcept {
        if (ctx_) ImGui::DestroyContext(ctx_);
    }
    ImGuiContextRAII(const ImGuiContextRAII&) = delete;
    ImGuiContextRAII& operator=(const ImGuiContextRAII&) = delete;
    ImGuiContextRAII(ImGuiContextRAII&&) = delete;
    ImGuiContextRAII& operator=(ImGuiContextRAII&&) = delete;
    [[nodiscard]] ::ImGuiContext* get() const noexcept { return ctx_; }
    explicit operator bool() const noexcept { return ctx_ != nullptr; }

   private:
    static ::ImGuiContext* create_context();
    ::ImGuiContext* ctx_ = nullptr;
};

}  // namespace garnish
