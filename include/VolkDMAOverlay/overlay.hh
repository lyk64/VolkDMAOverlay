#pragma once

#include "dx11.hh"
#include "monitor.hh"
#include "win32.hh"

#include <stop_token>

class ScopedFrame;

struct Overlay {
    ~Overlay() { shutdown(); }

    DX11 dx11;
    Win32 window;
    UINT width{};
    UINT height{};
    bool show_status_bar = true;
    bool vsync = false;

    bool init(const wchar_t* title, Win32::ResizeCallback on_resize = nullptr);
    [[nodiscard]] ScopedFrame next_frame(std::stop_token stop = {});
    void shutdown();

    void request_close() { PostQuitMessage(0); }

    [[nodiscard]] HMONITOR current_monitor() const { return window.current_monitor; }
    [[nodiscard]] std::vector<MonitorInfo> list_monitors() const { return ::list_monitors(); }
    void move_to_monitor(HMONITOR monitor) { window.move_to_monitor(monitor); }
    void move_to_monitor(const std::string& device_path);

private:
    friend class ScopedFrame;

    bool initialized = false;
    void begin_frame();
    void end_frame();
    void load_fonts();
    void draw_status_bar();
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
