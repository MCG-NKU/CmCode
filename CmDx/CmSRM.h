#pragma once

#include <wrl.h> 
using namespace Microsoft::WRL; //Microsoft::WRL::ComPtr

class CmSRM
{
public: // Manage shaders
	void addSRV(int slotId, DxSRV* srv); // Shader Resource View
	void addUAV(int slotId, DxUAV* uav, const UINT* pInitialCounts = NULL); // Unordered Access View
	void addUAVWithReset(int slotId, DxUAV* uav, DxContext* context, const UINT* pInitialCounts = NULL); // Initialize an UAV with zeros and add to context
	void addCB(int slotId, DxBuffer* const cb); // Constant Buffer

	// Requiring m_SRVs, m_UAVs, m_Buffers to be prepared before hand
	void setRunAndRelease(DxContext* context, DxCShader* cs, int dimX, int dimY);
	void setAndRun(DxContext* context, DxCShader* cs, int dimX, int dimY);

	static inline UINT getDim(UINT targetVal, UINT baseVal) {return (targetVal + baseVal - 1)/baseVal;}
	static inline void resetAnUAV(DxContext* context, DxUAV* uav) {context->ClearUnorderedAccessViewUint(uav, ZEROS);}

	static HRESULT loadComputeShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ComPtr<DxCShader> &pCS);
	static HRESULT loadPixelShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ComPtr<DxPsShader> &pPS); 
	static HRESULT loadVertexShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ComPtr<DxVtShader> &pVS, D3D11_INPUT_ELEMENT_DESC* layout, UINT numLayout, ComPtr<ID3D11InputLayout> &inputLayout);

	static HRESULT loadVertexShaderOnly(LPCWSTR csoFileName, DxDevice* pd3dDevice, DxVtShader** ppVS);
	static HRESULT loadGeometryShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, ID3D11GeometryShader*& ppGS);

	static HRESULT loadID3DBlob(LPCWSTR csoFileName, ID3DBlob*& pVSBlob);

public: // Create buffers
	static HRESULT createConstBuf(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer**cbBuffer, D3D11_SUBRESOURCE_DATA *pInitialData = NULL);
	static HRESULT createConstBuf(DxDevice* pd3dDevice, UINT byteWidth, ComPtr<DxBuffer> &cbBuffer, D3D11_SUBRESOURCE_DATA *pInitialData = NULL);
	static HRESULT createConstBufSRU(DxDevice* pd3dDevice, UINT byteWidth, ComPtr<DxBuffer> &cbBuffer); // SubResource for Update

	static HRESULT createCpuReadBuf(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&cbBuffer, D3D11_SUBRESOURCE_DATA *pInitialData = NULL);

	// Create a buffer initialized by standard D3D11_SUBRESOURCE_DATA
	static HRESULT createBuffer(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, DxSRV **ppSRView, 
		DxUAV **ppUAView, D3D11_SUBRESOURCE_DATA *pInitialData = NULL, 
		DXGI_FORMAT fmt = DXGI_FORMAT_R32_SINT, UINT structureByteStride = sizeof(int),
		UINT bufMiscFlags = 0, UINT uavFlags = 0);

	// Create a buffer with an element as initial (Element Initial, EI)
	static HRESULT createBufferEI(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, DxSRV **ppSRView, 
		DxUAV **ppUAView,  UINT structureByteStride = sizeof(int),
		DXGI_FORMAT fmt = DXGI_FORMAT_R32_SINT, void* iniSingleElementData = NULL,
		UINT bufMiscFlags = 0, UINT uavFlags = 0);

	// Create a buffer with initial elements values zero. (Zeror Initial, ZI)
	static HRESULT createBufferZI(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, DxSRV **ppSRView, 
		DxUAV **ppUAView,  DXGI_FORMAT fmt = DXGI_FORMAT_R32_SINT, UINT structureByteStride = sizeof(int),
		UINT bufMiscFlags = 0, UINT uavFlags = 0);

	// Create a buffer with cup memory initial (Memory Initial: MI)
	static HRESULT createBufferMI(DxDevice* pd3dDevice, UINT byteWidth, DxBuffer*&pBuffer, DxSRV **ppSRView, 
		DxUAV **ppUAView,  UINT structureByteStride = sizeof(int),
		DXGI_FORMAT fmt = DXGI_FORMAT_R32_SINT, void* initCpuMem = NULL,
		UINT bufMiscFlags = 0, UINT uavFlags = 0);

	static HRESULT createDefaultBuffer(DxDevice* pd3dDevice, UINT byteWidth, ComPtr<DxBuffer> &pBuffer, void* initCpuMem = NULL, UINT bindFlags = D3D11_BIND_VERTEX_BUFFER);

	// Must be released after use
	static D3D11_SUBRESOURCE_DATA getIniSubResourceData(void* iniSingleElementData, UINT byteWidth, UINT structByteStride = sizeof(int));
	static void relaseIniSubResourceData(D3D11_SUBRESOURCE_DATA *iniSubResD) {SAFE_DELETE_ARRAY(iniSubResD->pSysMem);}

	static HRESULT createSRV_UAV(DxDevice* pd3dDevice, DxBuffer*pBuffer, const UINT numElements,
		DxSRV **ppSRView, DxUAV **ppUAView, DXGI_FORMAT fmt = DXGI_FORMAT_R32_SINT, UINT uavFlags = 0);

	static void setUavIniCount(int slotId, DxUAV*& pUAV, const UINT initCount);

	static HRESULT creatTexture2D(DxDevice* pd3dDevice, DxTexture2D** texBuf, DxSRV **ppSRView, 
		DxUAV **ppUAView, UINT w, UINT h, DXGI_FORMAT fmt = DXGI_FORMAT_R32G32B32A32_FLOAT);

	static HRESULT creatTexture2D(DxDevice* pd3dDevice, DxTexture2D** texBuf, DxSRV **ppSRView, 
		DxUAV **ppUAView, const D3D11_TEXTURE2D_DESC &descr);

	static HRESULT creatTexture2D(DxDevice* pd3dDevice, ComPtr<DxTexture2D> &texBuf, ComPtr<DxSRV> &ppSRView, 
		ComPtr<DxUAV> &ppUAView, const D3D11_TEXTURE2D_DESC &descr);

	static D3D11_TEXTURE2D_DESC createTexture2dDescr(UINT w, UINT h, DXGI_FORMAT fmt = DXGI_FORMAT_R32G32B32A32_FLOAT, 
		UINT BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT, UINT cpuAccess = 0);

	static HRESULT createStencilView(DxDevice* pd3dDevice, DxTexture2D* texBuf, ComPtr<DxStencil>& pStenView, DXGI_FORMAT fmt = DXGI_FORMAT_D32_FLOAT);
	static HRESULT createTextureSRV(DxDevice* pd3dDevice, DxTexture2D* texBuf, DxSRV*& pSRV, DXGI_FORMAT fmt = DXGI_FORMAT_R32_FLOAT);

	static D3D11_BLEND_DESC getDefaultBlendDesc();

	static HRESULT createCuDxDeviceAndSwapChain(HWND hWnd, UINT winW, UINT winH, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppImmediateContext);

private: // For setting and releasing resources
	vector<pair<DxSRV*, int>> m_SRVs;
	vector<pair<DxUAV*, int>> m_UAVs;
	vector<const UINT*> m_UAVInitials;
	vector<pair<DxBuffer*, int>> m_CBs;

	template<class T> bool slotsUnique(vector<pair<T, int>> &resources){
		list<int> ids;
		for (size_t i = 0; i < resources.size(); i++)
			ids.push_back(resources[i].second);
		ids.unique();
		return ids.size() == resources.size();
	}
	static UINT ZEROS[4];

	static HRESULT loadShader(LPCWSTR csoFileName, DxDevice* pd3dDevice, DxCShader** ppCS = NULL, DxPsShader** ppPS = NULL, DxVtShader** ppVS = NULL, ID3D11GeometryShader** ppGS = NULL);
};
