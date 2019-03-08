#include "wiLensFlare.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ResourceMapping.h"
#include "ShaderInterop_Renderer.h"
#include "wiBackLog.h"

using namespace wiGraphics;

namespace wiLensFlare
{
	static GPUBuffer constantBuffer;
	static const PixelShader *pixelShader = nullptr;
	static const GeometryShader *geometryShader = nullptr;
	static const VertexShader *vertexShader = nullptr;
	static Sampler samplercmp;
	static RasterizerState rasterizerState;
	static DepthStencilState depthStencilState;
	static BlendState blendState;
	static GraphicsPSO	PSO;

	void Draw(GRAPHICSTHREAD threadID, const XMVECTOR& lightPos, const std::vector<const Texture2D*>& rims) {

		if (!rims.empty())
		{
			GraphicsDevice* device = wiRenderer::GetDevice();
			device->EventBegin("LensFlare", threadID);

			device->BindGraphicsPSO(&PSO, threadID);

			LensFlareCB cb;
			XMStoreFloat4(&cb.xSunPos, lightPos / XMVectorSet((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 1, 1));
			cb.xScreen = XMFLOAT4((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0, 0);

			device->UpdateBuffer(&constantBuffer, &cb, threadID);
			device->BindConstantBuffer(GS, &constantBuffer, CB_GETBINDSLOT(LensFlareCB), threadID);


			int i = 0;
			for (const Texture2D* x : rims)
			{
				if (x != nullptr)
				{
					device->BindResource(PS, x, TEXSLOT_ONDEMAND0 + i, threadID);
					device->BindResource(GS, x, TEXSLOT_ONDEMAND0 + i, threadID);
					i++;
				}
			}

			device->BindSampler(GS, &samplercmp, SSLOT_ONDEMAND0, threadID);


			device->Draw(i, 0, threadID);


			device->EventEnd(threadID);
		}
	}

	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		vertexShader = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(path + "lensFlareVS.cso", wiResourceManager::VERTEXSHADER));

		pixelShader = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "lensFlarePS.cso", wiResourceManager::PIXELSHADER));

		geometryShader = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(path + "lensFlareGS.cso", wiResourceManager::GEOMETRYSHADER));


		GraphicsDevice* device = wiRenderer::GetDevice();

		GraphicsPSODesc desc;
		desc.vs = vertexShader;
		desc.ps = pixelShader;
		desc.gs = geometryShader;
		desc.bs = &blendState;
		desc.rs = &rasterizerState;
		desc.dss = &depthStencilState;
		desc.pt = POINTLIST;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
		device->CreateGraphicsPSO(&desc, &PSO);
	}
	void Initialize() 
	{

		GPUBufferDesc bd;
		bd.Usage = USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(LensFlareCB);
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;
		wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &constantBuffer);


		RasterizerStateDesc rs;
		rs.FillMode = FILL_SOLID;
		rs.CullMode = CULL_NONE;
		rs.FrontCounterClockwise = true;
		rs.DepthBias = 0;
		rs.DepthBiasClamp = 0;
		rs.SlopeScaledDepthBias = 0;
		rs.DepthClipEnable = false;
		rs.MultisampleEnable = false;
		rs.AntialiasedLineEnable = false;
		wiRenderer::GetDevice()->CreateRasterizerState(&rs, &rasterizerState);





		DepthStencilStateDesc dsd;
		dsd.DepthEnable = false;
		dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
		dsd.DepthFunc = COMPARISON_GREATER;

		dsd.StencilEnable = false;
		dsd.StencilReadMask = 0xFF;
		dsd.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing.
		dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
		dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_INCR;
		dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
		dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing.
		dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
		dsd.BackFace.StencilDepthFailOp = STENCIL_OP_DECR;
		dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
		dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;

		// Create the depth stencil state.
		wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilState);


		{
			BlendStateDesc bd;
			bd.RenderTarget[0].BlendEnable = true;
			bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
			bd.RenderTarget[0].DestBlend = BLEND_ONE;
			bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
			bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
			bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
			bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
			bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
			wiRenderer::GetDevice()->CreateBlendState(&bd, &blendState);


			SamplerDesc samplerDesc;
			samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.MaxAnisotropy = 0;
			samplerDesc.ComparisonFunc = COMPARISON_GREATER_EQUAL;
			wiRenderer::GetDevice()->CreateSamplerState(&samplerDesc, &samplercmp);
		}



		LoadShaders();

		wiBackLog::post("wiLensFlare Initialized");
	}
	void CleanUp() 
	{
	}

}
