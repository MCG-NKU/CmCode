#include "stdafx.h"
#include "CmDxBase.h"
#include "atlstr.h"
using namespace DirectX;



CDXUTDialogResourceManager  CmDxBase::s_DlgRscManager; // manager for shared resources of dialogs


CmEffectFactory::CmEffectFactory(_In_ ID3D11Device* device) 
	: DirectX::EffectFactory( device ) 
{ 
	*searchPath = 0; 
}

void CmEffectFactory::SetPath(const WCHAR* path) 
{ 
	if ( path ) 
		wcscpy_s( searchPath, path ); 
	else 
		*searchPath = 0; 
}

void CmEffectFactory::CreateTexture( _In_z_ const WCHAR* name, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView)
{
	WCHAR fname[MAX_PATH] = {0};
	if ( *searchPath )
		wcscpy_s( fname, searchPath );
	wcscat_s( fname, name );

	WCHAR path[MAX_PATH] = {0};
	if ( FAILED( DXUTFindDXSDKMediaFileCch( path, MAX_PATH, fname ) ) )
		throw std::exception("Media not found");

	DirectX::EffectFactory::CreateTexture( path, deviceContext, textureView );
}


CmDxBase::CmDxBase(void)
	: g_pTextureRV1(nullptr)
	, g_pTextureRV2(nullptr)
	, g_pBatchInputLayout(nullptr)
	, g_pTxtHelper(nullptr)
{

}

bool gCmDxNotDestroied = true;
CmDxBase::~CmDxBase(void)
{
	gCmDxNotDestroied = false;
 	OnDestroyDevice();

	//ComPtr<ID3D11Debug> debugDev;
	//DXUTGetD3D11Device()->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(debugDev.GetAddressOf()));
	//debugDev->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY); //D3D11_RLDO_DETAIL
}

int CmDxBase::exec(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Set DXUT callbacks
	DXUTSetCallbackMsgProc(MsgProc, this);
	DXUTSetCallbackKeyboard(OnKeyboard, this);
	DXUTSetCallbackFrameMove(OnFrameMove, this);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings, this);

	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable, this);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice, this);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain, this);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain, this);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice, this);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender, this);

	DXUTInit(true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true );
	DXUTCreateWindow(L"Simple Sample for CmDxBase");

	InitApp();

	// Only require 10-level hardware, change to D3D_FEATURE_LEVEL_11_0 to require 11-class hardware
	// Switch to D3D_FEATURE_LEVEL_9_x for 10level9 hardware
	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 640, 480);

	DXUTMainLoop(); // Enter into the DXUT render loop

	return DXUTGetExitCode();
}

void CmDxBase::InitApp()
{
	g_SettingsDlg.Init( &s_DlgRscManager);
	g_HUD.Init( &s_DlgRscManager);
	g_SampleUI.Init( &s_DlgRscManager);

	g_HUD.SetCallback(OnGUIEvent, this);
	int iY = 30;
	int iYo = 26;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
	g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += iYo, 170, 22, VK_F2 );
	g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += iYo, 170, 22, VK_F3 );
	g_HUD.AddButton(IDC_TOGGLEWARP, L"Toggle WARP (F4)", 0, iY += iYo, 170, 22, VK_F4 );

	// Create sample UI
	CDXUTComboBox* pComboBox = nullptr;
	g_SampleUI.AddStatic( IDC_STATIC, L"Group", 10, 0, 170, 25 );
	g_SampleUI.AddComboBox( IDC_GROUP, 0, 25, 170, 24, 'G', false, &pComboBox );
	if( pComboBox )
		pComboBox->SetDropHeight( 50 );
	pComboBox->AddItem( L"Frustum", IntToPtr( 0 ) );
	pComboBox->AddItem( L"Axis-aligned Box", IntToPtr( 1 ) );
	pComboBox->AddItem( L"Oriented Box", IntToPtr( 2 ) );
	pComboBox->AddItem( L"Ray", IntToPtr( 3 ) );
	g_SampleUI.AddStatic(IDC_SLIDER_STATIC, L"Slider", 0, 50, 10, 25);
	g_SampleUI.AddSlider(IDC_SLIDER, 55, 50, 100, 20, 0, 100, 50, false);

	g_SampleUI.SetCallback(OnGUIEvent, this); 
}

void CmDxBase::RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos( 5, 5 );
	g_pTxtHelper->SetForegroundColor( Colors::Yellow );
	g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
	g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
	g_pTxtHelper->End();
}

void CmDxBase::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
	ComPtr<ID3D11DeviceContext> context = DXUTGetD3D11DeviceContext();
	g_BatchEffect->Apply(context.Get());
	context->IASetInputLayout(g_pBatchInputLayout.Get());
	g_Batch->Begin();
	xdivs = std::max<size_t>( 1, xdivs );
	ydivs = std::max<size_t>( 1, ydivs );

	for( size_t i = 0; i <= xdivs; ++i ){
		float fPercent = float(i) / float(xdivs);
		fPercent = ( fPercent * 2.0f ) - 1.0f;
		XMVECTOR vScale = XMVectorScale( xAxis, fPercent );
		vScale = XMVectorAdd( vScale, origin );

		VertexPositionColor v1( XMVectorSubtract( vScale, yAxis ), color );
		VertexPositionColor v2( XMVectorAdd( vScale, yAxis ), color );
		g_Batch->DrawLine( v1, v2 );
	}

	for( size_t i = 0; i <= ydivs; i++ ){
		FLOAT fPercent = float(i) / float(ydivs);
		fPercent = ( fPercent * 2.0f ) - 1.0f;
		XMVECTOR vScale = XMVectorScale( yAxis, fPercent );
		vScale = XMVectorAdd( vScale, origin );

		VertexPositionColor v1( XMVectorSubtract( vScale, xAxis ), color );
		VertexPositionColor v2( XMVectorAdd( vScale, xAxis ), color );
		g_Batch->DrawLine( v1, v2 );
	}

	g_Batch->End();

}

LRESULT CmDxBase::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing)
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = s_DlgRscManager.MsgProc( hWnd, uMsg, wParam, lParam );
	if( *pbNoFurtherProcessing )
		return 0;

	// Pass messages to settings dialog if its active
	if( g_SettingsDlg.IsActive() )
	{
		g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
		return 0;
	}

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
	if( *pbNoFurtherProcessing )
		return 0;
	*pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
	if( *pbNoFurtherProcessing )
		return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

	return 0;
}

HRESULT CmDxBase::OnCreateDevice(DxDevice* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	HRESULT hr;
	ComPtr<ID3D11DeviceContext> pd3dImmediateContext;
	pd3dDevice->GetImmediateContext(&pd3dImmediateContext);

	//auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN( s_DlgRscManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext.Get() ) );
	V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
	// TODO - 
	g_pTxtHelper.reset(new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext.Get(), &s_DlgRscManager, 15 ));

	// Create other render resources here
	g_States.reset( new CommonStates( pd3dDevice ) );
	g_Sprites.reset( new SpriteBatch( pd3dImmediateContext.Get() ) );
	g_FXFactory.reset( new CmEffectFactory( pd3dDevice ) );
	g_Batch.reset( new PrimitiveBatch<VertexPositionColor>( pd3dImmediateContext.Get() ) );

	g_BatchEffect.reset( new BasicEffect( pd3dDevice ) );
	g_BatchEffect->SetVertexColorEnabled(true);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		g_BatchEffect->GetVertexShaderBytecode( &shaderByteCode, &byteCodeLength );

		hr = pd3dDevice->CreateInputLayout( VertexPositionColor::InputElements,
			VertexPositionColor::InputElementCount,
			shaderByteCode, byteCodeLength,
			&g_pBatchInputLayout );
		if( FAILED( hr ) )
			return hr;
	}

	WCHAR str[ MAX_PATH ];
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"UI\\italic.spritefont" ) );
	g_Font.reset( new SpriteFont( pd3dDevice, str ) );

	g_Shape = GeometricPrimitive::CreateTeapot( pd3dImmediateContext.Get(), 4.f, 8, false );

	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Tiny\\tiny.sdkmesh" ) );
	g_FXFactory->SetPath( L"Tiny\\" );
	g_Model = Model::CreateFromSDKMESH( pd3dDevice, str, *g_FXFactory, true );

	// Load the Texture
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"misc\\seafloor.dds" ) );
	hr = CreateDDSTextureFromFile( pd3dDevice, str, nullptr, &g_pTextureRV1 );
	if( FAILED( hr ) )
		return hr;

	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"misc\\windowslogo.dds" ) );
	hr = CreateDDSTextureFromFile( pd3dDevice, str, nullptr, &g_pTextureRV2 );
	if( FAILED( hr ) )
		return hr;

	// Setup the camera's view parameters
	static const XMVECTORF32 s_vecEye = { 0.0f, 3.0f, -6.0f, 0.f };
	g_Camera.SetViewParams( s_vecEye, g_XMZero );

	//g_HUD.GetButton( IDC_TOGGLEWARP )->SetEnabled( true );

	return S_OK;
}

HRESULT CmDxBase::OnResizedSwapChain(DxDevice* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	HRESULT hr;

	V_RETURN( s_DlgRscManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
	V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 0.1f, 1000.0f );
	g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
	g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

	g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
	g_HUD.SetSize( 170, 170 );

	g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, 150 );
	g_SampleUI.SetSize( 170, 300 );
	return S_OK;
}


void CmDxBase::OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown)
{

}

void CmDxBase::OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl)
{
	switch( nControlID )
	{
	case IDC_TOGGLEFULLSCREEN:
		DXUTToggleFullScreen();
		break;
	case IDC_TOGGLEREF:
		DXUTToggleREF();
		break;
	case IDC_TOGGLEWARP:
		DXUTToggleWARP();
		break;
	case IDC_CHANGEDEVICE:
		g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() );
		break;
	case IDC_SLIDER:
		CStringW str;
		str.Format(L"S=%d", g_SampleUI.GetSlider(IDC_SLIDER)->GetValue());
		g_SampleUI.GetStatic(IDC_SLIDER_STATIC)->SetText(str);
		break;
	}
}

void CmDxBase::OnFrameMove(double fTime, float fElapsedTime)
{
	// Update the camera's position based on user input 
	g_Camera.FrameMove( fElapsedTime );
}

bool CmDxBase::ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings)
{
	return true;
}

bool CmDxBase::IsDeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed)
{
	return true;
}

void CmDxBase::OnDestroyDevice()
{
	s_DlgRscManager.OnD3D11DestroyDevice();
	g_SettingsDlg.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();


    g_States.reset();
    g_BatchEffect.reset();
    g_FXFactory.reset();
    g_Shape.reset();
    g_Model.reset();
    g_Batch.reset();
    g_Sprites.reset();
    g_Font.reset();
	g_pTxtHelper.reset();

    //SAFE_DELETE( g_pTxtHelper );
    //SAFE_RELEASE( g_pBatchInputLayout );
    //SAFE_RELEASE( g_pTextureRV1 );
    //SAFE_RELEASE( g_pTextureRV2 );

}

void CmDxBase::OnFrameRender(DxDevice* pd3dDevice, DxContext* pd3dImmediateContext, double fTime, float fElapsedTime)
{
	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if( g_SettingsDlg.IsActive() )
	{
		g_SettingsDlg.OnRender( fElapsedTime );
		return;
	}       

	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::MidnightBlue );

	// Clear the depth stencil
	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	// Get the projection & view matrix from the camera class
	XMMATRIX mWorld = g_Camera.GetWorldMatrix();
	XMMATRIX mView = g_Camera.GetViewMatrix();
	XMMATRIX mProj = g_Camera.GetProjMatrix();

	g_BatchEffect->SetView( mView );
	g_BatchEffect->SetProjection( mProj );

	// Draw procedurally generated dynamic grid
	const XMVECTORF32 xaxis = { 20.f, 0.f, 0.f };
	const XMVECTORF32 yaxis = { 0.f, 0.f, 20.f };
	DrawGrid( xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray );

	// Draw sprite
	g_Sprites->Begin( SpriteSortMode_Deferred );
	g_Sprites->Draw( g_pTextureRV2.Get(), XMFLOAT2(10, 75 ), nullptr, Colors::White );

	g_Font->DrawString( g_Sprites.get(), L"DirectXTK Simple Sample", XMFLOAT2( 100, 350 ), Colors::Yellow );
	g_Sprites->End();

	// Draw 3D object
	XMMATRIX local = XMMatrixMultiply( mWorld, XMMatrixTranslation( -2.f, -2.f, 4.f ) );
	g_Shape->Draw( local, mView, mProj, Colors::White, g_pTextureRV1.Get());

	XMVECTOR qid = XMQuaternionIdentity();
	const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f};
	const XMVECTORF32 translate = { 3.f, -2.f, 4.f };
	XMVECTOR rotate = XMQuaternionRotationRollPitchYaw( 0, XM_PI/2.f, XM_PI/2.f );
	local = XMMatrixMultiply( mWorld, XMMatrixTransformation( g_XMZero, qid, scale, g_XMZero, rotate, translate ) );
	g_Model->Draw( pd3dImmediateContext, *g_States, local, mView, mProj );

	// Render HUD
	//tm.Start();
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
	g_HUD.OnRender( fElapsedTime );
	g_SampleUI.OnRender( fElapsedTime );
	RenderText();
	DXUT_EndPerfEvent();

	//tm.StopAndReport();

	static ULONGLONG timefirst = GetTickCount64();
	if ( GetTickCount64() - timefirst > 5000 ){    
		OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
		OutputDebugString( L"\n" );
		timefirst = GetTickCount64();
	}
}


/************************************************************************/
/*    Implementation of callbacks                                       */
/************************************************************************/

LRESULT CALLBACK CmDxBase::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
	return ((CmDxBase*)pUserContext)->MsgProc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing);
}

void CALLBACK CmDxBase::OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	((CmDxBase*)pUserContext)->OnKeyboard(nChar, bKeyDown, bAltDown);
}

void CALLBACK CmDxBase::OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	((CmDxBase*)pUserContext)->OnGUIEvent(nEvent, nControlID, pControl);
}

void CALLBACK CmDxBase::OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	((CmDxBase*)pUserContext)->OnFrameMove(fTime, fElapsedTime);
}

bool CALLBACK CmDxBase::ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	return ((CmDxBase*)pUserContext)->ModifyDeviceSettings(pDeviceSettings);
}

bool CALLBACK CmDxBase::IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return ((CmDxBase*)pUserContext)->IsDeviceAcceptable(AdapterInfo, Output, DeviceInfo, BackBufferFormat, bWindowed);
}

HRESULT CALLBACK CmDxBase::OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	return ((CmDxBase*)pUserContext)->OnCreateDevice(pd3dDevice, pBackBufferSurfaceDesc);
}

HRESULT CALLBACK CmDxBase::OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	return ((CmDxBase*)pUserContext)->OnResizedSwapChain(pd3dDevice, pSwapChain, pBackBufferSurfaceDesc);
}

void CALLBACK CmDxBase::OnD3D11ReleasingSwapChain(void* pUserContext)
{
	s_DlgRscManager.OnD3D11ReleasingSwapChain();
}

void CALLBACK CmDxBase::OnD3D11DestroyDevice(void* pUserContext)
{
	if (gCmDxNotDestroied)
		((CmDxBase*)pUserContext)->OnDestroyDevice();
}


void CALLBACK CmDxBase::OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext )
{
	((CmDxBase*)pUserContext)->OnFrameRender(pd3dDevice, pd3dImmediateContext, fTime, fElapsedTime);
}