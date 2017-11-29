#include "wiLensFlare.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ResourceMapping.h"

using namespace wiGraphicsTypes;

GPUBuffer *wiLensFlare::constantBuffer = nullptr;
PixelShader *wiLensFlare::pixelShader = nullptr;
GeometryShader *wiLensFlare::geometryShader = nullptr;
VertexShader *wiLensFlare::vertexShader = nullptr;
VertexLayout *wiLensFlare::inputLayout = nullptr;
Sampler *wiLensFlare::samplercmp = nullptr;
RasterizerState wiLensFlare::rasterizerState;
DepthStencilState wiLensFlare::depthStencilState;
BlendState wiLensFlare::blendState;
GraphicsPSO	wiLensFlare::PSO;

void wiLensFlare::Initialize(){
	SetUpCB();
	SetUpStates();
	LoadShaders();
}
void wiLensFlare::CleanUp(){
	SAFE_DELETE(constantBuffer);
	SAFE_DELETE(pixelShader);
	SAFE_DELETE(geometryShader);
	SAFE_DELETE(vertexShader);
	SAFE_DELETE(inputLayout);
	SAFE_DELETE(samplercmp);
}
void wiLensFlare::Draw(GRAPHICSTHREAD threadID, const XMVECTOR& lightPos, std::vector<Texture2D*>& rims){

	if(!rims.empty())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("LensFlare", threadID);

		device->BindPrimitiveTopology(POINTLIST,threadID);
		//device->BindVertexLayout(inputLayout,threadID);
		//device->BindPS(pixelShader,threadID);
		//device->BindVS(vertexShader,threadID);
		//device->BindGS(geometryShader,threadID);

		ConstantBuffer cb;
		cb.mSunPos = lightPos / XMVectorSet((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 1, 1);
		cb.mScreen = XMFLOAT4((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0, 0);

		device->UpdateBuffer(constantBuffer,&cb,threadID);
		device->BindConstantBufferGS(constantBuffer, CB_GETBINDSLOT(ConstantBuffer),threadID);

	
		//device->BindRasterizerState(rasterizerState,threadID);
		//device->BindDepthStencilState(depthStencilState,1,threadID);
		//device->BindBlendState(blendState,threadID);

		int i=0;
		for(Texture2D* x : rims){
			if(x!=nullptr){
				device->BindResourcePS(x, TEXSLOT_ONDEMAND0 + i, threadID);
				device->BindResourceGS(x, TEXSLOT_ONDEMAND0 + i, threadID);
				i++;
			}
		}

		device->BindSamplerGS(samplercmp, SSLOT_ONDEMAND0, threadID);


		device->Draw(i, 0, threadID);

		


		//device->BindGS(nullptr,threadID);

		device->EventEnd(threadID);
	}
}

void wiLensFlare::LoadShaders(){

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lensFlareVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
		inputLayout = vsinfo->vertexLayout;
	}


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lensFlarePS.cso", wiResourceManager::PIXELSHADER));

	geometryShader = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lensFlareGS.cso", wiResourceManager::GEOMETRYSHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	GraphicsPSODesc desc;
	desc.vs = vertexShader;
	desc.ps = pixelShader;
	desc.gs = geometryShader;
	desc.bs = &blendState;
	desc.rs = &rasterizerState;
	desc.dss = &depthStencilState;
}
void wiLensFlare::SetUpCB()
{
	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	constantBuffer = new GPUBuffer;
    wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, constantBuffer );
	
}
void wiLensFlare::SetUpStates()
{
	RasterizerStateDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,&rasterizerState);




	
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


	
	BlendStateDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	wiRenderer::GetDevice()->CreateBlendState(&bd,&blendState);


	SamplerDesc samplerDesc;
	samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_GREATER_EQUAL;
	samplercmp = new Sampler;
	wiRenderer::GetDevice()->CreateSamplerState(&samplerDesc, samplercmp);
}
