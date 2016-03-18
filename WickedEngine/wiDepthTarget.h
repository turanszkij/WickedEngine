#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiDepthTarget
{
public:
	Texture2D			texture2D;
	DepthStencilView	depthTarget;
	TextureView			shaderResource;

	ShaderResourceViewDesc shaderResourceViewDesc;

	wiDepthTarget();
	~wiDepthTarget();
	
	void Initialize(int width, int height, UINT MSAAC, UINT MSAAQ);
	void InitializeCube(int size);
	void Clear(GRAPHICSTHREAD threadID);
	void CopyFrom(const wiDepthTarget&, GRAPHICSTHREAD threadID);
};

