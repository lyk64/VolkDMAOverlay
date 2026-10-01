#include "include/VolkDMAOverlay/monitor_picker.hh"
#include "include/VolkDMAOverlay/monitor.hh"
#include "include/VolkDMAOverlay/overlay.hh"
#include <imgui.h>
#include <algorithm>
#include <chrono>
#include <vector>

namespace volk::overlay {

namespace {
    constexpr auto refresh_interval = std::chrono::seconds(1);

    const std::vector<MonitorInfo>& cached_monitors() {
        static std::vector<MonitorInfo> monitors;
        static std::chrono::steady_clock::time_point next_refresh{};

        auto now = std::chrono::steady_clock::now();
        if (now >= next_refresh) {
            monitors = list_monitors();
            next_refresh = now + refresh_interval;
        }

        return monitors;
    }
} // namespace

bool monitor_picker(const char* label, Overlay& overlay) {
    const auto& monitors = cached_monitors();
    const HMONITOR current = overlay.current_monitor();

    auto current_it = std::ranges::find(monitors, current, &MonitorInfo::handle);
    std::string preview = current_it != monitors.end()
        ? monitor_label(*current_it, static_cast<size_t>(current_it - monitors.begin()))
        : "Unknown";

    if (!ImGui::BeginCombo(label, preview.c_str()))
        return false;

    bool changed = false;
    for (size_t i = 0; i < monitors.size(); ++i) {
        const MonitorInfo& monitor = monitors[i];
        const bool selected = monitor.handle == current;

        if (ImGui::Selectable(monitor_label(monitor, i).c_str(), selected)) {
            overlay.move_to_monitor(monitor.handle);
            changed = true;
        }

        if (selected)
            ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
    return changed;
}

} // namespace volk::overlay
