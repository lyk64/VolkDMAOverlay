#include "include/VolkDMAOverlay/dx11.hh"
#include <VolkLog/log.hh>
#include <dxgi1_5.h>

#pragma comment(lib, "dcomp.lib")

using namespace Microsoft::WRL;

namespace volk::overlay {

static constexpr volk::log::Logger logger{ "overlay.dx11" };

bool DX11::init(HWND hwnd, bool transparent) {
	this->hwnd = hwnd;
	this->transparent = transparent;
	tearing_supported = !transparent && check_tearing_support();

	UINT flags{};
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&device,
		nullptr,
		&device_context))) {
		logger.error("D3D11CreateDevice failed.");
		return false;
	}

	logger.info("Device created (tearing={}).", tearing_supported);
	return create_swap_chain(tearing_supported);
}

bool DX11::create_swap_chain(bool allow_tearing) {
	ComPtr<IDXGIDevice> dxgi_device;
	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIFactory2> factory;

	if (FAILED(device.As(&dxgi_device)) ||
		FAILED(dxgi_device->GetAdapter(&adapter)) ||
		FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) {
		logger.error("Failed to reach the DXGI factory.");
		return false;
	}

	RECT client{};
	GetClientRect(hwnd, &client);

	const DXGI_SWAP_CHAIN_DESC1 desc{
		.Width = static_cast<UINT>(client.right - client.left),
		.Height = static_cast<UINT>(client.bottom - client.top),
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc{
			.Count = 1
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = transparent ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
	};

	if (transparent) {
		if (FAILED(factory->CreateSwapChainForComposition(device.Get(), &desc, nullptr, &swap_chain))) {
			logger.error("CreateSwapChainForComposition failed.");
			return false;
		}

		if (!compose(dxgi_device.Get()))
			return false;
	}
	else if (FAILED(factory->CreateSwapChainForHwnd(device.Get(), hwnd, &desc, nullptr, nullptr, &swap_chain))) {
		logger.error("CreateSwapChainForHwnd failed (tearing={}).", allow_tearing);
		return false;
	}

	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	tearing_enabled = allow_tearing;
	return create_render_target();
}

bool DX11::compose(IDXGIDevice* dxgi_device) {
	if (FAILED(DCompositionCreateDevice(dxgi_device, IID_PPV_ARGS(&composition_device))) ||
		FAILED(composition_device->CreateTargetForHwnd(hwnd, TRUE, &composition_target)) ||
		FAILED(composition_device->CreateVisual(&composition_visual)) ||
		FAILED(composition_visual->SetContent(swap_chain.Get())) ||
		FAILED(composition_target->SetRoot(composition_visual.Get())) ||
		FAILED(composition_device->Commit())) {
		logger.error("Failed to compose the swap chain onto the window.");
		return false;
	}

	return true;
}

void DX11::set_vsync(bool vsync) {
	vsync_enabled = vsync;

	const bool allow_tearing = tearing_supported && !vsync;
	if (allow_tearing == tearing_enabled)
		return;

	logger.debug("Rebuilding the swap chain (tearing={}).", allow_tearing);

	device_context->OMSetRenderTargets(0, nullptr, nullptr);
	device_context->ClearState();
	device_context->Flush();

	render_target_view.Reset();
	swap_chain.Reset();

	if (!create_swap_chain(allow_tearing))
		logger.error("Swap chain rebuild failed; nothing will be presented.");
}

void DX11::cleanup() {
	if (device_context) {
		device_context->OMSetRenderTargets(0, nullptr, nullptr);
		device_context->ClearState();
		device_context->Flush();
	}

	render_target_view.Reset();
	composition_visual.Reset();
	composition_target.Reset();
	composition_device.Reset();
	swap_chain.Reset();
	device_context.Reset();
	device.Reset();
}

bool DX11::resize(UINT width, UINT height) {
	if (!swap_chain)
		return false;

	render_target_view.Reset();
	if (FAILED(swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN,
		tearing_enabled ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u))) {
		logger.error("ResizeBuffers failed ({}x{}).", width, height);
		return false;
	}
	return create_render_target();
}

void DX11::clear_and_set_target() {
	if (!render_target_view)
		return;

	device_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);
	float clear[4]{};
	device_context->ClearRenderTargetView(render_target_view.Get(), clear);
}

void DX11::present() {
	if (!swap_chain)
		return;

	swap_chain->Present(vsync_enabled, !vsync_enabled && tearing_enabled ? DXGI_PRESENT_ALLOW_TEARING : 0u);
}

bool DX11::check_tearing_support() {
	ComPtr<IDXGIFactory5> factory;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
		return false;

	BOOL allowed{};
	if (FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowed, sizeof(allowed))))
		return false;

	return allowed;
}

bool DX11::create_render_target() {
	ComPtr<ID3D11Texture2D> back_buffer;
	if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
		logger.error("GetBuffer failed.");
		return false;
	}
	if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view.ReleaseAndGetAddressOf()))) {
		logger.error("CreateRenderTargetView failed.");
		return false;
	}
	return true;
}

} // namespace volk::overlay
