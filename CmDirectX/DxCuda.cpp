#include "StdAfx.h"
#include "DXUT.h"
#include "DxCuda.h"

struct ConstantBuffer{float vQuadRect[4]; };

// Data structure for 2D texture shared between DX10 and CUDA
//DxCudaTexture2D g_texture2d;

DxCudaTexture2D::DxCudaTexture2D()
	: pTexture(NULL)
	, pSRView(NULL)
	, cudaResource(NULL)
	, cudaLinearMemory(NULL)
	, width(256)
	, height(256)
	, offsetInShader(0)
{

}
DxCudaTexture2D::~DxCudaTexture2D()
{
	Release();
}

void DxCudaTexture2D::Release()
{
	SAFE_RELEASE(pSRView);
	SAFE_RELEASE(pTexture); 
}

// The CUDA kernel launchers that get called
extern "C"
{
	bool cuda_texture_2d(void *surface, size_t width, size_t height, size_t pitch, float t);
}


DxCuda::DxCuda()
{
	
}

DxCuda::~DxCuda()
{
	
}

HRESULT DxCuda::InitDevice()
{
	HRESULT hr = S_OK;
	CmSRM::createCuDxDeviceAndSwapChain(g_hWnd, m_Width, m_Height, &g_pSwapChain, &g_pd3dDevice, &g_pImmediateContext);

	// Create a render target view of the swapchain
	ID3D11Texture2D *pBuffer;
	V_RETURN(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBuffer));
	V_RETURN(g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL, &g_pRenderTargetView));
	pBuffer->Release();
	g_pImmediateContext->OMSetRenderTargets(1, g_pRenderTargetView.GetAddressOf(), NULL);

	// Setup the viewport
	D3D11_VIEWPORT vp = {0.f, 0.f, (float)m_Width, (float)m_Height, 0.f, 1.f};
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Vertex shader & Pixel shader
	CmSRM srm;
	srm.loadVertexShaderOnly(L"VertexShader.cso", g_pd3dDevice.Get(), g_pVertexShader.GetAddressOf());
	g_pImmediateContext->VSSetShader(g_pVertexShader.Get(), NULL, 0);
	srm.loadPixelShader(L"PixelShader.cso", g_pd3dDevice.Get(), g_pPixelShader);
	g_pImmediateContext->PSSetShader(g_pPixelShader.Get(), NULL, 0);
	srm.createConstBuf(g_pd3dDevice.Get(), 16 * ((sizeof(ConstantBuffer) + 15) / 16), g_pConstantBuffer);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, g_pConstantBuffer.GetAddressOf());
	g_pImmediateContext->PSSetConstantBuffers(0, 1, g_pConstantBuffer.GetAddressOf());


	// Setup  no Input Layout
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	D3D11_RASTERIZER_DESC rasterizerState = {D3D11_FILL_SOLID, D3D11_CULL_FRONT, FALSE, 0, 0, 0, FALSE, FALSE, FALSE, FALSE};
	g_pd3dDevice->CreateRasterizerState(&rasterizerState, &g_pRasterState);
	g_pImmediateContext->RSSetState(g_pRasterState.Get());

	// begin interop
	cudaD3D11SetDirect3DDevice(g_pd3dDevice.Get());
	getLastCudaError("cudaD3D11SetDirect3DDevice failed");

	// Initial textures
	V_RETURN(CmSRM::creatTexture2D(g_pd3dDevice.Get(), g_texture_2d.pTexture, &g_texture_2d.pSRView, NULL, g_texture_2d.width, g_texture_2d.height));
	g_pImmediateContext->PSSetShaderResources(g_texture_2d.offsetInShader, 1, &g_texture_2d.pSRView);
	
	// Cuda registration
	cudaGraphicsD3D11RegisterResource(&g_texture_2d.cudaResource, g_texture_2d.pTexture, cudaGraphicsRegisterFlagsNone);
	getLastCudaError("cudaGraphicsD3D11RegisterResource (g_texture_2d) failed");
	cudaMallocPitch(&g_texture_2d.cudaLinearMemory, &g_texture_2d.pitch, g_texture_2d.width * sizeof(float) * 4, g_texture_2d.height);
	getLastCudaError("cudaMallocPitch (g_texture_2d) failed");
	cudaMemset(g_texture_2d.cudaLinearMemory, 1, g_texture_2d.pitch * g_texture_2d.height);

	return hr;
}

void DxCuda::RunKernels()
{
	static float t = 0.0f;

	// populate the 2d texture
	{
		cudaArray *cuArray;
		cudaGraphicsSubResourceGetMappedArray(&cuArray, g_texture_2d.cudaResource, 0, 0);
		getLastCudaError("cudaGraphicsSubResourceGetMappedArray (cuda_texture_2d) failed");

		// kick off the kernel and send the staging buffer cudaLinearMemory as an argument to allow the kernel to write to it
		cuda_texture_2d(g_texture_2d.cudaLinearMemory, g_texture_2d.width, g_texture_2d.height, g_texture_2d.pitch, t);
		getLastCudaError("cuda_texture_2d failed");

		// then we want to copy cudaLinearMemory to the D3D texture, via its mapped form : cudaArray
		cudaMemcpy2DToArray(
			cuArray, // dst array
			0, 0,    // offset
			g_texture_2d.cudaLinearMemory, g_texture_2d.pitch,       // src
			g_texture_2d.width*4*sizeof(float), g_texture_2d.height, // extent
			cudaMemcpyDeviceToDevice); // kind
		getLastCudaError("cudaMemcpy2DToArray failed");
	}

	t += 0.1f;
}

void DxCuda::Render()
{
	cudaStream_t    stream = 0;
	const int nbResources = 1;
	cudaGraphicsResource *ppResources[nbResources] = {g_texture_2d.cudaResource};
	cudaGraphicsMapResources(nbResources, ppResources, stream);
	getLastCudaError("cudaGraphicsMapResources(3) failed");

	RunKernels();

	// unmap the resources
	cudaGraphicsUnmapResources(nbResources, ppResources, stream);
	getLastCudaError("cudaGraphicsUnmapResources(3) failed");  

	// Just clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView.Get(), Colors::MidnightBlue );
	float quadRect[4] = { -0.9f, -0.9f, 1.f , 1.f };
	HRESULT hr = S_OK;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ConstantBuffer *pcb;
	VV(g_pImmediateContext->Map(g_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	pcb = (ConstantBuffer *) mappedResource.pData; 
	memcpy(pcb->vQuadRect, quadRect, sizeof(float)*4);
	g_pImmediateContext->Unmap(g_pConstantBuffer.Get(), 0);
	g_pImmediateContext->Draw(4, 0);

	g_pSwapChain->Present( 0, 0 );
}