#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <imgui.h>

namespace volk::overlay::widgets {

namespace detail {

template <typename T>
concept name_like = std::constructible_from<std::string_view, const T&>;

struct NameOf {
    template <typename T>
    constexpr decltype(auto) operator()(const T& item) const noexcept {
        if constexpr (name_like<T>)
            return (item);
        else
            return (item.name);
    }
};

template <name_like T>
[[nodiscard]] std::string label(const T& name) {
    if constexpr (std::is_pointer_v<T>) {
        if (!name)
            return {};
    }
    return std::string{ std::string_view{ name } };
}

inline constexpr std::uint8_t escape_key = 0x1B;

struct KeyBindFrame {
    bool listening;
    ImVec2 anchor_min;
    ImVec2 anchor_max;
};

KeyBindFrame key_bind_begin(const char* label, const char* preview, bool can_listen);
bool key_pressed(int code, bool held);
void key_poll_end();
void key_bind_stop_listening();
void key_bind_end();
bool key_list_begin(const KeyBindFrame& frame);
bool key_list_item(const char* name, bool selected);
void key_list_end();

} // namespace detail

template <typename Key, typename Code>
concept key_entry = requires(const Key& key, Code& code) {
    { key.code == code } -> std::convertible_to<bool>;
    code = key.code;
    requires detail::name_like<decltype(key.name)>;
};

struct SectionOptions {
    bool* enabled = nullptr;
    bool default_open = false;
};

class [[nodiscard]] ScopedSection {
public:
    ScopedSection(ScopedSection&& other) noexcept
        : open{ std::exchange(other.open, false) }, disabled{ std::exchange(other.disabled, false) } {}
    ScopedSection& operator=(ScopedSection&&) = delete;
    ~ScopedSection() {
        if (disabled) ImGui::EndDisabled();
        if (open) ImGui::TreePop();
    }

    explicit operator bool() const noexcept { return open; }

private:
    friend ScopedSection section(const char* label, SectionOptions options);
    ScopedSection(bool open, bool disabled) : open{ open }, disabled{ disabled } {}

    bool open;
    bool disabled;
};

struct ToggleOptions {
    ImVec4* color = nullptr;
    int* slider = nullptr;
    int slider_min = 0;
    int slider_max = 100;
    const char* slider_format = "%d";
};

ScopedSection section(const char* label, SectionOptions options = {});
bool toggle(const char* label, bool& enabled, ToggleOptions options = {});
bool color(const char* label, ImVec4& value);
bool slider(const char* label, int& value, int min, int max, const char* format = "%d");
bool slider(const char* label, float& value, float min, float max, const char* format = "%.2f");
void align_right(float width);

template <std::equality_comparable T>
bool radio(const char* label, T& value, std::type_identity_t<T> option) {
    if (!ImGui::RadioButton(label, value == option))
        return false;
    value = option;
    return true;
}

template <typename R, typename Proj = detail::NameOf>
    requires std::ranges::random_access_range<const R> && std::ranges::sized_range<const R> &&
             detail::name_like<std::remove_cvref_t<std::invoke_result_t<Proj&, std::ranges::range_reference_t<const R>>>>
bool combo(const char* label, int& index, const R& items, Proj name = {}) {
    const int count = static_cast<int>(std::ranges::size(items));
    if (count == 0) {
        ImGui::BeginDisabled();
        if (ImGui::BeginCombo(label, "None"))
            ImGui::EndCombo();
        ImGui::EndDisabled();
        return false;
    }

    if (index < 0 || index >= count)
        index = 0;

    bool changed = false;
    const auto first = std::ranges::begin(items);
    if (ImGui::BeginCombo(label, detail::label(std::invoke(name, first[index])).c_str())) {
        for (int i = 0; i < count; ++i) {
            const bool selected = (i == index);
            ImGui::PushID(i);
            if (ImGui::Selectable(detail::label(std::invoke(name, first[i])).c_str(), selected) && !selected) {
                index = i;
                changed = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return changed;
}

template <typename R, typename T, typename IsHeld = std::nullptr_t>
    requires std::ranges::forward_range<const R> && key_entry<std::ranges::range_value_t<const R>, T> &&
             (std::is_null_pointer_v<IsHeld> || std::predicate<IsHeld&, T>)
bool key_bind(const char* label, T& code, const R& keys, IsHeld is_held = nullptr) {
    constexpr bool can_listen = !std::is_null_pointer_v<IsHeld>;

    const auto current = std::ranges::find_if(keys, [&](const auto& key) { return key.code == code; });
    const std::string preview = current == std::ranges::end(keys) ? "Unknown" : detail::label(current->name);

    bool changed = false;
    const detail::KeyBindFrame frame = detail::key_bind_begin(label, preview.c_str(), can_listen);

    if constexpr (can_listen) {
        if (frame.listening) {
            std::optional<T> pressed;
            for (const auto& key : keys) {
                const T key_code = static_cast<T>(key.code);
                if (detail::key_pressed(static_cast<int>(key.code), std::invoke(is_held, key_code)) && !pressed)
                    pressed = key_code;
            }
            detail::key_poll_end();

            if (pressed) {
                if (*pressed != static_cast<T>(detail::escape_key) && *pressed != code) {
                    code = *pressed;
                    changed = true;
                }
                detail::key_bind_stop_listening();
            }
        }
    }

    if (detail::key_list_begin(frame)) {
        for (const auto& key : keys) {
            ImGui::PushID(static_cast<int>(key.code));
            if (detail::key_list_item(detail::label(key.name).c_str(), key.code == code) && key.code != code) {
                code = key.code;
                changed = true;
            }
            ImGui::PopID();
        }
        detail::key_list_end();
    }

    detail::key_bind_end();
    return changed;
}

} // namespace volk::overlay::widgets
