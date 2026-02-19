#include "include/VolkDMAOverlay/overlay.hh"
#include <VolkLog/log.hh>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <filesystem>

static constexpr Volk::Log::Logger logger{ "OVERLAY" };

static void apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = 1.0;
    style.ChildRounding = 3;
    style.WindowRounding = 3;
    style.GrabRounding = 1;
    style.GrabMinSize = 20;
    style.FrameRounding = 3;

    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.13f, 0.13f, 0.90f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.10f, 0.09f, 0.51f);
}

bool Overlay::init(const wchar_t* title, Win32::ResizeCallback on_resize) {
    ImGui_ImplWin32_EnableDpiAwareness();
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    window.on_resize = on_resize;

    if (!window.init(title, width, height, dx11)) {
        logger.error("Win32 init failed");
        return false;
    }

    if (!dx11.init(window.hwnd)) {
        logger.error("DX11 init failed");
        dx11.cleanup();
        window.cleanup();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    load_fonts();
    ImGui_ImplWin32_Init(window.hwnd);
    ImGui_ImplDX11_Init(dx11.device.Get(), dx11.device_context.Get());
    initialized = true;

    apply_theme();

    if (on_resize)
        on_resize(width, height);

    logger.info("Initialized ({}x{})", width, height);
    return true;
}

void Overlay::begin_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Overlay::end_frame(bool vsync) {
    ImGui::Render();
    dx11.clear_and_set_target();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    dx11.present(vsync);
}

void Overlay::shutdown() {
    if (!initialized) return;
    logger.info("Shutting down");
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

static std::filesystem::path get_fonts_dir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path() / "fonts";
}

void Overlay::load_fonts() {
    constexpr float size_pixels = 18.0f;
    auto fonts_dir = get_fonts_dir();

    logger.debug("Loading fonts from: {}", fonts_dir.string());

    ImFontConfig config{};
    config.PixelSnapH = true;

    ImGuiIO& io = ImGui::GetIO();

    auto load = [&](const char* name, const ImWchar* ranges) {
        auto path = (fonts_dir / name).string();
        if (!io.Fonts->AddFontFromFileTTF(path.c_str(), size_pixels, &config, ranges))
            logger.warn("Failed to load font: {}", name);
    };

    load("NotoSans-Regular.ttf", io.Fonts->GetGlyphRangesDefault());
    config.MergeMode = true;
    load("NotoSansSC-Regular.ttf", io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    load("NotoSansTC-Regular.ttf", io.Fonts->GetGlyphRangesChineseFull());
    load("NotoSansJP-Regular.ttf", io.Fonts->GetGlyphRangesJapanese());
    load("NotoSansKR-Regular.ttf", io.Fonts->GetGlyphRangesKorean());
    load("NotoSans-Regular.ttf", io.Fonts->GetGlyphRangesCyrillic());
}