#pragma once

#include <functional>
#include <windows.h>

struct DX11;

struct Win32 {
    ~Win32() { cleanup(); }

    HWND hwnd{};
    WNDCLASSEXW wc{};
    DX11* dx11{};
    HMONITOR current_monitor{};

    using ResizeCallback = std::function<void(UINT width, UINT height)>;
    ResizeCallback on_resize;

    bool init(const wchar_t* title, UINT width, UINT height, DX11& dx11);
    [[nodiscard]] bool pump_messages();
    void cleanup();
    void move_to_monitor(HMONITOR monitor);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};