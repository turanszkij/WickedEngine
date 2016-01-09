#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiDepthTarget
{
public:
	Texture2D			texture2D;
	DepthStencilView	depthTarget;
	TextureView			shaderResource;

	wiDepthTarget();
	~wiDepthTarget();
	
	void Initialize(int width, int height, UINT MSAAC, UINT MSAAQ);
	void InitializeCube(int size);
	void Clear(DeviceContext context);
	void CopyFrom(const wiDepthTarget&, DeviceContext);
};

