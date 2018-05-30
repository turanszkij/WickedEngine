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

Renderable3DComponent::Renderable3DComponent() : Renderable2DComponent()
{
}
Renderable3DComponent::~Renderable3DComponent()
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}
}

wiRenderTarget
	Renderable3DComponent::rtReflection
	, Renderable3DComponent::rtSSR
	, Renderable3DComponent::rtMotionBlur
	, Renderable3DComponent::rtSceneCopy
	, Renderable3DComponent::rtWaterRipple
	, Renderable3DComponent::rtLinearDepth
	, Renderable3DComponent::rtParticle
	, Renderable3DComponent::rtVolumetricLights
	, Renderable3DComponent::rtFinal[2]
	, Renderable3DComponent::rtDof[3]
	, Renderable3DComponent::rtTemporalAA[2]
;
std::vector<wiRenderTarget> Renderable3DComponent::rtSun, Renderable3DComponent::rtBloom, Renderable3DComponent::rtSSAO;
wiDepthTarget Renderable3DComponent::dtDepthCopy;
Texture2D* Renderable3DComponent::smallDepth = nullptr;
void Renderable3DComponent::ResizeBuffers()
{
	Renderable2DComponent::ResizeBuffers();

	FORMAT defaultTextureFormat = wiRenderer::GetDevice()->GetBackBufferFormat();

	// Protect against multiple buffer resizes when there is no change!
	static UINT lastBufferResWidth = 0, lastBufferResHeight = 0, lastBufferMSAA = 0;
	static FORMAT lastBufferFormat = FORMAT_UNKNOWN;
	if (lastBufferResWidth == wiRenderer::GetInternalResolution().x &&
		lastBufferResHeight == wiRenderer::GetInternalResolution().y &&
		lastBufferMSAA == getMSAASampleCount() && 
		lastBufferFormat == defaultTextureFormat)
	{
		return;
	}
	else
	{
		lastBufferResWidth = wiRenderer::GetInternalResolution().x;
		lastBufferResHeight = wiRenderer::GetInternalResolution().y;
		lastBufferMSAA = getMSAASampleCount();
		lastBufferFormat = defaultTextureFormat;
	}

	rtSSR.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_hdr);
	rtMotionBlur.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_hdr);
	rtLinearDepth.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_lineardepth);
	rtParticle.Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x*getParticleDownSample()), (UINT)(wiRenderer::GetInternalResolution().y*getParticleDownSample())
		, false, wiRenderer::RTFormat_hdr);
	rtVolumetricLights.Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x*0.25f), (UINT)(wiRenderer::GetInternalResolution().y*0.25f)
		, false, wiRenderer::RTFormat_hdr);
	rtWaterRipple.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_waterripple);
	rtSceneCopy.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_hdr, 8);
	rtReflection.Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x * getReflectionQuality())
		, (UINT)(wiRenderer::GetInternalResolution().y * getReflectionQuality())
		, true, wiRenderer::RTFormat_hdr);
	rtFinal[0].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, defaultTextureFormat);
	rtFinal[1].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, defaultTextureFormat, 1);

	rtDof[0].Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x*0.5f), (UINT)(wiRenderer::GetInternalResolution().y*0.5f)
		, false, defaultTextureFormat);
	rtDof[1].Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x*0.5f), (UINT)(wiRenderer::GetInternalResolution().y*0.5f)
		, false, defaultTextureFormat);
	rtDof[2].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, defaultTextureFormat);

	dtDepthCopy.Initialize(wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y, getMSAASampleCount());
	//dtSmallDepth.Initialize(wiRenderer::GetInternalResolution().x / 4, wiRenderer::GetInternalResolution().y / 4, 1);

	rtSSAO.resize(3);
	for (unsigned int i = 0; i<rtSSAO.size(); i++)
		rtSSAO[i].Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x*getSSAOQuality()), (UINT)(wiRenderer::GetInternalResolution().y*getSSAOQuality())
			, false, wiRenderer::RTFormat_ssao);


	rtSun.resize(2);
	rtSun[0].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, true, defaultTextureFormat, 1, getMSAASampleCount()
	);
	rtSun[1].Initialize(
		(UINT)(wiRenderer::GetInternalResolution().x*getLightShaftQuality())
		, (UINT)(wiRenderer::GetInternalResolution().y*getLightShaftQuality())
		, false, defaultTextureFormat
	);

	rtBloom.resize(3);
	rtBloom[0].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, defaultTextureFormat, 0);
	for (unsigned int i = 1; i<rtBloom.size(); ++i)
		rtBloom[i].Initialize(
			(UINT)(wiRenderer::GetInternalResolution().x / getBloomDownSample())
			, (UINT)(wiRenderer::GetInternalResolution().y / getBloomDownSample())
			, false, defaultTextureFormat);

	rtTemporalAA[0].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_hdr);
	rtTemporalAA[1].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, wiRenderer::RTFormat_hdr);


	SAFE_DELETE(smallDepth);
	TextureDesc desc;
	desc.ArraySize = 1;
	desc.BindFlags = BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.Format = wiRenderer::DSFormat_small;
	desc.Width = wiRenderer::GetInternalResolution().x / 4;
	desc.Height = wiRenderer::GetInternalResolution().y / 4;
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
	setParticleDownSample(1.0f);
	setReflectionQuality(0.5f);
	setSSAOQuality(0.5f);
	setSSAOBlur(2.3f);
	setBloomStrength(19.3f);
	setBloomThreshold(0.99f);
	setBloomSaturation(-3.86f);
	setDepthOfFieldFocus(10.f);
	setDepthOfFieldStrength(2.2f);
	setSharpenFilterAmount(0.28f); 

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
	wiRenderer::UpdatePerFrameData(dt);

	Renderable2DComponent::Update(dt);
}

void Renderable3DComponent::Compose()
{
	RenderColorGradedComposition();

	if (wiRenderer::GetDebugLightCulling())
	{
		wiImage::Draw((Texture2D*)wiRenderer::textures[TEXTYPE_2D_DEBUGUAV], wiImageEffects((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()), GRAPHICSTHREAD_IMMEDIATE);
	}

	Renderable2DComponent::Compose();
}

void Renderable3DComponent::RenderFrameSetUp(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindResource(CS, dtDepthCopy.GetTexture(), TEXSLOT_DEPTH, threadID);
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

	GPUResource* dsv[] = { smallDepth };
	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_DEPTH_READ, threadID);

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

			wiRenderer::DrawWorld(wiRenderer::getRefCamera(), false, threadID, SHADERTYPE_TEXTURE, getHairParticlesReflectionEnabled(), false, getLayerMask());
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
		wiRenderer::DrawForShadowMap(threadID, getLayerMask());
	}

	wiRenderer::VoxelRadiance(threadID);
}
void Renderable3DComponent::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, GRAPHICSTHREAD threadID)
{
	wiImageEffects fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.hdr = true;

	wiRenderer::GetDevice()->EventBegin("Downsample Depth Buffer", threadID);
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

		GPUResource* dsv[] = { smallDepth };
		wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		fx.process.setDepthBufferDownsampling(true);
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
	}
	wiRenderer::GetDevice()->EventEnd(threadID);

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
			fxs.blendFlag = BLENDMODE_OPAQUE;
			XMVECTOR sunPos = XMVector3Project(wiRenderer::GetSunPosition() * 100000, 0, 0, 
				(float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 1.0f, 
				wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
			{
				XMStoreFloat2(&fxs.sunPos, sunPos);
				wiImage::Draw(rtSun[0].GetTextureResolvedMSAA(threadID), fxs, threadID);
			}
		}
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getVolumeLightsEnabled())
	{
		rtVolumetricLights.Activate(threadID, 0, 0, 0, 0);
		wiRenderer::DrawVolumeLights(wiRenderer::getCamera(), threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		rtParticle.Activate(threadID, 0, 0, 0, 0);
		wiRenderer::DrawSoftParticles(wiRenderer::getCamera(), false, threadID);
	}

	rtWaterRipple.Activate(threadID, 0, 0, 0, 0); {
		wiRenderer::DrawWaterRipples(threadID);
	}

	rtSceneCopy.Set(threadID); {
		wiRenderer::GetDevice()->EventBegin("Refraction Target", threadID);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.quality = QUALITY_NEAREST;
		fx.presentFullScreen = true;
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_GBUFFER0, 1, threadID);
	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
	if (wiRenderer::GetAdvancedRefractionsEnabled())
	{
		wiRenderer::GenerateMipChain(rtSceneCopy.GetTexture(), wiRenderer::MIPGENFILTER_GAUSSIAN, threadID);
	}
	shadedSceneRT.Set(threadID, mainRT.depth, false, 0);{
		RenderTransparentScene(rtSceneCopy, threadID);

		wiRenderer::DrawTrails(threadID, rtSceneCopy.GetTexture());
		wiRenderer::DrawLightVisualizers(wiRenderer::getCamera(), threadID);

		fx.presentFullScreen = true;

		if (getEmittedParticlesEnabled()) {
			wiRenderer::GetDevice()->EventBegin("Contribute Emitters", threadID);
			fx.blendFlag = BLENDMODE_PREMULTIPLIED;
			wiImage::Draw(rtParticle.GetTexture(), fx, threadID);
			wiRenderer::GetDevice()->EventEnd(threadID);
		}

		if (getVolumeLightsEnabled())
		{
			wiRenderer::GetDevice()->EventBegin("Contribute Volumetric Lights", threadID);
			wiImage::Draw(rtVolumetricLights.GetTexture(), fx, threadID);
			wiRenderer::GetDevice()->EventEnd(threadID);
		}

		if (getLightShaftsEnabled()) {
			wiRenderer::GetDevice()->EventBegin("Contribute LightShafts", threadID);
			fx.blendFlag = BLENDMODE_ADDITIVE;
			wiImage::Draw(rtSun.back().GetTexture(), fx, threadID);
			wiRenderer::GetDevice()->EventEnd(threadID);
		}

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
		wiRenderer::DrawDebugForceFields(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugEmitters(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugCameras(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawTranslators(wiRenderer::getCamera(), threadID);
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Temporal AA Resolve", threadID);
		wiProfiler::GetInstance().BeginRange("Temporal AA Resolve", wiProfiler::DOMAIN_GPU, threadID);
		fx.blendFlag = BLENDMODE_OPAQUE;
		int current = wiRenderer::GetDevice()->GetFrameCount() % 2 == 0 ? 0 : 1;
		int history = 1 - current;
		rtTemporalAA[current].Set(threadID); {
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
			fx.quality = QUALITY_NEAREST;
			wiImage::Draw(rtTemporalAA[current].GetTexture(), fx, threadID);
			fx.presentFullScreen = false;
		}
		wiProfiler::GetInstance().EndRange(threadID);
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		rtParticle.Activate(threadID, 0, 0, 0, 0);
		wiRenderer::DrawSoftParticles(wiRenderer::getCamera(), true, threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID); // Secondary Scene
}
void Renderable3DComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, refractionRT.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, rtWaterRipple.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_FORWARD, threadID, false, true, getLayerMask());

	wiProfiler::GetInstance().EndRange(threadID); // Transparent Scene
}
void Renderable3DComponent::RenderComposition(wiRenderTarget& shadedSceneRT, wiRenderTarget& mainRT, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}
	wiProfiler::GetInstance().BeginRange("Post Processing", wiProfiler::DOMAIN_GPU, threadID);

	wiImageEffects fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.hdr = true;

	if (getBloomEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Bloom", threadID);
		fx.process.clear();
		fx.presentFullScreen = false;
		rtBloom[0].Set(threadID); // separate bright parts
		{
			fx.bloom.separate = true;
			fx.bloom.saturation = getBloomSaturation();
			fx.bloom.threshold = getBloomThreshold();
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		}

		rtBloom[1].Set(threadID); //horizontal
		{
			wiRenderer::GetDevice()->GenerateMips(rtBloom[0].GetTexture(), threadID);
			fx.mipLevel = 5.32f;
			fx.blur = getBloomStrength();
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtBloom[0].GetTexture(), fx, threadID);
		}
		rtBloom[2].Set(threadID); //vertical
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
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getMotionBlurEnabled()) {
		wiRenderer::GetDevice()->EventBegin("Motion Blur", threadID);
		rtMotionBlur.Set(threadID);
		fx.process.setMotionBlur(true);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.presentFullScreen = false;
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	fx.hdr = false;

	wiRenderer::GetDevice()->EventBegin("Tone Mapping", threadID);
	fx.blendFlag = BLENDMODE_OPAQUE;
	rtFinal[0].Set(threadID);
	fx.process.setToneMap(true);
	fx.setDistortionMap(rtParticle.GetTexture());
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
	wiRenderer::GetDevice()->EventEnd(threadID);

	wiRenderTarget* rt0 = &rtFinal[0];
	wiRenderTarget* rt1 = &rtFinal[1];
	if (getSharpenFilterEnabled())
	{
		rt1->Set(threadID);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.process.setSharpen(getSharpenFilterAmount());
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
		fx.process.clear();
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		SwapPtr(rt0, rt1);
	}

	if (getDepthOfFieldEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Depth Of Field", threadID);
		// downsample + blur
		fx.blendFlag = BLENDMODE_OPAQUE;
		rtDof[0].Set(threadID);
		fx.blur = getDepthOfFieldStrength();
		fx.blurDir = 0;
		wiImage::Draw(rt0->GetTexture(), fx, threadID);

		rtDof[1].Set(threadID);
		fx.blurDir = 1;
		wiImage::Draw(rtDof[0].GetTexture(), fx, threadID);
		fx.blur = 0;
		fx.process.clear();

		// depth of field compose pass
		rtDof[2].Set(threadID);
		fx.process.setDOF(getDepthOfFieldFocus());
		fx.setMaskMap(rtDof[1].GetTexture());
		//fx.setDepthMap(rtLinearDepth.shaderResource.back());
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
		fx.setMaskMap(nullptr);
		//fx.setDepthMap(nullptr);
		fx.process.clear();
		wiRenderer::GetDevice()->EventEnd(threadID);
	}


	rt1->Set(threadID);
	wiRenderer::GetDevice()->EventBegin("FXAA", threadID);
	if (getFXAAEnabled())
	{
		fx.process.setFXAA(true);
	}
	else
	{
		fx.presentFullScreen = true;
	}
	if (getDepthOfFieldEnabled())
		wiImage::Draw(rtDof[2].GetTexture(), fx, threadID);
	else
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
	fx.process.clear();
	wiRenderer::GetDevice()->EventEnd(threadID);


	wiProfiler::GetInstance().EndRange(threadID); // Post Processing 1
}
void Renderable3DComponent::RenderColorGradedComposition()
{
	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;
	fx.quality = QUALITY_BILINEAR;

	if (getStereogramEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Stereogram", GRAPHICSTHREAD_IMMEDIATE);
		fx.presentFullScreen = false;
		fx.process.clear();
		fx.process.setStereogram(true);
		wiImage::Draw(wiTextureHelper::getInstance()->getRandom64x64(), fx, GRAPHICSTHREAD_IMMEDIATE);
		wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
		return;
	}

	if (getColorGradingEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Color Graded Composition", GRAPHICSTHREAD_IMMEDIATE);
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
		wiRenderer::GetDevice()->EventBegin("Composition", GRAPHICSTHREAD_IMMEDIATE);
		fx.presentFullScreen = true;
	}

	if (getSharpenFilterEnabled())
	{
		wiImage::Draw(rtFinal[0].GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);
	}
	else
	{
		wiImage::Draw(rtFinal[1].GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);
	}
	wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
}


void Renderable3DComponent::setPreferredThreadingCount(unsigned short value)
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}

	workerThreads.clear();
}