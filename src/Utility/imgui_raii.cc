#include "Utility/imgui_raii.hpp"

#include <imgui.h>

namespace garnish {

ImGuiContext* ImGuiContextRAII::create_context() {
    IMGUI_CHECKVERSION();
    return ImGui::CreateContext();
}

}  // namespace garnish
