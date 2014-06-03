#pragma once


// This header includes all the necessary D3D11 and CUDA includes
#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>


struct DxCudaTexture2D
{
	ID3D11Texture2D         *pTexture;
	ID3D11ShaderResourceView *pSRView;
	cudaGraphicsResource    *cudaResource;
	void                    *cudaLinearMemory;
	size_t                  pitch;
	int                     width;
	int                     height;
	int                     offsetInShader;

	DxCudaTexture2D();
	~DxCudaTexture2D();
	void Release();
};


class DxCuda: public Direct3DBase
{
public:
	DxCuda();
	~DxCuda();

	virtual HRESULT InitDevice();
	virtual void Render();

	void RunKernels();

private:
	DxCudaTexture2D g_texture_2d;
	ComPtr<ID3D11RasterizerState>  g_pRasterState;

};
