#include "include/VolkDMAOverlay/screen.hh"
#include <atomic>

namespace volk::overlay {

static std::atomic<ImVec2> current_screen_size{};
static_assert(std::atomic<ImVec2>::is_always_lock_free);

ImVec2 screen_size() noexcept {
    return current_screen_size.load();
}

ImVec2 screen_center() noexcept {
    const ImVec2 size = current_screen_size.load();
    return { size.x * 0.5f, size.y * 0.5f };
}

void detail::set_screen_size(float width, float height) noexcept {
    current_screen_size.store({ width, height });
}

} // namespace volk::overlay
