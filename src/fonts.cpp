#include "fonts.hh"
#include <VolkLog/log.hh>
#include <imgui.h>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <windows.h>
#include <shlobj.h>

namespace volk::overlay::detail {

#include "noto_sans.inc"

static constexpr volk::log::Logger logger{ "OVERLAY" };

static std::filesystem::path windows_fonts_dir() {
    PWSTR raw = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &raw);
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> path{ raw, &CoTaskMemFree };
    return SUCCEEDED(result) ? std::filesystem::path{ path.get() } : std::filesystem::path{};
}

static std::string utf8(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return { text.begin(), text.end() };
}

void load_fonts() {
    constexpr float size_pixels = 18.0f;
    constexpr std::array cjk_fallbacks = { "msyh.ttc", "msjh.ttc", "YuGothR.ttc", "malgun.ttf" };

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config{};
    config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryCompressedTTF(noto_sans_compressed_data, static_cast<int>(noto_sans_compressed_size), size_pixels, &config);

    const std::filesystem::path fonts_dir = windows_fonts_dir();
    if (fonts_dir.empty()) {
        logger.warn("Windows fonts folder not found; CJK text will not render");
        return;
    }

    config.MergeMode = true;
    for (const char* name : cjk_fallbacks) {
        const std::filesystem::path path = fonts_dir / name;
        std::error_code error;
        if (!std::filesystem::exists(path, error)) {
            logger.warn("Fallback font not found: {}", utf8(path));
            continue;
        }
        io.Fonts->AddFontFromFileTTF(utf8(path).c_str(), size_pixels, &config);
    }
}

} // namespace volk::overlay::detail
