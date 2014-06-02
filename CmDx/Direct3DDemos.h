#pragma once
#include "Direct3DBase.h"

class Direct3D_DemoTriangle:public Direct3DBase
{
public:
	Direct3D_DemoTriangle(void);

	virtual HRESULT InitDevice();
	virtual void Render();
};

class Direct3D_DemoCube:public Direct3DBase
{
protected:
	struct SimpleVertex	{DirectX::XMFLOAT3 Pos; DirectX::XMFLOAT4 Color;};
	struct ConstantBuffer{DirectX::XMMATRIX mWorld, mView, mProjection;};

public:
	virtual HRESULT InitDevice();
	virtual void Render();	
	float getRelativeTime();
};

class Direct3D_DemoCube2:public Direct3D_DemoCube
{
	DirectX::XMMATRIX g_World2;
public:
	virtual HRESULT InitDevice();
	virtual void Render();	
};
