#include "Renderable3DComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiLoader.h"
#include "ResourceMapping.h"

using namespace wiGraphicsTypes;

Renderable3DComponent::Renderable3DComponent()
{
	Renderable2DComponent();
}
Renderable3DComponent::~Renderable3DComponent()
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}
}

void Renderable3DComponent::setProperties()
{
	setLightShaftQuality(0.4f);
	setBloomDownSample(4.0f);
	setAlphaParticleDownSample(1.0f);
	setAdditiveParticleDownSample(1.0f);
	setReflectionQuality(0.5f);
	setSSAOQuality(0.5f);
	setSSAOBlur(2.3f);
	setBloomStrength(19.3f);
	setBloomThreshold(0.99f);
	setBloomSaturation(-3.86f);
	setWaterPlane(wiWaterPlane());
	setDepthOfFieldFocus(10.f);
	setDepthOfFieldStrength(2.2f);

	setSSAOEnabled(true);
	setSSREnabled(true);
	setReflectionsEnabled(false);
	setShadowsEnabled(true);
	setFXAAEnabled(true);
	setBloomEnabled(true);
	setColorGradingEnabled(true);
	setEmitterParticlesEnabled(true);
	setHairParticlesEnabled(true);
	setHairParticlesReflectionEnabled(false);
	setVolumeLightsEnabled(true);
	setLightShaftsEnabled(true);
	setLensFlareEnabled(true);
	setMotionBlurEnabled(true);
	setSSSEnabled(true);
	setDepthOfFieldEnabled(false);
	setStereogramEnabled(false);
	setEyeAdaptionEnabled(false);

	setPreferredThreadingCount(0);
}

void Renderable3DComponent::Initialize()
{
	Renderable2DComponent::Initialize();

	rtSSR.Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()), (UINT)(wiRenderer::GetDevice()->GetScreenHeight())
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtMotionBlur.Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()), (UINT)(wiRenderer::GetDevice()->GetScreenHeight())
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtLinearDepth.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R32_FLOAT);
	rtParticle.Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*getAlphaParticleDownSample()), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*getAlphaParticleDownSample())
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtParticleAdditive.Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*getAdditiveParticleDownSample()), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*getAdditiveParticleDownSample())
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtWaterRipple.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth()
		, wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R8G8B8A8_SNORM);
	rtTransparent.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtVolumeLight.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false);
	rtReflection.Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth() * getReflectionQuality())
		, (UINT)(wiRenderer::GetDevice()->GetScreenHeight() * getReflectionQuality())
		, true, FORMAT_R16G16B16A16_FLOAT);
	rtFinal[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtFinal[1].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false);
	rtFinal[2].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false);

	rtDof[0].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f)
		, false);
	rtDof[1].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f)
		, false);
	rtDof[2].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false);

	dtDepthCopy.Initialize(wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight(), 1, 0);

	rtSSAO.resize(3);
	for (unsigned int i = 0; i<rtSSAO.size(); i++)
		rtSSAO[i].Initialize(
			(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*getSSAOQuality()), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*getSSAOQuality())
			, false, FORMAT_R8_UNORM);


	rtSun.resize(2);
	rtSun[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth()
		, wiRenderer::GetDevice()->GetScreenHeight()
		, true
		);
	rtSun[1].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*getLightShaftQuality())
		, (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*getLightShaftQuality())
		, false
		);
	rtLensFlare.Initialize(wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight(), false);

	rtBloom.resize(3);
	rtBloom[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth()
		, wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R8G8B8A8_UNORM, 0);
	for (unsigned int i = 1; i<rtBloom.size(); ++i)
		rtBloom[i].Initialize(
			(UINT)(wiRenderer::GetDevice()->GetScreenWidth() / getBloomDownSample())
			, (UINT)(wiRenderer::GetDevice()->GetScreenHeight() / getBloomDownSample())
			, false);
}

void Renderable3DComponent::Load()
{
	Renderable2DComponent::Load();

	wiRenderer::HAIRPARTICLEENABLED = getHairParticlesEnabled();
	wiRenderer::EMITTERSENABLED = getEmittedParticlesEnabled();
}

void Renderable3DComponent::Start()
{
	Renderable2DComponent::Start();
}

void Renderable3DComponent::Update(){
	wiRenderer::Update();
	wiRenderer::UpdateImages();

	Renderable2DComponent::Update();
}

void Renderable3DComponent::FixedUpdate(float dt) {
	wiRenderer::SynchronizeWithPhysicsEngine(dt);

	Renderable2DComponent::FixedUpdate(dt);
}

void Renderable3DComponent::Compose(){
	RenderableComponent::Compose();

	RenderColorGradedComposition();

	Renderable2DComponent::Compose();
}

void Renderable3DComponent::RenderFrameSetUp(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateRenderData(threadID);
}
void Renderable3DComponent::RenderReflections(GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	if (!getReflectionsEnabled() || getReflectionQuality() < 0.01f)
	{
		return;
	}

	rtReflection.Activate(threadID); {
		// reverse clipping if underwater
		XMFLOAT4 water = getWaterPlane().getXMFLOAT4();
		if (XMVectorGetX(XMPlaneDot(XMLoadFloat4(&getWaterPlane().getXMFLOAT4()), wiRenderer::getCamera()->GetEye())) < 0 )
		{
			water.x *= -1;
			water.y *= -1;
			water.z *= -1;
		}

		wiRenderer::SetClipPlane(water, threadID);

		wiRenderer::DrawWorld(wiRenderer::getRefCamera(), false, 0, threadID
			, true, SHADERTYPE_TEXTURE
			, nullptr, getHairParticlesReflectionEnabled());
		wiRenderer::DrawSky(threadID,true);
	}
}
void Renderable3DComponent::RenderShadows(GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	if (!getShadowsEnabled())
	{
		return;
	}

	wiRenderer::ClearShadowMaps(threadID);
	wiRenderer::DrawForShadowMap(threadID);
}
void Renderable3DComponent::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	if (getLensFlareEnabled())
	{
		rtLensFlare.Activate(threadID);
		if (!wiRenderer::GetRasterizer())
			wiRenderer::DrawLensFlares(threadID);
	}

	if (getVolumeLightsEnabled())
	{
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
		rtVolumeLight.Activate(threadID, mainRT.depth);
		wiRenderer::DrawVolumeLights(wiRenderer::getCamera(), threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		rtParticle.Activate(threadID, 0, 0, 0, 0);  //OFFSCREEN RENDER ALPHAPARTICLES
		wiRenderer::DrawSoftParticles(wiRenderer::getCamera(), threadID);
		
		rtParticleAdditive.Activate(threadID, 0, 0, 0, 1);  //OFFSCREEN RENDER ADDITIVEPARTICLES
		wiRenderer::DrawSoftPremulParticles(wiRenderer::getCamera(), threadID);
	}

	rtWaterRipple.Activate(threadID, 0, 0, 0, 0); {
		wiRenderer::DrawWaterRipples(threadID);
	}

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
	rtTransparent.Activate(threadID, mainRT.depth); {
		wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), shadedSceneRT.GetTexture(), rtReflection.GetTexture()
			, rtWaterRipple.GetTexture(), threadID);
		wiRenderer::DrawTrails(threadID, shadedSceneRT.GetTexture());
	}

}
void Renderable3DComponent::RenderBloom(GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}


	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());

	rtBloom[0].Activate(threadID);
	{
		fx.bloom.separate = true;
		fx.bloom.saturation = getBloomSaturation();
		fx.bloom.threshold = getBloomThreshold();
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		wiImage::Draw(rtFinal[0].GetTexture(), fx, threadID);
	}


	rtBloom[1].Activate(threadID); //horizontal
	{
		wiRenderer::GetDevice()->GenerateMips(rtBloom[0].GetTexture(), threadID);
		//wiRenderer::GetDevice()->GenerateMips(rtBloom[0].shaderResource[0], threadID);
		fx.mipLevel = 5.32f;
		fx.blur = getBloomStrength();
		fx.blurDir = 0;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[0].GetTexture(), fx, threadID);
	}
	rtBloom[2].Activate(threadID); //vertical
	{
		fx.blur = getBloomStrength();
		fx.blurDir = 1;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[1].GetTexture(), fx, threadID);
	}
}
void Renderable3DComponent::RenderLightShafts(wiRenderTarget& mainRT, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	if (!getLightShaftsEnabled())
	{
		return;
	}

	if (XMVectorGetX(XMVector3Dot(wiRenderer::GetSunPosition(), wiRenderer::getCamera()->GetAt())) < 0)
	{
		return;
	}

	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
	rtSun[0].Activate(threadID, mainRT.depth); {
		wiRenderer::DrawSun(threadID);
	}

	rtSun[1].Activate(threadID); {
		wiImageEffects fxs = fx;
		fxs.blendFlag = BLENDMODE_ADDITIVE;
		XMVECTOR sunPos = XMVector3Project(wiRenderer::GetSunPosition() * 100000, 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		{
			XMStoreFloat2(&fxs.sunPos, sunPos);
			wiImage::Draw(rtSun[0].GetTexture(), fxs, threadID);
		}
	}
}
void Renderable3DComponent::RenderComposition1(wiRenderTarget& shadedSceneRT, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.presentFullScreen = true;

	rtFinal[0].Activate(threadID);

	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(shadedSceneRT.GetTexture(), fx, threadID);


	fx.blendFlag = BLENDMODE_ALPHA;
	wiImage::Draw(rtTransparent.GetTexture(), fx, threadID);
	if (getEmittedParticlesEnabled()){
		wiImage::Draw(rtParticle.GetTexture(), fx, threadID);
	}

	fx.blendFlag = BLENDMODE_ADDITIVE;
	if (getVolumeLightsEnabled()){
		wiImage::Draw(rtVolumeLight.GetTexture(), fx, threadID);
	}
	if (getEmittedParticlesEnabled()){
		wiImage::Draw(rtParticleAdditive.GetTexture(), fx, threadID);
	}
	if (getLightShaftsEnabled()){
		wiImage::Draw(rtSun.back().GetTexture(), fx, threadID);
	}
	if (getLensFlareEnabled()){
		wiImage::Draw(rtLensFlare.GetTexture(), fx, threadID);
	}
}
void Renderable3DComponent::RenderComposition2(wiRenderTarget& mainRT, GRAPHICSTHREAD threadID){
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());

	if (getMotionBlurEnabled()) {
		rtMotionBlur.Activate(threadID);
		fx.process.setMotionBlur(true);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.presentFullScreen = false;
		wiImage::Draw(rtFinal[0].GetTexture(), fx, threadID);
		fx.process.clear();
	}


	fx.blendFlag = BLENDMODE_OPAQUE;

	rtFinal[1].Activate(threadID);
	fx.process.setToneMap(true);
	if (getEyeAdaptionEnabled())
	{
		fx.setMaskMap(wiRenderer::GetLuminance(rtFinal[0].GetTexture(), threadID));
	}
	else
	{
		fx.setMaskMap(wiTextureHelper::getInstance()->getColor(wiColor::Gray));
	}
	if (getMotionBlurEnabled())
	{
		wiImage::Draw(rtMotionBlur.GetTexture(), fx, threadID);
	}
	else
	{
		wiImage::Draw(rtFinal[0].GetTexture(), fx, threadID);
	}
	fx.process.clear();


	if (getDepthOfFieldEnabled())
	{
		// downsample + blur
		rtDof[0].Activate(threadID);
		fx.blur = getDepthOfFieldStrength();
		fx.blurDir = 0;
		wiImage::Draw(rtFinal[1].GetTexture(), fx, threadID);

		rtDof[1].Activate(threadID);
		fx.blurDir = 1;
		wiImage::Draw(rtDof[0].GetTexture(), fx, threadID);
		fx.blur = 0;
		fx.process.clear();

		// depth of field compose pass
		rtDof[2].Activate(threadID);
		fx.process.setDOF(getDepthOfFieldFocus());
		fx.setMaskMap(rtDof[1].GetTexture());
		//fx.setDepthMap(rtLinearDepth.shaderResource.back());
		wiImage::Draw(rtFinal[1].GetTexture(), fx, threadID);
		fx.setMaskMap(nullptr);
		//fx.setDepthMap(nullptr);
		fx.process.clear();
	}


	rtFinal[2].Activate(threadID, mainRT.depth);
	fx.process.setFXAA(getFXAAEnabled());
	if (getDepthOfFieldEnabled())
		wiImage::Draw(rtDof[2].GetTexture(), fx, threadID);
	else
		wiImage::Draw(rtFinal[1].GetTexture(), fx, threadID);
	fx.process.clear();

	if (getBloomEnabled())
	{
		fx.blendFlag = BLENDMODE_ADDITIVE;
		fx.presentFullScreen = true;
		wiImage::Draw(rtBloom.back().GetTexture(), fx, threadID);
	}

	wiRenderer::DrawDebugGridHelper(wiRenderer::getCamera(), threadID);
	wiRenderer::DrawDebugEnvProbes(wiRenderer::getCamera(), threadID);
	wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), threadID);
	wiRenderer::DrawDebugLines(wiRenderer::getCamera(), threadID);
	wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), threadID);
	wiRenderer::DrawTranslators(wiRenderer::getCamera(), threadID);
}
void Renderable3DComponent::RenderColorGradedComposition(){

	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_NEAREST;

	if (getStereogramEnabled())
	{
		fx.presentFullScreen = false;
		fx.process.clear();
		fx.process.setStereogram(true);
		wiImage::Draw(wiTextureHelper::getInstance()->getRandom64x64(), fx);
		return;
	}

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
			fx.setMaskMap(wiTextureHelper::getInstance()->getColorGradeDefault());
		}
	}
	else
	{
		fx.presentFullScreen = true;
	}

	wiImage::Draw(rtFinal[2].GetTexture(), fx);
}


void Renderable3DComponent::setPreferredThreadingCount(unsigned short value)
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}

	workerThreads.clear();
}