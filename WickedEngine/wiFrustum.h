#pragma once
#include "CommonInclude.h"

struct AABB;

class Frustum
{
private:
	XMFLOAT4 m_planesNorm[6];
	XMFLOAT4 m_planes[6];
	XMFLOAT4X4 view;
public:
	Frustum();
	void CleanUp();

	void ConstructFrustum(float screenDepth, XMFLOAT4X4 projectionMatrix, const XMFLOAT4X4& viewMatrix, const XMMATRIX& world = XMMatrixIdentity());

	bool CheckPoint(const XMFLOAT3&);
	bool CheckSphere(const XMFLOAT3&, float);

#define BOX_FRUSTUM_INTERSECTS 1
#define BOX_FRUSTUM_INSIDE 2
	int CheckBox(const AABB& box);

	const XMFLOAT4& getLeftPlane();
	const XMFLOAT4& getRightPlane();
	const XMFLOAT4& getTopPlane();
	const XMFLOAT4& getBottomPlane();
	const XMFLOAT4& getFarPlane();
	const XMFLOAT4& getNearPlane();
	XMFLOAT3 getCamPos();
};

