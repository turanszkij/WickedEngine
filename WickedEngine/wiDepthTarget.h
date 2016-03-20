#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiDepthTarget
{
private:
	wiGraphicsTypes::Texture2D*		texture;
	wiGraphicsTypes::TextureCube*	textureCube;
	bool isCube;
public:
	//Texture2D			texture2D;
	//DepthStencilView	depthTarget;
	//Texture2D*			shaderResource;

	//ShaderResourceViewDesc shaderResourceViewDesc;


	wiDepthTarget();
	~wiDepthTarget();
	
	void Initialize(int width, int height, UINT MSAAC, UINT MSAAQ);
	void InitializeCube(int size);
	void Clear(GRAPHICSTHREAD threadID);
	void CopyFrom(const wiDepthTarget&, GRAPHICSTHREAD threadID);

	wiGraphicsTypes::Texture2D* GetTexture() const { return(isCube ? textureCube : texture); }
	wiGraphicsTypes::Texture2DDesc GetDesc() const { return GetTexture()->desc; }
};

