#include "include/VolkDMAOverlay/overlay.hh"
#include "include/VolkDMAOverlay/monitor.hh"
#include "include/VolkDMAOverlay/monitor_picker.hh"
#include <VolkLog/log.hh>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <string_view>

static constexpr Volk::Log::Logger logger{ "OVERLAY" };
static constexpr ImGuiKeyChord overlay_exit = ImGuiMod_Shift | ImGuiKey_Equal;
static constexpr ImGuiKeyChord status_bar_toggle = ImGuiKey_Minus;
static constexpr const char* hint_text = "= menu | - status bar | Shift + = exit";
static constexpr const char* settings_type = "VolkDMAOverlay";
static constexpr const char* settings_entry = "Settings";
static constexpr const char* settings_popup = "##overlay_settings";

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
    const UINT width = GetSystemMetrics(SM_CXSCREEN);
    const UINT height = GetSystemMetrics(SM_CYSCREEN);
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

    ImGuiSettingsHandler handler{};
    handler.TypeName = settings_type;
    handler.TypeHash = ImHashStr(settings_type);
    handler.ReadOpenFn = settings_read_open;
    handler.ReadLineFn = settings_read_line;
    handler.WriteAllFn = settings_write_all;
    handler.UserData = this;
    ImGui::AddSettingsHandler(&handler);

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

static std::optional<std::string_view> setting_value(std::string_view line, std::string_view key) {
    if (!line.starts_with(key) || line.size() <= key.size() || line[key.size()] != '=')
        return std::nullopt;

    return line.substr(key.size() + 1);
}

void* Overlay::settings_read_open(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) {
    return std::string_view{ name } == settings_entry ? handler->UserData : nullptr;
}

void Overlay::settings_read_line(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
    Overlay& overlay = *static_cast<Overlay*>(entry);

    if (auto value = setting_value(line, "VSync")) {
        overlay.settings.vsync = *value == "1";
        return;
    }

    if (auto value = setting_value(line, "Monitor"))
        overlay.settings.monitor_path = *value;
}

void Overlay::settings_write_all(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
    const Overlay& overlay = *static_cast<const Overlay*>(handler->UserData);

    buf->appendf("[%s][%s]\n", handler->TypeName, settings_entry);
    buf->appendf("VSync=%d\n", overlay.settings.vsync ? 1 : 0);
    if (!overlay.settings.monitor_path.empty())
        buf->appendf("Monitor=%s\n", overlay.settings.monitor_path.c_str());
    buf->append("\n");
}

void Overlay::apply_settings() {
    move_to_monitor(settings.monitor_path);
    saved_settings = settings;
}

void Overlay::save_settings_if_changed() {
    if (settings == saved_settings)
        return;

    saved_settings = settings;
    ImGui::MarkIniSettingsDirty();
}

ScopedFrame Overlay::next_frame(std::stop_token stop) {
    return ScopedFrame{ *this, !stop.stop_requested() && window.pump_messages() };
}

void Overlay::begin_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (!settings_applied) {
        settings_applied = true;
        apply_settings();
    }

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

    if (ImGui::Button("Settings"))
        ImGui::OpenPopup(settings_popup);

    const float dropdown_right = ImGui::GetItemRectMax().x;

    ImGui::SameLine();

    if (ImGui::Button("Exit"))
        request_close();

    const float dropdown_top = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

    ImGui::SetNextWindowPos({ dropdown_right, dropdown_top }, ImGuiCond_Always, { 1.0f, 0.0f });
    if (ImGui::BeginPopup(settings_popup)) {
        ImGui::Checkbox("VSync", &settings.vsync);
        monitor_picker("Monitor", *this);
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Overlay::end_frame() {
    if (show_status_bar)
        draw_status_bar();

    save_settings_if_changed();

    ImGui::Render();
    dx11.set_vsync(settings.vsync);
    dx11.clear_and_set_target();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    dx11.present();
}

void Overlay::shutdown() {
    if (!initialized) return;
    logger.info("Shutting down");
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
        settings.monitor_path = it->device_path;
}

void Overlay::move_to_monitor(const std::string& device_path) {
    if (device_path.empty())
        return;

    auto monitors = list_monitors();
    auto it = std::ranges::find(monitors, device_path, &MonitorInfo::device_path);
    if (it == monitors.end())
        return;

    window.move_to_monitor(it->handle);
    settings.monitor_path = it->device_path;
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