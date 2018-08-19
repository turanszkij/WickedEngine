#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiImageEffects.h"
#include "wiSceneComponents.h"
#include "wiHelper.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"

#include <thread>

using namespace wiGraphicsTypes;
using namespace std;

#pragma region STATICS
GPUBuffer			wiImage::constantBuffer, wiImage::processCb;

VertexShader*		wiImage::vertexShader = nullptr;
VertexShader*		wiImage::screenVS = nullptr;

PixelShader*		wiImage::imagePS[IMAGE_SHADER_COUNT];
PixelShader*		wiImage::postprocessPS[POSTPROCESS_COUNT];
PixelShader*		wiImage::deferredPS = nullptr;

BlendState			wiImage::blendStates[BLENDMODE_COUNT];
RasterizerState		wiImage::rasterizerState;
DepthStencilState	wiImage::depthStencilStates[STENCILMODE_COUNT];
BlendState			wiImage::blendStateDisableColor;
DepthStencilState	wiImage::depthStencilStateDepthWrite;

GraphicsPSO			wiImage::imagePSO[IMAGE_SHADER_COUNT][BLENDMODE_COUNT][STENCILMODE_COUNT][IMAGE_HDR_COUNT];
GraphicsPSO			wiImage::postprocessPSO[POSTPROCESS_COUNT];
GraphicsPSO			wiImage::deferredPSO;

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
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &constantBuffer);

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PostProcessCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &processCb);

	BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
}

void wiImage::LoadShaders()
{
	vertexShader = static_cast<VertexShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imageVS.cso", wiResourceManager::VERTEXSHADER));
	screenVS = static_cast<VertexShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "screenVS.cso", wiResourceManager::VERTEXSHADER));

	imagePS[IMAGE_SHADER_STANDARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS.cso", wiResourceManager::PIXELSHADER));
	imagePS[IMAGE_SHADER_SEPARATENORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_separatenormalmap.cso", wiResourceManager::PIXELSHADER));
	imagePS[IMAGE_SHADER_DISTORTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_distortion.cso", wiResourceManager::PIXELSHADER));
	imagePS[IMAGE_SHADER_DISTORTION_MASKED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_distortion_masked.cso", wiResourceManager::PIXELSHADER));
	imagePS[IMAGE_SHADER_MASKED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "imagePS_masked.cso", wiResourceManager::PIXELSHADER));
	imagePS[IMAGE_SHADER_FULLSCREEN] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "screenPS.cso", wiResourceManager::PIXELSHADER));

	postprocessPS[POSTPROCESS_BLUR_H] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "horizontalBlurPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_BLUR_V] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "verticalBlurPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_LIGHTSHAFT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "lightShaftPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_OUTLINE] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "outlinePS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_DEPTHOFFIELD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "depthofFieldPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_MOTIONBLUR] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "motionBlurPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_BLOOMSEPARATE] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "bloomSeparatePS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_FXAA] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fxaa.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_SSAO] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssao.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_SSSS] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssss.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_LINEARDEPTH] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "linDepthPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_COLORGRADE] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "colorGradePS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_SSR] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "ssr.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_STEREOGRAM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "stereogramPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_TONEMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "toneMapPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_REPROJECTDEPTHBUFFER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "reprojectDepthBufferPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_DOWNSAMPLEDEPTHBUFFER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "downsampleDepthBuffer4xPS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_TEMPORALAA] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "temporalAAResolvePS.cso", wiResourceManager::PIXELSHADER));
	postprocessPS[POSTPROCESS_SHARPEN] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "sharpenPS.cso", wiResourceManager::PIXELSHADER));

	deferredPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "deferredPS.cso", wiResourceManager::PIXELSHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	vector<thread> thread_pool(0);

	thread_pool.push_back(thread([&] {
		for (int i = 0; i < IMAGE_SHADER_COUNT; ++i)
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShader;
			if (i == IMAGE_SHADER_FULLSCREEN)
			{
				desc.vs = screenVS;
			}
			desc.ps = imagePS[i];
			desc.rs = &rasterizerState;
			desc.pt = TRIANGLESTRIP;

			for (int j = 0; j < BLENDMODE_COUNT; ++j)
			{
				desc.bs = &blendStates[j];
				for (int k = 0; k < STENCILMODE_COUNT; ++k)
				{
					desc.dss = &depthStencilStates[k];

					if (k == STENCILMODE_DISABLED)
					{
						desc.DSFormat = FORMAT_UNKNOWN;
					}
					else
					{
						desc.DSFormat = wiRenderer::DSFormat_full;
					}

					desc.numRTs = 1;

					desc.RTFormats[0] = wiRenderer::GetDevice()->GetBackBufferFormat();
					device->CreateGraphicsPSO(&desc, &imagePSO[i][j][k][0]);

					desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
					device->CreateGraphicsPSO(&desc, &imagePSO[i][j][k][1]);
				}
			}
		}
	}));

	thread_pool.push_back(thread([&] {
		for (int i = 0; i < POSTPROCESS_COUNT; ++i)
		{
			GraphicsPSODesc desc;
			desc.vs = screenVS;
			desc.ps = postprocessPS[i];
			desc.bs = &blendStates[BLENDMODE_OPAQUE];
			desc.dss = &depthStencilStates[STENCILMODE_DISABLED];
			desc.rs = &rasterizerState;
			desc.pt = TRIANGLELIST;

			if (i == POSTPROCESS_DOWNSAMPLEDEPTHBUFFER || i == POSTPROCESS_REPROJECTDEPTHBUFFER)
			{
				desc.dss = &depthStencilStateDepthWrite;
				desc.DSFormat = wiRenderer::DSFormat_small;
				desc.numRTs = 0;
			}
			else if (i == POSTPROCESS_SSSS)
			{
				desc.dss = &depthStencilStates[STENCILMODE_LESS];
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_deferred_lightbuffer;
				desc.DSFormat = wiRenderer::DSFormat_full;
			}
			else if (i == POSTPROCESS_SSAO)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_ssao;
			}
			else if (i == POSTPROCESS_LINEARDEPTH)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_lineardepth;
			}
			else if (i == POSTPROCESS_TONEMAP)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::GetDevice()->GetBackBufferFormat();
			}
			else if (i == POSTPROCESS_BLOOMSEPARATE || i == POSTPROCESS_BLUR_H || POSTPROCESS_BLUR_V)
			{
				// todo: bloom and DoF blur should really be HDR lol...
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::GetDevice()->GetBackBufferFormat();
			}
			else
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
			}

			device->CreateGraphicsPSO(&desc, &postprocessPSO[i]);
		}

		GraphicsPSODesc desc;
		desc.vs = screenVS;
		desc.ps = deferredPS;
		desc.bs = &blendStates[BLENDMODE_OPAQUE];
		desc.dss = &depthStencilStates[STENCILMODE_LESS];
		desc.rs = &rasterizerState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
		desc.DSFormat = wiRenderer::DSFormat_full;
		desc.pt = TRIANGLELIST;
		device->CreateGraphicsPSO(&desc, &deferredPSO);
	}));


	for (auto& x : thread_pool)
	{
		x.join();
	}

}
void wiImage::SetUpStates()
{
	
	RasterizerStateDesc rs;
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = false;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs, &rasterizerState);




	
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
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_LESS]);

	dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
	dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_EQUAL]);

	
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
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_GREATER]);
	
	dsd.StencilEnable = false;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_DISABLED]);


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_ALWAYS;
	dsd.StencilEnable = false;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilStateDepthWrite);

	
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
	wiRenderer::GetDevice()->CreateBlendState(&bd,&blendStates[BLENDMODE_ALPHA]);

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
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_PREMULTIPLIED]);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_OPAQUE]);

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
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_ADDITIVE]);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable = false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_DISABLE;
	bd.IndependentBlendEnable=false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStateDisableColor);
}

void wiImage::BindPersistentState(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindConstantBuffer(VS, &constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);
	wiRenderer::GetDevice()->BindConstantBuffer(PS, &constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);

	wiRenderer::GetDevice()->BindConstantBuffer(PS, &processCb, CB_GETBINDSLOT(PostProcessCB), threadID);
}

void wiImage::Draw(Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("Image", threadID);

	bool fullScreenEffect = false;

	device->BindResource(PS, texture, TEXSLOT_ONDEMAND0, threadID);

	device->BindStencilRef(effects.stencilRef, threadID);

	if (effects.quality == QUALITY_NEAREST) 
	{
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_POINT_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_POINT_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if (effects.quality == QUALITY_BILINEAR) 
	{
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_LINEAR_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_LINEAR_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
	}
	else if (effects.quality == QUALITY_ANISOTROPIC) 
	{
		if (effects.sampleFlag == SAMPLEMODE_MIRROR)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_ANISO_MIRROR], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_WRAP)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_ANISO_WRAP], SSLOT_ONDEMAND0, threadID);
		else if (effects.sampleFlag == SAMPLEMODE_CLAMP)
			device->BindSampler(PS, wiRenderer::samplers[SSLOT_ANISO_CLAMP], SSLOT_ONDEMAND0, threadID);
	}

	if (effects.presentFullScreen)
	{
		device->BindGraphicsPSO(&imagePSO[IMAGE_SHADER_FULLSCREEN][effects.blendFlag][effects.stencilComp][effects.hdr], threadID);
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

			device->UpdateBuffer(&constantBuffer, &cb, threadID);

			// Determine relevant image rendering pixel shader:
			IMAGE_SHADER targetShader;
			bool NormalmapSeparate = effects.extractNormalMap;
			bool Mask = effects.maskMap != nullptr;
			bool Distort = effects.distortionMap != nullptr;
			if (NormalmapSeparate)
			{
				targetShader = IMAGE_SHADER_SEPARATENORMALMAP;
			}
			else
			{
				if (Mask)
				{
					if (Distort)
					{
						targetShader = IMAGE_SHADER_DISTORTION_MASKED;
					}
					else
					{
						targetShader = IMAGE_SHADER_MASKED;
					}
				}
				else if(Distort)
				{
					targetShader = IMAGE_SHADER_DISTORTION;
				}
				else
				{
					targetShader = IMAGE_SHADER_STANDARD;
				}
			}

			device->BindGraphicsPSO(&imagePSO[targetShader][effects.blendFlag][effects.stencilComp][effects.hdr], threadID);

			fullScreenEffect = false;
		}
		else if(abs(effects.sunPos.x + effects.sunPos.y) < FLT_EPSILON) // POSTPROCESS
		{
			PostProcessCB prcb;

			fullScreenEffect = true;

			POSTPROCESS targetShader;

			if (effects.process.outline) 
			{
				targetShader = POSTPROCESS_OUTLINE;

				prcb.params0[1] = effects.process.outline ? 1.0f : 0.0f;
				device->UpdateBuffer(&processCb, &prcb, threadID);
			}
			else if (effects.process.motionBlur) 
			{
				targetShader = POSTPROCESS_MOTIONBLUR;

				prcb.params0[0] = effects.process.motionBlur ? 1.0f : 0.0f;
				device->UpdateBuffer(&processCb, &prcb, threadID);
			}
			else if (effects.process.dofStrength) 
			{
				targetShader = POSTPROCESS_DEPTHOFFIELD;

				prcb.params0[2] = effects.process.dofStrength;
				device->UpdateBuffer(&processCb, &prcb, threadID);
			}
			else if (effects.process.fxaa) 
			{
				targetShader = POSTPROCESS_FXAA;
			}
			else if (effects.process.ssao) 
			{
				targetShader = POSTPROCESS_SSAO;
			}
			else if (effects.process.linDepth) 
			{
				targetShader = POSTPROCESS_LINEARDEPTH;
			}
			else if (effects.process.colorGrade)
			{
				targetShader = POSTPROCESS_COLORGRADE;
			}
			else if (effects.process.ssr) 
			{
				targetShader = POSTPROCESS_SSR;
			}
			else if (effects.process.stereogram) 
			{
				targetShader = POSTPROCESS_STEREOGRAM;
			}
			else if (effects.process.tonemap) 
			{
				targetShader = POSTPROCESS_TONEMAP;
			}
			else if (effects.process.ssss.x + effects.process.ssss.y > 0) 
			{
				targetShader = POSTPROCESS_SSSS;

				prcb.params0[0] = effects.process.ssss.x;
				prcb.params0[1] = effects.process.ssss.y;
				device->UpdateBuffer(&processCb, &prcb, threadID);
			}
			else if (effects.bloom.separate) 
			{
				targetShader = POSTPROCESS_BLOOMSEPARATE;

				prcb.params1[0] = effects.bloom.separate ? 1.0f : 0.0f;
				prcb.params1[1] = effects.bloom.threshold;
				prcb.params1[2] = effects.bloom.saturation;
				device->UpdateBuffer(&processCb, &prcb, threadID);
			}
			else if (effects.process.reprojectDepthBuffer)
			{
				targetShader = POSTPROCESS_REPROJECTDEPTHBUFFER;
			}
			else if (effects.process.downsampleDepthBuffer4x)
			{
				targetShader = POSTPROCESS_DOWNSAMPLEDEPTHBUFFER;
			}
			else if (effects.process.temporalAAResolve) 
			{
				targetShader = POSTPROCESS_TEMPORALAA;
			}
			else if (effects.process.sharpen > 0) 
			{
				targetShader = POSTPROCESS_SHARPEN;

				prcb.params0[0] = effects.process.sharpen;
				device->UpdateBuffer(&processCb, &prcb, threadID);
			}
			else 
			{
				assert(0); // not impl
			}

			device->BindGraphicsPSO(&postprocessPSO[targetShader], threadID);
		}
		else // LIGHTSHAFT
		{
			PostProcessCB prcb;

			fullScreenEffect = true;

			//Density|Weight|Decay|Exposure
			prcb.params0[0] = 0.65f;
			prcb.params0[1] = 0.25f;
			prcb.params0[2] = 0.945f;
			prcb.params0[3] = 0.2f;
			prcb.params1[0] = effects.sunPos.x;
			prcb.params1[1] = effects.sunPos.y;

			device->UpdateBuffer(&processCb,&prcb,threadID);

			device->BindGraphicsPSO(&postprocessPSO[POSTPROCESS_LIGHTSHAFT], threadID);
		}
		device->BindResource(PS, effects.maskMap, TEXSLOT_ONDEMAND1, threadID);
		device->BindResource(PS, effects.distortionMap, TEXSLOT_ONDEMAND2, threadID);
		device->BindResource(PS, effects.refractionSource, TEXSLOT_ONDEMAND3, threadID);
	}
	else // BLUR
	{
		PostProcessCB prcb;

		fullScreenEffect = true;
		
		if(effects.blurDir==0)
		{
			device->BindGraphicsPSO(&postprocessPSO[POSTPROCESS_BLUR_H], threadID);
			prcb.params1[3] = 1.0f / wiRenderer::GetInternalResolution().x;
		}
		else
		{
			device->BindGraphicsPSO(&postprocessPSO[POSTPROCESS_BLUR_V], threadID);
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

		device->UpdateBuffer(&processCb, &prcb, threadID);

	}
	
	device->Draw((fullScreenEffect ? 3 : 4), 0, threadID);

	device->EventEnd(threadID);
}

void wiImage::DrawDeferred(Texture2D* lightmap_diffuse, Texture2D* lightmap_specular, Texture2D* ao, 
	GRAPHICSTHREAD threadID, int stencilRef)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("DeferredComposition", threadID);

	device->BindStencilRef(stencilRef, threadID);

	device->BindResource(PS, lightmap_diffuse, TEXSLOT_ONDEMAND0, threadID);
	device->BindResource(PS, lightmap_specular, TEXSLOT_ONDEMAND1, threadID);
	device->BindResource(PS, ao,TEXSLOT_ONDEMAND2,threadID);

	device->BindGraphicsPSO(&deferredPSO, threadID);

	device->Draw(3, 0, threadID);

	device->EventEnd(threadID);
}


void wiImage::Load()
{
	LoadBuffers();
	SetUpStates();
	LoadShaders();
}
void wiImage::CleanUp()
{
	SAFE_DELETE(vertexShader);
	SAFE_DELETE(screenVS);
	SAFE_DELETE(deferredPS);
}
