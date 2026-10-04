#include "include/VolkDMAOverlay/widgets.hh"
#include <bitset>
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

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap;
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

static ImGuiID active_listener = 0;
static int bound_frame = -1;
static bool capture_baseline = false;
static std::bitset<256> held_before;

KeyBindFrame key_bind_begin(const char* label, const char* preview, bool can_listen) {
    ImGui::PushID(label);

    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID id = ImGui::GetID("##key_bind");
    const ImGuiID last_frame_id = ImGui::GetID("##last_frame");
    const int frame = ImGui::GetFrameCount();
    if (active_listener == id && storage->GetInt(last_frame_id, -1) != frame - 1)
        active_listener = 0;
    storage->SetInt(last_frame_id, frame);
    bool listening = can_listen && active_listener == id;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float width = ImGui::CalcItemWidth();
    const float arrow_width = can_listen ? ImGui::GetFrameHeight() : 0.0f;
    const std::string text = std::format("{}###key", listening ? "Press a key... (Esc to cancel)" : preview);

    ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_FrameBg]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_FrameBgHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_FrameBgActive]);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.0f, 0.5f });

    if (ImGui::Button(text.c_str(), { std::max(1.0f, width - arrow_width), 0.0f })) {
        if (!can_listen) {
            ImGui::OpenPopup(key_list_popup);
        }
        else if (!listening && bound_frame < 0) {
            active_listener = id;
            capture_baseline = true;
            listening = true;
        }
    }
    const ImVec2 anchor_min = ImGui::GetItemRectMin();
    const ImVec2 anchor_max{ anchor_min.x + width, ImGui::GetItemRectMax().y };

    if (can_listen) {
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginDisabled(listening);
        if (ImGui::ArrowButton("##open", ImGuiDir_Down))
            ImGui::OpenPopup(key_list_popup);
        ImGui::EndDisabled();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
    ImGui::TextUnformatted(label, std::strstr(label, "##"));

    if (bound_frame >= 0 && frame > bound_frame && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        bound_frame = -1;

    return { .listening = listening, .anchor_min = anchor_min, .anchor_max = anchor_max };
}

bool key_pressed(int code, bool held) {
    if (code < 0 || code >= static_cast<int>(held_before.size()))
        return false;
    const bool pressed = held && !held_before[code] && !capture_baseline;
    held_before[code] = held;
    return pressed;
}

void key_poll_end() {
    capture_baseline = false;
}

void key_bind_stop_listening() {
    active_listener = 0;
    bound_frame = ImGui::GetFrameCount();
}

void key_bind_end() {
    ImGui::PopID();
}

bool key_list_begin(const KeyBindFrame& frame) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = style.WindowPadding.y * 2.0f + ImGui::GetFrameHeightWithSpacing() + ImGui::GetTextLineHeightWithSpacing() * key_list_rows;
    const float room_below = viewport->WorkPos.y + viewport->WorkSize.y - frame.anchor_max.y;
    const float room_above = frame.anchor_min.y - viewport->WorkPos.y;
    const bool above = room_below < height && room_above > room_below;

    ImGui::SetNextWindowPos({ frame.anchor_min.x, above ? frame.anchor_min.y : frame.anchor_max.y }, ImGuiCond_Always, { 0.0f, above ? 1.0f : 0.0f });
    ImGui::SetNextWindowSizeConstraints({ frame.anchor_max.x - frame.anchor_min.x, 0.0f }, { FLT_MAX, FLT_MAX });
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
