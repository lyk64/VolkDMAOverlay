#include "include/VolkDMAOverlay/win32.hh"
#include "include/VolkDMAOverlay/dx11.hh"
#include <VolkLog/log.hh>
#include <imgui_impl_win32.h>
#include <utility>

static constexpr Volk::Log::Logger logger{ "WIN32" };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Win32::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    auto* self = reinterpret_cast<Win32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_MOVE: {
        if (self) {
            HMONITOR new_monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (self->current_monitor && new_monitor != self->current_monitor) {
                logger.debug("Monitor changed, repositioning window");
                self->move_to_monitor(new_monitor);
            } else {
                self->current_monitor = new_monitor;
            }
        }
        return 0;
    }

    case WM_SIZE:
        if (self && self->dx11 && self->dx11->device && wParam != SIZE_MINIMIZED) {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            if (width > 0 && height > 0) {
                logger.debug("Resized to {}x{}", width, height);
                self->dx11->resize(width, height);
                if (self->on_resize) {
                    self->on_resize(width, height);
                }
            }
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


bool Win32::init(std::wstring title, UINT width, UINT height, DX11& dx11) {
    this->dx11 = &dx11;
    class_name = std::move(title);

    wc = {
        .cbSize = sizeof(wc),
        .lpfnWndProc = WndProc,
        .hInstance = GetModuleHandleW(nullptr),
        .hCursor = LoadCursorW(nullptr, IDC_ARROW),
        .lpszClassName = class_name.c_str(),
    };

    RegisterClassExW(&wc);

    hwnd = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_TOPMOST,
        wc.lpszClassName,
        wc.lpszClassName,
        WS_POPUP | WS_VISIBLE,
        0, 0, width, height,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!hwnd) {
        logger.error("CreateWindowExW failed");
        return false;
    }

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    current_monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    logger.info("Window created ({}x{})", width, height);
    return true;
}

std::pair<UINT, UINT> Win32::client_size() const {
    RECT client{};
    if (!hwnd || !GetClientRect(hwnd, &client))
        return { 0, 0 };

    return { static_cast<UINT>(client.right - client.left), static_cast<UINT>(client.bottom - client.top) };
}

void Win32::move_to_monitor(HMONITOR monitor) {
    if (monitor == current_monitor)
        return;

    MONITORINFO mi = { .cbSize = sizeof(mi) };
    if (!GetMonitorInfoW(monitor, &mi)) {
        logger.warn("GetMonitorInfoW failed, staying put");
        return;
    }

    current_monitor = monitor;

    SetWindowPos(hwnd, HWND_TOP,
        mi.rcMonitor.left, mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        0);
}

bool Win32::pump_messages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

void Win32::cleanup() {
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }

    if (wc.lpszClassName) {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        wc.lpszClassName = nullptr;
    }
}
