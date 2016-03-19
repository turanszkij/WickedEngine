#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiImageEffects.h"
#include "wiLoader.h"
#include "wiHelper.h"
#include "SamplerMapping.h"
#include "TextureMapping.h"

#pragma region STATICS
BlendState		wiImage::blendState, wiImage::blendStateAdd, wiImage::blendStateNoBlend, wiImage::blendStateAvg;
BufferResource           wiImage::constantBuffer,wiImage::processCb;

VertexShader     wiImage::vertexShader,wiImage::screenVS;
PixelShader      wiImage::pixelShader,wiImage::blurHPS,wiImage::blurVPS,wiImage::shaftPS,wiImage::outlinePS
	,wiImage::dofPS,wiImage::motionBlurPS,wiImage::bloomSeparatePS,wiImage::fxaaPS,wiImage::ssaoPS,wiImage::deferredPS
	,wiImage::ssssPS,wiImage::linDepthPS,wiImage::colorGradePS,wiImage::ssrPS, wiImage::screenPS, wiImage::stereogramPS;
	

RasterizerState		wiImage::rasterizerState;
DepthStencilState	wiImage::depthStencilStateGreater,wiImage::depthStencilStateLess,wiImage::depthNoStencilState;

#pragma endregion

wiImage::wiImage()
{
}

void wiImage::LoadBuffers()
{
	BufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ImageCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PostProcessCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer(&bd, NULL, &processCb);

	BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
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
	stereogramPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "stereogramPS.cso", wiResourceManager::PIXELSHADER));

}
void wiImage::SetUpStates()
{

	
	RasterizerDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=false;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=FALSE;
	rs.ScissorEnable=FALSE;
	rs.MultisampleEnable=FALSE;
	rs.AntialiasedLineEnable=FALSE;
	wiRenderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
	DepthStencilDesc dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = 0;
	dsd.FrontFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	// Create the depth stencil state.
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilStateLess);

	
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = 0;
	dsd.FrontFace.StencilFunc = COMPARISON_GREATER;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_GREATER;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	// Create the depth stencil state.
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilStateGreater);
	
	dsd.StencilEnable = false;
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthNoStencilState);

	
	BlendDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.IndependentBlendEnable=true;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendState);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	//bd.IndependentBlendEnable=true;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendStateNoBlend);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	//bd.IndependentBlendEnable=true;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendStateAdd);
	
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_COLOR;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_MAX;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_MAX;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	//bd.IndependentBlendEnable=true;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendStateAvg);
}

void wiImage::BindPersistentState(GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->LOCK();

	wiRenderer::graphicsDevice->BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);
	wiRenderer::graphicsDevice->BindConstantBufferPS(constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);

	wiRenderer::graphicsDevice->BindConstantBufferPS(processCb, CB_GETBINDSLOT(PostProcessCB), threadID);

	wiRenderer::graphicsDevice->UNLOCK();
}

void wiImage::Draw(Texture2D* texture, const wiImageEffects& effects){
	Draw(texture,effects,GRAPHICSTHREAD_IMMEDIATE);
}
void wiImage::Draw(Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID)
{

	bool fullScreenEffect = false;

	wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
	wiRenderer::graphicsDevice->BindVertexBuffer(nullptr, 0, 0, threadID);
	wiRenderer::graphicsDevice->BindIndexBuffer(nullptr, threadID);
	wiRenderer::graphicsDevice->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP, threadID);
	wiRenderer::graphicsDevice->BindRasterizerState(rasterizerState, threadID);

	wiRenderer::graphicsDevice->BindTexturePS(texture, TEXSLOT_ONDEMAND0, threadID);

	if (effects.blendFlag == BLENDMODE_ALPHA)
		wiRenderer::graphicsDevice->BindBlendState(blendState, threadID);
	else if (effects.blendFlag == BLENDMODE_ADDITIVE)
		wiRenderer::graphicsDevice->BindBlendState(blendStateAdd, threadID);
	else if (effects.blendFlag == BLENDMODE_OPAQUE)
		wiRenderer::graphicsDevice->BindBlendState(blendStateNoBlend, threadID);
	else if (effects.blendFlag == BLENDMODE_MAX)
		wiRenderer::graphicsDevice->BindBlendState(blendStateAvg, threadID);
	else
		wiRenderer::graphicsDevice->BindBlendState(blendState, threadID);

	if (effects.presentFullScreen)
	{
		wiRenderer::graphicsDevice->BindVS(screenVS, threadID);
		wiRenderer::graphicsDevice->BindPS(screenPS, threadID);
		wiRenderer::graphicsDevice->Draw(3, threadID);
		return;
	}

	static thread_local ImageCB* cb = new ImageCB;
	static thread_local PostProcessCB* prcb = new PostProcessCB;
	{
		switch (effects.stencilComp)
		{
		case COMPARISON_LESS:
			wiRenderer::graphicsDevice->BindDepthStencilState(depthStencilStateLess, effects.stencilRef, threadID);
			break;
		case COMPARISON_GREATER:
			wiRenderer::graphicsDevice->BindDepthStencilState(depthStencilStateGreater, effects.stencilRef, threadID);
			break;
		default:
			wiRenderer::graphicsDevice->BindDepthStencilState(depthNoStencilState, effects.stencilRef, threadID);
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
			(*cb).mDistort = effects.distortionMap != nullptr; 
			(*cb).mMirror = effects.mirror;
			
			int normalmapmode = 0;
			if(effects.distortionMap && effects.refractionSource)
				normalmapmode=1;
			if(effects.extractNormalMap==true)
				normalmapmode=2;
			(*cb).mNormalmapSeparate = normalmapmode;
			(*cb).mMipLevel = effects.mipLevel;

			wiRenderer::graphicsDevice->UpdateBuffer(constantBuffer,cb,threadID);

			wiRenderer::graphicsDevice->BindVS(vertexShader, threadID);
			wiRenderer::graphicsDevice->BindPS(pixelShader, threadID);
			fullScreenEffect = false;
		}
		else if(!effects.sunPos.x && !effects.sunPos.y){
			wiRenderer::graphicsDevice->BindVS(screenVS, threadID);
			fullScreenEffect = true;

			if(effects.process.outline) 
				wiRenderer::graphicsDevice->BindPS(outlinePS,threadID);
			else if(effects.process.motionBlur) 
				wiRenderer::graphicsDevice->BindPS(motionBlurPS,threadID);
			else if(effects.process.dofStrength) 
				wiRenderer::graphicsDevice->BindPS(dofPS,threadID);
			else if(effects.process.fxaa) 
				wiRenderer::graphicsDevice->BindPS(fxaaPS,threadID);
			else if(effects.process.ssao)
				wiRenderer::graphicsDevice->BindPS(ssaoPS,threadID);
			else if(effects.process.linDepth) 
				wiRenderer::graphicsDevice->BindPS(linDepthPS, threadID);
			else if (effects.process.colorGrade)
				wiRenderer::graphicsDevice->BindPS(colorGradePS, threadID);
			else if (effects.process.ssr)
				wiRenderer::graphicsDevice->BindPS(ssrPS, threadID);
			else if (effects.process.stereogram)
				wiRenderer::graphicsDevice->BindPS(stereogramPS, threadID);
			else if(effects.process.ssss.x + effects.process.ssss.y > 0)
				wiRenderer::graphicsDevice->BindPS(ssssPS,threadID);
			else if(effects.bloom.separate)
				wiRenderer::graphicsDevice->BindPS(bloomSeparatePS,threadID);
			else 
				wiHelper::messageBox("Postprocess branch not implemented!");
			
			(*prcb).params0[0] = effects.process.motionBlur; 
			(*prcb).params0[1] = effects.process.outline;
			(*prcb).params0[2] = effects.process.dofStrength;
			(*prcb).params0[3] = effects.process.ssss.x;
			(*prcb).params1[0] = effects.bloom.separate;
			(*prcb).params1[1] = effects.bloom.threshold;
			(*prcb).params1[2] = effects.bloom.saturation;
			(*prcb).params1[3] = effects.process.ssss.y;

			wiRenderer::graphicsDevice->UpdateBuffer(processCb,prcb,threadID);
		}
		else{ 
			wiRenderer::graphicsDevice->BindVS(screenVS,threadID);
			wiRenderer::graphicsDevice->BindPS(shaftPS,threadID);
			fullScreenEffect = true;

			 //Density|Weight|Decay|Exposure
			(*prcb).params0[0] = 0.65f;
			(*prcb).params0[1] = 0.25f;
			(*prcb).params0[2] = 0.945f;
			(*prcb).params0[3] = 0.2f;
			(*prcb).params1[0] = effects.sunPos.x;
			(*prcb).params1[1] = effects.sunPos.y;

			wiRenderer::graphicsDevice->UpdateBuffer(processCb,prcb,threadID);
		}
		wiRenderer::graphicsDevice->BindTexturePS(effects.maskMap, TEXSLOT_ONDEMAND1, threadID);
		wiRenderer::graphicsDevice->BindTexturePS(effects.distortionMap, TEXSLOT_ONDEMAND2, threadID);
		wiRenderer::graphicsDevice->BindTexturePS(effects.refractionSource, TEXSLOT_ONDEMAND3, threadID);
	}
	else{ //BLUR
		wiRenderer::graphicsDevice->BindVS(screenVS,threadID);
		fullScreenEffect = true;
		
		if(effects.blurDir==0){
			wiRenderer::graphicsDevice->BindPS(blurHPS,threadID);
			(*prcb).params1[3] = 1.0f / wiRenderer::GetScreenWidth();
		}
		else{
			wiRenderer::graphicsDevice->BindPS(blurVPS,threadID);
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

		wiRenderer::graphicsDevice->UpdateBuffer(processCb,prcb,threadID);

	}


	if(effects.quality==QUALITY_NEAREST){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if(effects.quality==QUALITY_BILINEAR){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if(effects.quality==QUALITY_ANISOTROPIC){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::graphicsDevice->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_CLAMP], SSLOT_ONDEMAND0, threadID);
	}

	
	wiRenderer::graphicsDevice->Draw((fullScreenEffect ? 3 : 4), threadID);
}

void wiImage::DrawDeferred(Texture2D* texture
		, Texture2D* depth, Texture2D* lightmap, Texture2D* normal
		, Texture2D* ao, GRAPHICSTHREAD threadID, int stencilRef){

	wiRenderer::graphicsDevice->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,threadID);
	wiRenderer::graphicsDevice->BindRasterizerState(rasterizerState,threadID);
	wiRenderer::graphicsDevice->BindDepthStencilState(depthNoStencilState,stencilRef,threadID);

	wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
	wiRenderer::graphicsDevice->BindVertexBuffer(nullptr, 0, 0, threadID);
	wiRenderer::graphicsDevice->BindIndexBuffer(nullptr, threadID);

	wiRenderer::graphicsDevice->BindVS(screenVS,threadID);
	wiRenderer::graphicsDevice->BindPS(deferredPS,threadID);
	
	//wiRenderer::graphicsDevice->BindTexturePS(depth,0,threadID);
	//wiRenderer::graphicsDevice->BindTexturePS(normal,1,threadID);
	//wiRenderer::graphicsDevice->BindTexturePS(texture,6,threadID);
	wiRenderer::graphicsDevice->BindTexturePS(lightmap,TEXSLOT_ONDEMAND0,threadID);
	wiRenderer::graphicsDevice->BindTexturePS(ao,TEXSLOT_ONDEMAND1,threadID);

	wiRenderer::graphicsDevice->BindBlendState(blendStateNoBlend,threadID);

	wiRenderer::graphicsDevice->Draw(3,threadID);
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
	SAFE_RELEASE(colorGradePS);
	SAFE_RELEASE(ssrPS);
	SAFE_RELEASE(screenPS);
	SAFE_RELEASE(stereogramPS);

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
