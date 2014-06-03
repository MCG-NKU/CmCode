#pragma once

#include <vector>
#include <algorithm>
#include <list>
using namespace std;

//#include <atlcomcli.h>

#include <dxgi.h>

#include "../DXUT11/DXUT.h"
#include "../DXUT11/DXUTgui.h"
#include "../DXUT11/DXUTmisc.h"
#include "../DXUT11/DXUTCamera.h"
#include "../DXUT11/DXUTSettingsDlg.h"
#include "../DXUT11/SDKmisc.h"

#include "../DirectXTK/Inc/CommonStates.h"
#include "../DirectXTK/Inc/DDSTextureLoader.h"
#include "../DirectXTK/Inc/Effects.h"
#include "../DirectXTK/Inc/GeometricPrimitive.h"
#include "../DirectXTK/Inc/Model.h"
#include "../DirectXTK/Inc/PrimitiveBatch.h"
#include "../DirectXTK/Inc/ScreenGrab.h"
#include "../DirectXTK/Inc/SpriteBatch.h"
#include "../DirectXTK/Inc/SpriteFont.h"
#include "../DirectXTK/Inc/VertexTypes.h"

using namespace DirectX;

#include <DirectXColors.h>
#include <directxmath.h>

typedef ID3D11ShaderResourceView DxSRV;
typedef ID3D11UnorderedAccessView DxUAV;
typedef ID3D11DeviceContext DxContext;
typedef ID3D11ComputeShader DxCShader;
typedef ID3D11PixelShader DxPsShader;
typedef ID3D11VertexShader DxVtShader;
typedef ID3D11Buffer DxBuffer;
typedef ID3D11Device DxDevice;
typedef ID3D11Texture2D DxTexture2D;
typedef ID3D11DepthStencilView DxStencil;
typedef DirectX::PrimitiveBatch<DirectX::VertexPositionColor> CmVertexPosClrBatch;

#include "CmSRM.h"
#include "CmDxBase.h"
#include "Direct3DDemos.h"
#include "StdLogger.h"


#ifndef lnkLIB
#ifdef _DEBUG
#define lnkLIB(name) name "d"
#else
#define lnkLIB(name) name
#endif
#endif

#pragma comment(lib, lnkLIB("DXUT11"))
#pragma comment(lib, lnkLIB("DirectXTK"))
#pragma comment(lib, lnkLIB("CmDx"))
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")

