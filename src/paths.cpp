#include "include/VolkDMAOverlay/paths.hh"

std::filesystem::path volk::paths::shared() {
    return std::filesystem::path{ root };
}

std::filesystem::path volk::paths::app(std::string_view name) {
    return shared() / std::filesystem::path{ name }.filename();
}

std::filesystem::path volk::paths::configs(std::string_view name) {
    return app(name) / "Configs";
}

std::filesystem::path volk::paths::assets(std::string_view name) {
    return app(name) / "Assets";
}
