#include "RenderPath3D.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSceneSystem.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

RenderPath3D::RenderPath3D() : RenderPath2D()
{
}
RenderPath3D::~RenderPath3D()
{
}

wiRenderTarget
	RenderPath3D::rtReflection
	, RenderPath3D::rtSSR
	, RenderPath3D::rtMotionBlur
	, RenderPath3D::rtSceneCopy
	, RenderPath3D::rtWaterRipple
	, RenderPath3D::rtLinearDepth
	, RenderPath3D::rtParticle
	, RenderPath3D::rtVolumetricLights
	, RenderPath3D::rtFinal[2]
	, RenderPath3D::rtDof[3]
	, RenderPath3D::rtTemporalAA[2]
	, RenderPath3D::rtBloom
;
std::vector<wiRenderTarget> RenderPath3D::rtSun, RenderPath3D::rtSSAO;
wiDepthTarget RenderPath3D::dtDepthCopy;
Texture2D* RenderPath3D::smallDepth = nullptr;
void RenderPath3D::ResizeBuffers()
{
	RenderPath2D::ResizeBuffers();

	wiRenderer::GetDevice()->WaitForGPU();

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

	rtBloom.Initialize(wiRenderer::GetInternalResolution().x / 4, wiRenderer::GetInternalResolution().y / 4, false, defaultTextureFormat, 5);

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

void RenderPath3D::setProperties()
{
	setExposure(1.0f);
	setLightShaftQuality(0.4f);
	setParticleDownSample(1.0f);
	setReflectionQuality(0.5f);
	setSSAOQuality(0.5f);
	setSSAOBlur(2.3f);
	setBloomThreshold(1.0f);
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
}

void RenderPath3D::Initialize()
{
	RenderPath2D::Initialize();
}

void RenderPath3D::Load()
{
	RenderPath2D::Load();
}

void RenderPath3D::Start()
{
	RenderPath2D::Start();
}

void RenderPath3D::FixedUpdate()
{
	RenderPath2D::FixedUpdate();
}

void RenderPath3D::Update(float dt)
{
	wiRenderer::UpdatePerFrameData(dt);

	RenderPath2D::Update(dt);
}

void RenderPath3D::Compose()
{
	RenderColorGradedComposition();

	if (wiRenderer::GetDebugLightCulling())
	{
		wiImage::Draw((Texture2D*)wiRenderer::GetTexture(TEXTYPE_2D_DEBUGUAV), wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()), GRAPHICSTHREAD_IMMEDIATE);
	}

	RenderPath2D::Compose();
}

void RenderPath3D::RenderFrameSetUp(GRAPHICSTHREAD threadID)
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
void RenderPath3D::RenderReflections(GRAPHICSTHREAD threadID)
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
	wiProfiler::BeginRange("Reflection rendering", wiProfiler::DOMAIN_GPU, threadID);

	if (wiRenderer::IsRequestedReflectionRendering())
	{
		wiRenderer::UpdateCameraCB(wiRenderer::GetRefCamera(), threadID);

		rtReflection.Activate(threadID); {
			// reverse clipping if underwater
			XMFLOAT4 water = wiRenderer::GetWaterPlane();
			float d = XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&water), wiRenderer::GetCamera().GetEye()));
			if (d < 0)
			{
				water.x *= -1;
				water.y *= -1;
				water.z *= -1;
			}

			wiRenderer::SetClipPlane(water, threadID);

			wiRenderer::DrawScene(wiRenderer::GetRefCamera(), false, threadID, SHADERTYPE_TEXTURE, getHairParticlesReflectionEnabled(), false, getLayerMask());
			wiRenderer::DrawSky(threadID);

			wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), threadID);
		}
	}

	wiProfiler::EndRange(); // Reflection Rendering
}
void RenderPath3D::RenderShadows(GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	if (getShadowsEnabled())
	{
		wiRenderer::DrawForShadowMap(wiRenderer::GetCamera(), threadID, getLayerMask());
	}

	wiRenderer::VoxelRadiance(threadID);
}
void RenderPath3D::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, GRAPHICSTHREAD threadID)
{
	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.enableHDR();

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

		fx.process.setDepthBufferDownsampling();
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
	}
	wiRenderer::GetDevice()->EventEnd(threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}
	wiProfiler::BeginRange("Secondary Scene", wiProfiler::DOMAIN_GPU, threadID);

	XMVECTOR sunDirection = XMLoadFloat3(&wiRenderer::GetScene().weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, wiRenderer::GetCamera().GetAt())) > 0)
	{
		wiRenderer::GetDevice()->EventBegin("Light Shafts", threadID);
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
		rtSun[0].Activate(threadID, mainRT.depth); {
			wiRenderer::DrawSun(threadID);
		}

		rtSun[1].Activate(threadID); {
			wiImageParams fxs = fx;
			fxs.blendFlag = BLENDMODE_OPAQUE;
			XMVECTOR sunPos = XMVector3Project(sunDirection * 100000, 0, 0, 
				(float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 1.0f, 
				wiRenderer::GetCamera().GetProjection(), wiRenderer::GetCamera().GetView(), XMMatrixIdentity());
			{
				XMFLOAT2 sun;
				XMStoreFloat2(&sun, sunPos);
				fxs.process.setLightShaftCenter(sun);
				wiImage::Draw(rtSun[0].GetTextureResolvedMSAA(threadID), fxs, threadID);
			}
		}
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getVolumeLightsEnabled())
	{
		rtVolumetricLights.Activate(threadID, 0, 0, 0, 0);
		wiRenderer::DrawVolumeLights(wiRenderer::GetCamera(), threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		rtParticle.Activate(threadID, 0, 0, 0, 0);
		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), false, threadID);
	}

	rtWaterRipple.Activate(threadID, 0, 0, 0, 0); {
		wiRenderer::DrawWaterRipples(threadID);
	}

	rtSceneCopy.Set(threadID); {
		wiRenderer::GetDevice()->EventBegin("Refraction Target", threadID);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.quality = QUALITY_NEAREST;
		fx.enableFullScreen();
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	wiRenderer::GetDevice()->UnbindResources(TEXSLOT_GBUFFER0, 1, threadID);
	wiRenderer::GetDevice()->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
	if (wiRenderer::GetAdvancedRefractionsEnabled())
	{
		wiRenderer::GenerateMipChain(rtSceneCopy.GetTexture(), wiRenderer::MIPGENFILTER_GAUSSIAN, threadID);
	}
	shadedSceneRT.Set(threadID, mainRT.depth, false, 0);{
		RenderTransparentScene(rtSceneCopy, threadID);

		wiRenderer::DrawLightVisualizers(wiRenderer::GetCamera(), threadID);

		fx.enableFullScreen();

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

		wiRenderer::DrawDebugWorld(wiRenderer::GetCamera(), threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		rtParticle.Activate(threadID, 0, 0, 0, 0);
		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), true, threadID);
	}

	wiProfiler::EndRange(threadID); // Secondary Scene
}
void RenderPath3D::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, refractionRT.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, rtWaterRipple.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), SHADERTYPE_FORWARD, threadID, false, true, getLayerMask());

	wiProfiler::EndRange(threadID); // Transparent Scene
}
void RenderPath3D::RenderComposition(wiRenderTarget& shadedSceneRT, wiRenderTarget& mainRT, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}
	wiProfiler::BeginRange("Post Processing", wiProfiler::DOMAIN_GPU, threadID);

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Temporal AA Resolve", threadID);
		wiProfiler::BeginRange("Temporal AA Resolve", wiProfiler::DOMAIN_GPU, threadID);
		fx.enableHDR();
		fx.blendFlag = BLENDMODE_OPAQUE;
		int current = wiRenderer::GetDevice()->GetFrameCount() % 2 == 0 ? 0 : 1;
		int history = 1 - current;
		rtTemporalAA[current].Set(threadID); {
			wiRenderer::BindGBufferTextures(mainRT.GetTextureResolvedMSAA(threadID, 0), mainRT.GetTextureResolvedMSAA(threadID, 1), nullptr, threadID);
			fx.disableFullScreen();
			fx.process.setTemporalAAResolve();
			fx.setMaskMap(rtTemporalAA[history].GetTexture());
			wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
			fx.process.clear();
		}
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_GBUFFER0, 1, threadID);
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		shadedSceneRT.Set(threadID, nullptr, false, 0); {
			fx.enableFullScreen();
			fx.quality = QUALITY_NEAREST;
			wiImage::Draw(rtTemporalAA[current].GetTexture(), fx, threadID);
			fx.disableFullScreen();
		}
		fx.disableHDR();
		wiProfiler::EndRange(threadID);
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getBloomEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Bloom", threadID);
		fx.setMaskMap(nullptr);
		fx.process.clear();
		fx.disableFullScreen();
		fx.quality = QUALITY_LINEAR;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		rtBloom.Set(threadID); // separate bright parts
		{
			fx.process.setBloom(getBloomThreshold());
			wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
			wiRenderer::GetDevice()->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		}
		fx.process.clear();

		wiRenderer::GenerateMipChain(rtBloom.GetTexture(), wiRenderer::MIPGENFILTER_GAUSSIAN, threadID);
		shadedSceneRT.Set(threadID, false, 0); { // add to the scene
			// not full screen effect, because we draw specific mip levels, so some setup is required
			XMFLOAT2 siz = fx.siz;
			fx.siz = XMFLOAT2((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
			fx.blendFlag = BLENDMODE_ADDITIVE;
			//fx.quality = QUALITY_BICUBIC;
			fx.process.clear();
			fx.mipLevel = 1.5f;
			wiImage::Draw(rtBloom.GetTexture(), fx, threadID);
			fx.mipLevel = 3.5f;
			wiImage::Draw(rtBloom.GetTexture(), fx, threadID);
			fx.mipLevel = 5.5f;
			wiImage::Draw(rtBloom.GetTexture(), fx, threadID);
			fx.quality = QUALITY_LINEAR;
			fx.siz = siz;
		}

		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getMotionBlurEnabled()) {
		wiRenderer::GetDevice()->EventBegin("Motion Blur", threadID);
		rtMotionBlur.Set(threadID);
		fx.process.setMotionBlur();
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.disableFullScreen();
		wiImage::Draw(shadedSceneRT.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	fx.disableHDR();

	wiRenderer::GetDevice()->EventBegin("Tone Mapping", threadID);
	fx.blendFlag = BLENDMODE_OPAQUE;
	rtFinal[0].Set(threadID);
	fx.process.setToneMap(getExposure());
	fx.setDistortionMap(rtParticle.GetTexture());
	if (getEyeAdaptionEnabled())
	{
		fx.setMaskMap(wiRenderer::GetLuminance(shadedSceneRT.GetTextureResolvedMSAA(threadID), threadID));
	}
	else
	{
		fx.setMaskMap(wiTextureHelper::getColor(wiColor::Gray()));
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
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		SwapPtr(rt0, rt1);
	}

	if (getDepthOfFieldEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Depth Of Field", threadID);
		// downsample + blur
		fx.blendFlag = BLENDMODE_OPAQUE;
		rtDof[0].Set(threadID);
		fx.process.setBlur(XMFLOAT2(getDepthOfFieldStrength(), 0));
		wiImage::Draw(rt0->GetTexture(), fx, threadID);

		rtDof[1].Set(threadID);
		fx.process.setBlur(XMFLOAT2(0, getDepthOfFieldStrength()));
		wiImage::Draw(rtDof[0].GetTexture(), fx, threadID);
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
		fx.process.setFXAA();
	}
	else
	{
		fx.enableFullScreen();
	}
	if (getDepthOfFieldEnabled())
		wiImage::Draw(rtDof[2].GetTexture(), fx, threadID);
	else
		wiImage::Draw(rt0->GetTexture(), fx, threadID);
	fx.process.clear();
	wiRenderer::GetDevice()->EventEnd(threadID);


	wiProfiler::EndRange(threadID); // Post Processing 1
}
void RenderPath3D::RenderColorGradedComposition()
{
	wiImageParams fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;
	fx.quality = QUALITY_LINEAR;

	if (getStereogramEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Stereogram", GRAPHICSTHREAD_IMMEDIATE);
		fx.disableFullScreen();
		fx.process.clear();
		fx.process.setStereogram();
		wiImage::Draw(wiTextureHelper::getRandom64x64(), fx, GRAPHICSTHREAD_IMMEDIATE);
		wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
		return;
	}

	if (getColorGradingEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("Color Graded Composition", GRAPHICSTHREAD_IMMEDIATE);
		if (colorGradingTex != nullptr){
			fx.process.setColorGrade();
			fx.setMaskMap(colorGradingTex);
		}
		else
		{
			fx.process.setColorGrade();
			fx.setMaskMap(wiTextureHelper::getColorGradeDefault());
		}
	}
	else
	{
		wiRenderer::GetDevice()->EventBegin("Composition", GRAPHICSTHREAD_IMMEDIATE);
		fx.enableFullScreen();
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
