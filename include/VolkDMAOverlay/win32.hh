#pragma once

#include <functional>
#include <windows.h>

struct DX11;

struct Win32 {
    ~Win32() { cleanup(); }

    HWND hwnd{};
    WNDCLASSEXW wc{};
    DX11* dx11{};

    using ResizeCallback = std::function<void(UINT width, UINT height)>;
    ResizeCallback on_resize;

    bool init(const wchar_t* title, UINT width, UINT height, DX11& dx11);
    void cleanup();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};