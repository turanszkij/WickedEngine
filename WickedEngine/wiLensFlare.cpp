#include "wiLensFlare.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "TextureMapping.h"


BufferResource wiLensFlare::constantBuffer;
PixelShader wiLensFlare::pixelShader;
GeometryShader wiLensFlare::geometryShader;
VertexShader wiLensFlare::vertexShader;
VertexLayout wiLensFlare::inputLayout;
RasterizerState wiLensFlare::rasterizerState;
DepthStencilState wiLensFlare::depthStencilState;
BlendState wiLensFlare::blendState;

void wiLensFlare::Initialize(){
	LoadShaders();
	SetUpCB();
	SetUpStates();
}
void wiLensFlare::CleanUp(){
	if(vertexShader) vertexShader->Release(); vertexShader = NULL;
	if(pixelShader) pixelShader->Release(); pixelShader = NULL;
	if(geometryShader) geometryShader->Release(); geometryShader = NULL;
	if(inputLayout) inputLayout->Release(); inputLayout = NULL;

	if(constantBuffer) constantBuffer->Release(); constantBuffer = NULL;

	if(rasterizerState) rasterizerState->Release(); rasterizerState = NULL;
	if(blendState) blendState->Release(); blendState = NULL;
	if(depthStencilState) depthStencilState->Release(); depthStencilState = NULL;
}
void wiLensFlare::Draw(GRAPHICSTHREAD threadID, const XMVECTOR& lightPos, vector<TextureView>& rims){

	if(!rims.empty()){

		wiRenderer::graphicsDevice->BindPrimitiveTopology(POINTLIST,threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(inputLayout,threadID);
		wiRenderer::graphicsDevice->BindPS(pixelShader,threadID);
		wiRenderer::graphicsDevice->BindVS(vertexShader,threadID);
		wiRenderer::graphicsDevice->BindGS(geometryShader,threadID);

		static thread_local ConstantBuffer* cb = new ConstantBuffer;
		(*cb).mSunPos = lightPos / XMVectorSet((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), 1, 1);
		(*cb).mScreen = XMFLOAT4((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), 0, 0);

		wiRenderer::graphicsDevice->UpdateBuffer(constantBuffer,cb,threadID);
		wiRenderer::graphicsDevice->BindConstantBufferGS(constantBuffer, CB_GETBINDSLOT(ConstantBuffer),threadID);

	
		wiRenderer::graphicsDevice->BindRasterizerState(rasterizerState,threadID);
		wiRenderer::graphicsDevice->BindDepthStencilState(depthStencilState,1,threadID);
		wiRenderer::graphicsDevice->BindBlendState(blendState,threadID);

		//wiRenderer::graphicsDevice->BindTextureGS(depthMap,0,threadID);

		int i=0;
		for(TextureView x : rims){
			if(x!=nullptr){
				wiRenderer::graphicsDevice->BindTexturePS(x, TEXSLOT_ONDEMAND0 + i, threadID);
				wiRenderer::graphicsDevice->BindTextureGS(x, TEXSLOT_ONDEMAND0 + i, threadID);
				i++;
			}
		}
		wiRenderer::graphicsDevice->Draw(i,threadID);

		


		wiRenderer::graphicsDevice->BindGS(nullptr,threadID);
	}
}

void wiLensFlare::LoadShaders(){

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lensFlareVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
		inputLayout = vsinfo->vertexLayout;
	}


	pixelShader = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lensFlarePS.cso", wiResourceManager::PIXELSHADER));

	geometryShader = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lensFlareGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiLensFlare::SetUpCB()
{
	BufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
	
}
void wiLensFlare::SetUpStates()
{
	RasterizerDesc rs;
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
	wiRenderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
	DepthStencilDesc dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS;

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
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilState);


	
	BlendDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendState);
}
