# VolkDMAOverlay
A Dear ImGui overlay framework built on DirectX 11 for displaying DMA-driven visuals and menus.

### Currently supports:
- **Overlay window & rendering**
  - RAII overlay with scoped frames and `std::stop_token` support
  - Borderless, topmost, fullscreen window on any monitor
  - DirectX 11 swap chain with VSync toggle and tearing support
  - Monitor enumeration (name, resolution, refresh rate) and a built-in monitor picker
  - Status bar with custom popups

- **Menus & widgets**
  - RAII scoped menu, tab bar, and tab wrappers
  - Collapsible sections with optional enable checkboxes
  - Toggles with inline color and slider options, color pickers, sliders, radios, and combos
  - Key binding with a searchable key list

- **Settings & profiles**
  - Declarative typed settings groups saved to INI
  - Named profiles saved as `.volk` files with a built-in profile picker
  - Per-app config and asset folders under `C:\Volk`

- **Assets & fonts**
  - PNG texture loading and caching through WIC
  - Embedded Noto Sans with Windows CJK font fallbacks

### Hotkeys:
- `=` toggles the menu
- `-` toggles the status bar
- `Shift` + `=` exits

## Building

VolkDMAOverlay builds as a static library. It requires Visual Studio with the v145 toolset (C++23) and [vcpkg](https://github.com/microsoft/vcpkg) with Dear ImGui installed for the `x64-windows-static` triplet, including the DX11 and Win32 backends:

```
vcpkg install imgui[dx11-binding,win32-binding]:x64-windows-static
```

Clone with submodules to pull in [VolkLog](https://github.com/lyk64/VolkLog):

```
git clone --recursive https://github.com/lyk64/VolkDMAOverlay
```

## Contributors
- **Creator:** [lyk64](https://github.com/lyk64)

## Credits
This project uses [Dear ImGui](https://github.com/ocornut/imgui), created by [Omar Cornut](https://github.com/ocornut), and the [Noto Sans](https://fonts.google.com/noto/specimen/Noto+Sans) font.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

The embedded Noto Sans font is licensed under the SIL Open Font License 1.1 - see [`fonts/OFL.txt`](fonts/OFL.txt).
