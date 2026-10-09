#pragma once

#include <imgui.h>

namespace volk::overlay {

[[nodiscard]] ImVec2 screen_size() noexcept;
[[nodiscard]] ImVec2 screen_center() noexcept;

namespace detail {

void set_screen_size(float width, float height) noexcept;

} // namespace detail

} // namespace volk::overlay
