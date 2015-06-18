#include "Image.h"


#pragma region STATICS
mutex Image::MUTEX;
ID3D11BlendState*		Image::blendState, *Image::blendStateAdd, *Image::blendStateNoBlend, *Image::blendStateAvg;
ID3D11Buffer*           Image::constantBuffer,*Image::PSCb,*Image::blurCb,*Image::processCb,*Image::shaftCb,*Image::deferredCb;

ID3D11VertexShader*     Image::vertexShader,*Image::screenVS;
ID3D11PixelShader*      Image::pixelShader,*Image::blurHPS,*Image::blurVPS,*Image::shaftPS,*Image::outlinePS
	,*Image::dofPS,*Image::motionBlurPS,*Image::bloomSeparatePS,*Image::fxaaPS,*Image::ssaoPS,*Image::deferredPS
	,*Image::ssssPS,*Image::linDepthPS,*Image::colorGradePS;
	

ID3D11RasterizerState*		Image::rasterizerState;
ID3D11DepthStencilState*	Image::depthStencilStateGreater,*Image::depthStencilStateLess,*Image::depthNoStencilState;

int Image::RENDERWIDTH,Image::RENDERHEIGHT;

//map<string,Image::ImageResource> Image::images;
#pragma endregion

Image::Image()
{
}

void Image::LoadBuffers()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );


	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PSConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &PSCb );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(BlurBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &blurCb );
	
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ProcessBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &processCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(LightShaftBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &shaftCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DeferredBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &deferredCb );
}

void Image::LoadShaders()
{
    ID3DBlob* pVSBlob = NULL;
	
	if(FAILED(D3DReadFileToBlob(L"shaders/imageVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load imageVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );
	if(pVSBlob){ pVSBlob->Release();pVSBlob=NULL; }

	
	if(FAILED(D3DReadFileToBlob(L"shaders/screenVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load creenVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &screenVS );
	if(pVSBlob){ pVSBlob->Release();pVSBlob=NULL; }


	ID3DBlob* pPSBlob = NULL;
	
	if(FAILED(D3DReadFileToBlob(L"shaders/imagePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load imagePS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }

	if(FAILED(D3DReadFileToBlob(L"shaders/horizontalBlurPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load horizontalBlurPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &blurHPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/verticalBlurPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load verticalBlurPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &blurVPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/lightShaftPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load lightShaftPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &shaftPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/outlinePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load outlinePS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &outlinePS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/depthofFieldPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load depthofFieldPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &dofPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/motionBlurPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load motionBlurPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &motionBlurPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/bloomSeparatePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load bloomSeparatePS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &bloomSeparatePS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/fxaa.cso", &pPSBlob))){MessageBox(0,L"Failed To load fxaa.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &fxaaPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/ssao.cso", &pPSBlob))){MessageBox(0,L"Failed To load ssao.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &ssaoPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/ssss.cso", &pPSBlob))){MessageBox(0,L"Failed To load ssss.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &ssssPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/linDepth.cso", &pPSBlob))){MessageBox(0,L"Failed To load linDepth.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &linDepthPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/colorGrade.cso", &pPSBlob))){MessageBox(0,L"Failed To load colorGrade.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &colorGradePS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	
	if(FAILED(D3DReadFileToBlob(L"shaders/deferredPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load deferredPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &deferredPS );
	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
}
void Image::SetUpStates()
{

	
	D3D11_RASTERIZER_DESC rs;
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_BACK;
	rs.FrontCounterClockwise=false;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=FALSE;
	rs.ScissorEnable=FALSE;
	rs.MultisampleEnable=FALSE;
	rs.AntialiasedLineEnable=FALSE;
	Renderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
	D3D11_DEPTH_STENCIL_DESC dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = 0;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Create the depth stencil state.
	Renderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilStateLess);

	
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = 0;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_GREATER;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Create the depth stencil state.
	Renderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilStateGreater);
	
	dsd.StencilEnable = false;
	Renderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthNoStencilState);

	
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.IndependentBlendEnable=true;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendState);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	//bd.IndependentBlendEnable=true;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendStateNoBlend);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	//bd.IndependentBlendEnable=true;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendStateAdd);
	
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	//bd.IndependentBlendEnable=true;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendStateAvg);
}


void Image::Draw(Renderer::TextureView texture, const ImageEffects& effects){
	Draw(texture,effects,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const ImageEffects& effects,ID3D11DeviceContext* context){
	if(!context)
		return;
	if(!effects.blur){
		if(!effects.process.active && !effects.bloom.separate && !effects.sunPos.x && !effects.sunPos.y){
			ConstantBuffer cb;
			if(effects.typeFlag==SCREEN){
				cb.mViewProjection = XMMatrixTranspose( Renderer::cam->Oprojection );
				cb.mTrans = XMMatrixTranspose( XMMatrixTranslation(RENDERWIDTH/2-effects.siz.x/2,-RENDERHEIGHT/2+effects.siz.y/2,0) * XMMatrixRotationZ(effects.rotation)
					* XMMatrixTranslation(-RENDERWIDTH/2+effects.pos.x+effects.siz.x*0.5f,RENDERHEIGHT/2 + effects.pos.y-effects.siz.y*0.5f,0) ); //AUTO ORIGIN CORRECTION APPLIED! NO FURTHER TRANSLATIONS NEEDED!
				cb.mDimensions = XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,effects.siz.x,effects.siz.y);
			}
			else if(effects.typeFlag==WORLD){
				cb.mViewProjection = XMMatrixTranspose( Renderer::cam->View * Renderer::cam->Projection );
				XMMATRIX faceRot = XMMatrixIdentity();
				if(effects.lookAt.w){
					XMVECTOR vvv = (effects.lookAt.x==1 && !effects.lookAt.y && !effects.lookAt.z)?XMVectorSet(0,1,0,0):XMVectorSet(1,0,0,0);
					faceRot = 
						XMMatrixLookAtLH(XMVectorSet(0,0,0,0)
							,XMLoadFloat4(&effects.lookAt)
							,XMVector3Cross(
								XMLoadFloat4(&effects.lookAt),vvv
							)
						)
					;
				}
				else
					faceRot=XMMatrixRotationX(Renderer::cam->updownRot)*XMMatrixRotationY(Renderer::cam->leftrightRot);
				cb.mTrans = XMMatrixTranspose(
					XMMatrixScaling(effects.scale.x,effects.scale.y,1)
					*XMMatrixRotationZ(effects.rotation)
					*faceRot
					//*XMMatrixInverse(0,XMMatrixLookAtLH(XMVectorSet(0,0,0,0),XMLoadFloat3(&effects.pos)-Renderer::cam->Eye,XMVectorSet(0,1,0,0)))
					*XMMatrixTranslation(effects.pos.x,effects.pos.y,effects.pos.z)
					);
				cb.mDimensions = XMFLOAT4(0,0,effects.siz.x,effects.siz.y);
			}
	
			cb.mOffsetMirFade = XMFLOAT4(effects.offset.x,effects.offset.y,effects.mirror,effects.fade);
			cb.mDrawRec = effects.drawRec;
			cb.mBlurOpaPiv = XMFLOAT4(effects.blurDir,effects.blur,effects.opacity,effects.pivotFlag);
			cb.mTexOffset=XMFLOAT4(effects.texOffset.x,effects.texOffset.y,0,0);

			Renderer::UpdateBuffer(constantBuffer,&cb,context);
	
			//context->UpdateSubresource( constantBuffer, 0, NULL, &cb, 0, 0 );

			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//ConstantBuffer* dataPtr;
			//context->Map(constantBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (ConstantBuffer*)mappedResource.pData;
			//memcpy(dataPtr,&cb,sizeof(ConstantBuffer));
			//context->Unmap(constantBuffer,0);

			//context->VSSetConstantBuffers( 0, 1, &constantBuffer );

			Renderer::BindConstantBufferVS(constantBuffer,0,context);
		}
		else if(!effects.sunPos.x && !effects.sunPos.y){
			//context->VSSetShader(screenVS,0,0);
			Renderer::BindVS(screenVS,context);
			

			if(effects.process.outline) 
				//context->PSSetShader(outlinePS,0,0);
					Renderer::BindPS(outlinePS,context);
			else if(effects.process.motionBlur) 
				//context->PSSetShader(motionBlurPS,0,0);
				Renderer::BindPS(motionBlurPS,context);
			else if(effects.process.dofStrength) 
				//context->PSSetShader(dofPS,0,0);
				Renderer::BindPS(dofPS,context);
			else if(effects.process.fxaa) 
				//context->PSSetShader(fxaaPS,0,0);
				Renderer::BindPS(fxaaPS,context);
			else if(effects.process.ssao) {
				//context->PSSetShader(ssaoPS,0,0);
				Renderer::BindPS(ssaoPS,context);
			}
			else if(effects.process.linDepth) 
				//context->PSSetShader(linDepthPS,0,0);
				Renderer::BindPS(linDepthPS,context);
			else if(effects.process.colorGrade)
				Renderer::BindPS(colorGradePS,context);
			else if(effects.process.ssss.x||effects.process.ssss.y) 
				//context->PSSetShader(ssssPS,0,0);
				Renderer::BindPS(ssssPS,context);
			else if(effects.bloom.separate)
				//context->PSSetShader(bloomSeparatePS,0,0);
				Renderer::BindPS(bloomSeparatePS,context);
			else MessageBoxA(0,"Postprocess branch not implemented!","Warning!",0);
			
			ProcessBuffer prcb;
			prcb.mPostProcess=XMFLOAT4(effects.process.motionBlur,effects.process.outline,effects.process.dofStrength,effects.process.ssss.x);
			prcb.mBloom=XMFLOAT4(effects.bloom.separate,effects.bloom.threshold,effects.bloom.saturation,effects.process.ssss.y);

			Renderer::UpdateBuffer(processCb,&prcb,context);
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//ProcessBuffer* dataPtr2;
			//context->Map(processCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr2 = (ProcessBuffer*)mappedResource.pData;
			//memcpy(dataPtr2,&prcb,sizeof(ProcessBuffer));
			//context->Unmap(processCb,0);

			//context->PSSetConstantBuffers( 1, 1, &processCb );

			Renderer::BindConstantBufferPS(processCb,1,context);
		}
		else{ //LIGHTSHAFTS
			//context->VSSetShader(screenVS,0,0);
			//context->PSSetShader(shaftPS,0,0);
			Renderer::BindVS(screenVS,context);
			Renderer::BindPS(shaftPS,context);
			
			LightShaftBuffer scb;
			scb.mProperties=XMFLOAT4(0.65f,0.25f,0.945f,0.2f); //Density|Weight|Decay|Exposure
			scb.mLightShaft=effects.sunPos;

			Renderer::UpdateBuffer(shaftCb,&scb,context);
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//LightShaftBuffer* dataPtr2;
			//context->Map(shaftCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr2 = (LightShaftBuffer*)mappedResource.pData;
			//memcpy(dataPtr2,&scb,sizeof(LightShaftBuffer));
			//context->Unmap(shaftCb,0);

			//context->PSSetConstantBuffers( 0, 1, &shaftCb );
			Renderer::BindConstantBufferPS(shaftCb,0,context);
		}

		
		//D3D11_MAPPED_SUBRESOURCE mappedResource;
		PSConstantBuffer pscb;
		int normalmapmode=0;
		if(effects.normalMap && effects.refractionMap)
			normalmapmode=1;
		if(effects.extractNormalMap==true)
			normalmapmode=2;
		pscb.mMaskFadOpaDis=XMFLOAT4(effects.maskMap?1:0,effects.fade,effects.opacity,normalmapmode);
		pscb.mDimension=XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,effects.siz.x,effects.siz.y);

		Renderer::UpdateBuffer(PSCb,&pscb,context);
		//PSConstantBuffer* dataPtr2;
		//context->Map(PSCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
		//dataPtr2 = (PSConstantBuffer*)mappedResource.pData;
		//memcpy(dataPtr2,&pscb,sizeof(PSConstantBuffer));
		//context->Unmap(PSCb,0);

		//context->PSSetConstantBuffers( 2, 1, &PSCb );
		Renderer::BindConstantBufferPS(PSCb,2,context);
		
		Renderer::BindTexturePS(effects.depthMap,0,context);
		Renderer::BindTexturePS(effects.normalMap,1,context);
		Renderer::BindTexturePS(effects.velocityMap,3,context);
		Renderer::BindTexturePS(effects.refractionMap,4,context);
		Renderer::BindTexturePS(effects.maskMap,5,context);
	}
	else{ //BLUR
		Renderer::BindVS(screenVS,context);
		
		BlurBuffer cb;
		if(effects.blurDir==0){
			Renderer::BindPS(blurHPS,context);
			cb.mWeightTexelStren.y=1.0f/RENDERWIDTH;
		}
		else{
			Renderer::BindPS(blurVPS,context);
			cb.mWeightTexelStren.y=1.0f/RENDERHEIGHT;
		}

		float weight0 = 1.0f;
		float weight1 = 0.9f;
		float weight2 = 0.55f;
		float weight3 = 0.18f;
		float weight4 = 0.1f;
		float normalization = (weight0 + 2.0f * (weight1 + weight2 + weight3 + weight4));
		cb.mWeight=XMVectorSet(weight0,weight1,weight2,weight3) / normalization;
		cb.mWeightTexelStren.x=weight4 / normalization;
		cb.mWeightTexelStren.z=effects.blur;

		Renderer::UpdateBuffer(blurCb,&cb,context);

		Renderer::BindConstantBufferPS(blurCb,0,context);
	}

	//if(texture) 
	Renderer::BindTexturePS(texture,6,context);

	
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT sampleMask   = 0xffffffff;
	if(effects.blendFlag==BLENDMODE_ALPHA)
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);
		Renderer::BindBlendState(blendState,context);
	else if(effects.blendFlag==BLENDMODE_ADDITIVE)
		//context->OMSetBlendState(blendStateAdd, blendFactor, sampleMask);
		Renderer::BindBlendState(blendStateAdd,context);
	else if(effects.blendFlag==BLENDMODE_OPAQUE)
		//context->OMSetBlendState(blendStateNoBlend, blendFactor, sampleMask);
		Renderer::BindBlendState(blendStateNoBlend,context);
	else if(effects.blendFlag==BLENDMODE_MAX)
		//context->OMSetBlendState(blendStateAvg, blendFactor, sampleMask);
		Renderer::BindBlendState(blendStateAvg,context);

	if(effects.quality==QUALITY_NEAREST){
		if(effects.sampleFlag==SAMPLEMODE_MIRROR)
			//context->PSSetSamplers(0, 1, &ssMirrorPoi);
			Renderer::BindSamplerPS(Renderer::ssMirrorPoi,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_WRAP)
			//context->PSSetSamplers(0, 1, &ssWrapPoi);
			Renderer::BindSamplerPS(Renderer::ssWrapPoi,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_CLAMP)
			//context->PSSetSamplers(0, 1, &ssClampPoi);
			Renderer::BindSamplerPS(Renderer::ssClampPoi,0,context);
	}
	else if(effects.quality==QUALITY_BILINEAR){
		if(effects.sampleFlag==SAMPLEMODE_MIRROR)
			//context->PSSetSamplers(0, 1, &ssMirrorLin);
			Renderer::BindSamplerPS(Renderer::ssMirrorLin,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_WRAP)
			//context->PSSetSamplers(0, 1, &ssWrapLin);
			Renderer::BindSamplerPS(Renderer::ssWrapLin,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_CLAMP)
			//context->PSSetSamplers(0, 1, &ssClampLin);
			Renderer::BindSamplerPS(Renderer::ssClampLin,0,context);
	}
	else if(effects.quality==QUALITY_ANISOTROPIC){
		if(effects.sampleFlag==SAMPLEMODE_MIRROR)
			//context->PSSetSamplers(0, 1, &ssMirrorAni);
			Renderer::BindSamplerPS(Renderer::ssMirrorAni,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_WRAP)
			//context->PSSetSamplers(0, 1, &ssWrapAni);
			Renderer::BindSamplerPS(Renderer::ssWrapAni,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_CLAMP)
			//context->PSSetSamplers(0, 1, &ssClampAni);
			Renderer::BindSamplerPS(Renderer::ssClampAni,0,context);
	}

	
	//context->Draw(4,0);
	Renderer::Draw(4,context);
}

void Image::DrawDeferred(Renderer::TextureView texture
		, Renderer::TextureView depth, Renderer::TextureView lightmap, Renderer::TextureView normal
		, Renderer::TextureView ao, ID3D11DeviceContext* context, int stencilRef){
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	//UINT stride = sizeof( Vertex );
    //UINT offset = 0;
    //context->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
	//context->IASetInputLayout( vertexLayout );

	//context->RSSetState(rasterizerState);
	//context->OMSetDepthStencilState(depthStencilStateLess, stencilRef);

	Renderer::BindPrimitiveTopology(Renderer::PRIMITIVETOPOLOGY::TRIANGLESTRIP,context);
	Renderer::BindRasterizerState(rasterizerState,context);
	Renderer::BindDepthStencilState(depthStencilStateLess,stencilRef,context);

	//context->VSSetShader(screenVS,0,0);
	//context->PSSetShader(deferredPS,0,0);
	Renderer::BindVS(screenVS,context);
	Renderer::BindPS(deferredPS,context);
	
	Renderer::BindTexturePS(depth,0,context);
	Renderer::BindTexturePS(normal,1,context);
	Renderer::BindTexturePS(texture,6,context);
	Renderer::BindTexturePS(lightmap,7,context);
	Renderer::BindTexturePS(ao,8,context);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState, blendFactor, sampleMask);
	//context->PSSetSamplers(0, 1, &ssClampLin);
	Renderer::BindSamplerPS(Renderer::ssClampLin,0,context);
	//context->PSSetSamplers(1, 1, &ssComp);

	Renderer::BindBlendState(blendStateNoBlend,context);

	DeferredBuffer cb;
	//cb.mSun=XMVector3Normalize(Renderer::GetSunPosition());
	//cb.mEye=Renderer::cam->Eye;
	cb.mAmbient=Renderer::worldInfo.ambient;
	//cb.mBiasResSoftshadow=shadowProps;
	cb.mHorizon=Renderer::worldInfo.horizon;
	cb.mViewProjInv=XMMatrixInverse( 0,XMMatrixTranspose(Renderer::cam->View*Renderer::cam->Projection) );
	cb.mFogSEH=Renderer::worldInfo.fogSEH;

	Renderer::UpdateBuffer(deferredCb,&cb,context);

	/*cb.mShM[0] = shMs[0];
	cb.mShM[1] = shMs[1];
	cb.mShM[2] = shMs[2];*/
	//delete[] shMs;
	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//DeferredBuffer* dataPtr;
	//context->Map(deferredCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr = (DeferredBuffer*)mappedResource.pData;
	//memcpy(dataPtr,&cb,sizeof(DeferredBuffer));
	//context->Unmap(deferredCb,0);
	//context->PSSetConstantBuffers(0,1,&deferredCb);
	//
	//context->Draw(4,0);

	Renderer::BindConstantBufferPS(deferredCb,0,context);
	Renderer::Draw(4,context);
}


void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz)
{
	Draw(texture,newPosSiz,XMFLOAT4(0,0,0,0),1,0,0,0,0,0,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const float&newRot)
{
	Draw(texture,newPosSiz,XMFLOAT4(0,0,0,0),1,0,0,0,0,newRot,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const float&newRot, const float&opacity, ID3D11DeviceContext* context)
{
	Draw(texture,newPosSiz,XMFLOAT4(0,0,0,0),1,0,0,0,opacity,newRot,context);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec)
{
	Draw(texture,newPosSiz,newDrawRec,1,0,0,0,0,0,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror)
{
	Draw(texture,newPosSiz,newDrawRec,newMirror,0,0,0,0,0,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, const float&newBlur, const float&newBlurStrength, const float&newFade, const float&newOpacity)
{
	Draw(texture,newPosSiz,newDrawRec,newMirror,newBlur,newBlurStrength,newFade,newOpacity,0,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, const float&newBlur, const float&newBlurStrength, const float&newFade, const float&newOpacity, ID3D11DeviceContext* context)
{
	Draw(texture,newPosSiz,newDrawRec,newMirror,newBlur,newBlurStrength,newFade,newOpacity,0,context);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, const float&newBlur, const float&newBlurStrength, const float&newFade, const float&newOpacity, const float&newRot)
{
	Draw(texture,newPosSiz,newDrawRec,newMirror,newBlur,newBlurStrength,newFade,newOpacity,newRot,Renderer::immediateContext);
}
void Image::Draw(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, const float&newBlur, const float&newBlurStrength, const float&newFade, const float&newOpacity, const float&newRot, ID3D11DeviceContext* context)
{
	Draw(texture,0,newPosSiz,newDrawRec,newMirror,newBlur,newBlurStrength,newFade,newOpacity,newRot,XMFLOAT2(0,0),BLENDMODE_ALPHA,context);
}
void Image::Draw(Renderer::TextureView texture, Renderer::TextureView mask, const XMFLOAT4& newPosSiz
						, const XMFLOAT4& newDrawRec, const float&newMirror, const float&newBlur, const float&newBlurStrength
						, const float&newFade, const float&newOpacity, const float&newRot, XMFLOAT2 texOffset
						, BLENDMODE blendMode, ID3D11DeviceContext* context)
{
	ImageEffects fx = ImageEffects();
	fx.pos=XMFLOAT3(newPosSiz.x,newPosSiz.y,0);
	fx.siz=XMFLOAT2(newPosSiz.z,newPosSiz.w);
	fx.drawRec=newDrawRec;
	fx.mirror=newMirror;
	fx.blur=newBlurStrength;
	fx.fade=newFade;
	fx.opacity=newOpacity;
	fx.rotation=newRot;
	fx.texOffset=texOffset;
	fx.blendFlag=blendMode;
	fx.setMaskMap(mask);

	Draw(texture,fx,context);
}
void Image::DrawModifiedTexCoords(Renderer::TextureView texture, Renderer::TextureView mask, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, XMFLOAT2 texOffset, const float&newOpacity, BLENDMODE blendMode)
{
	Draw(texture,mask,newPosSiz,newDrawRec,newMirror,0,0,0,newOpacity,0,texOffset,blendMode,Renderer::immediateContext);
}
void Image::DrawModifiedTexCoords(Renderer::TextureView texture, Renderer::TextureView mask, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, XMFLOAT2 texOffset, const float&newOpacity, BLENDMODE blendMode, ID3D11DeviceContext* context)
{
	Draw(texture,mask,newPosSiz,newDrawRec,newMirror,0,0,0,newOpacity,0,texOffset,blendMode,context);
}
void Image::DrawOffset(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, XMFLOAT2 newOffset)
{
	ConstantBuffer cb;
	cb.mViewProjection = XMMatrixTranspose( Renderer::cam->Oprojection );
	cb.mTrans =  XMMatrixTranspose( XMMatrixTranslation(newPosSiz.x,newPosSiz.y,0) );
	cb.mDimensions = XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,newPosSiz.z,newPosSiz.w);
	cb.mOffsetMirFade = XMFLOAT4(newOffset.x,newOffset.y,1,0);
	cb.mDrawRec = XMFLOAT4(0,0,0,0);
	cb.mBlurOpaPiv = XMFLOAT4(0,0,0,0);
	cb.mTexOffset=XMFLOAT4(0,0,0,0);

	Renderer::UpdateBuffer(constantBuffer,&cb,Renderer::immediateContext);
	
	//Renderer::immediateContext->UpdateSubresource( constantBuffer, 0, NULL, &cb, 0, 0 );

	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//ConstantBuffer* dataPtr;
	//Renderer::immediateContext->Map(constantBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr = (ConstantBuffer*)mappedResource.pData;
	//memcpy(dataPtr,&cb,sizeof(ConstantBuffer));
	//Renderer::immediateContext->Unmap(constantBuffer,0);

	Renderer::immediateContext->VSSetConstantBuffers( 0, 1, &constantBuffer );


	//PSConstantBuffer pscb;
	////pscb.mMaskFxBlSa=XMFLOAT4(0,0,0,0);
	////Renderer::immediateContext->UpdateSubresource( PSCb, 0, NULL, &pscb, 0, 0 );
	//PSConstantBuffer* dataPtr2;
	//Renderer::immediateContext->Map(PSCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr2 = (PSConstantBuffer*)mappedResource.pData;
	//memcpy(dataPtr2,&pscb,sizeof(PSConstantBuffer));
	//Renderer::immediateContext->Unmap(PSCb,0);

	//Renderer::immediateContext->PSSetConstantBuffers( 0, 1, &PSCb );


	Renderer::BindTexturePS(texture,5,Renderer::immediateContext);
	Renderer::immediateContext->Draw(4,0);

}
void Image::DrawOffset(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, XMFLOAT2 newOffset)
{
	ConstantBuffer cb;
	cb.mViewProjection = XMMatrixTranspose( Renderer::cam->Oprojection );
	cb.mTrans =  XMMatrixTranspose( XMMatrixTranslation(newPosSiz.x,newPosSiz.y,0) );
	cb.mDimensions = XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,newPosSiz.z,newPosSiz.w);
	cb.mOffsetMirFade = XMFLOAT4(newOffset.x,newOffset.y,1,0);
	cb.mDrawRec = newDrawRec;
	cb.mBlurOpaPiv = XMFLOAT4(0,0,0,0);
	cb.mTexOffset=XMFLOAT4(0,0,0,0);

	Renderer::UpdateBuffer(constantBuffer,&cb,Renderer::immediateContext);
	
	Renderer::BindTexturePS(texture,5,Renderer::immediateContext);
	Renderer::immediateContext->Draw(4,0);

}
void Image::DrawAdditive(Renderer::TextureView texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec)
{
	ConstantBuffer cb;
	cb.mViewProjection = XMMatrixTranspose( Renderer::cam->Oprojection );
	cb.mTrans =  XMMatrixTranspose( XMMatrixTranslation(newPosSiz.x,newPosSiz.y,0) );
	cb.mDimensions = XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,newPosSiz.z,newPosSiz.w);
	cb.mOffsetMirFade = XMFLOAT4(0,0,1,0);
	cb.mDrawRec = newDrawRec;
	cb.mBlurOpaPiv = XMFLOAT4(0,0,0,0);
	cb.mTexOffset=XMFLOAT4(0,0,0,0);

	Renderer::UpdateBuffer(constantBuffer,&cb,Renderer::immediateContext);
	//

	//Renderer::immediateContext->VSSetConstantBuffers( 0, 1, &constantBuffer );

	//Renderer::BindTexturePS(texture,5,Renderer::immediateContext);

	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//Renderer::immediateContext->OMSetBlendState(blendStateAdd, blendFactor, sampleMask);
	//	Renderer::immediateContext->Draw(4,0);
	//Renderer::immediateContext->OMSetBlendState(blendState, blendFactor, sampleMask);

	Renderer::BindConstantBufferVS(constantBuffer,0);
	Renderer::BindTexturePS(texture,5);
	Renderer::BindBlendState(blendStateAdd);
	Renderer::Draw(4);
	Renderer::BindBlendState(blendState);

}


void Image::BatchBegin()
{
	BatchBegin(Renderer::immediateContext);
}
void Image::BatchBegin(ID3D11DeviceContext* context)
{
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	//context->RSSetState(rasterizerState);
	//context->OMSetDepthStencilState(depthNoStencilState, 0);

	
	Renderer::BindPrimitiveTopology(Renderer::PRIMITIVETOPOLOGY::TRIANGLESTRIP,context);
	Renderer::BindRasterizerState(rasterizerState,context);
	Renderer::BindDepthStencilState(depthNoStencilState, 0, context);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState,blendFactor,sampleMask);
	Renderer::BindBlendState(blendState,context);

	//context->VSSetShader( vertexShader, NULL, 0 );
	//context->PSSetShader( pixelShader, NULL, 0 );
	Renderer::BindVS(vertexShader,context);
	Renderer::BindPS(pixelShader,context);
}
void Image::BatchBegin(ID3D11DeviceContext* context, unsigned int stencilref, bool stencilOpLess)
{
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	//context->RSSetState(rasterizerState);
	//context->OMSetDepthStencilState(stencilOpLess?depthStencilStateLess:depthStencilStateGreater, stencilref);

	Renderer::BindPrimitiveTopology(Renderer::PRIMITIVETOPOLOGY::TRIANGLESTRIP,context);
	Renderer::BindRasterizerState(rasterizerState,context);
	Renderer::BindDepthStencilState(stencilOpLess?depthStencilStateLess:depthStencilStateGreater, stencilref, context);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState,blendFactor,sampleMask);
	Renderer::BindBlendState(blendState,context);

	//context->VSSetShader( vertexShader, NULL, 0 );
	//context->PSSetShader( pixelShader, NULL, 0 );
	Renderer::BindVS(vertexShader,context);
	Renderer::BindPS(pixelShader,context);
}


void Image::Load(){
	LoadShaders();
	LoadBuffers();
	SetUpStates();
}
void Image::CleanUp()
{
//	for(map<string,ImageResource>::iterator iter=images.begin();iter!=images.end();++iter)
//		iter->second.CleanUp();
//	images.clear();

	if(vertexShader) vertexShader->Release();
	if(pixelShader) pixelShader->Release();
	if(screenVS) screenVS->Release();
	if(blurHPS) blurHPS->Release();
	if(blurVPS) blurVPS->Release();
	if(shaftPS) shaftPS->Release();
	if(bloomSeparatePS) bloomSeparatePS->Release();
	if(motionBlurPS) motionBlurPS->Release();
	if(dofPS) dofPS->Release();
	if(outlinePS) outlinePS->Release();
	if(fxaaPS) fxaaPS->Release();
	if(deferredPS) deferredPS->Release();
	Renderer::SafeRelease(colorGradePS);

	if(constantBuffer) constantBuffer->Release();
	if(PSCb) PSCb->Release();
	if(blurCb) blurCb->Release();
	if(processCb) processCb->Release();
	if(shaftCb) shaftCb->Release();
	if(deferredCb) deferredCb->Release();

	if(rasterizerState) rasterizerState->Release();
	if(blendState) blendState->Release();
	if(blendStateAdd) blendStateAdd->Release();
	if(blendStateNoBlend) blendStateNoBlend->Release();
	if(blendStateAvg) blendStateAvg->Release();
	
	
	if(depthNoStencilState) depthNoStencilState->Release(); depthNoStencilState=nullptr;
	if(depthStencilStateLess) depthStencilStateLess->Release(); depthStencilStateLess=nullptr;
	if(depthStencilStateGreater) depthStencilStateGreater->Release(); depthStencilStateGreater=nullptr;
}

/*
Image::ImageResource::ImageResource(const string& newDirectory, const string& newName){
	name=newName;
	wstring wname(name.begin(),name.end());
	wstring wdir(newDirectory.begin(),newDirectory.end());
	wstringstream wss(L"");
	wss<<wdir<<wname;
	CreateWICTextureFromFile(false,Renderer::graphicsDevice,NULL,wss.str().c_str(),NULL,&texture,NULL);
	
	if(!texture) {
		stringstream ss("");
		ss<<newDirectory<<newName;
		MessageBoxA(0,ss.str().c_str(),"Loading Image Failed!",0);
	}
}
Renderer::TextureView Image::getImageByName(const string& get){
	map<string,ImageResource>::iterator iter = images.find(get);
	if(iter!=images.end())
		return iter->second.texture;
	return nullptr;
}
void Image::addImageResource(const string& dir, const string& name){
	MUTEX.lock();
	images.insert( pair<string,ImageResource>(name,ImageResource(dir,name)) );
	MUTEX.unlock();
}
*/