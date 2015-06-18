#pragma once
#include "WickedEngine.h"

class DepthTarget
{
public:
	ID3D11Texture2D*			texture2D;
	ID3D11DepthStencilView*		depthTarget;
	ID3D11ShaderResourceView*	shaderResource;

	DepthTarget();
	~DepthTarget();
	
	void Initialize(int width, int height, UINT MSAAC, UINT MSAAQ);
	void InitializeCube(int size);
	void Clear(ID3D11DeviceContext*);
	void CopyFrom(const DepthTarget&, ID3D11DeviceContext*);
};

