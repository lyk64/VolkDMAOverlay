#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace volk::overlay {
    struct Overlay;
}

namespace volk::config {
    class ProfileStore;

    using ProfileAction = std::move_only_function<bool(std::string_view name) const>;

    bool profile_picker(const ProfileStore& store, std::string& active,
                        const ProfileAction& save, const ProfileAction& load);

    void add_profile_popup(overlay::Overlay& overlay, const ProfileStore& store, std::string& active,
                           ProfileAction save, ProfileAction load);
} // namespace volk::config
