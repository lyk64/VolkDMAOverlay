#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

struct DX11 {
	~DX11() { cleanup(); }

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;

	bool init(HWND hwnd);
	void cleanup();
	bool resize(UINT width, UINT height);
	void clear_and_set_target();
	void present(bool vsync);

private:
	bool tearing_supported{};
	bool check_tearing_support();
	bool create_render_target();
};