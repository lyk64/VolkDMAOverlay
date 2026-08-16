#pragma once

#include "dx11.hh"
#include "settings.hh"
#include "win32.hh"

#include <functional>
#include <stop_token>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

class ScopedFrame;

struct Overlay {
    ~Overlay() { shutdown(); }

    DX11 dx11;
    Win32 window;

    bool init(std::string_view app_name, Win32::ResizeCallback on_resize = nullptr);
    [[nodiscard]] ScopedFrame next_frame(std::stop_token stop = {});
    void shutdown();

    void request_close() { PostQuitMessage(0); }

    [[nodiscard]] IniSettings::Registry& ini() noexcept { return ini_registry; }

    void add_settings(IniSettings::ReadLine read, IniSettings::WriteAll write, IniSettings::Applied applied = {});

    using StatusBarPopup = std::move_only_function<void()>;
    void add_status_bar_popup(std::string_view label, StatusBarPopup draw);

    [[nodiscard]] HMONITOR current_monitor() const { return window.current_monitor; }
    void move_to_monitor(HMONITOR monitor);
    void move_to_monitor(const std::string& device_path);

private:
    friend class ScopedFrame;

    struct Settings {
        struct Display {
            bool vsync = false;
            std::string monitor_path;
        } display;
    };

    static constexpr auto display_group = IniSettings::Group{ "Display", &Settings::display, std::tuple{
        IniSettings::Field{ "VSync", &Settings::Display::vsync },
        IniSettings::Field{ "Monitor", &Settings::Display::monitor_path },
    } };

    struct StatusItem {
        std::string label;
        std::string popup_id;
        StatusBarPopup draw;
        float right{};
    };

    IniSettings::Registry ini_registry;
    Settings settings;
    std::string app_name;
    std::string ini_path;
    std::vector<StatusItem> status_items;

    bool show_status_bar = true;
    bool initialized = false;

    void begin_frame();
    void end_frame();
    void load_fonts();
    void draw_status_bar();

    void read_setting(std::string_view group, std::string_view line);
    void write_settings(IniSettings::DocumentWriter& out) const;
};

class ScopedFrame {
public:
    ScopedFrame(const ScopedFrame&) = delete;
    ScopedFrame& operator=(const ScopedFrame&) = delete;
    ~ScopedFrame() { if (active) overlay.end_frame(); }

    explicit operator bool() const noexcept { return active; }

private:
    friend struct Overlay;
    ScopedFrame(Overlay& owner, bool active) : overlay{ owner }, active{ active } {
        if (active)
            overlay.begin_frame();
    }

    Overlay& overlay;
    bool active;
};
