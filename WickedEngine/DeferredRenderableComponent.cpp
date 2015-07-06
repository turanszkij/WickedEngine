#include "DeferredRenderableComponent.h"
#include "WickedEngine.h"

DeferredRenderableComponent::DeferredRenderableComponent() :ssao(false), ssr(false){
	static const float lightShaftQuality = .4f;
	static const float bloomDownSample = 4.f;
	static const float particleDownSample = 1.0f;
	static const float reflectionDownSample = 0.5f;
	static const float ssaoQuality = 0.3f;

	wiRenderer::SetEnviromentMap(nullptr);
	wiRenderer::SetColorGrading(nullptr);

	rtSun.resize(2);
	rtSun[0].Initialize(
		screenW
		, screenH
		, 1, true
		);
	rtSun[1].Initialize(
		screenW*lightShaftQuality
		, screenH*lightShaftQuality
		, 1, false
		);
	rtLensFlare.Initialize(screenW, screenH, 1, false);

	rtBloom.resize(3);
	rtBloom[0].Initialize(
		screenW
		, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0
		);
	for (int i = 1; i<rtBloom.size(); ++i)
		rtBloom[i].Initialize(
		screenW / bloomDownSample
		, screenH / bloomDownSample
		, 1, false
		);

	rtGBuffer.Initialize(
		screenW, screenH
		, 3, true, 1, 0
		, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtDeferred.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
	rtSSR.Initialize(
		screenW / 2, screenH / 2
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	rtLinearDepth.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R32_FLOAT
		);
	rtParticle.Initialize(
		screenW*particleDownSample, screenH*particleDownSample
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtParticleAdditive.Initialize(
		screenW*particleDownSample, screenH*particleDownSample
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtWater.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtWaterRipple.Initialize(
		screenW
		, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R8G8B8A8_SNORM
		);
	rtWaterRipple.Activate(wiRenderer::immediateContext, 0, 0, 0, 0);
	rtTransparent.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtLight.Initialize(
		screenW, screenH
		, 1, false, 1, 0
		, DXGI_FORMAT_R11G11B10_FLOAT
		);
	rtVolumeLight.Initialize(
		screenW, screenH
		, 1, false
		);
	rtReflection.Initialize(
		screenW * reflectionDownSample
		, screenH * reflectionDownSample
		, 1, true, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtFinal[0].Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	rtFinal[1].Initialize(
		screenW, screenH
		, 1, false);

	dtDepthCopy.Initialize(screenW, screenH
		, 1, 0
		);

	rtSSAO.resize(3);
	for (int i = 0; i<rtSSAO.size(); i++)
		rtSSAO[i].Initialize(
		screenW*ssaoQuality, screenH*ssaoQuality
		, 1, false, 1, 0, DXGI_FORMAT_R8_UNORM
		);
	rtSSAO.back().Activate(wiRenderer::immediateContext, 1, 1, 1, 1);

}
DeferredRenderableComponent::~DeferredRenderableComponent(){}
void DeferredRenderableComponent::Update(){
	wiRenderer::Update();
	wiRenderer::UpdateLights();
	wiRenderer::SychronizeWithPhysicsEngine();
	wiRenderer::UpdateRenderInfo(wiRenderer::immediateContext);
	wiRenderer::UpdateSkinnedVB();
}
void DeferredRenderableComponent::Render(){
	RenderReflections();
	RenderShadows();
	RenderScene();
	RenderComposition1();
	RenderBloom();
	RenderLightShafts();
	RenderComposition2();
}
void DeferredRenderableComponent::Compose(){

	RenderColorGradedComposition();

}

void DeferredRenderableComponent::RenderReflections(){
	static const XMFLOAT4 waterPlane = XMFLOAT4(0, 1, 0, 0);

	rtReflection.Activate(wiRenderer::immediateContext); {
		wiRenderer::UpdatePerRenderCB(wiRenderer::immediateContext, 0);
		wiRenderer::UpdatePerEffectCB(wiRenderer::immediateContext, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::UpdatePerViewCB(wiRenderer::immediateContext, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->View, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->refEye, waterPlane);
		wiRenderer::DrawWorld(wiRenderer::getCamera()->refView, false, 0, wiRenderer::immediateContext
			, false, wiRenderer::SHADED_NONE
			, nullptr, false, 1);
		wiRenderer::DrawSky(wiRenderer::getCamera()->refEye, wiRenderer::immediateContext);
	}
}
void DeferredRenderableComponent::RenderShadows(){
	wiRenderer::ClearShadowMaps(wiRenderer::immediateContext);
	wiRenderer::DrawForShadowMap(wiRenderer::immediateContext);
}
void DeferredRenderableComponent::RenderScene(){
	static const int tessellationQuality = 0;

	wiRenderer::UpdatePerFrameCB(wiRenderer::immediateContext);
	ImageEffects fx(screenW, screenH);

	rtGBuffer.Activate(wiRenderer::immediateContext); {
		wiRenderer::UpdatePerRenderCB(wiRenderer::immediateContext, tessellationQuality);
		wiRenderer::UpdatePerViewCB(wiRenderer::immediateContext, wiRenderer::getCamera()->View, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->Eye);


		wiRenderer::UpdatePerEffectCB(wiRenderer::immediateContext, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawWorld(wiRenderer::getCamera()->View, wiRenderer::DX11, tessellationQuality, wiRenderer::immediateContext, false
			, wiRenderer::SHADED_DEFERRED, rtReflection.shaderResource.front(), true, 2);


	}

	rtLensFlare.Activate(wiRenderer::immediateContext);
	if (!wiRenderer::GetRasterizer())
		wiRenderer::DrawLensFlares(wiRenderer::immediateContext, rtGBuffer.depth->shaderResource, screenW, screenH);

	rtLinearDepth.Activate(wiRenderer::immediateContext); {
		wiImage::BatchBegin(wiRenderer::immediateContext);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtGBuffer.depth->shaderResource, fx, wiRenderer::immediateContext);
		fx.process.clear();
	}
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, wiRenderer::immediateContext);

	rtGBuffer.Set(wiRenderer::immediateContext); {
		wiRenderer::DrawDecals(wiRenderer::getCamera()->View, wiRenderer::immediateContext, dtDepthCopy.shaderResource);
	}

	rtLight.Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiRenderer::DrawLights(wiRenderer::getCamera()->View, wiRenderer::immediateContext,
			dtDepthCopy.shaderResource, rtGBuffer.shaderResource[1], rtGBuffer.shaderResource[2]);
	}

	rtVolumeLight.Activate(wiRenderer::immediateContext, rtGBuffer.depth);
	wiRenderer::DrawVolumeLights(wiRenderer::getCamera()->View, wiRenderer::immediateContext);



	if (ssao){
		wiImage::BatchBegin(wiRenderer::immediateContext, STENCILREF_DEFAULT);
		rtSSAO[0].Activate(wiRenderer::immediateContext); {
			fx.process.setSSAO(true);
			fx.setDepthMap(rtLinearDepth.shaderResource.back());
			fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setMaskMap((ID3D11ShaderResourceView*)wiResourceManager::add("images/noise.png"));
			//fx.sampleFlag=SAMPLEMODE_CLAMP;
			fx.quality = QUALITY_BILINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, wiRenderer::immediateContext);
			//fx.sampleFlag=SAMPLEMODE_CLAMP;
			fx.process.clear();
		}
		static const float ssaoBlur = 2.f;
		rtSSAO[1].Activate(wiRenderer::immediateContext); {
			fx.blur = ssaoBlur;
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].shaderResource.back(), fx, wiRenderer::immediateContext);
		}
		rtSSAO[2].Activate(wiRenderer::immediateContext); {
			fx.blur = ssaoBlur;
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].shaderResource.back(), fx, wiRenderer::immediateContext);
			fx.blur = 0;
		}
	}


	rtDeferred.Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiImage::DrawDeferred(rtGBuffer.shaderResource[0]
			, rtLinearDepth.shaderResource.back(), rtLight.shaderResource.front(), rtGBuffer.shaderResource[1]
			, rtSSAO.back().shaderResource.back(), wiRenderer::immediateContext, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, wiRenderer::immediateContext);
	}


	if (ssr){
		rtSSR.Activate(wiRenderer::immediateContext); {
			wiImage::BatchBegin(wiRenderer::immediateContext);
			wiRenderer::immediateContext->GenerateMips(rtDeferred.shaderResource[0]);
			fx.process.setSSR(true);
			fx.setDepthMap(dtDepthCopy.shaderResource);
			fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setVelocityMap(rtGBuffer.shaderResource[2]);
			fx.setMaskMap(rtLinearDepth.shaderResource.front());
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, wiRenderer::immediateContext);
			fx.process.clear();
		}
	}


	rtParticle.Activate(wiRenderer::immediateContext, 0, 0, 0, 0);  //OFFSCREEN RENDER ALPHAPARTICLES
	wiRenderer::DrawSoftParticles(wiRenderer::getCamera()->Eye, wiRenderer::getCamera()->View, wiRenderer::immediateContext, rtLinearDepth.shaderResource.back());
	rtParticleAdditive.Activate(wiRenderer::immediateContext, 0, 0, 0, 1);  //OFFSCREEN RENDER ADDITIVEPARTICLES
	wiRenderer::DrawSoftPremulParticles(wiRenderer::getCamera()->Eye, wiRenderer::getCamera()->View, wiRenderer::immediateContext, rtLinearDepth.shaderResource.back());
	rtWater.Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiRenderer::DrawWorldWater(wiRenderer::getCamera()->View, rtDeferred.shaderResource.front(), rtReflection.shaderResource.front(), rtLinearDepth.shaderResource.back()
			, rtWaterRipple.shaderResource.back(), wiRenderer::immediateContext, 2);
	}
	rtTransparent.Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiRenderer::DrawWorldTransparent(wiRenderer::getCamera()->View, rtDeferred.shaderResource.front(), rtReflection.shaderResource.front(), rtLinearDepth.shaderResource.back()
			, wiRenderer::immediateContext, 2);
	}
}
void DeferredRenderableComponent::RenderBloom(){
	static const float bloomStren = 19.3f, bloomThreshold = 0.99f, bloomSaturation = -3.86f;


	ImageEffects fx(screenW, screenH);

	wiImage::BatchBegin();

	rtBloom[0].Activate(wiRenderer::immediateContext);
	{
		fx.bloom.separate = true;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		wiImage::Draw(rtFinal[0].shaderResource.front(), fx);
	}


	rtBloom[1].Activate(wiRenderer::immediateContext); //horizontal
	{
		wiRenderer::immediateContext->GenerateMips(rtBloom[0].shaderResource[0]);
		fx.mipLevel = 5.32f;
		fx.blur = bloomStren;
		fx.blurDir = 0;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[0].shaderResource.back(), fx);
	}
	rtBloom[2].Activate(wiRenderer::immediateContext); //vertical
	{
		wiRenderer::immediateContext->GenerateMips(rtBloom[0].shaderResource[0]);
		fx.blur = bloomStren;
		fx.blurDir = 1;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[1].shaderResource.back(), fx);
	}


	//if (rtBloom.size()>2){
	//	for (int i = 0; i<rtBloom.size() - 1; ++i){
	//		rtBloom[i].Activate(wiRenderer::immediateContext);
	//		if (i == 0){
	//			fx.bloom.separate = true;
	//			fx.bloom.threshold = bloomThreshold;
	//			fx.bloom.saturation = bloomSaturation;
	//			fx.blendFlag = BLENDMODE_OPAQUE;
	//			fx.sampleFlag = SAMPLEMODE_CLAMP;
	//			wiImage::Draw(rtFinal[0].shaderResource.front(), fx);
	//		}
	//		else { //horizontal blurs
	//			if (i == 1)
	//			{
	//				wiRenderer::immediateContext->GenerateMips(rtBloom[0].shaderResource[0]);
	//			}
	//			fx.mipLevel = 4;
	//			fx.blur = bloomStren;
	//			fx.blurDir = 0;
	//			fx.blendFlag = BLENDMODE_OPAQUE;
	//			wiImage::Draw(rtBloom[i - 1].shaderResource.back(), fx);
	//		}
	//	}

	//	rtBloom.back().Activate(wiRenderer::immediateContext);
	//	//vertical blur
	//	fx.blur = bloomStren;
	//	fx.blurDir = 1;
	//	fx.blendFlag = BLENDMODE_OPAQUE;
	//	wiImage::Draw(rtBloom[rtBloom.size() - 2].shaderResource.back(), fx);
	//}
}
void DeferredRenderableComponent::RenderLightShafts(){
	ImageEffects fx(screenW, screenH);


	rtSun[0].Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiRenderer::UpdatePerRenderCB(wiRenderer::immediateContext, 0);
		wiRenderer::UpdatePerEffectCB(wiRenderer::immediateContext, XMFLOAT4(1, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, wiRenderer::immediateContext);
	}

	wiImage::BatchBegin();
	rtSun[1].Activate(wiRenderer::immediateContext); {
		ImageEffects fxs = fx;
		fxs.blendFlag = BLENDMODE_ADDITIVE;
		XMVECTOR sunPos = XMVector3Project(wiRenderer::GetSunPosition() * 100000, 0, 0, screenW, screenH, 0.1f, 1.0f, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->View, XMMatrixIdentity());
		{
			XMStoreFloat2(&fxs.sunPos, sunPos);
			wiImage::Draw(rtSun[0].shaderResource.back(), fxs, wiRenderer::immediateContext);
		}
	}
}
void DeferredRenderableComponent::RenderComposition1(){
	ImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();

	rtFinal[0].Activate(wiRenderer::immediateContext);

	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(rtDeferred.shaderResource.back(), fx);

	fx.blendFlag = BLENDMODE_ALPHA;
	if (ssr){
		wiImage::Draw(rtSSR.shaderResource.back(), fx);
	}
	wiImage::Draw(rtWater.shaderResource.back(), fx);
	wiImage::Draw(rtTransparent.shaderResource.back(), fx);
	wiImage::Draw(rtParticle.shaderResource.back(), fx);

	fx.blendFlag = BLENDMODE_ADDITIVE;
	wiImage::Draw(rtVolumeLight.shaderResource.back(), fx);
	wiImage::Draw(rtParticleAdditive.shaderResource.back(), fx);
	wiImage::Draw(rtLensFlare.shaderResource.back(), fx);
}
void DeferredRenderableComponent::RenderComposition2(){
	ImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();

	rtFinal[1].Activate(wiRenderer::immediateContext);

	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.process.setFXAA(true);
	wiImage::Draw(rtFinal[0].shaderResource.back(), fx);
	fx.process.clear();

	fx.blendFlag = BLENDMODE_ADDITIVE;
	wiImage::Draw(rtBloom.back().shaderResource.back(), fx);
	wiImage::Draw(rtSun.back().shaderResource.back(), fx);
}
void DeferredRenderableComponent::RenderColorGradedComposition(){

	ImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();

	if (wiRenderer::GetColorGrading()){
		fx.process.setColorGrade(true);
		fx.setMaskMap(wiRenderer::GetColorGrading());
	}
	wiImage::Draw(rtFinal[1].shaderResource.back(), fx);
}