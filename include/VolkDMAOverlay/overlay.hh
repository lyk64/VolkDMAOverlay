#pragma once

#include "dx11.hh"
#include "win32.hh"

#include <stop_token>
#include <string>

struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

class ScopedFrame;

struct Overlay {
    ~Overlay() { shutdown(); }

    DX11 dx11;
    Win32 window;

    bool init(const wchar_t* title, Win32::ResizeCallback on_resize = nullptr);
    [[nodiscard]] ScopedFrame next_frame(std::stop_token stop = {});
    void shutdown();

    void request_close() { PostQuitMessage(0); }

    [[nodiscard]] HMONITOR current_monitor() const { return window.current_monitor; }
    void move_to_monitor(HMONITOR monitor);
    void move_to_monitor(const std::string& device_path);

private:
    friend class ScopedFrame;

    struct Settings {
        bool vsync = false;
        std::string monitor_path;

        bool operator==(const Settings&) const = default;
    };

    Settings settings;
    Settings saved_settings;

    bool show_status_bar = true;
    bool initialized = false;
    bool settings_applied = false;

    void begin_frame();
    void end_frame();
    void load_fonts();
    void draw_status_bar();
    void apply_settings();
    void save_settings_if_changed();

    static void* settings_read_open(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name);
    static void settings_read_line(ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line);
    static void settings_write_all(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
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
