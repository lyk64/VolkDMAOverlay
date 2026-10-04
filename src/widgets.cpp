#include "include/VolkDMAOverlay/widgets.hh"
#include <cfloat>
#include <cstring>
#include <format>
#include <string>

namespace volk::overlay::widgets {

static constexpr float trailing_slider_width = 130.0f;
static constexpr ImGuiSliderFlags slider_flags = ImGuiSliderFlags_AlwaysClamp;

static bool swatch(ImVec4& value) {
    return ImGui::ColorEdit4("##Color", &value.x, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
}

ScopedSection section(const char* label, SectionOptions options) {
    if (options.enabled) {
        ImGui::PushID(label);
        ImGui::Checkbox("##enabled", options.enabled);
        ImGui::PopID();
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
    if (options.default_open)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool open = ImGui::TreeNodeEx(label, flags);
    const bool disabled = open && options.enabled && !*options.enabled;
    if (disabled)
        ImGui::BeginDisabled();
    return ScopedSection{ open, disabled };
}

bool toggle(const char* label, bool& enabled, ToggleOptions options) {
    ImGui::PushID(label);
    bool changed = ImGui::Checkbox(label, &enabled);

    float width = 0.0f;
    if (options.color)
        width += ImGui::GetFrameHeight();
    if (options.slider)
        width += (options.color ? ImGui::GetStyle().ItemSpacing.x : 0.0f) + trailing_slider_width;

    if (width > 0.0f)
        align_right(width);

    if (options.color)
        changed |= swatch(*options.color);

    if (options.slider) {
        if (options.color)
            ImGui::SameLine();
        ImGui::SetNextItemWidth(trailing_slider_width);
        changed |= ImGui::SliderInt("##Slider", options.slider, options.slider_min, options.slider_max, options.slider_format, slider_flags);
    }

    ImGui::PopID();
    return changed;
}

bool color(const char* label, ImVec4& value) {
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label, std::strstr(label, "##"));
    align_right(ImGui::GetFrameHeight());
    const bool changed = swatch(value);
    ImGui::PopID();
    return changed;
}

bool slider(const char* label, int& value, int min, int max, const char* format) {
    return ImGui::SliderInt(label, &value, min, max, format, slider_flags);
}

bool slider(const char* label, float& value, float min, float max, const char* format) {
    return ImGui::SliderFloat(label, &value, min, max, format, slider_flags);
}

void align_right(float width) {
    ImGui::SameLine();
    const float available = ImGui::GetContentRegionAvail().x;
    if (available > width)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available - width);
}

namespace detail {

static constexpr const char* key_list_popup = "##key_list";
static constexpr float key_list_rows = 12.0f;
static ImGuiTextFilter key_filter;

KeyBindFrame key_bind_begin(const char* label, const char* preview, bool can_listen) {
    ImGui::PushID(label);

    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID listening_id = ImGui::GetID("##listening");
    bool listening = can_listen && storage->GetBool(listening_id);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float width = ImGui::CalcItemWidth();
    const float arrow_width = can_listen ? ImGui::GetFrameHeight() : 0.0f;
    const std::string text = std::format("{}###key", listening ? "Press a key... (Esc to cancel)" : preview);

    ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_FrameBg]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_FrameBgHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_FrameBgActive]);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.0f, 0.5f });

    if (ImGui::Button(text.c_str(), { std::max(1.0f, width - arrow_width), 0.0f })) {
        if (can_listen)
            listening = !listening;
        else
            ImGui::OpenPopup(key_list_popup);
    }
    const ImVec2 popup_pos{ ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y };

    if (can_listen) {
        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::ArrowButton("##open", ImGuiDir_Down)) {
            listening = false;
            ImGui::OpenPopup(key_list_popup);
        }
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
    ImGui::TextUnformatted(label, std::strstr(label, "##"));

    storage->SetBool(listening_id, listening);
    return { .listening = listening, .popup_pos = popup_pos, .popup_width = width };
}

void key_bind_stop_listening() {
    ImGui::GetStateStorage()->SetBool(ImGui::GetID("##listening"), false);
}

void key_bind_end() {
    ImGui::PopID();
}

bool key_list_begin(const KeyBindFrame& frame) {
    ImGui::SetNextWindowPos(frame.popup_pos);
    ImGui::SetNextWindowSizeConstraints({ frame.popup_width, 0.0f }, { FLT_MAX, FLT_MAX });
    if (!ImGui::BeginPopup(key_list_popup))
        return false;

    if (ImGui::IsWindowAppearing()) {
        key_filter.Clear();
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint("##search", "Search", key_filter.InputBuf, IM_ARRAYSIZE(key_filter.InputBuf)))
        key_filter.Build();

    ImGui::BeginChild("##keys", { 0.0f, ImGui::GetTextLineHeightWithSpacing() * key_list_rows });
    return true;
}

bool key_list_item(const char* name, bool selected) {
    if (!key_filter.PassFilter(name))
        return false;

    const bool clicked = ImGui::Selectable(name, selected, ImGuiSelectableFlags_NoAutoClosePopups);
    if (selected && ImGui::IsWindowAppearing())
        ImGui::SetScrollHereY();
    if (clicked)
        ImGui::CloseCurrentPopup();
    return clicked;
}

void key_list_end() {
    ImGui::EndChild();
    ImGui::EndPopup();
}

} // namespace detail

} // namespace volk::overlay::widgets
