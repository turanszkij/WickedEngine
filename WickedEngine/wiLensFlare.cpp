#include "wiLensFlare.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"


BufferResource wiLensFlare::constantBuffer;
PixelShader wiLensFlare::pixelShader;
GeometryShader wiLensFlare::geometryShader;
VertexShader wiLensFlare::vertexShader;
VertexLayout wiLensFlare::inputLayout;
Sampler wiLensFlare::samplerState;
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

	if(samplerState) samplerState->Release(); samplerState = NULL;
	if(rasterizerState) rasterizerState->Release(); rasterizerState = NULL;
	if(blendState) blendState->Release(); blendState = NULL;
	if(depthStencilState) depthStencilState->Release(); depthStencilState = NULL;
}
void wiLensFlare::Draw(TextureView depthMap, DeviceContext context, const XMVECTOR& lightPos
					 , vector<TextureView>& rims){

	if(!rims.empty()){

		wiRenderer::BindPrimitiveTopology(POINTLIST,context);
		wiRenderer::BindVertexLayout(inputLayout,context);
		wiRenderer::BindPS(pixelShader,context);
		wiRenderer::BindVS(vertexShader,context);
		wiRenderer::BindGS(geometryShader,context);

		static thread_local ConstantBuffer* cb = new ConstantBuffer;
		(*cb).mSunPos = lightPos / XMVectorSet((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), 1, 1);
		(*cb).mScreen = XMFLOAT4((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), 0, 0);

		wiRenderer::UpdateBuffer(constantBuffer,cb,context);
		wiRenderer::BindConstantBufferGS(constantBuffer, CB_GETBINDSLOT(ConstantBuffer),context);

	
		wiRenderer::BindRasterizerState(rasterizerState,context);
		wiRenderer::BindDepthStencilState(depthStencilState,1,context);
		wiRenderer::BindBlendState(blendState,context);

		wiRenderer::BindTextureGS(depthMap,0,context);

		wiRenderer::BindSamplerPS(wiRenderer::ssClampLin,0,context);
		wiRenderer::BindSamplerGS(samplerState,0,context);

		int i=0;
		for(TextureView x : rims){
			if(x!=nullptr){
				wiRenderer::BindTexturePS(x,i+1,context);
				wiRenderer::BindTextureGS(x,i+1,context);
				i++;
			}
		}
		wiRenderer::Draw(i,context);

		


		wiRenderer::BindGS(nullptr,context);
	}
}

void wiLensFlare::LoadShaders(){

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
	
}
void wiLensFlare::SetUpStates()
{
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof(D3D11_SAMPLER_DESC) );
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	wiRenderer::graphicsDevice->CreateSamplerState(&samplerDesc, &samplerState);



	
	D3D11_RASTERIZER_DESC rs;
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	wiRenderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
	D3D11_DEPTH_STENCIL_DESC dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilState);


	
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendState);
}
