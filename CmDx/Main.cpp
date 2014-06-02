// CmDx.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Direct3DBase.h"
#include "Direct3DDemos.h"


//*
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{	
	STD_LOGGER; 
	
	//Direct3DBase demoDx;
	//Direct3D_DemoTriangle demoDx;
	//Direct3D_DemoCube demoDx;
	Direct3D_DemoCube2 demoDx;
	return demoDx.exec(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}//*/

