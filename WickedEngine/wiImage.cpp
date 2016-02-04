#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiImageEffects.h"
#include "wiLoader.h"
#include "wiHelper.h"
#include "SamplerMapping.h"

#pragma region STATICS
BlendState		wiImage::blendState, wiImage::blendStateAdd, wiImage::blendStateNoBlend, wiImage::blendStateAvg;
BufferResource           wiImage::constantBuffer,wiImage::processCb;

VertexShader     wiImage::vertexShader,wiImage::screenVS;
PixelShader      wiImage::pixelShader,wiImage::blurHPS,wiImage::blurVPS,wiImage::shaftPS,wiImage::outlinePS
	,wiImage::dofPS,wiImage::motionBlurPS,wiImage::bloomSeparatePS,wiImage::fxaaPS,wiImage::ssaoPS,wiImage::deferredPS
	,wiImage::ssssPS,wiImage::linDepthPS,wiImage::colorGradePS,wiImage::ssrPS, wiImage::screenPS;
	

RasterizerState		wiImage::rasterizerState;
DepthStencilState	wiImage::depthStencilStateGreater,wiImage::depthStencilStateLess,wiImage::depthNoStencilState;

#pragma endregion

wiImage::wiImage()
{
}

void wiImage::LoadBuffers()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ImageCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PostProcessCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer(&bd, NULL, &processCb);

	BindPersistentState(wiRenderer::getImmediateContext());
}

void wiImage::LoadShaders()
{


	vertexShader = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imageVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	screenVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "screenVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	pixelShader = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS.cso", wiResourceManager::PIXELSHADER));
	blurHPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "horizontalBlurPS.cso", wiResourceManager::PIXELSHADER));
	blurVPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "verticalBlurPS.cso", wiResourceManager::PIXELSHADER));
	shaftPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lightShaftPS.cso", wiResourceManager::PIXELSHADER));
	outlinePS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "outlinePS.cso", wiResourceManager::PIXELSHADER));
	dofPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "depthofFieldPS.cso", wiResourceManager::PIXELSHADER));
	motionBlurPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "motionBlurPS.cso", wiResourceManager::PIXELSHADER));
	bloomSeparatePS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "bloomSeparatePS.cso", wiResourceManager::PIXELSHADER));
	fxaaPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fxaa.cso", wiResourceManager::PIXELSHADER));
	ssaoPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssao.cso", wiResourceManager::PIXELSHADER));
	ssssPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssss.cso", wiResourceManager::PIXELSHADER));
	linDepthPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "linDepthPS.cso", wiResourceManager::PIXELSHADER));
	colorGradePS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "colorGradePS.cso", wiResourceManager::PIXELSHADER));
	deferredPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "deferredPS.cso", wiResourceManager::PIXELSHADER));
	ssrPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssr.cso", wiResourceManager::PIXELSHADER));
	screenPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "screenPS.cso", wiResourceManager::PIXELSHADER));
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

void wiImage::BindPersistentState(DeviceContext context)
{
	wiRenderer::Lock();

	wiRenderer::BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(ImageCB), context);
	wiRenderer::BindConstantBufferPS(constantBuffer, CB_GETBINDSLOT(ImageCB), context);

	wiRenderer::BindConstantBufferPS(processCb, CB_GETBINDSLOT(PostProcessCB), context);

	wiRenderer::Unlock();
}

void wiImage::Draw(TextureView texture, const wiImageEffects& effects){
	Draw(texture,effects,wiRenderer::getImmediateContext());
}
void wiImage::Draw(TextureView texture, const wiImageEffects& effects,DeviceContext context){
	if(!context)
		return;

	bool fullScreenEffect = false;

	wiRenderer::BindVertexLayout(nullptr, context);
	wiRenderer::BindVertexBuffer(nullptr, 0, 0, context);
	wiRenderer::BindIndexBuffer(nullptr, context);
	wiRenderer::BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP, context);
	wiRenderer::BindRasterizerState(rasterizerState, context);

	wiRenderer::BindTexturePS(texture, 6, context);

	if (effects.blendFlag == BLENDMODE_ALPHA)
		wiRenderer::BindBlendState(blendState, context);
	else if (effects.blendFlag == BLENDMODE_ADDITIVE)
		wiRenderer::BindBlendState(blendStateAdd, context);
	else if (effects.blendFlag == BLENDMODE_OPAQUE)
		wiRenderer::BindBlendState(blendStateNoBlend, context);
	else if (effects.blendFlag == BLENDMODE_MAX)
		wiRenderer::BindBlendState(blendStateAvg, context);
	else
		wiRenderer::BindBlendState(blendState, context);

	if (effects.presentFullScreen)
	{
		wiRenderer::BindVS(screenVS, context);
		wiRenderer::BindPS(screenPS, context);
		wiRenderer::Draw(3, context);
		return;
	}

	static thread_local ImageCB* cb = new ImageCB;
	static thread_local PostProcessCB* prcb = new PostProcessCB;
	{
		switch (effects.stencilComp)
		{
		case D3D11_COMPARISON_LESS:
			wiRenderer::BindDepthStencilState(depthStencilStateLess, effects.stencilRef, context);
			break;
		case D3D11_COMPARISON_GREATER:
			wiRenderer::BindDepthStencilState(depthStencilStateGreater, effects.stencilRef, context);
			break;
		default:
			wiRenderer::BindDepthStencilState(depthNoStencilState, effects.stencilRef, context);
			break;
		}
	}

	if(!effects.blur){
		if(!effects.process.active && !effects.bloom.separate && !effects.sunPos.x && !effects.sunPos.y){
			if(effects.typeFlag==SCREEN){
				(*cb).mViewProjection = XMMatrixTranspose(XMMatrixOrthographicLH((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), 0, 100));
				(*cb).mTrans = XMMatrixTranspose(XMMatrixTranslation(wiRenderer::GetScreenWidth() / 2 - effects.siz.x / 2, -wiRenderer::GetScreenHeight() / 2 + effects.siz.y / 2, 0) * XMMatrixRotationZ(effects.rotation)
					* XMMatrixTranslation(-wiRenderer::GetScreenWidth() / 2 + effects.pos.x + effects.siz.x*0.5f, wiRenderer::GetScreenHeight() / 2 + effects.pos.y - effects.siz.y*0.5f, 0)); //AUTO ORIGIN CORRECTION APPLIED! NO FURTHER TRANSLATIONS NEEDED!
				(*cb).mDimensions = XMFLOAT4((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), effects.siz.x, effects.siz.y);
			}
			else if(effects.typeFlag==WORLD){
				(*cb).mViewProjection = XMMatrixTranspose( wiRenderer::getCamera()->GetView() * wiRenderer::getCamera()->GetProjection() );
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
				(*cb).mTrans = XMMatrixTranspose(
					XMMatrixScaling(effects.scale.x,effects.scale.y,1)
					*XMMatrixRotationZ(effects.rotation)
					*faceRot
					//*XMMatrixInverse(0,XMMatrixLookAtLH(XMVectorSet(0,0,0,0),XMLoadFloat3(&effects.pos)-wiRenderer::getCamera()->Eye,XMVectorSet(0,1,0,0)))
					*XMMatrixTranslation(effects.pos.x,effects.pos.y,effects.pos.z)
					);
				(*cb).mDimensions = XMFLOAT4(0,0,effects.siz.x,effects.siz.y);
			}
	
			(*cb).mDrawRec = effects.drawRec;
			(*cb).mTexMulAdd = XMFLOAT4(1,1,effects.texOffset.x, effects.texOffset.y);
			(*cb).mOffsetX = effects.offset.x;
			(*cb).mOffsetY = effects.offset.y;
			(*cb).mPivot = (UINT)effects.pivotFlag;
			(*cb).mFade = effects.fade;
			(*cb).mOpacity = effects.opacity;
			(*cb).mMask = effects.maskMap != nullptr;
			(*cb).mDistort = effects.normalMap != nullptr; 
			(*cb).mMirror = effects.mirror;
			
			int normalmapmode = 0;
			if(effects.normalMap && effects.refractionMap)
				normalmapmode=1;
			if(effects.extractNormalMap==true)
				normalmapmode=2;
			(*cb).mNormalmapSeparate = normalmapmode;
			(*cb).mMipLevel = effects.mipLevel;

			wiRenderer::UpdateBuffer(constantBuffer,cb,context);

			wiRenderer::BindVS(vertexShader, context);
			wiRenderer::BindPS(pixelShader, context);
			fullScreenEffect = false;
		}
		else if(!effects.sunPos.x && !effects.sunPos.y){
			wiRenderer::BindVS(screenVS, context);
			fullScreenEffect = true;

			if(effects.process.outline) 
					wiRenderer::BindPS(outlinePS,context);
			else if(effects.process.motionBlur) 
				wiRenderer::BindPS(motionBlurPS,context);
			else if(effects.process.dofStrength) 
				wiRenderer::BindPS(dofPS,context);
			else if(effects.process.fxaa) 
				wiRenderer::BindPS(fxaaPS,context);
			else if(effects.process.ssao) {
				wiRenderer::BindPS(ssaoPS,context);
			}
			else if(effects.process.linDepth) 
				wiRenderer::BindPS(linDepthPS, context);
			else if (effects.process.colorGrade)
				wiRenderer::BindPS(colorGradePS, context);
			else if (effects.process.ssr){
				wiRenderer::BindPS(ssrPS, context);
			}
			else if(effects.process.ssss.x + effects.process.ssss.y > 0)
				wiRenderer::BindPS(ssssPS,context);
			else if(effects.bloom.separate)
				wiRenderer::BindPS(bloomSeparatePS,context);
			else wiHelper::messageBox("Postprocess branch not implemented!");
			
			(*prcb).params0[0] = effects.process.motionBlur; 
			(*prcb).params0[1] = effects.process.outline;
			(*prcb).params0[2] = effects.process.dofStrength;
			(*prcb).params0[3] = effects.process.ssss.x;
			(*prcb).params1[0] = effects.bloom.separate;
			(*prcb).params1[1] = effects.bloom.threshold;
			(*prcb).params1[2] = effects.bloom.saturation;
			(*prcb).params1[3] = effects.process.ssss.y;

			wiRenderer::UpdateBuffer(processCb,prcb,context);
		}
		else{ 
			wiRenderer::BindVS(screenVS,context);
			wiRenderer::BindPS(shaftPS,context);
			fullScreenEffect = true;

			 //Density|Weight|Decay|Exposure
			(*prcb).params0[0] = 0.65f;
			(*prcb).params0[1] = 0.25f;
			(*prcb).params0[2] = 0.945f;
			(*prcb).params0[3] = 0.2f;
			(*prcb).params1[0] = effects.sunPos.x;
			(*prcb).params1[1] = effects.sunPos.y;

			wiRenderer::UpdateBuffer(processCb,prcb,context);
		}

		
		//int normalmapmode=0;
		//if(effects.normalMap && effects.refractionMap)
		//	normalmapmode=1;
		//if(effects.extractNormalMap==true)
		//	normalmapmode=2;
		//(*prcb).params[0] = (effects.maskMap ? 1.f : 0.f);
		//(*prcb).params[1] = effects.fade;
		//(*prcb).params[2] = effects.opacity;
		//(*prcb).params[3] = (float)normalmapmode;
		//(*prcb).params[4] = (float)wiRenderer::GetScreenWidth();
		//(*prcb).params[5] = (float)wiRenderer::GetScreenHeight();
		//(*prcb).params[6] = effects.siz.x;
		//(*prcb).params[7] = effects.siz.y;

		//wiRenderer::UpdateBuffer(processCb,prcb,context);
		
		wiRenderer::BindTexturePS(effects.depthMap,0,context);
		wiRenderer::BindTexturePS(effects.normalMap,1,context);
		wiRenderer::BindTexturePS(effects.velocityMap,3,context);
		wiRenderer::BindTexturePS(effects.refractionMap,4,context);
		wiRenderer::BindTexturePS(effects.maskMap,5,context);
	}
	else{ //BLUR
		wiRenderer::BindVS(screenVS,context);
		fullScreenEffect = true;
		
		if(effects.blurDir==0){
			wiRenderer::BindPS(blurHPS,context);
			(*prcb).params1[3] = 1.0f / wiRenderer::GetScreenWidth();
		}
		else{
			wiRenderer::BindPS(blurVPS,context);
			(*prcb).params1[3] = 1.0f / wiRenderer::GetScreenHeight();
		}

		static float weight0 = 1.0f;
		static float weight1 = 0.9f;
		static float weight2 = 0.55f;
		static float weight3 = 0.18f;
		static float weight4 = 0.1f;
		const float normalization = 1.0f / (weight0 + 2.0f * (weight1 + weight2 + weight3 + weight4));
		(*prcb).params0[0] = weight0 * normalization;
		(*prcb).params0[1] = weight1 * normalization;
		(*prcb).params0[2] = weight2 * normalization;
		(*prcb).params0[3] = weight3 * normalization;
		(*prcb).params1[0] = weight4 * normalization;
		(*prcb).params1[1] = effects.blur;
		(*prcb).params1[2] = effects.mipLevel;

		wiRenderer::UpdateBuffer(processCb,prcb,context);

	}


	if(effects.quality==QUALITY_NEAREST){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_MIRROR], SSLOT_ONDEMAND0, context);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_WRAP], SSLOT_ONDEMAND0, context);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, context);
	}
	else if(effects.quality==QUALITY_BILINEAR){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_MIRROR], SSLOT_ONDEMAND0, context);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_WRAP], SSLOT_ONDEMAND0, context);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, context);
	}
	else if(effects.quality==QUALITY_ANISOTROPIC){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_MIRROR], SSLOT_ONDEMAND0, context);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_WRAP], SSLOT_ONDEMAND0, context);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_CLAMP], SSLOT_ONDEMAND0, context);
	}

	
	wiRenderer::Draw((fullScreenEffect ? 3 : 4), context);
}

void wiImage::DrawDeferred(TextureView texture
		, TextureView depth, TextureView lightmap, TextureView normal
		, TextureView ao, DeviceContext context, int stencilRef){

	wiRenderer::BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,context);
	wiRenderer::BindRasterizerState(rasterizerState,context);
	wiRenderer::BindDepthStencilState(depthNoStencilState,stencilRef,context);

	wiRenderer::BindVertexLayout(nullptr, context);
	wiRenderer::BindVertexBuffer(nullptr, 0, 0, context);
	wiRenderer::BindIndexBuffer(nullptr, context);

	wiRenderer::BindVS(screenVS,context);
	wiRenderer::BindPS(deferredPS,context);
	
	wiRenderer::BindTexturePS(depth,0,context);
	wiRenderer::BindTexturePS(normal,1,context);
	wiRenderer::BindTexturePS(texture,6,context);
	wiRenderer::BindTexturePS(lightmap,7,context);
	wiRenderer::BindTexturePS(ao,8,context);

	wiRenderer::BindBlendState(blendStateNoBlend,context);

	wiRenderer::Draw(3,context);
}


void wiImage::Load(){
	LoadShaders();
	LoadBuffers();
	SetUpStates();
}
void wiImage::CleanUp()
{
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
	wiRenderer::SafeRelease(screenPS);

	if(constantBuffer) constantBuffer->Release();
	if(processCb) processCb->Release();

	if(rasterizerState) rasterizerState->Release();
	if(blendState) blendState->Release();
	if(blendStateAdd) blendStateAdd->Release();
	if(blendStateNoBlend) blendStateNoBlend->Release();
	if(blendStateAvg) blendStateAvg->Release();
	
	
	if(depthNoStencilState) depthNoStencilState->Release(); depthNoStencilState=nullptr;
	if(depthStencilStateLess) depthStencilStateLess->Release(); depthStencilStateLess=nullptr;
	if(depthStencilStateGreater) depthStencilStateGreater->Release(); depthStencilStateGreater=nullptr;
}
