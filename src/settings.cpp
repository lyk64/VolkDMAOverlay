#include "include/VolkDMAOverlay/settings.hh"
#include <VolkLog/log.hh>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static constexpr volk::log::Logger logger{ "SETTINGS" };

static_assert(volk::config::value("Key=1", "Key") == "1");
static_assert(volk::config::value("Key=", "Key") == "");
static_assert(!volk::config::value("KeySize=1", "Key"), "key match must not be a prefix match");
static_assert(!volk::config::value("Key", "Key"));
static_assert(!volk::config::value("Other=1", "Key"));

static_assert(!volk::config::detail::Composite<ImVec4>, "ImVec4 must stay a FloatTuple, not a Composite");
static_assert(!volk::config::detail::Composite<std::string>, "std::string must stay Text, not a Composite");

struct volk::config::detail::Section {
    std::string type_name;
    std::string document;
    ReadLine read;
    WriteAll write;
    Applied applied;
    detail::GroupTracker groups;
    ImGuiID hash{};
    bool hashed{};
};

namespace {
    using volk::config::detail::Section;

    constexpr const char* entry_name = "Settings";
    constexpr float poll_interval = 1.0f;

    [[nodiscard]] std::vector<std::string_view> split_path(std::string_view path) {
        std::vector<std::string_view> parts;

        while (!path.empty()) {
            const auto slash = path.find('/');
            parts.push_back(path.substr(0, slash));

            if (slash == std::string_view::npos)
                break;

            path.remove_prefix(slash + 1);
        }

        return parts;
    }

    [[nodiscard]] std::optional<std::string> read_file(const std::filesystem::path& path) {
        std::ifstream in{ path, std::ios::binary };
        if (!in)
            return std::nullopt;

        return std::string{ std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
    }

    void write_file(const std::filesystem::path& path, std::string_view text) {
        try {
            std::filesystem::create_directories(path.parent_path());
        }
        catch (const std::filesystem::filesystem_error& e) {
            logger.warn("couldn't create settings folder {}: {}", path.parent_path().string(), e.what());
            return;
        }

        std::ofstream out{ path, std::ios::binary };
        if (!out) {
            logger.warn("couldn't write settings: {}", path.string());
            return;
        }

        out.write(text.data(), static_cast<std::streamsize>(text.size()));
    }

    [[nodiscard]] bool refresh_hash(Section& section, ImGuiTextBuffer& scratch) {
        scratch.resize(0);

        volk::config::DocumentWriter out{ scratch };
        section.write(out);

        const ImGuiID hash = ImHashData(scratch.c_str(), static_cast<size_t>(scratch.size()));
        const bool changed = section.hashed && hash != section.hash;

        section.hash = hash;
        section.hashed = true;
        return changed;
    }

    void* section_read_open(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) {
        if (std::string_view{ name } != entry_name)
            return nullptr;

        auto* section = static_cast<Section*>(handler->UserData);
        section->groups.reset();
        return section;
    }

    void section_read_line(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
        Section& section = *static_cast<Section*>(entry);

        const std::string_view raw{ line };
        const auto trimmed = volk::config::detail::trim(raw);
        if (trimmed.empty())
            return;

        if (!section.groups.feed(raw, trimmed))
            section.read(section.groups.path(), trimmed);
    }

    void section_apply_all(ImGuiContext*, ImGuiSettingsHandler* handler) {
        Section& section = *static_cast<Section*>(handler->UserData);
        if (section.applied)
            section.applied();
    }

    void section_write_all(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        const Section& section = *static_cast<const Section*>(handler->UserData);

        buf->appendf("[%s][%s]\n", handler->TypeName, entry_name);

        volk::config::DocumentWriter out{ *buf };
        section.write(out);

        buf->append("\n");
    }
} // namespace

std::string_view volk::config::detail::trim(std::string_view text) {
    constexpr std::string_view blank = " \t\r";

    const auto first = text.find_first_not_of(blank);
    if (first == std::string_view::npos)
        return {};

    return text.substr(first, text.find_last_not_of(blank) - first + 1);
}

void volk::config::DocumentWriter::group(std::string_view path) {
    const auto next = split_path(path);
    const auto current = split_path(open);

    size_t shared = 0;
    while (shared < next.size() && shared < current.size() && next[shared] == current[shared])
        ++shared;

    depth = static_cast<int>(shared);

    for (size_t i = shared; i < next.size(); ++i) {
        indent();
        buf.appendf("%.*s:\n", static_cast<int>(next[i].size()), next[i].data());
        ++depth;
    }

    open = path;
}

void volk::config::DocumentWriter::indent() {
    for (int i = 0; i < depth; ++i)
        buf.append("    ");
}

void volk::config::DocumentWriter::key(std::string_view name) {
    indent();
    buf.appendf("%.*s=", static_cast<int>(name.size()), name.data());
}

bool volk::config::detail::GroupTracker::feed(std::string_view raw, std::string_view trimmed) {
    constexpr std::string_view blank = " \t";

    const auto marker = group_marker(trimmed);
    if (!marker)
        return false;

    const auto columns = raw.find_first_not_of(blank);

    while (!stack.empty() && stack.back().columns >= columns)
        stack.pop_back();

    stack.push_back({ columns, std::string{ *marker } });

    joined.clear();
    for (const Level& level : stack) {
        if (!joined.empty())
            joined += '/';

        joined += level.name;
    }

    return true;
}

void volk::config::detail::GroupTracker::reset() {
    stack.clear();
    joined.clear();
}

std::optional<std::string_view> volk::config::detail::group_marker(std::string_view line) {
    if (!line.ends_with(':') || line.contains('='))
        return std::nullopt;

    return line.substr(0, line.size() - 1);
}

bool volk::config::detail::parse_floats(std::string_view text, std::span<float> out) {
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

void volk::config::detail::write_floats(ImGuiTextBuffer& buf, std::span<const float> values) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            buf.append(",");

        buf.appendf("%g", static_cast<double>(values[i]));
    }
}

volk::config::Registry::Registry() = default;

volk::config::Registry::~Registry() {
    if (!ImGui::GetCurrentContext())
        return;

    for (const auto& section : sections)
        if (section->document.empty())
            ImGui::RemoveSettingsHandler(section->type_name.c_str());
}

void volk::config::Registry::add(std::string_view type_name, ReadLine read, WriteAll write, Applied applied) {
    assert(ImGui::GetCurrentContext() && "no ImGui context; register after the overlay is initialised");
    assert(read && write && "settings section needs both callbacks");

    const bool registered = std::ranges::any_of(sections,
        [type_name](const auto& section) { return section->type_name == type_name; });

    assert(!registered && "settings section registered twice");

    if (registered)
        return;

    const auto& section = sections.emplace_back(std::make_unique<detail::Section>(
        std::string{ type_name }, std::string{}, std::move(read), std::move(write), std::move(applied)));

    ImGuiSettingsHandler handler{};
    handler.TypeName = section->type_name.c_str();
    handler.TypeHash = ImHashStr(section->type_name.c_str());
    handler.ReadOpenFn = section_read_open;
    handler.ReadLineFn = section_read_line;
    handler.WriteAllFn = section_write_all;
    handler.ApplyAllFn = section_apply_all;
    handler.UserData = section.get();
    ImGui::AddSettingsHandler(&handler);
}

void volk::config::Registry::add_document(std::string_view path, ReadLine read, WriteAll write, Applied applied) {
    assert(read && write && "settings document needs both callbacks");
    assert(!path.empty() && "settings document needs a path");

    const bool registered = std::ranges::any_of(sections,
        [path](const auto& section) { return section->document == path; });

    assert(!registered && "settings document registered twice");

    if (registered)
        return;

    const auto& section = sections.emplace_back(std::make_unique<detail::Section>(
        std::string{}, std::string{ path }, std::move(read), std::move(write), std::move(applied)));

    if (const auto text = read_file(section->document))
        detail::parse_lines(*text, section->groups, [&](std::string_view group, std::string_view line) {
            section->read(group, line);
        });

    if (section->applied)
        section->applied();
}

void volk::config::Registry::poll() {
    elapsed += ImGui::GetIO().DeltaTime;
    if (elapsed < poll_interval)
        return;

    elapsed = 0.0f;

    for (const auto& section : sections) {
        if (!refresh_hash(*section, scratch))
            continue;

        if (section->document.empty())
            ImGui::MarkIniSettingsDirty();
        else
            write_file(section->document, { scratch.c_str(), static_cast<size_t>(scratch.size()) });
    }
}

void volk::config::Registry::flush() {
    for (const auto& section : sections) {
        if (section->document.empty())
            continue;

        if (refresh_hash(*section, scratch))
            write_file(section->document, { scratch.c_str(), static_cast<size_t>(scratch.size()) });
    }
}
