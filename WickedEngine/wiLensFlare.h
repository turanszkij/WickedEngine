#ifndef LENSFLARE
#define LENSFLARE
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiLensFlare
{
private:
	static BufferResource constantBuffer;
	static PixelShader pixelShader;
	static GeometryShader geometryShader;
	static VertexShader vertexShader;
	static VertexLayout inputLayout;
	static RasterizerState rasterizerState;
	static DepthStencilState depthStencilState;
	static BlendState blendState;

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
	static void Draw(TextureView depthMap, DeviceContext context, const XMVECTOR& lightPos
		, vector<TextureView>& rims);
};

#endif
