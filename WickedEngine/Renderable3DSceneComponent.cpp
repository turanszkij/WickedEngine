#include "Renderable3DSceneComponent.h"
#include "WickedEngine.h"

Renderable3DSceneComponent::Renderable3DSceneComponent()
{
	Initialize();
}
Renderable3DSceneComponent::~Renderable3DSceneComponent()
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}
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
	setWaterPlane(wiWaterPlane());

	setSSAOEnabled(true);
	setSSREnabled(true);
	setReflectionsEnabled(false);
	setShadowsEnabled(true);
	setFXAAEnabled(true);
	setBloomEnabled(true);
	setColorGradingEnabled(true);
	setEmitterParticlesEnabled(true);
	setHairParticlesEnabled(true);
	setVolumeLightsEnabled(true);
	setLightShaftsEnabled(true);
	setLensFlareEnabled(true);

	setPreferredThreadingCount(4);
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
	rtTransparent.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
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

void Renderable3DSceneComponent::Start()
{
	wiRenderer::SetEnviromentMap(nullptr);
	wiRenderer::SetColorGrading(nullptr);
	wiRenderer::SetToDrawDebugBoxes(false);
	wiRenderer::SetToDrawDebugLines(false);
	wiRenderer::HAIRPARTICLEENABLED = getHairParticlesEnabled();
	wiRenderer::EMITTERSENABLED = getEmittedParticlesEnabled();
}

void Renderable3DSceneComponent::Update(){
	RenderableComponent::Update();

	wiRenderer::Update();
	wiRenderer::UpdateLights();
	wiRenderer::SychronizeWithPhysicsEngine();
	wiRenderer::UpdateRenderInfo(wiRenderer::getImmediateContext());
	wiRenderer::UpdateSkinnedVB();
	wiRenderer::UpdateImages();
}

void Renderable3DSceneComponent::Compose(){
	RenderableComponent::Compose();

	RenderColorGradedComposition();

}

void Renderable3DSceneComponent::RenderReflections(wiRenderer::DeviceContext context){
	if (!getReflectionsEnabled() || getReflectionQuality() < 0.01f)
	{
		return;
	}

	rtReflection.Activate(context); {
		wiRenderer::UpdatePerRenderCB(context, 0);
		wiRenderer::UpdatePerEffectCB(context, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::UpdatePerViewCB(context, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->View, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->refEye, getWaterPlane().getXMFLOAT4());
		wiRenderer::DrawWorld(wiRenderer::getCamera()->refView, false, 0, context
			, false, wiRenderer::SHADED_NONE
			, nullptr, false, GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::DrawSky(wiRenderer::getCamera()->refEye, context);
	}
}
void Renderable3DSceneComponent::RenderShadows(wiRenderer::DeviceContext context){
	if (!getShadowsEnabled())
	{
		return;
	}

	wiRenderer::ClearShadowMaps(context);
	wiRenderer::DrawForShadowMap(context);
}
void Renderable3DSceneComponent::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, wiRenderer::DeviceContext context)
{
	if (getLensFlareEnabled())
	{
		rtLensFlare.Activate(context);
		if (!wiRenderer::GetRasterizer())
			wiRenderer::DrawLensFlares(context, mainRT.depth->shaderResource, screenW, screenH);
	}

	if (getVolumeLightsEnabled())
	{
		rtVolumeLight.Activate(context, mainRT.depth);
		wiRenderer::DrawVolumeLights(wiRenderer::getCamera()->View, context);
	}

	if (getEmittedParticlesEnabled())
	{
		rtParticle.Activate(context, 0, 0, 0, 0);  //OFFSCREEN RENDER ALPHAPARTICLES
		wiRenderer::DrawSoftParticles(wiRenderer::getCamera()->Eye, wiRenderer::getCamera()->View, context, rtLinearDepth.shaderResource.back());
		
		rtParticleAdditive.Activate(context, 0, 0, 0, 1);  //OFFSCREEN RENDER ADDITIVEPARTICLES
		wiRenderer::DrawSoftPremulParticles(wiRenderer::getCamera()->Eye, wiRenderer::getCamera()->View, context, rtLinearDepth.shaderResource.back());
	}

	rtWaterRipple.Activate(context, 0, 0, 0, 0); {
		wiRenderer::DrawWaterRipples(context);
	}
	rtWater.Activate(context, mainRT.depth); {
		wiRenderer::DrawWorldWater(wiRenderer::getCamera()->View, shadedSceneRT.shaderResource.front(), rtReflection.shaderResource.front(), rtLinearDepth.shaderResource.back()
			, rtWaterRipple.shaderResource.back(), context);
	}

	rtTransparent.Activate(context, mainRT.depth); {
		wiRenderer::DrawWorldTransparent(wiRenderer::getCamera()->View, shadedSceneRT.shaderResource.front(), rtReflection.shaderResource.front(), rtLinearDepth.shaderResource.back()
			, context);
	}

}
void Renderable3DSceneComponent::RenderBloom(wiRenderer::DeviceContext context){

	wiImageEffects fx(screenW, screenH);

	wiImage::BatchBegin(context);

	rtBloom[0].Activate(context);
	{
		fx.bloom.separate = true;
		fx.bloom.saturation = getBloomSaturation();
		fx.bloom.threshold = getBloomThreshold();
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		wiImage::Draw(rtFinal[0].shaderResource.front(), fx, context);
	}


	rtBloom[1].Activate(context); //horizontal
	{
		context->GenerateMips(rtBloom[0].shaderResource[0]);
		fx.mipLevel = 5.32f;
		fx.blur = getBloomStrength();
		fx.blurDir = 0;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[0].shaderResource.back(), fx, context);
	}
	rtBloom[2].Activate(context); //vertical
	{
		context->GenerateMips(rtBloom[0].shaderResource[0]);
		fx.blur = getBloomStrength();
		fx.blurDir = 1;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[1].shaderResource.back(), fx, context);
	}
}
void Renderable3DSceneComponent::RenderLightShafts(wiRenderTarget& mainRT, wiRenderer::DeviceContext context){
	if (!getLightShaftsEnabled())
	{
		return;
	}

	wiImageEffects fx(screenW, screenH);

	rtSun[0].Activate(context, mainRT.depth); {
		wiRenderer::UpdatePerRenderCB(context, 0);
		wiRenderer::UpdatePerEffectCB(context, XMFLOAT4(1, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, context);
	}

	wiImage::BatchBegin(context);
	rtSun[1].Activate(context); {
		wiImageEffects fxs = fx;
		fxs.blendFlag = BLENDMODE_ADDITIVE;
		XMVECTOR sunPos = XMVector3Project(wiRenderer::GetSunPosition() * 100000, 0, 0, screenW, screenH, 0.1f, 1.0f, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->View, XMMatrixIdentity());
		{
			XMStoreFloat2(&fxs.sunPos, sunPos);
			wiImage::Draw(rtSun[0].shaderResource.back(), fxs, context);
		}
	}
}
void Renderable3DSceneComponent::RenderComposition1(wiRenderTarget& shadedSceneRT, wiRenderer::DeviceContext context){
	wiImageEffects fx(screenW, screenH);
	wiImage::BatchBegin(context);

	rtFinal[0].Activate(context);

	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(shadedSceneRT.shaderResource.front(), fx, context);

	fx.blendFlag = BLENDMODE_ALPHA;
	if (getSSREnabled()){
		wiImage::Draw(rtSSR.shaderResource.back(), fx, context);
	}
	wiImage::Draw(rtWater.shaderResource.back(), fx, context);
	wiImage::Draw(rtTransparent.shaderResource.back(), fx, context);
	if (getEmittedParticlesEnabled()){
		wiImage::Draw(rtParticle.shaderResource.back(), fx, context);
	}

	fx.blendFlag = BLENDMODE_ADDITIVE;
	if (getVolumeLightsEnabled()){
		wiImage::Draw(rtVolumeLight.shaderResource.back(), fx, context);
	}
	if (getEmittedParticlesEnabled()){
		wiImage::Draw(rtParticleAdditive.shaderResource.back(), fx, context);
	}
	if (getLightShaftsEnabled()){
		wiImage::Draw(rtSun.back().shaderResource.back(), fx, context);
	}
	if (getLensFlareEnabled()){
		wiImage::Draw(rtLensFlare.shaderResource.back(), fx, context);
	}
}
void Renderable3DSceneComponent::RenderComposition2(wiRenderer::DeviceContext context){
	wiImageEffects fx(screenW, screenH);
	wiImage::BatchBegin(context);

	rtFinal[1].Activate(context);

	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.process.setFXAA(getFXAAEnabled());
	wiImage::Draw(rtFinal[0].shaderResource.back(), fx, context);
	fx.process.clear();

	if (getBloomEnabled())
	{
		fx.blendFlag = BLENDMODE_ADDITIVE;
		wiImage::Draw(rtBloom.back().shaderResource.back(), fx, context);
	}
}
void Renderable3DSceneComponent::RenderColorGradedComposition(){

	wiImageEffects fx(screenW, screenH);
	wiImage::BatchBegin();
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_NEAREST;

	if (getColorGradingEnabled())
	{
		fx.quality = QUALITY_BILINEAR;
		if (wiRenderer::GetColorGrading() != nullptr){
			fx.process.setColorGrade(true);
			fx.setMaskMap(wiRenderer::GetColorGrading());
		}
		else
		{
			fx.process.setColorGrade(true);
			fx.setMaskMap(wiTextureHelper::getInstance()->getColorGradeDefaultTex());
		}
	}
	wiImage::Draw(rtFinal[1].shaderResource.back(), fx);
}


void Renderable3DSceneComponent::setPreferredThreadingCount(unsigned short value)
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}

	workerThreads.clear();
}