#include "stdafx.h"
#include "Direct3DDemos.h"
using namespace DirectX;

Direct3D_DemoTriangle::Direct3D_DemoTriangle(void)
{
	m_wndProc = WndProc; // This is useful only if want to overload WndPro
}

HRESULT Direct3D_DemoTriangle::InitDevice()
{
	HRESULT hr = S_OK;
	V_RETURN(Direct3DBase::InitDevice());

	CmSRM srm;
	D3D11_INPUT_ELEMENT_DESC layout[] = {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },};
	srm.loadVertexShader(L"VsPos.cso", g_pd3dDevice.Get(), g_pVertexShader, layout, ARRAYSIZE(layout), g_pVertexLayout);
	g_pImmediateContext->IASetInputLayout( g_pVertexLayout.Get() );	
	srm.loadPixelShader(L"PsYellow.cso", g_pd3dDevice.Get(), g_pPixelShader);

	// Create vertex buffer
	XMFLOAT3 vertices[] = {XMFLOAT3( 0.0f, 0.5f, 0.5f ), XMFLOAT3( 0.5f, -0.5f, 0.5f ), XMFLOAT3( -0.5f, -0.5f, 0.5f ),	};
	srm.createDefaultBuffer(g_pd3dDevice.Get(), sizeof(XMFLOAT3)*3, g_pVertexBuffer, vertices);

	UINT stride = sizeof( XMFLOAT3 ), offset = 0;
	g_pImmediateContext->IASetVertexBuffers( 0, 1, g_pVertexBuffer.GetAddressOf(), &stride, &offset );	// Set vertex buffer
	g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );// Set primitive topology

	return hr;
}

void Direct3D_DemoTriangle::Render()
{
	// Just clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView.Get(), Colors::MidnightBlue );

	// Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader.Get(), nullptr, 0 );
	g_pImmediateContext->PSSetShader(g_pPixelShader.Get(), nullptr, 0 );
	g_pImmediateContext->Draw( 3, 0 );

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present( 0, 0 );
}

HRESULT Direct3D_DemoCube::InitDevice()
{
	HRESULT hr = S_OK;
	V_RETURN(Direct3DBase::InitDevice());

	CmSRM srm;
	D3D11_INPUT_ELEMENT_DESC layout[] = {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }};
	srm.loadVertexShader(L"VsOrg.cso", g_pd3dDevice.Get(), g_pVertexShader, layout, ARRAYSIZE(layout), g_pVertexLayout);
	g_pImmediateContext->IASetInputLayout( g_pVertexLayout.Get() );	
	srm.loadPixelShader(L"PsOrg.cso", g_pd3dDevice.Get(), g_pPixelShader);

	// Create vertex buffer    
	SimpleVertex vertices[] = {
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT4( 0.0f, 1.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f ) },
	};
	srm.createDefaultBuffer(g_pd3dDevice.Get(), sizeof(SimpleVertex)*8, g_pVertexBuffer, vertices);
	UINT stride = sizeof(SimpleVertex), offset = 0;
	g_pImmediateContext->IASetVertexBuffers( 0, 1, g_pVertexBuffer.GetAddressOf(), &stride, &offset );	// Set vertex buffer


	WORD indices[] = {3,1,0,	2,1,3,	0,5,4,	1,5,0,	3,4,7,	0,4,3,	1,6,5,	2,6,1,	2,7,6,	3,7,2,	6,4,5,	7,4,6,};
	srm.createDefaultBuffer(g_pd3dDevice.Get(), sizeof(WORD)*36, g_pIndexBuffer, indices, D3D11_BIND_INDEX_BUFFER);
	g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 );
	g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );// Set primitive topology
	
	// Create the constant buffer
	srm.createConstBufSRU(g_pd3dDevice.Get(), sizeof(ConstantBuffer), g_pConstantBuffer);
	g_World = XMMatrixIdentity(); // Initialize the world matrix

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 1.0f, -5.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, m_Width / (FLOAT)m_Height, 0.01f, 100.0f );
	return hr;
}

float Direct3D_DemoCube::getRelativeTime()
{
	static float t = 0.0f; // Update our time
	if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
		t += ( float )XM_PI * 0.0125f;
	else{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if( timeStart == 0 )
			timeStart = timeCur;
		t = ( timeCur - timeStart ) / 1000.0f;
	}
	return t;
}

void Direct3D_DemoCube::Render()
{	
	float t = getRelativeTime();
	g_World = XMMatrixRotationY( t ); // Animate the cube
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView.Get(), Colors::MidnightBlue ); // Clear the back buffer

	ConstantBuffer cb; // Update variables
	cb.mWorld = g_World; // XMMatrixTranspose( g_World );
	cb.mView =  XMMatrixTranspose( g_View );
	cb.mProjection = XMMatrixTranspose( g_Projection );
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );

	// Renders a triangle
	g_pImmediateContext->VSSetShader( g_pVertexShader.Get(), nullptr, 0 );
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, g_pConstantBuffer.GetAddressOf());
	g_pImmediateContext->PSSetShader( g_pPixelShader.Get(), nullptr, 0 );
	g_pImmediateContext->DrawIndexed( 36, 0, 0 );        // 36 vertices needed for 12 triangles in a triangle list

	// Present our back buffer to our front buffer
	g_pSwapChain->Present( 0, 0 );
}


HRESULT Direct3D_DemoCube2::InitDevice()
{
	HRESULT hr = S_OK;
	V_RETURN(Direct3D_DemoCube::InitDevice());
	CmSRM srm;
	D3D11_TEXTURE2D_DESC descDepth = srm.createTexture2dDescr(m_Width, m_Height, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
	V_RETURN(g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil));
	srm.createStencilView(g_pd3dDevice.Get(), g_pDepthStencil.Get(), g_pDepthStencilView, DXGI_FORMAT_D24_UNORM_S8_UINT);
	g_pImmediateContext->OMSetRenderTargets( 1, g_pRenderTargetView.GetAddressOf(), g_pDepthStencilView.Get());
	return hr;
}

void Direct3D_DemoCube2::Render()
{	
	// Update our time
	float t = getRelativeTime();
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView.Get(), Colors::MidnightBlue);// Clear the back buffer
	g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0 );// Clear the depth buffer to 1.0 (max depth)

	// Update variables for the first cube
	g_World = XMMatrixRotationY( t );
	ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose( g_World );
	cb1.mView = XMMatrixTranspose( g_View );
	cb1.mProjection = XMMatrixTranspose( g_Projection );
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0 );

	// Render the first cube
	g_pImmediateContext->VSSetShader( g_pVertexShader.Get(), nullptr, 0 );
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, g_pConstantBuffer.GetAddressOf());
	g_pImmediateContext->PSSetShader( g_pPixelShader.Get(), nullptr, 0 );
	g_pImmediateContext->DrawIndexed( 36, 0, 0 );

	// Update variables for the second cube
	XMMATRIX mSpin = XMMatrixRotationZ( -t );
	XMMATRIX mOrbit = XMMatrixRotationY( -t * 2.0f );
	XMMATRIX mTranslate = XMMatrixTranslation( -4.0f, 0.0f, 0.0f );
	XMMATRIX mScale = XMMatrixScaling( 0.3f, 0.3f, 0.3f );
	g_World2 = mScale * mSpin * mTranslate * mOrbit;
	ConstantBuffer cb2;
	cb2.mWorld = XMMatrixTranspose( g_World2 );
	cb2.mView = XMMatrixTranspose( g_View );
	cb2.mProjection = XMMatrixTranspose( g_Projection );
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer.Get(), 0, nullptr, &cb2, 0, 0 );
	g_pImmediateContext->DrawIndexed( 36, 0, 0 );// Render the second cube

	// Present our back buffer to our front buffer
	g_pSwapChain->Present( 0, 0 );
}