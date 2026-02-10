#include "include/VolkDMAOverlay/dx11.hh"

using namespace Microsoft::WRL;

bool DX11::init(HWND hwnd) {
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
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
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
		return false;
	}

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
	if (FAILED(swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)))
		return false;
	return create_render_target();
}

void DX11::clear_and_set_target() {
	device_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);
	float clear[4]{};
	device_context->ClearRenderTargetView(render_target_view.Get(), clear);
}

void DX11::present(bool vsync) {
	swap_chain->Present(vsync, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
}

bool DX11::create_render_target() {
	ComPtr<ID3D11Texture2D> back_buffer;
	if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))))
		return false;
	if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view.ReleaseAndGetAddressOf())))
		return false;
	return true;
}