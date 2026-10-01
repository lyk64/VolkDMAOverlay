#include "include/VolkDMAOverlay/texture.hh"
#include <VolkLog/log.hh>
#include <wincodec.h>
#include <optional>
#include <vector>

using namespace Microsoft::WRL;

namespace volk::overlay {

static constexpr volk::log::Logger logger{ "TEXTURE" };

namespace {
	struct Image {
		std::vector<BYTE> pixels;
		UINT width{};
		UINT height{};
	};

	std::optional<Image> decode_image(IWICImagingFactory* factory, const std::filesystem::path& path) {
		if (!factory)
			return std::nullopt;

		ComPtr<IWICBitmapDecoder> decoder;
		if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder)))
			return std::nullopt;

		ComPtr<IWICBitmapFrameDecode> frame;
		if (FAILED(decoder->GetFrame(0, &frame)))
			return std::nullopt;

		ComPtr<IWICFormatConverter> converter;
		if (FAILED(factory->CreateFormatConverter(&converter)))
			return std::nullopt;

		if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
			return std::nullopt;

		Image image;
		if (FAILED(converter->GetSize(&image.width, &image.height)))
			return std::nullopt;

		image.pixels.resize(static_cast<size_t>(image.width) * image.height * 4);
		if (FAILED(converter->CopyPixels(nullptr, image.width * 4, static_cast<UINT>(image.pixels.size()), image.pixels.data())))
			return std::nullopt;

		return image;
	}

	ComPtr<ID3D11ShaderResourceView> create_texture(DX11& dx11, const Image& image) {
		D3D11_TEXTURE2D_DESC desc{
			.Width = image.width,
			.Height = image.height,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc{
				.Count = 1
			},
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE
		};

		D3D11_SUBRESOURCE_DATA subresource{
			.pSysMem = image.pixels.data(),
			.SysMemPitch = image.width * 4
		};

		ComPtr<ID3D11Texture2D> texture;
		if (FAILED(dx11.device->CreateTexture2D(&desc, &subresource, &texture))) {
			logger.error("CreateTexture2D failed ({}x{})", image.width, image.height);
			return nullptr;
		}

		ComPtr<ID3D11ShaderResourceView> srv;
		if (FAILED(dx11.device->CreateShaderResourceView(texture.Get(), nullptr, &srv))) {
			logger.error("CreateShaderResourceView failed");
			return nullptr;
		}

		return srv;
	}
} // namespace

TextureCache::TextureCache(DX11& dx11, std::filesystem::path directory, std::string extension)
	: dx11(dx11), directory(std::move(directory)), extension(std::move(extension)) {
	com_initialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
		logger.error("Failed to create WIC imaging factory");
}

TextureCache::~TextureCache() {
	cache.clear();
	factory.Reset();

	if (com_initialized)
		CoUninitialize();
}

ImTextureID TextureCache::load(std::string_view name) {
	auto it = cache.find(name);
	if (it == cache.end()) {
		auto path = directory / name;
		path += extension;

		ComPtr<ID3D11ShaderResourceView> srv;
		if (auto image = decode_image(factory.Get(), path))
			srv = create_texture(dx11, *image);
		else
			logger.warn("Failed to decode: {}", path.string());

		it = cache.emplace(name, std::move(srv)).first;
	}

	return reinterpret_cast<ImTextureID>(it->second.Get());
}

} // namespace volk::overlay
