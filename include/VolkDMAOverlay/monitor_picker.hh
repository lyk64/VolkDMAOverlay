#pragma once

#include <string>

struct Overlay;

bool monitor_picker(const char* label, Overlay& overlay, std::string& device_path);
