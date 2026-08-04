#include "include/VolkDMAOverlay/monitor.hh"
#include <format>

static std::string narrow(const std::wstring& wide) {
    if (wide.empty())
        return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), size, nullptr, nullptr);
    return out;
}

struct MonitorTargetInfo {
    std::wstring name;
    std::wstring device_path;
};

static MonitorTargetInfo monitor_target_info(const std::wstring& gdi_device_name) {
    UINT32 num_paths{};
    UINT32 num_modes{};
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &num_paths, &num_modes) != ERROR_SUCCESS)
        return {};

    std::vector<DISPLAYCONFIG_PATH_INFO> paths(num_paths);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(num_modes);
    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &num_paths, paths.data(), &num_modes, modes.data(), nullptr) != ERROR_SUCCESS)
        return {};

    for (const auto& path : paths) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name{
            .header = {
                .type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,
                .size = sizeof(source_name),
                .adapterId = path.sourceInfo.adapterId,
                .id = path.sourceInfo.id,
            },
        };
        if (DisplayConfigGetDeviceInfo(&source_name.header) != ERROR_SUCCESS)
            continue;
        if (gdi_device_name != source_name.viewGdiDeviceName)
            continue;

        DISPLAYCONFIG_TARGET_DEVICE_NAME target_name{
            .header = {
                .type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,
                .size = sizeof(target_name),
                .adapterId = path.targetInfo.adapterId,
                .id = path.targetInfo.id,
            },
        };
        if (DisplayConfigGetDeviceInfo(&target_name.header) != ERROR_SUCCESS)
            continue;

        return {
            .name = target_name.flags.friendlyNameFromEdid ? target_name.monitorFriendlyDeviceName : L"",
            .device_path = target_name.monitorDevicePath,
        };
    }

    return {};
}

std::vector<MonitorInfo> list_monitors() {
    std::vector<MonitorInfo> monitors;

    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR monitor, HDC, LPRECT, LPARAM param) -> BOOL {
        MONITORINFOEXW mi{};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(monitor, &mi);
        auto target = monitor_target_info(mi.szDevice);

        std::optional<DWORD> refresh_hz;
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &mode))
            refresh_hz = mode.dmDisplayFrequency;

        reinterpret_cast<std::vector<MonitorInfo>*>(param)->push_back({
            .handle = monitor,
            .rect = mi.rcMonitor,
            .name = narrow(target.name),
            .device_path = narrow(target.device_path),
            .refresh_hz = refresh_hz,
        });
        return TRUE;
    }, reinterpret_cast<LPARAM>(&monitors));

    return monitors;
}

std::string monitor_label(const MonitorInfo& monitor, size_t index) {
    std::string name = monitor.name.empty() ? std::format("Monitor {}", index + 1) : monitor.name;
    std::string size = std::format("{}x{}",
        monitor.rect.right - monitor.rect.left,
        monitor.rect.bottom - monitor.rect.top);

    if (!monitor.refresh_hz)
        return std::format("{} ({})", name, size);

    return std::format("{} ({} @ {}Hz)", name, size, *monitor.refresh_hz);
}
