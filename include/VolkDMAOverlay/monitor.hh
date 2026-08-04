#pragma once

#include <optional>
#include <string>
#include <vector>
#include <windows.h>

struct MonitorInfo {
    HMONITOR handle;
    RECT rect;
    std::string name;
    std::string device_path;
    std::optional<DWORD> refresh_hz;
};

[[nodiscard]] std::vector<MonitorInfo> list_monitors();
[[nodiscard]] std::string monitor_label(const MonitorInfo& monitor, size_t index);
