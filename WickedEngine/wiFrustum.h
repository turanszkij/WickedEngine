#pragma once
#include "CommonInclude.h"

class Frustum
{
private:
	XMFLOAT4 m_planesNorm[6];
	XMFLOAT4 m_planes[6];
	XMFLOAT4X4 view;
public:
	Frustum();
	void CleanUp();

	void ConstructFrustum(float screenDepth, XMFLOAT4X4 projectionMatrix, XMFLOAT4X4 viewMatrix);

	bool CheckPoint(const XMFLOAT3&);
	bool CheckSphere(const XMFLOAT3&, float);

#define BOX_FRUSTUM_INTERSECTS 1
#define BOX_FRUSTUM_INSIDE 2
	int CheckBox(XMFLOAT3[8]);

	XMFLOAT4 getFarPlane();
	XMFLOAT4 getNearPlane();
	XMFLOAT3 getCamPos();
};

