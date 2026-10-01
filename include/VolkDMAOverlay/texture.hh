#pragma once

#include "dx11.hh"

#include <imgui.h>

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

struct IWICImagingFactory;

namespace volk::overlay {

struct TextureCache {
	TextureCache(DX11& dx11, std::filesystem::path directory, std::string extension);
	~TextureCache();

	[[nodiscard]] ImTextureID load(std::string_view name);

private:
	DX11& dx11;
	std::filesystem::path directory;
	std::string extension;
	Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
	bool com_initialized{};
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, std::less<>> cache;
};

}
