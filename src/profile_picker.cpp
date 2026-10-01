#include "include/VolkDMAOverlay/profile_picker.hh"
#include "include/VolkDMAOverlay/overlay.hh"
#include "include/VolkDMAOverlay/profiles.hh"
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

bool volk::config::profile_picker(const ProfileStore& store, std::string& active,
                                 const ProfileAction& save, const ProfileAction& load) {
    static char name_buffer[128] = "config";
    static std::vector<std::string> names;
    static int selected = 0;
    static bool scanned = false;

    const auto select = [&](std::string_view name) {
        const auto it = std::ranges::find(names, name);
        if (it != names.end())
            selected = static_cast<int>(std::distance(names.begin(), it));
    };

    if (!scanned) {
        scanned = true;
        names = store.list();

        if (!active.empty()) {
            std::snprintf(name_buffer, sizeof(name_buffer), "%s", active.c_str());
            select(active);
        }
    }

    if (selected < 0 || selected >= static_cast<int>(names.size()))
        selected = 0;

    bool changed = false;

    ImGui::TextUnformatted("Save profile:");
    ImGui::SetNextItemWidth(250.0f);
    ImGui::InputText("##profile_name", name_buffer, IM_ARRAYSIZE(name_buffer));
    ImGui::SameLine();
    if (ImGui::Button("Save") && save(name_buffer)) {
        active = store.path_for(name_buffer).stem().string();
        names = store.list();
        select(active);
        changed = true;
    }

    ImGui::TextUnformatted("Load profile:");
    ImGui::SetNextItemWidth(250.0f);
    if (ImGui::BeginCombo("##profile_list", names.empty() ? "No profiles found" : names[selected].c_str())) {
        for (auto&& [i, name] : std::views::enumerate(names)) {
            const bool is_selected = (selected == i);
            if (ImGui::Selectable(name.c_str(), is_selected))
                selected = static_cast<int>(i);

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Load") && !names.empty() && load(names[selected])) {
        active = names[selected];
        changed = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        names = store.list();
        select(active);
    }

    return changed;
}

void volk::config::add_profile_popup(overlay::Overlay& overlay, const ProfileStore& store, std::string& active,
                                    ProfileAction save, ProfileAction load) {
    overlay.add_status_bar_popup("Configs",
        [&store, &active, save = std::move(save), load = std::move(load)] {
            profile_picker(store, active, save, load);
        });
}
