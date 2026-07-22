#pragma once

#include "dx11.hh"
#include "monitor.hh"
#include "win32.hh"

struct Overlay {
    ~Overlay() { shutdown(); }

    DX11 dx11;
    Win32 window;
    UINT width{};
    UINT height{};

    bool init(const wchar_t* title, Win32::ResizeCallback on_resize = nullptr);
    [[nodiscard]] bool pump_messages();
    void begin_frame();
    void end_frame(bool vsync);
    void shutdown();

    [[nodiscard]] HMONITOR current_monitor() const { return window.current_monitor; }
    [[nodiscard]] std::vector<MonitorInfo> list_monitors() const { return ::list_monitors(); }
    void move_to_monitor(HMONITOR monitor) { window.move_to_monitor(monitor); }
    void move_to_monitor(const std::string& device_path);

private:
    bool initialized = false;
    void load_fonts();
};