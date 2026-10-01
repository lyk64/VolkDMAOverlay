#include "include/VolkDMAOverlay/menu.hh"

namespace volk::overlay {

static constexpr ImGuiKeyChord menu_toggle = ImGuiKey_Equal;

ScopedMenu Menu::begin(ImGuiWindowFlags flags) {
    if (ImGui::Shortcut(menu_toggle, ImGuiInputFlags_RouteGlobal))
        visible = !visible;

    if (visible)
        ImGui::SetNextWindowSize(default_size, ImGuiCond_FirstUseEver);

    return ScopedMenu{ title, &visible, flags };
}

} // namespace volk::overlay
