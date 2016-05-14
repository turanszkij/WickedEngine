#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiImageEffects.h"
#include "wiLoader.h"
#include "wiHelper.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"

using namespace wiGraphicsTypes;

#pragma region STATICS
BlendState		*wiImage::blendState = nullptr, *wiImage::blendStateAdd = nullptr, *wiImage::blendStateNoBlend = nullptr, *wiImage::blendStateAvg = nullptr;
GPUBuffer           *wiImage::constantBuffer = nullptr,*wiImage::processCb = nullptr;

VertexShader     *wiImage::vertexShader = nullptr,*wiImage::screenVS = nullptr;
PixelShader      *wiImage::pixelShader = nullptr, *wiImage::blurHPS = nullptr, *wiImage::blurVPS = nullptr, *wiImage::shaftPS = nullptr, *wiImage::outlinePS = nullptr
	, *wiImage::dofPS = nullptr, *wiImage::motionBlurPS = nullptr, *wiImage::bloomSeparatePS = nullptr, *wiImage::fxaaPS = nullptr, *wiImage::ssaoPS = nullptr, *wiImage::deferredPS = nullptr
	, *wiImage::ssssPS = nullptr, *wiImage::linDepthPS = nullptr, *wiImage::colorGradePS = nullptr, *wiImage::ssrPS = nullptr, *wiImage::screenPS = nullptr, *wiImage::stereogramPS = nullptr
	, *wiImage::tonemapPS = nullptr;
	

RasterizerState		*wiImage::rasterizerState = nullptr;
DepthStencilState	*wiImage::depthStencilStateGreater = nullptr,*wiImage::depthStencilStateLess = nullptr,*wiImage::depthNoStencilState = nullptr;

#pragma endregion

wiImage::wiImage()
{
}

void wiImage::LoadBuffers()
{

	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ImageCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	constantBuffer = new GPUBuffer;
    wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, constantBuffer );

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PostProcessCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	processCb = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, NULL, processCb);

	BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
}

void wiImage::LoadShaders()
{

	vertexShader = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imageVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	screenVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "screenVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS.cso", wiResourceManager::PIXELSHADER));
	blurHPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "horizontalBlurPS.cso", wiResourceManager::PIXELSHADER));
	blurVPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "verticalBlurPS.cso", wiResourceManager::PIXELSHADER));
	shaftPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lightShaftPS.cso", wiResourceManager::PIXELSHADER));
	outlinePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "outlinePS.cso", wiResourceManager::PIXELSHADER));
	dofPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "depthofFieldPS.cso", wiResourceManager::PIXELSHADER));
	motionBlurPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "motionBlurPS.cso", wiResourceManager::PIXELSHADER));
	bloomSeparatePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "bloomSeparatePS.cso", wiResourceManager::PIXELSHADER));
	fxaaPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fxaa.cso", wiResourceManager::PIXELSHADER));
	ssaoPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssao.cso", wiResourceManager::PIXELSHADER));
	ssssPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssss.cso", wiResourceManager::PIXELSHADER));
	linDepthPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "linDepthPS.cso", wiResourceManager::PIXELSHADER));
	colorGradePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "colorGradePS.cso", wiResourceManager::PIXELSHADER));
	deferredPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "deferredPS.cso", wiResourceManager::PIXELSHADER));
	ssrPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssr.cso", wiResourceManager::PIXELSHADER));
	screenPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "screenPS.cso", wiResourceManager::PIXELSHADER));
	stereogramPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "stereogramPS.cso", wiResourceManager::PIXELSHADER));
	tonemapPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "toneMapPS.cso", wiResourceManager::PIXELSHADER));

}
void wiImage::SetUpStates()
{
	
	RasterizerStateDesc rs;
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
	rasterizerState = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,rasterizerState);




	
	DepthStencilStateDesc dsd;
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
	depthStencilStateLess = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilStateLess);

	
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
	depthStencilStateGreater = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilStateGreater);
	
	dsd.StencilEnable = false;
	depthNoStencilState = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthNoStencilState);

	
	BlendStateDesc bd;
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
	blendState = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendState);

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
	blendStateNoBlend = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateNoBlend);

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
	blendStateAdd = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateAdd);
	
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
	blendStateAvg = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateAvg);
}

void wiImage::BindPersistentState(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->LOCK();

	wiRenderer::GetDevice()->BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);
	wiRenderer::GetDevice()->BindConstantBufferPS(constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);

	wiRenderer::GetDevice()->BindConstantBufferPS(processCb, CB_GETBINDSLOT(PostProcessCB), threadID);

	wiRenderer::GetDevice()->UNLOCK();
}

void wiImage::Draw(Texture2D* texture, const wiImageEffects& effects){
	Draw(texture,effects,GRAPHICSTHREAD_IMMEDIATE);
}
void wiImage::Draw(Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID)
{

	bool fullScreenEffect = false;

	wiRenderer::GetDevice()->BindVertexLayout(nullptr, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
	wiRenderer::GetDevice()->BindIndexBuffer(nullptr, threadID);
	wiRenderer::GetDevice()->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP, threadID);
	wiRenderer::GetDevice()->BindRasterizerState(rasterizerState, threadID);

	wiRenderer::GetDevice()->BindResourcePS(texture, TEXSLOT_ONDEMAND0, threadID);

	if (effects.blendFlag == BLENDMODE_ALPHA)
		wiRenderer::GetDevice()->BindBlendState(blendState, threadID);
	else if (effects.blendFlag == BLENDMODE_ADDITIVE)
		wiRenderer::GetDevice()->BindBlendState(blendStateAdd, threadID);
	else if (effects.blendFlag == BLENDMODE_OPAQUE)
		wiRenderer::GetDevice()->BindBlendState(blendStateNoBlend, threadID);
	else if (effects.blendFlag == BLENDMODE_MAX)
		wiRenderer::GetDevice()->BindBlendState(blendStateAvg, threadID);
	else
		wiRenderer::GetDevice()->BindBlendState(blendState, threadID);

	if (effects.presentFullScreen)
	{
		wiRenderer::GetDevice()->BindVS(screenVS, threadID);
		wiRenderer::GetDevice()->BindPS(screenPS, threadID);
		wiRenderer::GetDevice()->Draw(3, threadID);
		return;
	}

	ImageCB cb;
	PostProcessCB prcb;
	{
		switch (effects.stencilComp)
		{
		case COMPARISON_LESS:
			wiRenderer::GetDevice()->BindDepthStencilState(depthStencilStateLess, effects.stencilRef, threadID);
			break;
		case COMPARISON_GREATER:
			wiRenderer::GetDevice()->BindDepthStencilState(depthStencilStateGreater, effects.stencilRef, threadID);
			break;
		default:
			wiRenderer::GetDevice()->BindDepthStencilState(depthNoStencilState, effects.stencilRef, threadID);
			break;
		}
	}

	if(!effects.blur){
		if(!effects.process.active && !effects.bloom.separate && !effects.sunPos.x && !effects.sunPos.y){
			if(effects.typeFlag==SCREEN){
				cb.mViewProjection = XMMatrixTranspose(XMMatrixOrthographicLH((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0, 100));
				cb.mTrans = XMMatrixTranspose(XMMatrixTranslation(wiRenderer::GetDevice()->GetScreenWidth() / 2 - effects.siz.x / 2, -wiRenderer::GetDevice()->GetScreenHeight() / 2 + effects.siz.y / 2, 0) * XMMatrixRotationZ(effects.rotation)
					* XMMatrixTranslation(-wiRenderer::GetDevice()->GetScreenWidth() / 2 + effects.pos.x + effects.siz.x*0.5f, wiRenderer::GetDevice()->GetScreenHeight() / 2 + effects.pos.y - effects.siz.y*0.5f, 0)); //AUTO ORIGIN CORRECTION APPLIED! NO FURTHER TRANSLATIONS NEEDED!
				cb.mDimensions = XMFLOAT4((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), effects.siz.x, effects.siz.y);
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
					*XMMatrixTranslation(effects.pos.x,effects.pos.y,effects.pos.z)
					);
				cb.mDimensions = XMFLOAT4(0,0,effects.siz.x,effects.siz.y);
			}
	
			cb.mDrawRec = effects.drawRec;
			cb.mTexMulAdd = XMFLOAT4(1,1,effects.texOffset.x, effects.texOffset.y);
			cb.mOffsetX = effects.offset.x;
			cb.mOffsetY = effects.offset.y;
			cb.mPivot = (UINT)effects.pivotFlag;
			cb.mFade = effects.fade;
			cb.mOpacity = effects.opacity;
			cb.mMask = effects.maskMap != nullptr;
			cb.mDistort = effects.distortionMap != nullptr; 
			cb.mMirror = effects.mirror;
			
			int normalmapmode = 0;
			if(effects.distortionMap && effects.refractionSource)
				normalmapmode=1;
			if(effects.extractNormalMap==true)
				normalmapmode=2;
			cb.mNormalmapSeparate = normalmapmode;
			cb.mMipLevel = effects.mipLevel;

			wiRenderer::GetDevice()->UpdateBuffer(constantBuffer,&cb,threadID);

			wiRenderer::GetDevice()->BindVS(vertexShader, threadID);
			wiRenderer::GetDevice()->BindPS(pixelShader, threadID);
			fullScreenEffect = false;
		}
		else if(!effects.sunPos.x && !effects.sunPos.y){
			wiRenderer::GetDevice()->BindVS(screenVS, threadID);
			fullScreenEffect = true;

			if(effects.process.outline) 
				wiRenderer::GetDevice()->BindPS(outlinePS,threadID);
			else if(effects.process.motionBlur) 
				wiRenderer::GetDevice()->BindPS(motionBlurPS,threadID);
			else if(effects.process.dofStrength) 
				wiRenderer::GetDevice()->BindPS(dofPS,threadID);
			else if(effects.process.fxaa) 
				wiRenderer::GetDevice()->BindPS(fxaaPS,threadID);
			else if(effects.process.ssao)
				wiRenderer::GetDevice()->BindPS(ssaoPS,threadID);
			else if(effects.process.linDepth) 
				wiRenderer::GetDevice()->BindPS(linDepthPS, threadID);
			else if (effects.process.colorGrade)
				wiRenderer::GetDevice()->BindPS(colorGradePS, threadID);
			else if (effects.process.ssr)
				wiRenderer::GetDevice()->BindPS(ssrPS, threadID);
			else if (effects.process.stereogram)
				wiRenderer::GetDevice()->BindPS(stereogramPS, threadID);
			else if (effects.process.tonemap)
				wiRenderer::GetDevice()->BindPS(tonemapPS, threadID);
			else if(effects.process.ssss.x + effects.process.ssss.y > 0)
				wiRenderer::GetDevice()->BindPS(ssssPS,threadID);
			else if(effects.bloom.separate)
				wiRenderer::GetDevice()->BindPS(bloomSeparatePS,threadID);
			else 
				wiHelper::messageBox("Postprocess branch not implemented!");
			
			prcb.params0[0] = effects.process.motionBlur; 
			prcb.params0[1] = effects.process.outline;
			prcb.params0[2] = effects.process.dofStrength;
			prcb.params0[3] = effects.process.ssss.x;
			prcb.params1[0] = effects.bloom.separate;
			prcb.params1[1] = effects.bloom.threshold;
			prcb.params1[2] = effects.bloom.saturation;
			prcb.params1[3] = effects.process.ssss.y;

			wiRenderer::GetDevice()->UpdateBuffer(processCb, &prcb, threadID);
		}
		else{ 
			wiRenderer::GetDevice()->BindVS(screenVS,threadID);
			wiRenderer::GetDevice()->BindPS(shaftPS,threadID);
			fullScreenEffect = true;

			 //Density|Weight|Decay|Exposure
			prcb.params0[0] = 0.65f;
			prcb.params0[1] = 0.25f;
			prcb.params0[2] = 0.945f;
			prcb.params0[3] = 0.2f;
			prcb.params1[0] = effects.sunPos.x;
			prcb.params1[1] = effects.sunPos.y;

			wiRenderer::GetDevice()->UpdateBuffer(processCb,&prcb,threadID);
		}
		wiRenderer::GetDevice()->BindResourcePS(effects.maskMap, TEXSLOT_ONDEMAND1, threadID);
		wiRenderer::GetDevice()->BindResourcePS(effects.distortionMap, TEXSLOT_ONDEMAND2, threadID);
		wiRenderer::GetDevice()->BindResourcePS(effects.refractionSource, TEXSLOT_ONDEMAND3, threadID);
	}
	else{ //BLUR
		wiRenderer::GetDevice()->BindVS(screenVS,threadID);
		fullScreenEffect = true;
		
		if(effects.blurDir==0){
			wiRenderer::GetDevice()->BindPS(blurHPS,threadID);
			prcb.params1[3] = 1.0f / wiRenderer::GetDevice()->GetScreenWidth();
		}
		else{
			wiRenderer::GetDevice()->BindPS(blurVPS,threadID);
			prcb.params1[3] = 1.0f / wiRenderer::GetDevice()->GetScreenHeight();
		}

		static float weight0 = 1.0f;
		static float weight1 = 0.9f;
		static float weight2 = 0.55f;
		static float weight3 = 0.18f;
		static float weight4 = 0.1f;
		const float normalization = 1.0f / (weight0 + 2.0f * (weight1 + weight2 + weight3 + weight4));
		prcb.params0[0] = weight0 * normalization;
		prcb.params0[1] = weight1 * normalization;
		prcb.params0[2] = weight2 * normalization;
		prcb.params0[3] = weight3 * normalization;
		prcb.params1[0] = weight4 * normalization;
		prcb.params1[1] = effects.blur;
		prcb.params1[2] = effects.mipLevel;

		wiRenderer::GetDevice()->UpdateBuffer(processCb, &prcb, threadID);

	}


	if(effects.quality==QUALITY_NEAREST){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if(effects.quality==QUALITY_BILINEAR){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if(effects.quality==QUALITY_ANISOTROPIC){
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			wiRenderer::GetDevice()->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_CLAMP], SSLOT_ONDEMAND0, threadID);
	}

	
	wiRenderer::GetDevice()->Draw((fullScreenEffect ? 3 : 4), threadID);
}

void wiImage::DrawDeferred(Texture2D* lightmap, Texture2D* ao, GRAPHICSTHREAD threadID, int stencilRef){

	wiRenderer::GetDevice()->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,threadID);
	wiRenderer::GetDevice()->BindRasterizerState(rasterizerState,threadID);
	wiRenderer::GetDevice()->BindDepthStencilState(depthNoStencilState,stencilRef,threadID);

	wiRenderer::GetDevice()->BindVertexLayout(nullptr, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
	wiRenderer::GetDevice()->BindIndexBuffer(nullptr, threadID);

	wiRenderer::GetDevice()->BindVS(screenVS,threadID);
	wiRenderer::GetDevice()->BindPS(deferredPS,threadID);

	wiRenderer::GetDevice()->BindResourcePS(lightmap,TEXSLOT_ONDEMAND0,threadID);
	wiRenderer::GetDevice()->BindResourcePS(ao,TEXSLOT_ONDEMAND1,threadID);

	wiRenderer::GetDevice()->BindBlendState(blendStateNoBlend,threadID);

	wiRenderer::GetDevice()->Draw(3,threadID);
}


void wiImage::Load(){
	LoadShaders();
	LoadBuffers();
	SetUpStates();
}
void wiImage::CleanUp()
{
	SAFE_DELETE(blendState);
	SAFE_DELETE(blendStateAdd);
	SAFE_DELETE(blendStateNoBlend);
	SAFE_DELETE(blendStateAvg);

	SAFE_DELETE(constantBuffer);
	SAFE_DELETE(processCb);

	SAFE_DELETE(rasterizerState);
	SAFE_DELETE(depthStencilStateGreater);
	SAFE_DELETE(depthStencilStateLess);
	SAFE_DELETE(depthNoStencilState);

	SAFE_DELETE(vertexShader);
	SAFE_DELETE(screenVS);
	SAFE_DELETE(pixelShader);
	SAFE_DELETE(blurHPS);
	SAFE_DELETE(blurVPS);
	SAFE_DELETE(shaftPS);
	SAFE_DELETE(outlinePS);
	SAFE_DELETE(dofPS);
	SAFE_DELETE(motionBlurPS);
	SAFE_DELETE(bloomSeparatePS);
	SAFE_DELETE(fxaaPS);
	SAFE_DELETE(ssaoPS);
	SAFE_DELETE(deferredPS);
	SAFE_DELETE(ssssPS);
	SAFE_DELETE(linDepthPS);
	SAFE_DELETE(colorGradePS);
	SAFE_DELETE(ssrPS);
	SAFE_DELETE(screenPS);
	SAFE_DELETE(stereogramPS);
	SAFE_DELETE(tonemapPS);
}
