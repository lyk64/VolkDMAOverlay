#pragma once

#include "settings.hh"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace IniSettings {
    class ProfileStore {
    public:
        ProfileStore(std::filesystem::path directory, std::string extension);

        [[nodiscard]] std::vector<std::string> list() const;

        [[nodiscard]] std::filesystem::path path_for(std::string_view name) const;

        template <typename Owner, typename... Groups>
        bool save(std::string_view name, const Owner& owner, const Groups&... groups) const {
            ImGuiTextBuffer buf;
            write_document(buf, owner, groups...);

            return write_file(name, { buf.c_str(), static_cast<size_t>(buf.size()) });
        }

        template <typename Owner, typename... Groups>
        bool load(std::string_view name, Owner& owner, const Groups&... groups) const {
            const auto text = read_file(name);
            if (!text)
                return false;

            read_document(*text, owner, groups...);
            return true;
        }

    private:
        [[nodiscard]] std::optional<std::string> read_file(std::string_view name) const;

        bool write_file(std::string_view name, std::string_view text) const;

        std::filesystem::path directory;
        std::string extension;
    };
}
