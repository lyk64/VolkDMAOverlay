#include "include/VolkDMAOverlay/dx11.hh"
#include <VolkLog/log.hh>
#include <dxgi1_5.h>

using namespace Microsoft::WRL;

static constexpr Volk::Log::Logger logger{ "DX11" };

bool DX11::init(HWND hwnd) {
	tearing_supported = check_tearing_support();

	DXGI_SWAP_CHAIN_DESC sd{
		.BufferDesc{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM
		},
		.SampleDesc{
			.Count = 1
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.OutputWindow = hwnd,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
	};

	UINT flags{};
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		nullptr,
		&device_context))) {
		logger.error("D3D11CreateDeviceAndSwapChain failed");
		return false;
	}

	logger.info("Device created (tearing={})", tearing_supported);
	return create_render_target();
}

void DX11::cleanup() {
	if (device_context) {
		device_context->OMSetRenderTargets(0, nullptr, nullptr);
		device_context->ClearState();
		device_context->Flush();
	}

	render_target_view.Reset();
	swap_chain.Reset();
	device_context.Reset();
	device.Reset();
}

bool DX11::resize(UINT width, UINT height) {
	render_target_view.Reset();
	if (FAILED(swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u))) {
		logger.error("ResizeBuffers failed ({}x{})", width, height);
		return false;
	}
	return create_render_target();
}

void DX11::clear_and_set_target() {
	device_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);
	float clear[4]{};
	device_context->ClearRenderTargetView(render_target_view.Get(), clear);
}

void DX11::present(bool vsync) {
	swap_chain->Present(vsync, !vsync && tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0u);
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
		logger.error("GetBuffer failed");
		return false;
	}
	if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view.ReleaseAndGetAddressOf()))) {
		logger.error("CreateRenderTargetView failed");
		return false;
	}
	return true;
}