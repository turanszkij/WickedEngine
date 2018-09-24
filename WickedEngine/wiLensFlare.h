#ifndef LENSFLARE
#define LENSFLARE
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiLensFlare
{
private:
	static wiGraphicsTypes::GPUBuffer			*constantBuffer;
	static wiGraphicsTypes::PixelShader			*pixelShader;
	static wiGraphicsTypes::GeometryShader		*geometryShader;
	static wiGraphicsTypes::VertexShader		*vertexShader;
	static wiGraphicsTypes::RasterizerState		rasterizerState;
	static wiGraphicsTypes::DepthStencilState	depthStencilState;
	static wiGraphicsTypes::BlendState			blendState;
	static wiGraphicsTypes::Sampler				*samplercmp;
	static wiGraphicsTypes::GraphicsPSO			PSO;

public:
	static void LoadShaders();
private:
	static void SetUpStates();
	static void SetUpCB();
public:
	static void Initialize();
	static void CleanUp();
	static void Draw(GRAPHICSTHREAD threadID, const XMVECTOR& lightPos, const std::vector<wiGraphicsTypes::Texture2D*>& rims);
};

#endif
