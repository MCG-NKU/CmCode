#pragma once

class CmEffectFactory : public DirectX::EffectFactory
{
public:
	CmEffectFactory(_In_ ID3D11Device* device);
	virtual void CreateTexture(_In_z_ const WCHAR* name, _In_opt_ ID3D11DeviceContext* deviceContext, _Outptr_ ID3D11ShaderResourceView** textureView ) override;
	void SetPath(const WCHAR* path);

private:
	WCHAR searchPath[ MAX_PATH ];
};

	
class CmDxBase
{
public:
	CmDxBase(void);
	~CmDxBase(void);

	virtual int exec(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);
	
protected:
	virtual void InitApp();
	virtual void RenderText();
	virtual void DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color);

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing);
	virtual HRESULT OnCreateDevice(DxDevice* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);
	virtual HRESULT OnResizedSwapChain(DxDevice* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);

	virtual void OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown);
	virtual void OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl);
	virtual void OnFrameMove(double fTime, float fElapsedTime);
	virtual bool ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings);

	virtual bool IsDeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed);
	virtual void OnDestroyDevice();
	virtual void OnRender(DxDevice* pd3dDevice, DxContext* pd3dImmediateContext, double fTime, float fElapsedTime);

	
protected:	
	CModelViewerCamera          _Camera;           // A model viewing camera

	std::unique_ptr<CommonStates>		_States;
	std::unique_ptr<BasicEffect>		_BatchEffect;
	std::unique_ptr<CmEffectFactory>	_FXFactory;
	std::unique_ptr<GeometricPrimitive> _Shape;
	std::unique_ptr<Model>				_Model;
	std::unique_ptr<CmVertexPosClrBatch>_Batch;
	std::unique_ptr<SpriteBatch>		_Sprites;
	std::unique_ptr<SpriteFont>			_Font; 
	std::unique_ptr<CDXUTTextHelper>	_pTxtHelper;


	ComPtr<ID3D11ShaderResourceView>   _pTextureRV1;
	ComPtr<ID3D11ShaderResourceView>   _pTextureRV2;
	ComPtr<ID3D11InputLayout>	_pBatchInputLayout;

	CDXUTDialog		_SampleUI;             // dialog for sample specific controls
	CD3DSettingsDlg	_SettingsDlg;          // Device settings dialog
	CDXUTDialog		_HUD;                  // dialog for standard controls
	static CDXUTDialogResourceManager  s_DlgRscManager; // manager for shared resources of dialogs

protected:
	int _winW, _winH; // Width and height of the DirectX window
	int _txtX, _txtY; // Position of the text
	wstring _winTitle;

protected: // Callbacks
	static LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);
	static void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
	static void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
	static void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
	static bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
	static bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
	static HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static void CALLBACK OnD3D11DestroyDevice(void* pUserContext);
	static HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext);
	static void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext);
	
	static inline RECT getRect(int x, int y, int width, int height){RECT tagR = {x, y, x + width, y + height};	return tagR;} 

	enum {IDC_TOGGLEFULLSCREEN = 1, IDC_TOGGLEREF, IDC_CHANGEDEVICE, IDC_TOGGLEWARP, IDC_STATIC, IDC_GROUP, IDC_SLIDER, IDC_SLIDER_STATIC};
};

#pragma warning(disable:4100)