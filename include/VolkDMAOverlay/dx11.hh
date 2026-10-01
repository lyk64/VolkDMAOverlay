#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace volk::overlay {

struct DX11 {
	~DX11() { cleanup(); }

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;

	bool init(HWND hwnd);
	void cleanup();
	bool resize(UINT width, UINT height);
	void clear_and_set_target();
	void set_vsync(bool vsync);
	void present();

private:
	HWND hwnd{};
	bool vsync_enabled{};
	bool tearing_supported{};
	bool tearing_enabled{};

	static bool check_tearing_support();
	bool create_swap_chain(bool allow_tearing);
	bool create_render_target();
};

} // namespace volk::overlay
