#include "include/VolkDMAOverlay/settings.hh"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cassert>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

static_assert(IniSettings::value("Key=1", "Key") == "1");
static_assert(IniSettings::value("Key=", "Key") == "");
static_assert(!IniSettings::value("KeySize=1", "Key"), "key match must not be a prefix match");
static_assert(!IniSettings::value("Key", "Key"));
static_assert(!IniSettings::value("Other=1", "Key"));

struct IniSettings::detail::Section {
    std::string type_name;
    ReadLine read;
    WriteAll write;
    std::string current_group;
    ImGuiID hash{};
    bool hashed{};
};

namespace {
    using IniSettings::detail::Section;

    constexpr const char* entry_name = "Settings";
    constexpr float poll_interval = 1.0f;

    [[nodiscard]] std::string_view trim(std::string_view text) {
        const auto first = text.find_first_not_of(" \t");
        if (first == std::string_view::npos)
            return {};

        return text.substr(first, text.find_last_not_of(" \t") - first + 1);
    }

    [[nodiscard]] std::optional<std::string_view> group_marker(std::string_view line) {
        if (!line.ends_with(':') || line.contains('='))
            return std::nullopt;

        return line.substr(0, line.size() - 1);
    }

    void* section_read_open(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) {
        if (std::string_view{ name } != entry_name)
            return nullptr;

        auto* section = static_cast<Section*>(handler->UserData);
        section->current_group.clear();
        return section;
    }

    void section_read_line(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
        Section& section = *static_cast<Section*>(entry);

        const auto trimmed = trim(line);
        if (trimmed.empty())
            return;

        if (const auto marker = group_marker(trimmed)) {
            section.current_group = *marker;
            return;
        }

        section.read(section.current_group, trimmed);
    }

    void section_write_all(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        const Section& section = *static_cast<const Section*>(handler->UserData);

        buf->appendf("[%s][%s]\n", handler->TypeName, entry_name);

        IniSettings::Writer out{ *buf };
        section.write(out);

        buf->append("\n");
    }
}

bool IniSettings::detail::parse_floats(std::string_view text, std::span<float> out) {
    for (size_t i = 0; i < out.size(); ++i) {
        const auto separator = text.find(',');
        const bool last = i + 1 == out.size();

        if ((separator == std::string_view::npos) != last)
            return false;

        const auto parsed = parse<float>(text.substr(0, separator));
        if (!parsed)
            return false;

        out[i] = *parsed;

        if (!last)
            text.remove_prefix(separator + 1);
    }

    return true;
}

void IniSettings::detail::write_floats(ImGuiTextBuffer& buf, std::span<const float> values) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            buf.append(",");

        buf.appendf("%g", static_cast<double>(values[i]));
    }
}

IniSettings::Registry::Registry() = default;

IniSettings::Registry::~Registry() {
    if (!ImGui::GetCurrentContext())
        return;

    for (const auto& section : sections)
        ImGui::RemoveSettingsHandler(section->type_name.c_str());
}

void IniSettings::Registry::add(std::string_view type_name, ReadLine read, WriteAll write) {
    assert(ImGui::GetCurrentContext() && "no ImGui context; register after the overlay is initialised");
    assert(read && write && "settings section needs both callbacks");

    const bool registered = std::ranges::any_of(sections,
        [type_name](const auto& section) { return section->type_name == type_name; });

    assert(!registered && "settings section registered twice");

    if (registered)
        return;

    const auto& section = sections.emplace_back(std::make_unique<detail::Section>(
        std::string{ type_name }, std::move(read), std::move(write)));

    ImGuiSettingsHandler handler{};
    handler.TypeName = section->type_name.c_str();
    handler.TypeHash = ImHashStr(section->type_name.c_str());
    handler.ReadOpenFn = section_read_open;
    handler.ReadLineFn = section_read_line;
    handler.WriteAllFn = section_write_all;
    handler.UserData = section.get();
    ImGui::AddSettingsHandler(&handler);
}

void IniSettings::Registry::poll() {
    elapsed += ImGui::GetIO().DeltaTime;
    if (elapsed < poll_interval)
        return;

    elapsed = 0.0f;

    for (const auto& section : sections) {
        scratch.resize(0);

        Writer out{ scratch };
        section->write(out);

        const ImGuiID hash = ImHashData(scratch.c_str(), static_cast<size_t>(scratch.size()));
        if (section->hashed && hash != section->hash)
            ImGui::MarkIniSettingsDirty();

        section->hash = hash;
        section->hashed = true;
    }
}
