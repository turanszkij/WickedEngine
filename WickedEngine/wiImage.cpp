#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiImageEffects.h"
#include "wiLoader.h"
#include "wiHelper.h"

#pragma region STATICS
mutex wiImage::MUTEX;
ID3D11BlendState*		wiImage::blendState, *wiImage::blendStateAdd, *wiImage::blendStateNoBlend, *wiImage::blendStateAvg;
ID3D11Buffer*           wiImage::constantBuffer,*wiImage::PSCb,*wiImage::blurCb,*wiImage::processCb,*wiImage::shaftCb,*wiImage::deferredCb;

ID3D11VertexShader*     wiImage::vertexShader,*wiImage::screenVS;
ID3D11PixelShader*      wiImage::pixelShader,*wiImage::blurHPS,*wiImage::blurVPS,*wiImage::shaftPS,*wiImage::outlinePS
	,*wiImage::dofPS,*wiImage::motionBlurPS,*wiImage::bloomSeparatePS,*wiImage::fxaaPS,*wiImage::ssaoPS,*wiImage::deferredPS
	,*wiImage::ssssPS,*wiImage::linDepthPS,*wiImage::colorGradePS,*wiImage::ssrPS;
	

ID3D11RasterizerState*		wiImage::rasterizerState;
ID3D11DepthStencilState*	wiImage::depthStencilStateGreater,*wiImage::depthStencilStateLess,*wiImage::depthNoStencilState;


//map<string,wiImage::ImageResource> wiImage::images;
#pragma endregion

wiImage::wiImage()
{
}

void wiImage::LoadBuffers()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );


	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PSConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &PSCb );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(BlurBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &blurCb );
	
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ProcessBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &processCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(LightShaftBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &shaftCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DeferredBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &deferredCb );
}

void wiImage::LoadShaders()
{


	vertexShader = static_cast<wiRenderer::VertexShader>(wiResourceManager::GetGlobal()->add("shaders/imageVS.cso", wiResourceManager::VERTEXSHADER));
	screenVS = static_cast<wiRenderer::VertexShader>(wiResourceManager::GetGlobal()->add("shaders/screenVS.cso", wiResourceManager::VERTEXSHADER));

	pixelShader = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/imagePS.cso", wiResourceManager::PIXELSHADER));
	blurHPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/horizontalBlurPS.cso", wiResourceManager::PIXELSHADER));
	blurVPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/verticalBlurPS.cso", wiResourceManager::PIXELSHADER));
	shaftPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/lightShaftPS.cso", wiResourceManager::PIXELSHADER));
	outlinePS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/outlinePS.cso", wiResourceManager::PIXELSHADER));
	dofPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/depthofFieldPS.cso", wiResourceManager::PIXELSHADER));
	motionBlurPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/motionBlurPS.cso", wiResourceManager::PIXELSHADER));
	bloomSeparatePS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/bloomSeparatePS.cso", wiResourceManager::PIXELSHADER));
	fxaaPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/fxaa.cso", wiResourceManager::PIXELSHADER));
	ssaoPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/ssao.cso", wiResourceManager::PIXELSHADER));
	ssssPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/ssss.cso", wiResourceManager::PIXELSHADER));
	linDepthPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/linDepthPS.cso", wiResourceManager::PIXELSHADER));
	colorGradePS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/colorGradePS.cso", wiResourceManager::PIXELSHADER));
	deferredPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/deferredPS.cso", wiResourceManager::PIXELSHADER));
	ssrPS = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetGlobal()->add("shaders/ssr.cso", wiResourceManager::PIXELSHADER));
	



 //   ID3DBlob* pVSBlob = NULL;
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/imageVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load imageVS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );
	//if(pVSBlob){ pVSBlob->Release();pVSBlob=NULL; }

	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/screenVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load screenVS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &screenVS );
	//if(pVSBlob){ pVSBlob->Release();pVSBlob=NULL; }


	//ID3DBlob* pPSBlob = NULL;
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/imagePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load imagePS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }

	//if(FAILED(D3DReadFileToBlob(L"shaders/horizontalBlurPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load horizontalBlurPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &blurHPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/verticalBlurPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load verticalBlurPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &blurVPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/lightShaftPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load lightShaftPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &shaftPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/outlinePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load outlinePS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &outlinePS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/depthofFieldPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load depthofFieldPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &dofPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/motionBlurPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load motionBlurPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &motionBlurPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/bloomSeparatePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load bloomSeparatePS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &bloomSeparatePS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/fxaa.cso", &pPSBlob))){MessageBox(0,L"Failed To load fxaa.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &fxaaPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/ssao.cso", &pPSBlob))){MessageBox(0,L"Failed To load ssao.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &ssaoPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/ssss.cso", &pPSBlob))){MessageBox(0,L"Failed To load ssss.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &ssssPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/linDepthPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load linDepthPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &linDepthPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/colorGradePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load colorGradePS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &colorGradePS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
	//
	//if(FAILED(D3DReadFileToBlob(L"shaders/deferredPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load deferredPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &deferredPS );
	//if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }
}
void wiImage::SetUpStates()
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
	wiRenderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
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
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilStateLess);

	
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
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilStateGreater);
	
	dsd.StencilEnable = false;
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthNoStencilState);

	
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
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendState);

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
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendStateNoBlend);

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
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendStateAdd);
	
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
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendStateAvg);
}


void wiImage::Draw(wiRenderer::TextureView texture, const wiImageEffects& effects){
	Draw(texture,effects,wiRenderer::getImmediateContext());
}
void wiImage::Draw(wiRenderer::TextureView texture, const wiImageEffects& effects,ID3D11DeviceContext* context){
	if(!context)
		return;
	if(!effects.blur){
		if(!effects.process.active && !effects.bloom.separate && !effects.sunPos.x && !effects.sunPos.y){
			ConstantBuffer cb;
			if(effects.typeFlag==SCREEN){
				cb.mViewProjection = XMMatrixTranspose( wiRenderer::getCamera()->GetOProjection() );
				cb.mTrans = XMMatrixTranspose(XMMatrixTranslation(wiRenderer::RENDERWIDTH / 2 - effects.siz.x / 2, -wiRenderer::RENDERHEIGHT / 2 + effects.siz.y / 2, 0) * XMMatrixRotationZ(effects.rotation)
					* XMMatrixTranslation(-wiRenderer::RENDERWIDTH / 2 + effects.pos.x + effects.siz.x*0.5f, wiRenderer::RENDERHEIGHT / 2 + effects.pos.y - effects.siz.y*0.5f, 0)); //AUTO ORIGIN CORRECTION APPLIED! NO FURTHER TRANSLATIONS NEEDED!
				cb.mDimensions = XMFLOAT4((float)wiRenderer::RENDERWIDTH, (float)wiRenderer::RENDERHEIGHT, effects.siz.x, effects.siz.y);
			}
			else if(effects.typeFlag==WORLD){
				cb.mViewProjection = XMMatrixTranspose( wiRenderer::getCamera()->GetView() * wiRenderer::getCamera()->GetProjection() );
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
					faceRot=XMMatrixRotationQuaternion(XMLoadFloat4(&wiRenderer::getCamera()->rotation));
				cb.mTrans = XMMatrixTranspose(
					XMMatrixScaling(effects.scale.x,effects.scale.y,1)
					*XMMatrixRotationZ(effects.rotation)
					*faceRot
					//*XMMatrixInverse(0,XMMatrixLookAtLH(XMVectorSet(0,0,0,0),XMLoadFloat3(&effects.pos)-wiRenderer::getCamera()->Eye,XMVectorSet(0,1,0,0)))
					*XMMatrixTranslation(effects.pos.x,effects.pos.y,effects.pos.z)
					);
				cb.mDimensions = XMFLOAT4(0,0,effects.siz.x,effects.siz.y);
			}
	
			cb.mOffsetMirFade = XMFLOAT4(effects.offset.x,effects.offset.y,effects.mirror,effects.fade);
			cb.mDrawRec = effects.drawRec;
			cb.mBlurOpaPiv = XMFLOAT4(effects.blurDir, effects.blur, effects.opacity, (float)effects.pivotFlag);
			cb.mTexOffset = XMFLOAT4(effects.texOffset.x, effects.texOffset.y, effects.mipLevel, 0);

			wiRenderer::UpdateBuffer(constantBuffer,&cb,context);
	
			//context->UpdateSubresource( constantBuffer, 0, NULL, &cb, 0, 0 );

			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//ConstantBuffer* dataPtr;
			//context->Map(constantBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (ConstantBuffer*)mappedResource.pData;
			//memcpy(dataPtr,&cb,sizeof(ConstantBuffer));
			//context->Unmap(constantBuffer,0);

			//context->VSSetConstantBuffers( 0, 1, &constantBuffer );

			wiRenderer::BindConstantBufferVS(constantBuffer,0,context);
		}
		else if(!effects.sunPos.x && !effects.sunPos.y){
			//context->VSSetShader(screenVS,0,0);
			wiRenderer::BindVS(screenVS,context);
			

			if(effects.process.outline) 
				//context->PSSetShader(outlinePS,0,0);
					wiRenderer::BindPS(outlinePS,context);
			else if(effects.process.motionBlur) 
				//context->PSSetShader(motionBlurPS,0,0);
				wiRenderer::BindPS(motionBlurPS,context);
			else if(effects.process.dofStrength) 
				//context->PSSetShader(dofPS,0,0);
				wiRenderer::BindPS(dofPS,context);
			else if(effects.process.fxaa) 
				//context->PSSetShader(fxaaPS,0,0);
				wiRenderer::BindPS(fxaaPS,context);
			else if(effects.process.ssao) {
				//context->PSSetShader(ssaoPS,0,0);
				wiRenderer::BindPS(ssaoPS,context);
			}
			else if(effects.process.linDepth) 
				//context->PSSetShader(linDepthPS,0,0);
				wiRenderer::BindPS(linDepthPS, context);
			else if (effects.process.colorGrade)
				wiRenderer::BindPS(colorGradePS, context);
			else if (effects.process.ssr){
				wiRenderer::BindConstantBufferPS(deferredCb, 0, context);
				wiRenderer::BindPS(ssrPS, context);
			}
			else if(effects.process.ssss.x||effects.process.ssss.y) 
				//context->PSSetShader(ssssPS,0,0);
				wiRenderer::BindPS(ssssPS,context);
			else if(effects.bloom.separate)
				//context->PSSetShader(bloomSeparatePS,0,0);
				wiRenderer::BindPS(bloomSeparatePS,context);
			else wiHelper::messageBox("Postprocess branch not implemented!");
			
			ProcessBuffer prcb;
			prcb.mPostProcess=XMFLOAT4(effects.process.motionBlur,effects.process.outline,effects.process.dofStrength,effects.process.ssss.x);
			prcb.mBloom=XMFLOAT4(effects.bloom.separate,effects.bloom.threshold,effects.bloom.saturation,effects.process.ssss.y);

			wiRenderer::UpdateBuffer(processCb,&prcb,context);
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//ProcessBuffer* dataPtr2;
			//context->Map(processCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr2 = (ProcessBuffer*)mappedResource.pData;
			//memcpy(dataPtr2,&prcb,sizeof(ProcessBuffer));
			//context->Unmap(processCb,0);

			//context->PSSetConstantBuffers( 1, 1, &processCb );

			wiRenderer::BindConstantBufferPS(processCb,1,context);
		}
		else{ //LIGHTSHAFTS
			//context->VSSetShader(screenVS,0,0);
			//context->PSSetShader(shaftPS,0,0);
			wiRenderer::BindVS(screenVS,context);
			wiRenderer::BindPS(shaftPS,context);
			
			LightShaftBuffer scb;
			scb.mProperties=XMFLOAT4(0.65f,0.25f,0.945f,0.2f); //Density|Weight|Decay|Exposure
			scb.mLightShaft=effects.sunPos;

			wiRenderer::UpdateBuffer(shaftCb,&scb,context);
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//LightShaftBuffer* dataPtr2;
			//context->Map(shaftCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr2 = (LightShaftBuffer*)mappedResource.pData;
			//memcpy(dataPtr2,&scb,sizeof(LightShaftBuffer));
			//context->Unmap(shaftCb,0);

			//context->PSSetConstantBuffers( 0, 1, &shaftCb );
			wiRenderer::BindConstantBufferPS(shaftCb,0,context);
		}

		
		//D3D11_MAPPED_SUBRESOURCE mappedResource;
		PSConstantBuffer pscb;
		int normalmapmode=0;
		if(effects.normalMap && effects.refractionMap)
			normalmapmode=1;
		if(effects.extractNormalMap==true)
			normalmapmode=2;
		pscb.mMaskFadOpaDis = XMFLOAT4((effects.maskMap ? 1.f : 0.f), effects.fade, effects.opacity, (float)normalmapmode);
		pscb.mDimension = XMFLOAT4((float)wiRenderer::RENDERWIDTH, (float)wiRenderer::RENDERHEIGHT, effects.siz.x, effects.siz.y);

		wiRenderer::UpdateBuffer(PSCb,&pscb,context);
		//PSConstantBuffer* dataPtr2;
		//context->Map(PSCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
		//dataPtr2 = (PSConstantBuffer*)mappedResource.pData;
		//memcpy(dataPtr2,&pscb,sizeof(PSConstantBuffer));
		//context->Unmap(PSCb,0);

		//context->PSSetConstantBuffers( 2, 1, &PSCb );
		wiRenderer::BindConstantBufferPS(PSCb,2,context);
		
		wiRenderer::BindTexturePS(effects.depthMap,0,context);
		wiRenderer::BindTexturePS(effects.normalMap,1,context);
		wiRenderer::BindTexturePS(effects.velocityMap,3,context);
		wiRenderer::BindTexturePS(effects.refractionMap,4,context);
		wiRenderer::BindTexturePS(effects.maskMap,5,context);
	}
	else{ //BLUR
		wiRenderer::BindVS(screenVS,context);
		
		BlurBuffer cb;
		if(effects.blurDir==0){
			wiRenderer::BindPS(blurHPS,context);
			cb.mWeightTexelStrenMip.y = 1.0f / wiRenderer::RENDERWIDTH;
		}
		else{
			wiRenderer::BindPS(blurVPS,context);
			cb.mWeightTexelStrenMip.y = 1.0f / wiRenderer::RENDERHEIGHT;
		}

		float weight0 = 1.0f;
		float weight1 = 0.9f;
		float weight2 = 0.55f;
		float weight3 = 0.18f;
		float weight4 = 0.1f;
		float normalization = (weight0 + 2.0f * (weight1 + weight2 + weight3 + weight4));
		cb.mWeight=XMVectorSet(weight0,weight1,weight2,weight3) / normalization;
		cb.mWeightTexelStrenMip.x = weight4 / normalization;
		cb.mWeightTexelStrenMip.z = effects.blur;
		cb.mWeightTexelStrenMip.w = effects.mipLevel;

		wiRenderer::UpdateBuffer(blurCb,&cb,context);

		wiRenderer::BindConstantBufferPS(blurCb,0,context);
	}

	//if(texture) 
	wiRenderer::BindTexturePS(texture,6,context);

	
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT sampleMask   = 0xffffffff;
	if(effects.blendFlag==BLENDMODE_ALPHA)
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);
		wiRenderer::BindBlendState(blendState,context);
	else if(effects.blendFlag==BLENDMODE_ADDITIVE)
		//context->OMSetBlendState(blendStateAdd, blendFactor, sampleMask);
		wiRenderer::BindBlendState(blendStateAdd,context);
	else if(effects.blendFlag==BLENDMODE_OPAQUE)
		//context->OMSetBlendState(blendStateNoBlend, blendFactor, sampleMask);
		wiRenderer::BindBlendState(blendStateNoBlend,context);
	else if(effects.blendFlag==BLENDMODE_MAX)
		//context->OMSetBlendState(blendStateAvg, blendFactor, sampleMask);
		wiRenderer::BindBlendState(blendStateAvg,context);

	if(effects.quality==QUALITY_NEAREST){
		if(effects.sampleFlag==SAMPLEMODE_MIRROR)
			//context->PSSetSamplers(0, 1, &ssMirrorPoi);
			wiRenderer::BindSamplerPS(wiRenderer::ssMirrorPoi,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_WRAP)
			//context->PSSetSamplers(0, 1, &ssWrapPoi);
			wiRenderer::BindSamplerPS(wiRenderer::ssWrapPoi,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_CLAMP)
			//context->PSSetSamplers(0, 1, &ssClampPoi);
			wiRenderer::BindSamplerPS(wiRenderer::ssClampPoi,0,context);
	}
	else if(effects.quality==QUALITY_BILINEAR){
		if(effects.sampleFlag==SAMPLEMODE_MIRROR)
			//context->PSSetSamplers(0, 1, &ssMirrorLin);
			wiRenderer::BindSamplerPS(wiRenderer::ssMirrorLin,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_WRAP)
			//context->PSSetSamplers(0, 1, &ssWrapLin);
			wiRenderer::BindSamplerPS(wiRenderer::ssWrapLin,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_CLAMP)
			//context->PSSetSamplers(0, 1, &ssClampLin);
			wiRenderer::BindSamplerPS(wiRenderer::ssClampLin,0,context);
	}
	else if(effects.quality==QUALITY_ANISOTROPIC){
		if(effects.sampleFlag==SAMPLEMODE_MIRROR)
			//context->PSSetSamplers(0, 1, &ssMirrorAni);
			wiRenderer::BindSamplerPS(wiRenderer::ssMirrorAni,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_WRAP)
			//context->PSSetSamplers(0, 1, &ssWrapAni);
			wiRenderer::BindSamplerPS(wiRenderer::ssWrapAni,0,context);
		else if(effects.sampleFlag==SAMPLEMODE_CLAMP)
			//context->PSSetSamplers(0, 1, &ssClampAni);
			wiRenderer::BindSamplerPS(wiRenderer::ssClampAni,0,context);
	}

	
	//context->Draw(4,0);
	wiRenderer::Draw(4,context);
}

void wiImage::DrawDeferred(wiRenderer::TextureView texture
		, wiRenderer::TextureView depth, wiRenderer::TextureView lightmap, wiRenderer::TextureView normal
		, wiRenderer::TextureView ao, ID3D11DeviceContext* context, int stencilRef){
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	//UINT stride = sizeof( Vertex );
    //UINT offset = 0;
    //context->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
	//context->IASetInputLayout( vertexLayout );

	//context->RSSetState(rasterizerState);
	//context->OMSetDepthStencilState(depthStencilStateLess, stencilRef);

	wiRenderer::BindPrimitiveTopology(wiRenderer::PRIMITIVETOPOLOGY::TRIANGLESTRIP,context);
	wiRenderer::BindRasterizerState(rasterizerState,context);
	wiRenderer::BindDepthStencilState(depthStencilStateLess,stencilRef,context);

	//context->VSSetShader(screenVS,0,0);
	//context->PSSetShader(deferredPS,0,0);
	wiRenderer::BindVS(screenVS,context);
	wiRenderer::BindPS(deferredPS,context);
	
	wiRenderer::BindTexturePS(depth,0,context);
	wiRenderer::BindTexturePS(normal,1,context);
	wiRenderer::BindTexturePS(texture,6,context);
	wiRenderer::BindTexturePS(lightmap,7,context);
	wiRenderer::BindTexturePS(ao,8,context);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState, blendFactor, sampleMask);
	//context->PSSetSamplers(0, 1, &ssClampLin);
	wiRenderer::BindSamplerPS(wiRenderer::ssClampLin,0,context);
	//context->PSSetSamplers(1, 1, &ssComp);

	wiRenderer::BindBlendState(blendStateNoBlend,context);

	DeferredBuffer cb;
	//cb.mSun=XMVector3Normalize(wiRenderer::GetSunPosition());
	//cb.mEye=wiRenderer::getCamera()->Eye;
	cb.mAmbient=wiRenderer::worldInfo.ambient;
	//cb.mBiasResSoftshadow=shadowProps;
	cb.mHorizon=wiRenderer::worldInfo.horizon;
	cb.mViewProjInv=XMMatrixInverse( 0,XMMatrixTranspose(wiRenderer::getCamera()->GetView()*wiRenderer::getCamera()->GetProjection()) );
	cb.mFogSEH=wiRenderer::worldInfo.fogSEH;

	wiRenderer::UpdateBuffer(deferredCb,&cb,context);

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

	wiRenderer::BindConstantBufferPS(deferredCb,0,context);
	wiRenderer::Draw(4,context);
}


void wiImage::BatchBegin()
{
	BatchBegin(wiRenderer::getImmediateContext());
}
void wiImage::BatchBegin(ID3D11DeviceContext* context)
{
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	//context->RSSetState(rasterizerState);
	//context->OMSetDepthStencilState(depthNoStencilState, 0);

	
	wiRenderer::BindPrimitiveTopology(wiRenderer::PRIMITIVETOPOLOGY::TRIANGLESTRIP,context);
	wiRenderer::BindRasterizerState(rasterizerState,context);
	wiRenderer::BindDepthStencilState(depthNoStencilState, 0, context);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState,blendFactor,sampleMask);
	wiRenderer::BindBlendState(blendState,context);

	//context->VSSetShader( vertexShader, NULL, 0 );
	//context->PSSetShader( pixelShader, NULL, 0 );
	wiRenderer::BindVS(vertexShader,context);
	wiRenderer::BindPS(pixelShader,context);
}
void wiImage::BatchBegin(ID3D11DeviceContext* context, unsigned int stencilref, bool stencilOpLess)
{
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	//context->RSSetState(rasterizerState);
	//context->OMSetDepthStencilState(stencilOpLess?depthStencilStateLess:depthStencilStateGreater, stencilref);

	wiRenderer::BindPrimitiveTopology(wiRenderer::PRIMITIVETOPOLOGY::TRIANGLESTRIP,context);
	wiRenderer::BindRasterizerState(rasterizerState,context);
	wiRenderer::BindDepthStencilState(stencilOpLess?depthStencilStateLess:depthStencilStateGreater, stencilref, context);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState,blendFactor,sampleMask);
	wiRenderer::BindBlendState(blendState,context);

	//context->VSSetShader( vertexShader, NULL, 0 );
	//context->PSSetShader( pixelShader, NULL, 0 );
	wiRenderer::BindVS(vertexShader,context);
	wiRenderer::BindPS(pixelShader,context);
}


void wiImage::Load(){
	LoadShaders();
	LoadBuffers();
	SetUpStates();
}
void wiImage::CleanUp()
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
	wiRenderer::SafeRelease(colorGradePS);
	wiRenderer::SafeRelease(ssrPS);

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
wiImage::ImageResource::ImageResource(const string& newDirectory, const string& newName){
	name=newName;
	wstring wname(name.begin(),name.end());
	wstring wdir(newDirectory.begin(),newDirectory.end());
	wstringstream wss(L"");
	wss<<wdir<<wname;
	CreateWICTextureFromFile(false,wiRenderer::graphicsDevice,NULL,wss.str().c_str(),NULL,&texture,NULL);
	
	if(!texture) {
		stringstream ss("");
		ss<<newDirectory<<newName;
		MessageBoxA(0,ss.str().c_str(),"Loading Image Failed!",0);
	}
}
wiRenderer::TextureView wiImage::getImageByName(const string& get){
	map<string,ImageResource>::iterator iter = images.find(get);
	if(iter!=images.end())
		return iter->second.texture;
	return nullptr;
}
void wiImage::addImageResource(const string& dir, const string& name){
	MUTEX.lock();
	images.insert( pair<string,ImageResource>(name,ImageResource(dir,name)) );
	MUTEX.unlock();
}
*/