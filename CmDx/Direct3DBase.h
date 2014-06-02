#pragma once

#include <atlcomcli.h>

class Direct3DBase
{
public:
	Direct3DBase(void);
	~Direct3DBase(void);

	//virtual void HandleDeviceLost();
	//virtual void CreateDeviceResources();
	//virtual void CreateWindowSizeDependentResources();
	//virtual void UpdateForWindowSizeChange();
	//virtual void Present();
	//virtual float ConvertDipsToPixels(float dips);

	virtual HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow, LPCTSTR icon = L"directx.ico");
	virtual HRESULT InitDevice();
	virtual void CleanupDevice();
	virtual void Render();

	virtual int exec(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);

	// Initialize m_wndProc in construction function if overload this function
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); // Called when receives a message

protected:
	HINSTANCE	g_hInst;
	HWND		g_hWnd;
	WNDPROC		m_wndProc;
	D3D_DRIVER_TYPE	g_driverType;
	D3D_FEATURE_LEVEL	g_featureLevel;
	CComPtr<ID3D11Device>	g_pd3dDevice;
	CComPtr<IDXGISwapChain> g_pSwapChain;
	CComPtr<ID3D11DeviceContext> g_pImmediateContext;
	CComPtr<ID3D11RenderTargetView> g_pRenderTargetView;

	CComPtr<ID3D11VertexShader> g_pVertexShader;
	CComPtr<ID3D11PixelShader>  g_pPixelShader;
	CComPtr<ID3D11InputLayout>  g_pVertexLayout;
	CComPtr<ID3D11Buffer>	g_pVertexBuffer;
	CComPtr<ID3D11Buffer>	g_pIndexBuffer;
	CComPtr<ID3D11Buffer>	g_pConstantBuffer;

	CComPtr<ID3D11Texture2D>        g_pDepthStencil;
	CComPtr<ID3D11DepthStencilView> g_pDepthStencilView;

	DirectX::XMMATRIX g_World;
	DirectX::XMMATRIX g_View;
	DirectX::XMMATRIX g_Projection;

	UINT m_Width, m_Height; // Client width and height
};

