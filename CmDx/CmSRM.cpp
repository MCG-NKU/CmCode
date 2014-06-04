#include "stdafx.h"
#include "CmSRM.h"
#include <d3dcompiler.h>


UINT CmSRM::ZEROS[4] = {0,0,0,0};

void CmSRM::addSRV(int slotId, DxSRV* srv)
{
	assert(slotId >= 0 && slotId < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
	m_SRVs.push_back(make_pair(srv, slotId));
}


void CmSRM::addUAV(int slotId, DxUAV* uav, const UINT* pInitialCounts)
{
	assert(slotId >= 0 && slotId < D3D11_PS_CS_UAV_REGISTER_COUNT);
	m_UAVs.push_back(make_pair(uav, slotId));
	m_UAVInitials.push_back(pInitialCounts);
}

void CmSRM::addCB(int slotId, DxBuffer* const cb)
{
	assert(slotId >= 0 && slotId < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
	m_CBs.push_back(make_pair(cb, slotId));
}

// Requiring m_SRVs, m_UAVs, m_Buffers to be prepared before hand
void CmSRM::setAndRun(DxContext* context, DxCShader* cs, int dimX, int dimY)
{
	assert(slotsUnique(m_SRVs));
	assert(slotsUnique(m_UAVs));
	assert(slotsUnique(m_CBs));

	// set resource slots & shader
	for (size_t i = 0; i < m_SRVs.size(); i++)
		context->CSSetShaderResources(m_SRVs[i].second, 1, &m_SRVs[i].first);
	for (size_t i = 0; i < m_UAVs.size(); i++)
		context->CSSetUnorderedAccessViews(m_UAVs[i].second, 1, &m_UAVs[i].first, m_UAVInitials[i]);
	for (size_t i = 0; i < m_CBs.size(); i++)
		context->CSSetConstantBuffers(m_CBs[i].second, 1, &m_CBs[i].first);
	context->CSSetShader(cs, NULL, 0);

	// Run shader
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	context->Dispatch(dimX, dimY, 1);
}

// Requiring m_SRVs, m_UAVs, m_Buffers to be prepared before hand
void CmSRM::setRunAndRelease(DxContext* context, DxCShader* cs, int dimX, int dimY)
{
	setAndRun(context, cs, dimX, dimY);

	// Release resource slots and context shader
	DxSRV* nullSRV[] = { NULL};
	DxUAV* nullUAV[] = { NULL};
	DxBuffer* nullCB[] = { NULL };
	for (size_t i = 0; i < m_SRVs.size(); i++)
		context->CSSetShaderResources(m_SRVs[i].second, 1, nullSRV);
	for (size_t i = 0; i < m_UAVs.size(); i++)
		context->CSSetUnorderedAccessViews(m_UAVs[i].second, 1, nullUAV, NULL);
	for (size_t i = 0; i < m_CBs.size(); i++)
		context->CSSetConstantBuffers(m_CBs[i].second, 1, nullCB);
	context->CSSetShader(NULL, NULL, NULL);
	m_SRVs.clear();
	m_UAVs.clear();
	m_CBs.clear();
}

// Initialize an UAV with zeros and add to context
void CmSRM::addUAVWithReset(int slotId, DxUAV* uav, DxContext* context, const UINT* pInitialCounts)
{
	resetAnUAV(context, uav);
	addUAV(slotId, uav, pInitialCounts);
}

HRESULT CmSRM::loadShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, DxCShader** ppCS, DxPsShader** ppPS, DxVtShader** ppVS, ID3D11GeometryShader** ppGS)
{
	ID3DBlob* pBlob = nullptr;
	HRESULT hr = S_OK;
	V_RETURN(loadID3DBlob(csoFileName, pBlob));
	if (ppCS != NULL)
		V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppCS));
	if (ppPS != NULL)
		V_RETURN(pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppPS));
	if (ppVS != NULL)
		V_RETURN(pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppVS));
	if (ppGS != NULL)
		V_RETURN(pd3dDevice->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppGS));
	pBlob->Release();
	return hr;
}

//HRESULT CmSRM::loadComputeShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, DxCShader*& ppCS) {
//	return loadShader(csoFileName, pd3dDevice, &ppCS, NULL, NULL);
//}

HRESULT CmSRM::loadComputeShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ComPtr<DxCShader> &pCS){
	HRESULT hr = S_OK;
	V_RETURN(loadShader(csoFileName, pd3dDevice, pCS.ReleaseAndGetAddressOf(), NULL, NULL));
	return hr;
}

//HRESULT CmSRM::loadPixelShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, DxPsShader*& ppPS)     {
//	return loadShader(csoFileName, pd3dDevice, NULL, &ppPS, NULL);
//}

HRESULT CmSRM::loadPixelShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ComPtr<DxPsShader> &pPS){
	HRESULT hr = S_OK;
	V_RETURN(loadShader(csoFileName, pd3dDevice, NULL, pPS.ReleaseAndGetAddressOf(), NULL));
	return hr;
}

HRESULT CmSRM::loadGeometryShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ID3D11GeometryShader*& ppGS){
	return loadShader(csoFileName, pd3dDevice, NULL, NULL, NULL, &ppGS);
}

HRESULT CmSRM::loadVertexShaderOnly(LPCWSTR csoFileName, DxDevice* pd3dDevice, DxVtShader** ppVS){
	return loadShader(csoFileName, pd3dDevice, NULL, NULL, ppVS, NULL);
}

HRESULT CmSRM::loadVertexShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ComPtr<DxVtShader> &pVS, D3D11_INPUT_ELEMENT_DESC* layout, UINT numLayout, ComPtr<ID3D11InputLayout> &inputLayout){
	ID3DBlob* pBlob = nullptr;
	HRESULT hr = S_OK;
	V_RETURN(loadID3DBlob(csoFileName, pBlob));
	V_RETURN(pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pVS.ReleaseAndGetAddressOf()));
	V_RETURN(pd3dDevice->CreateInputLayout(layout, numLayout, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), inputLayout.ReleaseAndGetAddressOf()));
	pBlob->Release();
	return hr;
}

HRESULT CmSRM::loadID3DBlob(LPCWSTR csoFileName, ID3DBlob*& pVSBlob)
{
#ifdef DEBUG
	std::wstring compiledFilename(L"../x64/Debug/");
#else
	std::wstring compiledFilename(L"../x64/Release/");
#endif // DEBUG
	compiledFilename += csoFileName;
	HRESULT hr = D3DReadFileToBlob(compiledFilename.c_str(), &pVSBlob);
	if(FAILED(hr))
		MessageBox(nullptr, L"The cso file can't be loaded.", csoFileName, MB_OK );
	return hr;
}

// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476092(v=vs.85).aspx
HRESULT CmSRM::createConstBuf(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer**cbBuffer, D3D11_SUBRESOURCE_DATA *pInitialData)
{
	HRESULT hr = S_OK;
	// UINT ByteWidth; D3D11_USAGE Usage; UINT BindFlags; UINT CPUAccessFlags; UINT MiscFlags; UINT StructureByteStride;
	D3D11_BUFFER_DESC bDescCB;
	ZeroMemory(&bDescCB, sizeof(D3D11_BUFFER_DESC));
	bDescCB.BindFlags	= D3D11_BIND_CONSTANT_BUFFER;
	bDescCB.Usage		= D3D11_USAGE_DYNAMIC;
	bDescCB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bDescCB.ByteWidth	= byteWidth;  // should be updated
	V_RETURN(pd3dDevice->CreateBuffer(&bDescCB, pInitialData, cbBuffer));
	return hr;
}

HRESULT CmSRM::createConstBuf(DxDevice* pd3dDevice, UINT byteWidth, ComPtr<DxBuffer> &cbBuffer, D3D11_SUBRESOURCE_DATA *pInitialData)
{
	return createConstBuf(pd3dDevice, byteWidth, cbBuffer.ReleaseAndGetAddressOf(), pInitialData);
}

HRESULT CmSRM::createConstBufSRU(DxDevice* pd3dDevice, UINT byteWidth, ComPtr<DxBuffer> &cbBuffer)
{
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bDescCB;
	ZeroMemory(&bDescCB, sizeof(D3D11_BUFFER_DESC));
	bDescCB.BindFlags	= D3D11_BIND_CONSTANT_BUFFER;
	bDescCB.ByteWidth	= byteWidth;  // should be updated
	V_RETURN(pd3dDevice->CreateBuffer(&bDescCB, NULL, cbBuffer.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT CmSRM::createCpuReadBuf(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&cbBuffer, D3D11_SUBRESOURCE_DATA *pInitialData)
{
	HRESULT hr = S_OK;
	// UINT ByteWidth; D3D11_USAGE Usage; UINT BindFlags; UINT CPUAccessFlags; UINT MiscFlags; UINT StructureByteStride;
	D3D11_BUFFER_DESC bDescCB;
	ZeroMemory(&bDescCB, sizeof(D3D11_BUFFER_DESC));
	bDescCB.BindFlags	= 0;
	bDescCB.Usage		= D3D11_USAGE_STAGING;
	bDescCB.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	bDescCB.ByteWidth	= byteWidth;  // should be updated
	V_RETURN(pd3dDevice->CreateBuffer(&bDescCB, pInitialData, &cbBuffer));
	return hr;

}

HRESULT CmSRM::createBuffer(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, 
										  DxSRV **ppSRView, DxUAV **ppUAView, D3D11_SUBRESOURCE_DATA *pInitialData, 
										  DXGI_FORMAT fmt, UINT structureByteStride, UINT bufMiscFlags, UINT uavFlags)
{
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bDescBUF;
	ZeroMemory(&bDescBUF, sizeof(D3D11_BUFFER_DESC));
	bDescBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bDescBUF.ByteWidth	= byteWidth;
	bDescBUF.MiscFlags = bufMiscFlags;
	bDescBUF.StructureByteStride = structureByteStride;
	V_RETURN(pd3dDevice->CreateBuffer(&bDescBUF, pInitialData, &pBuffer));
	V_RETURN(createSRV_UAV(pd3dDevice, pBuffer, byteWidth/structureByteStride, ppSRView, ppUAView, fmt, uavFlags));
	return hr;
}

HRESULT CmSRM::createBufferEI(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, 
											DxSRV **ppSRView, DxUAV **ppUAView,  UINT structureByteStride, 
											DXGI_FORMAT fmt, void* iniSingleElementData, UINT bufMiscFlags, UINT uavFlags)
{
	if (iniSingleElementData == NULL)
		return createBuffer(pd3dDevice, byteWidth, pBuffer, ppSRView, ppUAView, NULL, fmt, structureByteStride, bufMiscFlags, uavFlags);

	D3D11_SUBRESOURCE_DATA initData = getIniSubResourceData(iniSingleElementData, byteWidth, structureByteStride);
	HRESULT hr = createBuffer(pd3dDevice, byteWidth, pBuffer, ppSRView, ppUAView, &initData, fmt, structureByteStride, bufMiscFlags, uavFlags);
	relaseIniSubResourceData(&initData);
	return hr;
}

HRESULT CmSRM::createBufferMI(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, 
	DxSRV **ppSRView, DxUAV **ppUAView, UINT structureByteStride, 
	DXGI_FORMAT fmt, void* initCpuMem, UINT bufMiscFlags, UINT uavFlags)
{
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA) );
	initData.pSysMem = initCpuMem;
	HRESULT hr = createBuffer(pd3dDevice, byteWidth, pBuffer, ppSRView, ppUAView, &initData, fmt, structureByteStride, bufMiscFlags, uavFlags);
	return hr;
}

HRESULT CmSRM::createBufferZI(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, 
											DxSRV **ppSRView, DxUAV **ppUAView,  DXGI_FORMAT fmt, 
											UINT structByteStride, UINT bufMiscFlags, UINT uavFlags)
{
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA) );
	byte* cpuMem = new byte[byteWidth];
	memset(cpuMem, 0, byteWidth);
	initData.pSysMem = cpuMem;
	HRESULT hr = createBuffer(pd3dDevice, byteWidth, pBuffer, ppSRView, ppUAView, &initData, fmt, structByteStride, bufMiscFlags, uavFlags);
	relaseIniSubResourceData(&initData);
	return hr;	
}


HRESULT CmSRM::createDefaultBuffer(DxDevice* pd3dDevice, UINT byteWidth, ComPtr<DxBuffer> &pBuffer, void* initCpuMem, UINT bindFlags)
{
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = byteWidth;
	bd.BindFlags = bindFlags;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = initCpuMem;
	V_RETURN(pd3dDevice->CreateBuffer( &bd, &InitData, pBuffer.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT CmSRM::creatTexture2D(DxDevice* pd3dDevice, DxTexture2D** texBuf, 
											DxSRV **ppSRView, DxUAV **ppUAView, UINT w, UINT h, DXGI_FORMAT fmt)
{
	D3D11_TEXTURE2D_DESC descTex = createTexture2dDescr(w, h, fmt);
	return creatTexture2D(pd3dDevice, texBuf, ppSRView, ppUAView, descTex);
}

HRESULT CmSRM::creatTexture2D(DxDevice* pd3dDevice, DxTexture2D** texBuf, 
											DxSRV **ppSRView, DxUAV **ppUAView, const D3D11_TEXTURE2D_DESC &descr)
{
	HRESULT hr = S_OK;
	V_RETURN(pd3dDevice->CreateTexture2D(&descr, NULL, texBuf));
	if (ppSRView != NULL)
		{V_RETURN(pd3dDevice->CreateShaderResourceView(*texBuf, NULL, ppSRView));}
	if (ppUAView != NULL)
		{V_RETURN(pd3dDevice->CreateUnorderedAccessView(*texBuf, NULL, ppUAView));}
	return hr;
}

HRESULT CmSRM::creatTexture2D(DxDevice* pd3dDevice, ComPtr<DxTexture2D> &texBuf, ComPtr<DxSRV> &ppSRView, ComPtr<DxUAV> &ppUAView, const D3D11_TEXTURE2D_DESC &descr)
{
	return creatTexture2D(pd3dDevice, texBuf.ReleaseAndGetAddressOf(), ppSRView.ReleaseAndGetAddressOf(), ppUAView.ReleaseAndGetAddressOf(), descr);
}

D3D11_TEXTURE2D_DESC CmSRM::createTexture2dDescr(UINT w, UINT h, DXGI_FORMAT fmt, UINT BindFlags, D3D11_USAGE usage, UINT cpuAccess)
{
	D3D11_TEXTURE2D_DESC descTex;
	ZeroMemory(&descTex, sizeof(D3D11_TEXTURE2D_DESC));
	descTex.Usage = usage;
	descTex.BindFlags = BindFlags;
	descTex.CPUAccessFlags = cpuAccess;
	//descTex.MiscFlags = 0;
	descTex.SampleDesc.Count = 1;
	//descTex.SampleDesc.Quality = 0;
	descTex.ArraySize = 1;
	descTex.MipLevels = 1;
	descTex.Format = fmt;
	descTex.Width = w;
	descTex.Height = h;
	return descTex;
}

HRESULT CmSRM::createStencilView(DxDevice* pd3dDevice, DxTexture2D* texBuf, ComPtr<DxStencil>& pStenView, DXGI_FORMAT fmt)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC) );
	descDSV.Format = fmt;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//descDSV.Texture2D.MipSlice = 0;
	HRESULT hr = S_OK;
	V_RETURN(pd3dDevice->CreateDepthStencilView(texBuf, &descDSV, pStenView.ReleaseAndGetAddressOf()));
	return hr;
	
}

HRESULT CmSRM::createTextureSRV(DxDevice* pd3dDevice, DxTexture2D* texBuf, DxSRV*& pSRV, DXGI_FORMAT fmt)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	ZeroMemory(&descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	descSRV.Format = fmt;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MipLevels = 1;
	//descSRV.Texture2D.MostDetailedMip = 0;
	HRESULT hr = S_OK;
	V_RETURN(pd3dDevice->CreateShaderResourceView(texBuf, &descSRV, &pSRV));
	return hr;
}

D3D11_BLEND_DESC CmSRM::getDefaultBlendDesc()
{
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	//blendDesc.AlphaToCoverageEnable = false;
	//blendDesc.IndependentBlendEnable = false;
	//blendDesc.RenderTarget[0].BlendEnable = false;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	return blendDesc;
}

HRESULT CmSRM::createCuDxDeviceAndSwapChain(HWND hWnd, UINT winW, UINT winH, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppImmediateContext)
{
	HRESULT hr = S_OK;
	ComPtr<IDXGIAdapter>   cuCapableAdapter;
	ComPtr<IDXGIFactory> pFactory;
	V_RETURN(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)(&pFactory)));
	V_RETURN(pFactory->EnumAdapters(0, &cuCapableAdapter));

	// Set up the structure used to create the device and swapchain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = winW;
	sd.BufferDesc.Height = winH;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL tour_fl[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
	D3D_FEATURE_LEVEL flRes;
	// Create device and swapchain
	return D3D11CreateDeviceAndSwapChain(cuCapableAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN,//D3D_DRIVER_TYPE_HARDWARE,
		NULL, 0, tour_fl, 3, D3D11_SDK_VERSION, &sd, ppSwapChain, ppDevice, &flRes, ppImmediateContext);
}

D3D11_SUBRESOURCE_DATA CmSRM::getIniSubResourceData(void* iniSingleElementData, UINT byteWidth, UINT structByteStride)
{
	D3D11_SUBRESOURCE_DATA initData;		
	ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA) );
	byte* cpuNull = new byte[byteWidth];
	int numEle = byteWidth / structByteStride;
	for (int i = 0; i < numEle; i++)
		memcpy(cpuNull + i*structByteStride, iniSingleElementData, structByteStride);
	initData.pSysMem = cpuNull;
	return initData;
}


HRESULT CmSRM::createSRV_UAV(DxDevice* pd3dDevice, DxBuffer*pBuffer, const UINT numElements, 
										   DxSRV **ppSRView, DxUAV **ppUAView, DXGI_FORMAT fmt, UINT uavFlags)
{
	HRESULT hr = S_OK;
	if (ppUAView != NULL){
		D3D11_SHADER_RESOURCE_VIEW_DESC bDescSRV;
		ZeroMemory(&bDescSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		bDescSRV.Format = fmt; //DXGI_FORMAT_R32_SINT;
		bDescSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		//bDescSRV.Buffer.FirstElement = 0;
		bDescSRV.Buffer.NumElements = numElements;
		V_RETURN(pd3dDevice->CreateShaderResourceView(pBuffer, &bDescSRV, ppSRView));
	}
	if (ppUAView != NULL){
		D3D11_UNORDERED_ACCESS_VIEW_DESC bDescUAV;
		ZeroMemory( &bDescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		bDescUAV.Format = fmt; //DXGI_FORMAT_R32_SINT;
		bDescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		bDescUAV.Buffer.Flags = uavFlags;
		bDescUAV.Buffer.NumElements =  numElements;
		V_RETURN(pd3dDevice->CreateUnorderedAccessView(pBuffer, &bDescUAV, ppUAView));
	}
	return hr;
}

void CmSRM::setUavIniCount(int slotId, DxUAV*& pUAV, const UINT initCount)
{
	DXUTGetD3D11DeviceContext()->CSSetUnorderedAccessViews(slotId, 1, &pUAV, &initCount);
	DxUAV* nullUAV[] = {NULL};
	DXUTGetD3D11DeviceContext()->CSSetUnorderedAccessViews(slotId, 1, nullUAV, NULL);
}