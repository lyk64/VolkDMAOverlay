#include "include/VolkDMAOverlay/overlay.hh"
#include "include/VolkDMAOverlay/monitor.hh"
#include "include/VolkDMAOverlay/monitor_picker.hh"
#include "include/VolkDMAOverlay/paths.hh"
#include "include/VolkDMAOverlay/settings.hh"
#include <VolkLog/log.hh>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

static constexpr Volk::Log::Logger logger{ "OVERLAY" };
static constexpr ImGuiKeyChord overlay_exit = ImGuiMod_Shift | ImGuiKey_Equal;
static constexpr ImGuiKeyChord status_bar_toggle = ImGuiKey_Minus;
static constexpr const char* hint_text = "= menu | - status bar | Shift + = exit";
static constexpr const char* settings_popup = "##overlay_settings";
static constexpr const char* shared_settings_file = "overlay.ini";
static constexpr const char* app_settings_file = "imgui.ini";

static std::wstring widen(std::string_view text) {
    if (text.empty())
        return {};

    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring out(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), size);
    return out;
}

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

bool Overlay::init(std::string_view name, Win32::ResizeCallback on_resize) {
    app_name = name;

    ImGui_ImplWin32_EnableDpiAwareness();
    const UINT width = GetSystemMetrics(SM_CXSCREEN);
    const UINT height = GetSystemMetrics(SM_CYSCREEN);
    window.on_resize = on_resize;

    if (!window.init(widen(app_name), width, height, dx11)) {
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

    const auto app_dir = Volk::Paths::app(app_name);

    try {
        std::filesystem::create_directories(app_dir);
    }
    catch (const std::filesystem::filesystem_error& e) {
        logger.warn("couldn't create app folder {}: {}", app_dir.string(), e.what());
    }

    ini_path = (app_dir / app_settings_file).string();
    io.IniFilename = ini_path.c_str();

    ini_registry.add_document((Volk::Paths::shared() / shared_settings_file).string(),
        [this](std::string_view group, std::string_view line) { read_setting(group, line); },
        [this](IniSettings::DocumentWriter& out) { write_settings(out); },
        [this] { move_to_monitor(settings.display.monitor_path); });

    load_fonts();
    ImGui_ImplWin32_Init(window.hwnd);
    ImGui_ImplDX11_Init(dx11.device.Get(), dx11.device_context.Get());
    initialized = true;

    apply_theme();

    const auto [client_width, client_height] = window.client_size();

    if (on_resize)
        on_resize(client_width, client_height);

    logger.info("Initialized ({}x{})", client_width, client_height);
    return true;
}

void Overlay::add_settings(IniSettings::ReadLine read, IniSettings::WriteAll write, IniSettings::Applied applied) {
    assert(!app_name.empty() && "add_settings before init");

    ini_registry.add(app_name, std::move(read), std::move(write), std::move(applied));
}

void Overlay::add_status_bar_popup(std::string_view label, StatusBarPopup draw) {
    status_items.emplace_back(std::string{ label }, "##statusbar_" + std::string{ label }, std::move(draw));
}

void Overlay::read_setting(std::string_view group, std::string_view line) {
    IniSettings::read_grouped(group, line, settings, display_group);
}

void Overlay::write_settings(IniSettings::DocumentWriter& out) const {
    IniSettings::write_grouped(out, settings, display_group);
}

ScopedFrame Overlay::next_frame(std::stop_token stop) {
    return ScopedFrame{ *this, !stop.stop_requested() && window.pump_messages() };
}

void Overlay::begin_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Shortcut(status_bar_toggle, ImGuiInputFlags_RouteGlobal))
        show_status_bar = !show_status_bar;

    if (ImGui::Shortcut(overlay_exit, ImGuiInputFlags_RouteGlobal))
        request_close();
}

void Overlay::draw_status_bar() {
    constexpr float margin = 10.0f;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        { viewport->WorkPos.x + viewport->WorkSize.x - margin, viewport->WorkPos.y + margin },
        ImGuiCond_Always, { 1.0f, 0.0f });
    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("##status", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextUnformatted(hint_text);
    ImGui::SameLine();
    ImGui::Text("| FPS: %.1f |", ImGui::GetIO().Framerate);
    ImGui::SameLine();

    for (StatusItem& item : status_items) {
        if (ImGui::Button(item.label.c_str()))
            ImGui::OpenPopup(item.popup_id.c_str());

        item.right = ImGui::GetItemRectMax().x;
        ImGui::SameLine();
    }

    if (ImGui::Button("Settings"))
        ImGui::OpenPopup(settings_popup);

    const float settings_right = ImGui::GetItemRectMax().x;

    ImGui::SameLine();

    if (ImGui::Button("Exit"))
        request_close();

    const float dropdown_top = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

    for (StatusItem& item : status_items) {
        ImGui::SetNextWindowPos({ item.right, dropdown_top }, ImGuiCond_Always, { 1.0f, 0.0f });
        if (ImGui::BeginPopup(item.popup_id.c_str())) {
            item.draw();
            ImGui::EndPopup();
        }
    }

    ImGui::SetNextWindowPos({ settings_right, dropdown_top }, ImGuiCond_Always, { 1.0f, 0.0f });
    if (ImGui::BeginPopup(settings_popup)) {
        ImGui::Checkbox("VSync", &settings.display.vsync);
        monitor_picker("Monitor", *this);
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Overlay::end_frame() {
    if (show_status_bar)
        draw_status_bar();

    ini_registry.poll();

    ImGui::Render();
    dx11.set_vsync(settings.display.vsync);
    dx11.clear_and_set_target();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    dx11.present();
}

void Overlay::shutdown() {
    if (!initialized) return;
    logger.info("Shutting down");
    ini_registry.flush();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

void Overlay::move_to_monitor(HMONITOR monitor) {
    window.move_to_monitor(monitor);

    auto monitors = list_monitors();
    auto it = std::ranges::find(monitors, monitor, &MonitorInfo::handle);
    if (it != monitors.end())
        settings.display.monitor_path = it->device_path;
}

void Overlay::move_to_monitor(const std::string& device_path) {
    if (device_path.empty())
        return;

    auto monitors = list_monitors();
    auto it = std::ranges::find(monitors, device_path, &MonitorInfo::device_path);
    if (it == monitors.end())
        return;

    window.move_to_monitor(it->handle);
    settings.display.monitor_path = it->device_path;
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
