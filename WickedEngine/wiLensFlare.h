#ifndef LENSFLARE
#define LENSFLARE
#include "CommonInclude.h"

class wiLensFlare
{
private:
	static ID3D11Buffer* constantBuffer;
	static ID3D11PixelShader* pixelShader;
	static ID3D11GeometryShader* geometryShader;
	static ID3D11VertexShader* vertexShader;
	static ID3D11InputLayout* inputLayout;
	static ID3D11SamplerState* samplerState;
	static ID3D11RasterizerState* rasterizerState;
	static ID3D11DepthStencilState* depthStencilState;
	static ID3D11BlendState* blendState;

	struct ConstantBuffer
	{
		XMVECTOR mSunPos;
		XMFLOAT4 mScreen;
	};

	static void LoadShaders();
	static void SetUpStates();
	static void SetUpCB();
public:
	static void Initialize();
	static void CleanUp();
	static void Draw(ID3D11ShaderResourceView* depthMap, ID3D11DeviceContext* context, const XMVECTOR& lightPos
		, vector<ID3D11ShaderResourceView*>& rims);
};

#endif
