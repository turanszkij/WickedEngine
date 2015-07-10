#include "wiCamera.h"



Camera::Camera(int width, int height, float newNear, float newFar, const XMVECTOR& newEye)
{
	zNearP=newNear;
	zFarP=newFar;

	this->width=width;
	this->height=height;

#ifndef WINSTORE_SUPPORT
	GetCursorPos(&originalMouse);
#endif

    //Initialize the projection matrix
	Projection = XMMatrixPerspectiveFovLH( XM_PI / 3.0f, width / (FLOAT)height, zNearP, zFarP );
	//Projection = XMMatrixOrthographicLH(RENDERWIDTH,RENDERHEIGHT,0.1f,10000.0f);
	Oprojection = XMMatrixOrthographicLH(width,height,0.1f,5000.0f);

	//View Matrix
	defaultEye = Eye = newEye;
	At = XMVectorAdd(Eye , XMVectorSet( 0.0f, -1, 0, 0.0f ));
	Up = XMVectorAdd(Eye , XMVectorSet( 0.0f, 0, 1, 0.0f ));
	defaultView = View = XMMatrixLookAtLH(Eye,At,Up);
	refView=prevView=View;


	leftrightRot=0;
	updownRot=0;
}


void Camera::ProcessInput(float t, bool enter)
{

#ifndef WINSTORE_SUPPORT
	POINT currentMouse;
	GetCursorPos(&currentMouse);
	if(enter)
	{
		LONG xDif = currentMouse.x-originalMouse.x;
		LONG yDif = currentMouse.y-originalMouse.y;
		leftrightRot+=0.1f*xDif*t;
		updownRot+=0.1f*yDif*t;
		SetCursorPos(originalMouse.x,originalMouse.y);

	}
	else
	{
		GetCursorPos(&originalMouse);
	}
#endif
	
	
	XMMATRIX cameraRot = XMMatrixRotationX(updownRot-XM_PI/2.0f)*XMMatrixRotationY(leftrightRot);

	XMVECTOR cameraOrigTarg=XMVectorSet(0,-1,0,0);
	At=XMVector3Transform(cameraOrigTarg,cameraRot);
	XMVECTOR cameraFinalTarg=XMVectorAdd(Eye,At);
	
	

	XMVECTOR cameraOrigUp=XMVectorSet(0,0,1,0);
	Up=XMVector3Transform(cameraOrigUp,cameraRot);
	prevView=View;
	View = XMMatrixLookAtLH( Eye, cameraFinalTarg, Up );



	XMFLOAT4 refEyeHelper;
	XMStoreFloat4(&refEyeHelper,Eye);
	refEyeHelper.y*=-1.0f;
	refEye = XMVectorSet( refEyeHelper.x, refEyeHelper.y, refEyeHelper.z, refEyeHelper.w );

	
	XMFLOAT4 refTarHelper;
	XMStoreFloat4( &refTarHelper, cameraFinalTarg );
	refTarHelper.y*=-1.0f;
	XMVECTOR refTarget = XMVectorSet(refTarHelper.x,refTarHelper.y,refTarHelper.z,refTarHelper.w);

	XMVECTOR camRight = XMVector3Transform(XMVectorSet(1,0,0,0),cameraRot);
	XMVECTOR invUp = XMVector3Cross(camRight,XMVectorSubtract(refTarget,refEye));


	refView = XMMatrixLookAtLH( refEye, refTarget, invUp );

}


void Camera::AddtoCameraPosition(const XMVECTOR& movevector)
{
	XMMATRIX cameraRot=XMMatrixRotationX(updownRot)*XMMatrixRotationY(leftrightRot);
	XMVECTOR rotVect=XMVector3Transform(movevector * 0.1f,cameraRot);

	Eye += rotVect;
}

void Camera::Reset()
{
	leftrightRot=0;
	updownRot=0;
	Eye = defaultEye;
	XMVECTOR At = XMVectorSet( 0.0f, 270, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	View = XMMatrixLookAtLH(Eye,At,Up);
}

void Camera::CameraRelease()
{
}

void* Camera::operator new(size_t size)
{
	void* result = _aligned_malloc(size,16);
	return result;
}
void Camera::operator delete(void* p)
{
	if(p) _aligned_free(p);
}
