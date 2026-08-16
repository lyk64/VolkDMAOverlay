#include "include/VolkDMAOverlay/paths.hh"

std::filesystem::path Volk::Paths::shared() {
    return std::filesystem::path{ root };
}

std::filesystem::path Volk::Paths::app(std::string_view name) {
    return shared() / std::filesystem::path{ name }.filename();
}

std::filesystem::path Volk::Paths::configs(std::string_view name) {
    return app(name) / "Configs";
}

std::filesystem::path Volk::Paths::assets(std::string_view name) {
    return app(name) / "Assets";
}
