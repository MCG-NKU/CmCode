#include "stdafx.h"
#include "Direct3DBase.h"
using namespace DirectX;

Direct3DBase::Direct3DBase(void)
	:g_hInst(NULL)
	,g_hWnd(NULL)
	,g_driverType(D3D_DRIVER_TYPE_NULL)
	,g_featureLevel(D3D_FEATURE_LEVEL_11_0)
	,g_pd3dDevice(NULL)
	,g_pSwapChain(NULL)
	,g_pRenderTargetView(NULL)
{
	m_wndProc = WndProc;
}


Direct3DBase::~Direct3DBase(void)
{
	CleanupDevice();
}

HRESULT Direct3DBase::InitWindow(HINSTANCE hInstance, int nCmdShow, LPCTSTR iconStr)
{
	HICON icon = 0;
	if (iconStr)
		icon = LoadIcon(hInstance, iconStr);

	// Register class
	WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, m_wndProc, 0L, 0L, hInstance, icon, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, L"Direct3DbaseClass", NULL};
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	m_Width = 640, m_Height = 480;
	RECT rc = { 0, 0, m_Width, m_Height };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	g_hWnd = CreateWindow(wcex.lpszClassName, L"Direct3Dbase", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr );
	if( !g_hWnd )
		return E_FAIL;

	ShowWindow( g_hWnd, nCmdShow );
	return S_OK;
}

// Initialize m_wndProc in construction function if overload this function
LRESULT CALLBACK Direct3DBase::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch( message ){
	case WM_PAINT: hdc = BeginPaint( hWnd, &ps ); EndPaint( hWnd, &ps ); break;
	case WM_DESTROY: PostQuitMessage( 0 ); break;
	default: return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}


HRESULT Direct3DBase::InitDevice()
{
	HRESULT hr = S_OK;
	RECT rc;
	GetClientRect( g_hWnd, &rc );
	m_Width = rc.right - rc.left;
	m_Height = rc.bottom - rc.top;
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_DRIVER_TYPE driverTypes[] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);
	D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = m_Width;
	sd.BufferDesc.Height = m_Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ ) {
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
		if ( hr == E_INVALIDARG ) // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it			
			hr = D3D11CreateDeviceAndSwapChain( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
		if( SUCCEEDED( hr ) )
			break;
	}
	V_RETURN(hr);

	// Create a render target view
	ComPtr<ID3D11Texture2D> pBackBuffer;
	V_RETURN(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)pBackBuffer.GetAddressOf()));
	V_RETURN(g_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &g_pRenderTargetView));
	g_pImmediateContext->OMSetRenderTargets(1, g_pRenderTargetView.GetAddressOf(), NULL);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);
	return S_OK;
}

void Direct3DBase::CleanupDevice()
{
	if( g_pImmediateContext ) g_pImmediateContext->ClearState();	
}

void Direct3DBase::Render()
{
	// Just clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView.Get(), Colors::MidnightBlue );

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present( 0, 0 );
}



int Direct3DBase::exec(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );

	if(FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if(FAILED( InitDevice())){
		CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = {0};
	while( WM_QUIT != msg.message ){
		if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE)){
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
			Render();
	}
	CleanupDevice();
	return (int)msg.wParam;
}