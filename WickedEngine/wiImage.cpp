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
BlendState		*wiImage::blendStateAlpha = nullptr, *wiImage::blendStatePremul = nullptr, *wiImage::blendStateAdd = nullptr, *wiImage::blendStateOpaque = nullptr, *wiImage::blendStateDisable = nullptr;
GPUBuffer       *wiImage::constantBuffer = nullptr, *wiImage::processCb = nullptr;

VertexShader     *wiImage::vertexShader = nullptr,*wiImage::screenVS = nullptr;
PixelShader		 *wiImage::imagePS = nullptr, *wiImage::imagePS_separatenormalmap = nullptr, *wiImage::imagePS_distortion = nullptr, *wiImage::imagePS_distortion_masked = nullptr, *wiImage::imagePS_masked = nullptr;
PixelShader      *wiImage::blurHPS = nullptr, *wiImage::blurVPS = nullptr, *wiImage::shaftPS = nullptr, *wiImage::outlinePS = nullptr
	, *wiImage::dofPS = nullptr, *wiImage::motionBlurPS = nullptr, *wiImage::bloomSeparatePS = nullptr, *wiImage::fxaaPS = nullptr, *wiImage::ssaoPS = nullptr, *wiImage::deferredPS = nullptr
	, *wiImage::ssssPS = nullptr, *wiImage::linDepthPS = nullptr, *wiImage::colorGradePS = nullptr, *wiImage::ssrPS = nullptr, *wiImage::screenPS = nullptr, *wiImage::stereogramPS = nullptr
	, *wiImage::tonemapPS = nullptr, *wiImage::reprojectDepthBufferPS = nullptr, *wiImage::downsampleDepthBufferPS = nullptr, *wiImage::temporalAAResolvePS = nullptr, *wiImage::sharpenPS = nullptr;
	

RasterizerState		*wiImage::rasterizerState = nullptr;
DepthStencilState	*wiImage::depthStencilStateGreater = nullptr, *wiImage::depthStencilStateLess = nullptr, *wiImage::depthStencilStateEqual = nullptr
	, *wiImage::depthNoStencilState = nullptr, *wiImage::depthStencilStateDepthWrite = nullptr;

#pragma endregion

wiImage::wiImage()
{
}

void wiImage::LoadBuffers()
{

	GPUBufferDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ImageCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	constantBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, NULL, constantBuffer);

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

	imagePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS.cso", wiResourceManager::PIXELSHADER));
	imagePS_separatenormalmap = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_separatenormalmap.cso", wiResourceManager::PIXELSHADER));
	imagePS_distortion = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_distortion.cso", wiResourceManager::PIXELSHADER));
	imagePS_distortion_masked = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_distortion_masked.cso", wiResourceManager::PIXELSHADER));
	imagePS_masked = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_masked.cso", wiResourceManager::PIXELSHADER));

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
	reprojectDepthBufferPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "reprojectDepthBufferPS.cso", wiResourceManager::PIXELSHADER));
	downsampleDepthBufferPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "downsampleDepthBuffer4xPS.cso", wiResourceManager::PIXELSHADER));
	temporalAAResolvePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "temporalAAResolvePS.cso", wiResourceManager::PIXELSHADER));
	sharpenPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "sharpenPS.cso", wiResourceManager::PIXELSHADER));

}
void wiImage::SetUpStates()
{
	
	RasterizerStateDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
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
	dsd.DepthFunc = COMPARISON_GREATER;

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
	depthStencilStateLess = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilStateLess);

	dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
	dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
	depthStencilStateEqual = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilStateEqual);

	
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER;

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
	depthStencilStateGreater = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilStateGreater);
	
	dsd.StencilEnable = false;
	depthNoStencilState = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthNoStencilState);


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_ALWAYS;
	dsd.StencilEnable = false;
	depthStencilStateDepthWrite = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilStateDepthWrite);

	
	BlendStateDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStateAlpha = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateAlpha);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStatePremul = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd, blendStatePremul);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStateOpaque = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateOpaque);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStateAdd = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateAdd);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable = false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_DISABLE;
	bd.IndependentBlendEnable=false;
	blendStateDisable = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd, blendStateDisable);
}

void wiImage::BindPersistentState(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);
	wiRenderer::GetDevice()->BindConstantBufferPS(constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);

	wiRenderer::GetDevice()->BindConstantBufferPS(processCb, CB_GETBINDSLOT(PostProcessCB), threadID);
}

void wiImage::Draw(Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("Image", threadID);

	bool fullScreenEffect = false;

	device->BindVertexLayout(nullptr, threadID);
	device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP, threadID);
	device->BindRasterizerState(rasterizerState, threadID);

	device->BindResourcePS(texture, TEXSLOT_ONDEMAND0, threadID);

	if (effects.blendFlag == BLENDMODE_ALPHA)
		device->BindBlendState(blendStateAlpha, threadID);
	else if (effects.blendFlag == BLENDMODE_PREMULTIPLIED)
		device->BindBlendState(blendStatePremul, threadID);
	else if (effects.blendFlag == BLENDMODE_ADDITIVE)
		device->BindBlendState(blendStateAdd, threadID);
	else if (effects.blendFlag == BLENDMODE_OPAQUE)
		device->BindBlendState(blendStateOpaque, threadID);
	else
		device->BindBlendState(blendStateAlpha, threadID);


	{
		switch (effects.stencilComp)
		{
		case COMPARISON_LESS:
		case COMPARISON_LESS_EQUAL:
			device->BindDepthStencilState(depthStencilStateLess, effects.stencilRef, threadID);
			break;
		case COMPARISON_GREATER:
		case COMPARISON_GREATER_EQUAL:
			device->BindDepthStencilState(depthStencilStateGreater, effects.stencilRef, threadID);
			break;
		case COMPARISON_EQUAL:
			device->BindDepthStencilState(depthStencilStateEqual, effects.stencilRef, threadID);
			break;
		default:
			device->BindDepthStencilState(depthNoStencilState, effects.stencilRef, threadID);
			break;
		}
	}

	if (effects.quality == QUALITY_NEAREST) {
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if (effects.quality == QUALITY_BILINEAR) {
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if (effects.quality == QUALITY_ANISOTROPIC) {
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			device->BindSamplerPS(wiRenderer::samplers[SSLOT_ANISO_CLAMP], SSLOT_ONDEMAND0, threadID);
	}

	if (effects.presentFullScreen)
	{
		device->BindVS(screenVS, threadID);
		device->BindPS(screenPS, threadID);
		device->Draw(3, 0, threadID);
		device->EventEnd(threadID);
		return;
	}

	if(!effects.blur) // NORMAL IMAGE
	{
		ImageCB cb;

		if(!effects.process.active && !effects.bloom.separate && !effects.sunPos.x && !effects.sunPos.y){
			if(effects.typeFlag==SCREEN){
				cb.mTransform = XMMatrixTranspose(
					XMMatrixScaling(effects.scale.x*effects.siz.x, effects.scale.y*effects.siz.y, 1)
					* XMMatrixRotationZ(effects.rotation)
					* XMMatrixTranslation(effects.pos.x, effects.pos.y, 0)
					* device->GetScreenProjection()
				);
			}
			else if(effects.typeFlag==WORLD){
				XMMATRIX faceRot = XMMatrixIdentity();
				if(effects.lookAt.w){
					XMVECTOR vvv = (effects.lookAt.x==1 && !effects.lookAt.y && !effects.lookAt.z)?XMVectorSet(0,1,0,0):XMVectorSet(1,0,0,0);
					faceRot = 
						XMMatrixLookAtLH(XMVectorSet(0,0,0,0)
							,XMLoadFloat4(&effects.lookAt)
							,XMVector3Cross(
								vvv, XMLoadFloat4(&effects.lookAt)
							)
						)
					;
				}
				else
				{
					faceRot = XMMatrixRotationQuaternion(XMLoadFloat4(&wiRenderer::getCamera()->rotation));
				}

				XMMATRIX view = wiRenderer::getCamera()->GetView();
				XMMATRIX projection = wiRenderer::getCamera()->GetProjection();
				// Remove possible jittering from temporal camera:
				projection.r[2] = XMVectorSetX(projection.r[2], 0);
				projection.r[2] = XMVectorSetY(projection.r[2], 0);

				cb.mTransform = XMMatrixTranspose(
						XMMatrixScaling(effects.scale.x*effects.siz.x,-1*effects.scale.y*effects.siz.y,1)
						*XMMatrixRotationZ(effects.rotation)
						*faceRot
						*XMMatrixTranslation(effects.pos.x,effects.pos.y,effects.pos.z)
						*view * projection
					);
			}

			// todo: effects.drawRec -> texmuladd!

			cb.mTexMulAdd = XMFLOAT4(1,1,effects.texOffset.x, effects.texOffset.y);
			cb.mColor = effects.col;
			cb.mColor.x *= 1 - effects.fade;
			cb.mColor.y *= 1 - effects.fade;
			cb.mColor.z *= 1 - effects.fade;
			cb.mColor.w *= effects.opacity;
			cb.mPivot = effects.pivot;
			cb.mMirror = effects.mirror;
			cb.mPivot = effects.pivot;
			cb.mMipLevel = effects.mipLevel;

			device->UpdateBuffer(constantBuffer, &cb, threadID);

			device->BindVS(vertexShader, threadID);

			// Determine relevant image rendering pixel shader:
			bool NormalmapSeparate = effects.extractNormalMap;
			bool Mask = effects.maskMap != nullptr;
			bool Distort = effects.distortionMap != nullptr;
			if (NormalmapSeparate)
			{
				device->BindPS(imagePS_separatenormalmap, threadID);
			}
			else
			{
				if (Mask)
				{
					if (Distort)
					{
						device->BindPS(imagePS_distortion_masked, threadID);
					}
					else
					{
						device->BindPS(imagePS_masked, threadID);
					}
				}
				else if(Distort)
				{
					device->BindPS(imagePS_distortion, threadID);
				}
				else
				{
					device->BindPS(imagePS, threadID);
				}
			}


			fullScreenEffect = false;
		}
		else if(abs(effects.sunPos.x + effects.sunPos.y) < FLT_EPSILON) // POSTPROCESS
		{
			PostProcessCB prcb;

			device->BindVS(screenVS, threadID);
			fullScreenEffect = true;

			if (effects.process.outline) {
				device->BindPS(outlinePS, threadID);

				prcb.params0[1] = effects.process.outline ? 1.0f : 0.0f;
				device->UpdateBuffer(processCb, &prcb, threadID);
			}
			else if (effects.process.motionBlur) {
				device->BindPS(motionBlurPS, threadID);

				prcb.params0[0] = effects.process.motionBlur ? 1.0f : 0.0f;
				device->UpdateBuffer(processCb, &prcb, threadID);
			}
			else if (effects.process.dofStrength) {
				device->BindPS(dofPS, threadID);

				prcb.params0[2] = effects.process.dofStrength;
				device->UpdateBuffer(processCb, &prcb, threadID);
			}
			else if (effects.process.fxaa) {
				device->BindPS(fxaaPS, threadID);
			}
			else if (effects.process.ssao) {
				device->BindPS(ssaoPS, threadID);
			}
			else if (effects.process.linDepth) {
				device->BindPS(linDepthPS, threadID);
			}
			else if (effects.process.colorGrade) {
				device->BindPS(colorGradePS, threadID);
			}
			else if (effects.process.ssr) {
				device->BindPS(ssrPS, threadID);
			}
			else if (effects.process.stereogram) {
				device->BindPS(stereogramPS, threadID);
			}
			else if (effects.process.tonemap) {
				device->BindPS(tonemapPS, threadID);
			}
			else if (effects.process.ssss.x + effects.process.ssss.y > 0) {
				device->BindPS(ssssPS, threadID);

				prcb.params0[3] = effects.process.ssss.x;
				prcb.params1[3] = effects.process.ssss.y;
				device->UpdateBuffer(processCb, &prcb, threadID);
			}
			else if (effects.bloom.separate) {
				device->BindPS(bloomSeparatePS, threadID);

				prcb.params1[0] = effects.bloom.separate ? 1.0f : 0.0f;
				prcb.params1[1] = effects.bloom.threshold;
				prcb.params1[2] = effects.bloom.saturation;
				device->UpdateBuffer(processCb, &prcb, threadID);
			}
			else if (effects.process.reprojectDepthBuffer)
			{
				device->BindPS(reprojectDepthBufferPS, threadID);
				device->BindDepthStencilState(depthStencilStateDepthWrite, 0, threadID);
				device->BindBlendState(blendStateDisable, threadID);
			}
			else if (effects.process.downsampleDepthBuffer4x)
			{
				device->BindPS(downsampleDepthBufferPS, threadID);
				device->BindDepthStencilState(depthStencilStateDepthWrite, 0, threadID);
				device->BindBlendState(blendStateDisable, threadID);
			}
			else if (effects.process.temporalAAResolve) {
				device->BindPS(temporalAAResolvePS, threadID);
			}
			else if (effects.process.sharpen > 0) {
				device->BindPS(sharpenPS, threadID);

				prcb.params0[0] = effects.process.sharpen;
				device->UpdateBuffer(processCb, &prcb, threadID);
			}
			else {
				assert(0); // not impl
			}
		}
		else // LIGHTSHAFT
		{
			PostProcessCB prcb;

			device->BindVS(screenVS,threadID);
			device->BindPS(shaftPS,threadID);
			fullScreenEffect = true;

			//Density|Weight|Decay|Exposure
			prcb.params0[0] = 0.65f;
			prcb.params0[1] = 0.25f;
			prcb.params0[2] = 0.945f;
			prcb.params0[3] = 0.2f;
			prcb.params1[0] = effects.sunPos.x;
			prcb.params1[1] = effects.sunPos.y;

			device->UpdateBuffer(processCb,&prcb,threadID);
		}
		device->BindResourcePS(effects.maskMap, TEXSLOT_ONDEMAND1, threadID);
		device->BindResourcePS(effects.distortionMap, TEXSLOT_ONDEMAND2, threadID);
		device->BindResourcePS(effects.refractionSource, TEXSLOT_ONDEMAND3, threadID);
	}
	else // BLUR
	{
		PostProcessCB prcb;

		device->BindVS(screenVS,threadID);
		fullScreenEffect = true;
		
		if(effects.blurDir==0){
			device->BindPS(blurHPS,threadID);
			prcb.params1[3] = 1.0f / wiRenderer::GetInternalResolution().x;
		}
		else{
			device->BindPS(blurVPS,threadID);
			prcb.params1[3] = 1.0f / wiRenderer::GetInternalResolution().y;
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

		device->UpdateBuffer(processCb, &prcb, threadID);

	}
	
	device->Draw((fullScreenEffect ? 3 : 4), 0, threadID);

	device->EventEnd(threadID);
}

void wiImage::DrawDeferred(Texture2D* lightmap_diffuse, Texture2D* lightmap_specular, Texture2D* ao, 
	GRAPHICSTHREAD threadID, int stencilRef)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("DeferredComposition", threadID);

	device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,threadID);
	device->BindRasterizerState(rasterizerState,threadID);
	device->BindDepthStencilState(depthStencilStateLess,stencilRef,threadID);

	device->BindVertexLayout(nullptr, threadID);

	device->BindVS(screenVS,threadID);
	device->BindPS(deferredPS,threadID);

	device->BindResourcePS(lightmap_diffuse, TEXSLOT_ONDEMAND0, threadID);
	device->BindResourcePS(lightmap_specular, TEXSLOT_ONDEMAND1, threadID);
	device->BindResourcePS(ao,TEXSLOT_ONDEMAND2,threadID);

	device->BindBlendState(blendStateOpaque,threadID);

	device->Draw(3, 0, threadID);

	device->EventEnd(threadID);
}


void wiImage::Load(){
	LoadShaders();
	LoadBuffers();
	SetUpStates();
}
void wiImage::CleanUp()
{
	SAFE_DELETE(blendStateAlpha);
	SAFE_DELETE(blendStatePremul);
	SAFE_DELETE(blendStateAdd);
	SAFE_DELETE(blendStateOpaque);
	SAFE_DELETE(blendStateDisable);

	SAFE_DELETE(constantBuffer);
	SAFE_DELETE(processCb);

	SAFE_DELETE(rasterizerState);
	SAFE_DELETE(depthStencilStateGreater);
	SAFE_DELETE(depthStencilStateLess);
	SAFE_DELETE(depthStencilStateEqual);
	SAFE_DELETE(depthNoStencilState);
	SAFE_DELETE(depthStencilStateDepthWrite);

	SAFE_DELETE(vertexShader);
	SAFE_DELETE(screenVS);
	SAFE_DELETE(imagePS);
	SAFE_DELETE(imagePS_separatenormalmap);
	SAFE_DELETE(imagePS_distortion);
	SAFE_DELETE(imagePS_distortion_masked);
	SAFE_DELETE(imagePS_masked);
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
	SAFE_DELETE(reprojectDepthBufferPS);
	SAFE_DELETE(downsampleDepthBufferPS);
	SAFE_DELETE(temporalAAResolvePS);
	SAFE_DELETE(sharpenPS);
}
