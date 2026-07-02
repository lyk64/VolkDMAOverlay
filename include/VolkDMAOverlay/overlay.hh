#pragma once

#include "dx11.hh"
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

private:
    bool initialized = false;
    void load_fonts();
};