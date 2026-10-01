#pragma once

#include <imgui.h>

namespace volk::overlay {

class ScopedTab {
public:
    ScopedTab(const ScopedTab&) = delete;
    ScopedTab& operator=(const ScopedTab&) = delete;
    ~ScopedTab() { if (open) ImGui::EndTabItem(); }

    explicit operator bool() const noexcept { return open; }

private:
    friend class ScopedTabBar;
    explicit ScopedTab(const char* name) : open{ ImGui::BeginTabItem(name) } {}

    bool open;
};

class ScopedTabBar {
public:
    ScopedTabBar(const ScopedTabBar&) = delete;
    ScopedTabBar& operator=(const ScopedTabBar&) = delete;
    ~ScopedTabBar() { if (open) ImGui::EndTabBar(); }

    explicit operator bool() const noexcept { return open; }
    [[nodiscard]] ScopedTab tab(const char* name) const { return ScopedTab{ name }; }

private:
    friend class ScopedMenu;
    explicit ScopedTabBar(const char* id) : open{ ImGui::BeginTabBar(id) } {}

    bool open;
};

class ScopedMenu {
public:
    ScopedMenu(const ScopedMenu&) = delete;
    ScopedMenu& operator=(const ScopedMenu&) = delete;
    ~ScopedMenu() { if (began) ImGui::End(); }

    explicit operator bool() const noexcept { return open; }
    [[nodiscard]] ScopedTabBar tabs(const char* id = "##tabs") const { return ScopedTabBar{ id }; }

private:
    friend class Menu;
    ScopedMenu(const char* title, bool* visible, ImGuiWindowFlags flags)
        : began{ *visible }, open{ began && ImGui::Begin(title, visible, flags) } {}

    bool began;
    bool open;
};

class Menu {
public:
    explicit Menu(const char* title, ImVec2 default_size = { 500.0f, 350.0f })
        : title{ title }, default_size{ default_size } {}

    [[nodiscard]] ScopedMenu begin(ImGuiWindowFlags flags = ImGuiWindowFlags_None);

    [[nodiscard]] bool is_visible() const noexcept { return visible; }
    void set_visible(bool value) noexcept { visible = value; }

private:
    const char* title;
    ImVec2 default_size;
    bool visible = true;
};

}
