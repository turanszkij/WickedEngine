#ifndef MAIN_CAMERA_H
#define MAIN_CAMERA_H

#include "CommonInclude.h"

class Camera
{
private:
	POINT					originalMouse;
public:
	XMMATRIX                View, refView, prevView;
	XMMATRIX                Projection, Oprojection;
	XMVECTOR				Eye, refEye;
	XMVECTOR				Up,At;
	float leftrightRot;
	float updownRot;
	float width,height;
	float zNearP, zFarP;
	
	XMVECTOR defaultEye;
	XMMATRIX defaultView;

	Camera(int newWidth,int newHeight, float newNear, float newFar, const XMVECTOR&);
	void* operator new(size_t);
	void operator delete(void*);

	void ProcessInput(float, bool);
	void AddtoCameraPosition(const XMVECTOR&);
	void Reset();
	void CameraRelease();

	void SetDefaultPosition(const XMVECTOR& newDefaultEye){ defaultEye = newDefaultEye; Reset(); }

}; 
#endif


