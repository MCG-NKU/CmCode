//--------------------------------------------------------------------------------------
// File: SimpleSampleTK.cpp
//
// Starting point for new Direct3D 11 Win32 desktop samples using DirectX Tool Kit
// and DXUT.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "DxCuda.h"


int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
	STD_LOGGER; 
	//CmDxBase cmDx;
	DxCuda cmDx;
	//Direct3D_DemoCube2 cmDx;
	return cmDx.exec(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
