#pragma once

#include <functional>
#include <string>
#include <utility>
#include <windows.h>

struct DX11;

struct Win32 {
    ~Win32() { cleanup(); }

    HWND hwnd{};
    WNDCLASSEXW wc{};
    DX11* dx11{};
    HMONITOR current_monitor{};
    std::wstring class_name;

    using ResizeCallback = std::function<void(UINT width, UINT height)>;
    ResizeCallback on_resize;

    bool init(std::wstring title, UINT width, UINT height, DX11& dx11);
    [[nodiscard]] std::pair<UINT, UINT> client_size() const;
    [[nodiscard]] bool pump_messages();
    void cleanup();
    void move_to_monitor(HMONITOR monitor);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};