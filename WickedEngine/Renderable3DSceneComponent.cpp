#include "Renderable3DSceneComponent.h"
#include "WickedEngine.h"

Renderable3DSceneComponent::Renderable3DSceneComponent()
{
	Initialize();
}
Renderable3DSceneComponent::~Renderable3DSceneComponent()
{

}

void Renderable3DSceneComponent::Initialize()
{
	RenderableComponent::Initialize();

	setLightShaftQuality(0.4f);
	setBloomDownSample(4.0f);
	setAlphaParticleDownSample(1.0f);
	setAdditiveParticleDownSample(1.0f);
	setReflectionQuality(0.5f);
	setSSAOQuality(0.3f);
	setSSAOBlur(2.0f);
	setSSRQuality(0.5f);
	setBloomStrength(19.3f);
	setBloomThreshold(0.99f);
	setBloomSaturation(-3.86f);
	setWaterPlane(XMFLOAT4(0, 1, 0, 0));

	setSSAOEnabled(true);
	setSSREnabled(true);

	RenderableComponent::Initialize();
}

void Renderable3DSceneComponent::Load()
{
	RenderableComponent::Load();

	rtSSR.Initialize(
		screenW * getSSRQuality(), screenH * getSSRQuality()
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	rtLinearDepth.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R32_FLOAT
		);
	rtParticle.Initialize(
		screenW*getAlphaParticleDownSample(), screenH*getAlphaParticleDownSample()
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtParticleAdditive.Initialize(
		screenW*getAdditiveParticleDownSample(), screenH*getAdditiveParticleDownSample()
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
		screenW * getReflectionQuality()
		, screenH * getReflectionQuality()
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
		screenW*getSSAOQuality(), screenH*getSSAOQuality()
		, 1, false, 1, 0, DXGI_FORMAT_R8_UNORM
		);
	rtSSAO.back().Activate(wiRenderer::immediateContext, 1, 1, 1, 1);


	rtSun.resize(2);
	rtSun[0].Initialize(
		screenW
		, screenH
		, 1, true
		);
	rtSun[1].Initialize(
		screenW*getLightShaftQuality()
		, screenH*getLightShaftQuality()
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
		screenW / getBloomDownSample()
		, screenH / getBloomDownSample()
		, 1, false
		);

	RenderableComponent::Load();
}

void Renderable3DSceneComponent::Update(){
	RenderableComponent::Update();

	wiRenderer::Update();
	wiRenderer::UpdateLights();
	wiRenderer::SychronizeWithPhysicsEngine();
	wiRenderer::UpdateRenderInfo(wiRenderer::immediateContext);
	wiRenderer::UpdateSkinnedVB();
}

void Renderable3DSceneComponent::Compose(){
	RenderableComponent::Compose();

	RenderColorGradedComposition();

}

void Renderable3DSceneComponent::RenderReflections(){

	rtReflection.Activate(wiRenderer::immediateContext); {
		wiRenderer::UpdatePerRenderCB(wiRenderer::immediateContext, 0);
		wiRenderer::UpdatePerEffectCB(wiRenderer::immediateContext, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::UpdatePerViewCB(wiRenderer::immediateContext, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->View, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->refEye, getWaterPlane());
		wiRenderer::DrawWorld(wiRenderer::getCamera()->refView, false, 0, wiRenderer::immediateContext
			, false, wiRenderer::SHADED_NONE
			, nullptr, false, 1);
		wiRenderer::DrawSky(wiRenderer::getCamera()->refEye, wiRenderer::immediateContext);
	}
}
void Renderable3DSceneComponent::RenderShadows(){
	wiRenderer::ClearShadowMaps(wiRenderer::immediateContext);
	wiRenderer::DrawForShadowMap(wiRenderer::immediateContext);
}
void Renderable3DSceneComponent::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT)
{
	rtLensFlare.Activate(wiRenderer::immediateContext);
	if (!wiRenderer::GetRasterizer())
		wiRenderer::DrawLensFlares(wiRenderer::immediateContext, mainRT.depth->shaderResource, screenW, screenH);

	rtVolumeLight.Activate(wiRenderer::immediateContext, mainRT.depth);
		wiRenderer::DrawVolumeLights(wiRenderer::getCamera()->View, wiRenderer::immediateContext);

	rtParticle.Activate(wiRenderer::immediateContext, 0, 0, 0, 0);  //OFFSCREEN RENDER ALPHAPARTICLES
		wiRenderer::DrawSoftParticles(wiRenderer::getCamera()->Eye, wiRenderer::getCamera()->View, wiRenderer::immediateContext, rtLinearDepth.shaderResource.back());
	rtParticleAdditive.Activate(wiRenderer::immediateContext, 0, 0, 0, 1);  //OFFSCREEN RENDER ADDITIVEPARTICLES
		wiRenderer::DrawSoftPremulParticles(wiRenderer::getCamera()->Eye, wiRenderer::getCamera()->View, wiRenderer::immediateContext, rtLinearDepth.shaderResource.back());
		rtWater.Activate(wiRenderer::immediateContext, mainRT.depth); {
		wiRenderer::DrawWorldWater(wiRenderer::getCamera()->View, shadedSceneRT.shaderResource.front(), rtReflection.shaderResource.front(), rtLinearDepth.shaderResource.back()
			, rtWaterRipple.shaderResource.back(), wiRenderer::immediateContext, 2);
	}
	rtTransparent.Activate(wiRenderer::immediateContext, mainRT.depth); {
		wiRenderer::DrawWorldTransparent(wiRenderer::getCamera()->View, shadedSceneRT.shaderResource.front(), rtReflection.shaderResource.front(), rtLinearDepth.shaderResource.back()
			, wiRenderer::immediateContext, 2);
	}

}
void Renderable3DSceneComponent::RenderBloom(){

	wiImageEffects fx(screenW, screenH);

	wiImage::BatchBegin();

	rtBloom[0].Activate(wiRenderer::immediateContext);
	{
		fx.bloom.separate = true;
		fx.bloom.saturation = getBloomSaturation();
		fx.bloom.threshold = getBloomThreshold();
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		wiImage::Draw(rtFinal[0].shaderResource.front(), fx);
	}


	rtBloom[1].Activate(wiRenderer::immediateContext); //horizontal
	{
		wiRenderer::immediateContext->GenerateMips(rtBloom[0].shaderResource[0]);
		fx.mipLevel = 5.32f;
		fx.blur = getBloomStrength();
		fx.blurDir = 0;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[0].shaderResource.back(), fx);
	}
	rtBloom[2].Activate(wiRenderer::immediateContext); //vertical
	{
		wiRenderer::immediateContext->GenerateMips(rtBloom[0].shaderResource[0]);
		fx.blur = getBloomStrength();
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
void Renderable3DSceneComponent::RenderLightShafts(wiRenderTarget& mainRT){
	wiImageEffects fx(screenW, screenH);


	rtSun[0].Activate(wiRenderer::immediateContext, mainRT.depth); {
		wiRenderer::UpdatePerRenderCB(wiRenderer::immediateContext, 0);
		wiRenderer::UpdatePerEffectCB(wiRenderer::immediateContext, XMFLOAT4(1, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, wiRenderer::immediateContext);
	}

	wiImage::BatchBegin();
	rtSun[1].Activate(wiRenderer::immediateContext); {
		wiImageEffects fxs = fx;
		fxs.blendFlag = BLENDMODE_ADDITIVE;
		XMVECTOR sunPos = XMVector3Project(wiRenderer::GetSunPosition() * 100000, 0, 0, screenW, screenH, 0.1f, 1.0f, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->View, XMMatrixIdentity());
		{
			XMStoreFloat2(&fxs.sunPos, sunPos);
			wiImage::Draw(rtSun[0].shaderResource.back(), fxs, wiRenderer::immediateContext);
		}
	}
}
void Renderable3DSceneComponent::RenderComposition1(wiRenderTarget& shadedSceneRT){
	wiImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();

	rtFinal[0].Activate(wiRenderer::immediateContext);

	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(shadedSceneRT.shaderResource.front(), fx);

	fx.blendFlag = BLENDMODE_ALPHA;
	if (getSSREnabled()){
		wiImage::Draw(rtSSR.shaderResource.back(), fx);
	}
	wiImage::Draw(rtWater.shaderResource.back(), fx);
	wiImage::Draw(rtTransparent.shaderResource.back(), fx);
	wiImage::Draw(rtParticle.shaderResource.back(), fx);

	fx.blendFlag = BLENDMODE_ADDITIVE;
	wiImage::Draw(rtVolumeLight.shaderResource.back(), fx);
	wiImage::Draw(rtParticleAdditive.shaderResource.back(), fx);
	wiImage::Draw(rtSun.back().shaderResource.back(), fx);
	wiImage::Draw(rtLensFlare.shaderResource.back(), fx);
}
void Renderable3DSceneComponent::RenderComposition2(){
	wiImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();

	rtFinal[1].Activate(wiRenderer::immediateContext);

	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.process.setFXAA(true);
	wiImage::Draw(rtFinal[0].shaderResource.back(), fx);
	fx.process.clear();

	fx.blendFlag = BLENDMODE_ADDITIVE;
	wiImage::Draw(rtBloom.back().shaderResource.back(), fx);
}
void Renderable3DSceneComponent::RenderColorGradedComposition(){

	wiImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();

	if (wiRenderer::GetColorGrading() != nullptr){
		fx.process.setColorGrade(true);
		fx.setMaskMap(wiRenderer::GetColorGrading());
	}
	wiImage::Draw(rtFinal[1].shaderResource.back(), fx);
}