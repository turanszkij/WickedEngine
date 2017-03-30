#include "Renderable3DComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiLoader.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

Renderable3DComponent::Renderable3DComponent()
{
	SAFE_INIT(smallDepth);

	Renderable2DComponent();
}
Renderable3DComponent::~Renderable3DComponent()
{
	SAFE_DELETE(smallDepth);

	for (auto& wt : workerThreads)
	{
		delete wt;
	}
}

void Renderable3DComponent::ResizeBuffers()
{
	Renderable2DComponent::ResizeBuffers();

	FORMAT defaultTextureFormat = GraphicsDevice::GetBackBufferFormat();

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
	rtSceneCopy.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R16G16B16A16_FLOAT, 8);
	rtReflection.Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth() * getReflectionQuality())
		, (UINT)(wiRenderer::GetDevice()->GetScreenHeight() * getReflectionQuality())
		, true, FORMAT_R16G16B16A16_FLOAT);
	rtFinal[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, defaultTextureFormat);
	rtFinal[1].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, defaultTextureFormat, 1);

	rtDof[0].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f)
		, false, defaultTextureFormat);
	rtDof[1].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f)
		, false, defaultTextureFormat);
	rtDof[2].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, defaultTextureFormat);

	dtDepthCopy.Initialize(wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight(), getMSAASampleCount());
	//dtSmallDepth.Initialize(wiRenderer::GetDevice()->GetScreenWidth() / 4, wiRenderer::GetDevice()->GetScreenHeight() / 4, 1);

	rtSSAO.resize(3);
	for (unsigned int i = 0; i<rtSSAO.size(); i++)
		rtSSAO[i].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*getSSAOQuality()), (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*getSSAOQuality())
			, false, FORMAT_R8_UNORM);


	rtSun.resize(2);
	rtSun[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth()
		, wiRenderer::GetDevice()->GetScreenHeight()
		, true, defaultTextureFormat, 1, getMSAASampleCount()
	);
	rtSun[1].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth()*getLightShaftQuality())
		, (UINT)(wiRenderer::GetDevice()->GetScreenHeight()*getLightShaftQuality())
		, false, defaultTextureFormat
	);

	rtBloom.resize(3);
	rtBloom[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth()
		, wiRenderer::GetDevice()->GetScreenHeight()
		, false, defaultTextureFormat, 0);
	for (unsigned int i = 1; i<rtBloom.size(); ++i)
		rtBloom[i].Initialize(
		(UINT)(wiRenderer::GetDevice()->GetScreenWidth() / getBloomDownSample())
			, (UINT)(wiRenderer::GetDevice()->GetScreenHeight() / getBloomDownSample())
			, false, defaultTextureFormat);

	rtTemporalAA[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtTemporalAA[1].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R16G16B16A16_FLOAT);


	SAFE_DELETE(smallDepth);
	Texture2DDesc desc;
	desc.ArraySize = 1;
	desc.BindFlags = BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.Format = FORMAT_D16_UNORM;
	desc.Width = wiRenderer::GetDevice()->GetScreenWidth() / 4;
	desc.Height = wiRenderer::GetDevice()->GetScreenHeight() / 4;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = USAGE_DEFAULT;
	wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &smallDepth);
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
	setTessellationEnabled(false);
	setSharpenFilterEnabled(false);

	setMSAASampleCount(1);

	setPreferredThreadingCount(0);
}

void Renderable3DComponent::Initialize()
{
	Renderable2DComponent::Initialize();
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

void Renderable3DComponent::FixedUpdate()
{
	wiRenderer::FixedUpdate();
	wiRenderer::UpdateImages();

	Renderable2DComponent::FixedUpdate();
}

void Renderable3DComponent::Update(float dt)
{
	wiRenderer::SynchronizeWithPhysicsEngine(dt);
	wiRenderer::UpdatePerFrameData(dt);

	Renderable2DComponent::Update(dt);
}

void Renderable3DComponent::Compose()
{
	RenderableComponent::Compose();

	RenderColorGradedComposition();

	if (wiRenderer::GetDebugLightCulling())
	{
		wiImage::Draw((Texture2D*)wiRenderer::textures[TEXTYPE_2D_DEBUGUAV], wiImageEffects((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()));
	}

	Renderable2DComponent::Compose();
}

void Renderable3DComponent::RenderFrameSetUp(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateRenderData(threadID);
	
	ViewPort viewPort;
	viewPort.TopLeftX = 0.0f;
	viewPort.TopLeftY = 0.0f;
	viewPort.Width = (float)smallDepth->GetDesc().Width;
	viewPort.Height = (float)smallDepth->GetDesc().Height;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	wiRenderer::GetDevice()->BindRenderTargets(0, nullptr, smallDepth, threadID);

	wiRenderer::OcclusionCulling_Render(threadID);
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
	wiProfiler::GetInstance().BeginRange("Reflection rendering", wiProfiler::DOMAIN_GPU, threadID);

	if (wiRenderer::IsRequestedReflectionRendering())
	{
		wiRenderer::UpdateCameraCB(wiRenderer::getRefCamera(), threadID);

		rtReflection.Activate(threadID); {
			// reverse clipping if underwater
			XMFLOAT4 water = wiRenderer::GetWaterPlane().getXMFLOAT4();
			if (XMVectorGetX(XMPlaneDot(XMLoadFloat4(&water), wiRenderer::getCamera()->GetEye())) < 0)
			{
				water.x *= -1;
				water.y *= -1;
				water.z *= -1;
			}

			wiRenderer::SetClipPlane(water, threadID);

			wiRenderer::DrawWorld(wiRenderer::getRefCamera(), false, threadID, SHADERTYPE_TEXTURE, nullptr, getHairParticlesReflectionEnabled(), false);
			wiRenderer::DrawSky(threadID);

			wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), threadID);
		}
	}

	wiProfiler::GetInstance().EndRange(); // Reflection Rendering
}
void Renderable3DComponent::RenderShadows(GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	if (getShadowsEnabled())
	{
		wiRenderer::DrawForShadowMap(threadID);
	}

	wiRenderer::VoxelRadiance(threadID);
}
void Renderable3DComponent::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, GRAPHICSTHREAD threadID)
{
	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());

	wiRenderer::GetDevice()->EventBegin("Downsample Depth Buffer");
	{
		// Downsample the depth buffer for the occlusion culling phase...
		ViewPort viewPort;
		viewPort.TopLeftX = 0.0f;
		viewPort.TopLeftY = 0.0f;
		viewPort.Width = (float)smallDepth->GetDesc().Width;
		viewPort.Height = (float)smallDepth->GetDesc().Height;
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0f;
		wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
		wiRenderer::GetDevice()->BindRenderTargets(0, nullptr, smallDepth, threadID);
		// This depth buffer is not cleared because we don't have to (we overwrite it anyway because depthfunc is ALWAYS)

		fx.process.setDepthBufferDownsampling(true);
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
	}
	wiRenderer::GetDevice()->EventEnd();

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}
	wiProfiler::GetInstance().BeginRange("Secondary Scene", wiProfiler::DOMAIN_GPU, threadID);

	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(wiRenderer::GetSunPosition(), wiRenderer::getCamera()->GetAt())) > 0)
	{
		wiRenderer::GetDevice()->EventBegin("Light Shafts", threadID);
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
				wiImage::Draw(rtSun[0].GetTextureResolvedMSAA(threadID), fxs, threadID);
			}
		}
		wiRenderer::GetDevice()->EventEnd();
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

	rtSceneCopy.Activate(threadID, 0, 0, 0, 0); {
		wiRenderer::GetDevice()->EventBegin("Refraction Target");
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.quality = QUALITY_NEAREST;
		fx.presentFullScreen = true;
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		if (getVolumeLightsEnabled())
		{
			// first draw light volumes to refraction target
			wiRenderer::DrawVolumeLights(wiRenderer::getCamera(), threadID);
		}
		wiRenderer::GetDevice()->EventEnd();
	}

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_GBUFFER0, 1, threadID);
	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
	wiRenderer::GenerateMipChain(rtSceneCopy.GetTexture(), wiRenderer::MIPGENFILTER_GAUSSIAN, threadID);
	shadedSceneRT.Set(threadID, mainRT.depth, false, 0);{
		RenderTransparentScene(rtSceneCopy, threadID);

		wiRenderer::DrawTrails(threadID, rtSceneCopy.GetTexture());
		if (getVolumeLightsEnabled())
		{
			// second draw volume lights on top of transparent scene
			wiRenderer::DrawVolumeLights(wiRenderer::getCamera(), threadID);
		}

		wiRenderer::GetDevice()->EventBegin("Contribute Emitters");
		fx.presentFullScreen = true;
		fx.blendFlag = BLENDMODE_ALPHA;
		if (getEmittedParticlesEnabled()) {
			wiImage::Draw(rtParticle.GetTexture(), fx, threadID);
		}

		fx.blendFlag = BLENDMODE_ADDITIVE;
		if (getEmittedParticlesEnabled()) {
			wiImage::Draw(rtParticleAdditive.GetTexture(), fx, threadID);
		}
		wiRenderer::GetDevice()->EventEnd();

		wiRenderer::GetDevice()->EventBegin("Contribute LightShafts");
		if (getLightShaftsEnabled()) {
			wiImage::Draw(rtSun.back().GetTexture(), fx, threadID);
		}
		wiRenderer::GetDevice()->EventEnd();

		if (getLensFlareEnabled())
		{
			if (!wiRenderer::IsWireRender())
				wiRenderer::DrawLensFlares(threadID);
		}

		wiRenderer::GetDevice()->EventBegin("Debug Geometry", threadID);
		wiRenderer::DrawDebugGridHelper(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugVoxels(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugEnvProbes(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawTranslators(wiRenderer::getCamera(), threadID);
		wiRenderer::GetDevice()->EventEnd();
	}

	if (wiRenderer::GetTemporalAAEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Temporal AA Resolve", threadID);
		int current = wiRenderer::GetDevice()->GetFrameCount() % 2 == 0 ? 0 : 1;
		int history = 1 - current;
		rtTemporalAA[current].Activate(threadID); {
			wiRenderer::UpdateGBuffer(mainRT.GetTextureResolvedMSAA(threadID, 0), mainRT.GetTextureResolvedMSAA(threadID, 1), nullptr, nullptr, nullptr, threadID);
			fx.presentFullScreen = false;
			fx.process.setTemporalAAResolve(true);
			fx.setMaskMap(rtTemporalAA[history].GetTexture());
			wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
			fx.process.clear();
		}
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_GBUFFER0, 1, threadID);
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		shadedSceneRT.Set(threadID, nullptr, false, 0); {
			fx.presentFullScreen = true;
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.quality = QUALITY_NEAREST;
			wiImage::Draw(rtTemporalAA[current].GetTexture(), fx, threadID);
			fx.presentFullScreen = false;
		}
		wiRenderer::GetDevice()->EventEnd();
	}

	wiProfiler::GetInstance().EndRange(); // Secondary Scene
}
void Renderable3DComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_FORWARD, refractionRT.GetTexture(), rtReflection.GetTexture()
		, rtWaterRipple.GetTexture(), threadID, false, true);

	wiProfiler::GetInstance().EndRange(); // Transparent Scene
}
void Renderable3DComponent::RenderComposition(wiRenderTarget& shadedSceneRT, wiRenderTarget& mainRT, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}
	wiProfiler::GetInstance().BeginRange("Post Processing", wiProfiler::DOMAIN_GPU, threadID);

	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());

	if (getBloomEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Bloom", threadID);
		fx.process.clear();
		fx.presentFullScreen = false;
		rtBloom[0].Activate(threadID); // separate bright parts
		{
			fx.bloom.separate = true;
			fx.bloom.saturation = getBloomSaturation();
			fx.bloom.threshold = getBloomThreshold();
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		}

		rtBloom[1].Activate(threadID); //horizontal
		{
			wiRenderer::GetDevice()->GenerateMips(rtBloom[0].GetTexture(), threadID);
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
		fx.bloom.clear();
		fx.blur = 0;
		fx.blurDir = 0;

		shadedSceneRT.Set(threadID, false, 0); { // add to the scene
			fx.blendFlag = BLENDMODE_ADDITIVE;
			fx.presentFullScreen = true;
			fx.process.clear();
			wiImage::Draw(rtBloom.back().GetTexture(), fx, threadID);
			fx.presentFullScreen = false;
		}
		wiRenderer::GetDevice()->EventEnd();
	}

	if (getMotionBlurEnabled()) {
		wiRenderer::GetDevice()->EventBegin("Motion Blur", threadID);
		rtMotionBlur.Activate(threadID);
		fx.process.setMotionBlur(true);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.presentFullScreen = false;
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
		wiRenderer::GetDevice()->EventEnd();
	}


	wiRenderer::GetDevice()->EventBegin("Tone Mapping", threadID);
	fx.blendFlag = BLENDMODE_OPAQUE;
	rtFinal[0].Activate(threadID);
	fx.process.setToneMap(true);
	if (getEyeAdaptionEnabled())
	{
		fx.setMaskMap(wiRenderer::GetLuminance(shadedSceneRT.GetTextureResolvedMSAA(threadID), threadID));
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
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
	}
	fx.process.clear();
	wiRenderer::GetDevice()->EventEnd();

	wiRenderTarget* rt0 = &rtFinal[0];
	wiRenderTarget* rt1 = &rtFinal[1];
	if (getSharpenFilterEnabled())
	{
		rt1->Activate(threadID);
		fx.process.setSharpen(true);
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
		fx.process.clear();
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		SwapPtr(rt0, rt1);
	}

	if (getDepthOfFieldEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Depth Of Field", threadID);
		// downsample + blur
		rtDof[0].Activate(threadID);
		fx.blur = getDepthOfFieldStrength();
		fx.blurDir = 0;
		wiImage::Draw(rt0->GetTexture(), fx, threadID);

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
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
		fx.setMaskMap(nullptr);
		//fx.setDepthMap(nullptr);
		fx.process.clear();
		wiRenderer::GetDevice()->EventEnd();
	}


	rt1->Activate(threadID);
	wiRenderer::GetDevice()->EventBegin("FXAA", threadID);
	fx.process.setFXAA(getFXAAEnabled());
	if (getDepthOfFieldEnabled())
		wiImage::Draw(rtDof[2].GetTexture(), fx, threadID);
	else
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
	fx.process.clear();
	wiRenderer::GetDevice()->EventEnd();


	wiProfiler::GetInstance().EndRange(); // Post Processing 1
}
void Renderable3DComponent::RenderColorGradedComposition()
{
	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_NEAREST;

	if (getStereogramEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Stereogram");
		fx.presentFullScreen = false;
		fx.process.clear();
		fx.process.setStereogram(true);
		wiImage::Draw(wiTextureHelper::getInstance()->getRandom64x64(), fx);
		wiRenderer::GetDevice()->EventEnd();
		return;
	}

	if (getColorGradingEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Color Graded Composition");
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
		wiRenderer::GetDevice()->EventBegin("Composition");
		fx.presentFullScreen = true;
	}

	if (getSharpenFilterEnabled())
	{
		wiImage::Draw(rtFinal[0].GetTexture(), fx);
	}
	else
	{
		wiImage::Draw(rtFinal[1].GetTexture(), fx);
	}
	wiRenderer::GetDevice()->EventEnd();
}


void Renderable3DComponent::setPreferredThreadingCount(unsigned short value)
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}

	workerThreads.clear();
}