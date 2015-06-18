#include "LensFlare.h"



ID3D11Buffer* LensFlare::constantBuffer;
ID3D11PixelShader* LensFlare::pixelShader;
ID3D11GeometryShader* LensFlare::geometryShader;
ID3D11VertexShader* LensFlare::vertexShader;
ID3D11InputLayout* LensFlare::inputLayout;
ID3D11SamplerState* LensFlare::samplerState;
ID3D11RasterizerState* LensFlare::rasterizerState;
ID3D11DepthStencilState* LensFlare::depthStencilState;
ID3D11BlendState* LensFlare::blendState;
float LensFlare::RENDERWIDTH, LensFlare::RENDERHEIGHT;

void LensFlare::Initialize(float width, float height){
	LoadShaders();
	SetUpCB();
	SetUpStates();
	RENDERWIDTH=width;
	RENDERHEIGHT=height;
	//
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare0.jpg",0,&textures[0],0);
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare1.jpg",0,&textures[1],0);
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare2.jpg",0,&textures[2],0);
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare3.jpg",0,&textures[3],0);
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare4.jpg",0,&textures[4],0);
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare5.jpg",0,&textures[5],0);
	//CreateWICTextureFromFile(0,Renderer::graphicsDevice,0,L"images/flare6.jpg",0,&textures[6],0);
}
void LensFlare::CleanUp(){
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
void LensFlare::Draw(ID3D11ShaderResourceView* depthMap, ID3D11DeviceContext* context, const XMVECTOR& lightPos
					 , vector<Renderer::TextureView>& rims){

	if(!rims.empty()){

		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
		Renderer::BindPrimitiveTopology(Renderer::POINTLIST,context);
		Renderer::BindVertexLayout(inputLayout,context);
		//context->IASetInputLayout( inputLayout );
		//context->VSSetShader( vertexShader, NULL, 0 );
		//context->GSSetShader( geometryShader, NULL, 0 );
		Renderer::BindPS(pixelShader,context);
		Renderer::BindVS(vertexShader,context);
		Renderer::BindGS(geometryShader,context);

		ConstantBuffer cb;
		cb.mSunPos = lightPos/XMVectorSet(RENDERWIDTH,RENDERHEIGHT,1,1);
		cb.mScreen = XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,0,0);

		Renderer::UpdateBuffer(constantBuffer,&cb,context);

	
		//context->RSSetState(rasterizerState);
		//context->OMSetDepthStencilState(depthStencilState, 1);
		//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		//UINT sampleMask   = 0xffffffff;
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);
	
		//context->GSSetConstantBuffers( 0, 1, &constantBuffer );
		Renderer::BindRasterizerState(rasterizerState,context);
		Renderer::BindDepthStencilState(depthStencilState,1,context);
		Renderer::BindBlendState(blendState,context);
		Renderer::BindConstantBufferGS(constantBuffer,0,context);

		//context->GSSetShaderResources( 0,1,&depthMap );
		Renderer::BindTextureGS(depthMap,0,context);
		//context->GSSetSamplers(0, 1, &samplerState);
		//context->PSSetSamplers(0, 1, &samplerState);
		Renderer::BindSamplerPS(Renderer::ssClampLin,0,context);
		Renderer::BindSamplerGS(samplerState,0,context);

		int i=0;
		for(Renderer::TextureView x : rims){
			if(x!=nullptr){
				Renderer::BindTexturePS(x,i+1,context);
				Renderer::BindTextureGS(x,i+1,context);
				i++;
			}
		}
		//context->Draw(i,0);
		Renderer::Draw(i,context);

		


		//context->GSSetShader(0,0,0);
		Renderer::BindGS(nullptr,context);
		//context->GSSetConstantBuffers(0,0,0);
		Renderer::BindConstantBufferGS(nullptr,0,context);
	}
}

void LensFlare::LoadShaders(){
	ID3DBlob* pVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/lensFlareVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load lensFlareVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	UINT numElements = ARRAYSIZE( layout );
	
    // Create the input layout
	Renderer::graphicsDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &inputLayout );

	if(pVSBlob){ pVSBlob->Release();pVSBlob=NULL; }
    

	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/lensFlarePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load lensFlarePS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }


	ID3DBlob* pGSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/lensFlareGS.cso", &pGSBlob))){MessageBox(0,L"Failed To load lensFlareGS.cso",0,0);}
	Renderer::graphicsDevice->CreateGeometryShader( pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &geometryShader );
	if(pGSBlob){ pGSBlob->Release();pGSBlob=NULL; }
}
void LensFlare::SetUpCB()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );

	
}
void LensFlare::SetUpStates()
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
	Renderer::graphicsDevice->CreateSamplerState(&samplerDesc, &samplerState);



	
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
	Renderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
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
	Renderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilState);


	
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
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendState);
}
