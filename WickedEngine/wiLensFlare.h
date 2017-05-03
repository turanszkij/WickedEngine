#ifndef LENSFLARE
#define LENSFLARE
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ConstantBufferMapping.h"

class wiLensFlare
{
private:
	static wiGraphicsTypes::GPUBuffer			*constantBuffer;
	static wiGraphicsTypes::PixelShader			*pixelShader;
	static wiGraphicsTypes::GeometryShader		*geometryShader;
	static wiGraphicsTypes::VertexShader		*vertexShader;
	static wiGraphicsTypes::VertexLayout		*inputLayout;
	static wiGraphicsTypes::RasterizerState		*rasterizerState;
	static wiGraphicsTypes::DepthStencilState	*depthStencilState;
	static wiGraphicsTypes::BlendState			*blendState;

	GFX_STRUCT ConstantBuffer
	{
		XMVECTOR mSunPos;
		XMFLOAT4 mScreen;

		CB_SETBINDSLOT(CBSLOT_OTHER_LENSFLARE)

		ALIGN_16
	};

public:
	static void LoadShaders();
private:
	static void SetUpStates();
	static void SetUpCB();
public:
	static void Initialize();
	static void CleanUp();
	static void Draw(GRAPHICSTHREAD threadID, const XMVECTOR& lightPos, std::vector<wiGraphicsTypes::Texture2D*>& rims);
};

#endif
