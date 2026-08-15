#pragma once

#include "dx11.hh"
#include "settings.hh"
#include "win32.hh"

#include <stop_token>
#include <string>
#include <string_view>
#include <tuple>

class ScopedFrame;

struct Overlay {
    ~Overlay() { shutdown(); }

    DX11 dx11;
    Win32 window;

    bool init(const wchar_t* title, Win32::ResizeCallback on_resize = nullptr);
    [[nodiscard]] ScopedFrame next_frame(std::stop_token stop = {});
    void shutdown();

    void request_close() { PostQuitMessage(0); }

    [[nodiscard]] IniSettings::Registry& ini() noexcept { return ini_registry; }

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

    IniSettings::Registry ini_registry;
    Settings settings;

    bool show_status_bar = true;
    bool initialized = false;
    bool settings_applied = false;

    void begin_frame();
    void end_frame();
    void load_fonts();
    void draw_status_bar();

    void read_setting(std::string_view group, std::string_view line);
    void write_settings(IniSettings::Writer& out) const;
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
