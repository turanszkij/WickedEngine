#pragma once
#include "WickedEngine.h"

class wiDepthTarget
{
public:
	ID3D11Texture2D*			texture2D;
	ID3D11DepthStencilView*		depthTarget;
	ID3D11ShaderResourceView*	shaderResource;

	wiDepthTarget();
	~wiDepthTarget();
	
	void Initialize(int width, int height, UINT MSAAC, UINT MSAAQ);
	void InitializeCube(int size);
	void Clear(ID3D11DeviceContext*);
	void CopyFrom(const wiDepthTarget&, ID3D11DeviceContext*);
};

