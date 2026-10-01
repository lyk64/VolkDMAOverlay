#pragma once

#include <filesystem>
#include <string_view>

namespace volk::paths {
    inline constexpr std::string_view root = "C:\\Volk";
    inline constexpr std::string_view config_extension = ".volk";

    [[nodiscard]] std::filesystem::path shared();
    [[nodiscard]] std::filesystem::path app(std::string_view name);
    [[nodiscard]] std::filesystem::path configs(std::string_view name);
    [[nodiscard]] std::filesystem::path assets(std::string_view name);
}
