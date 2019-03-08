#include "RenderPath3D.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSceneSystem.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;

void RenderPath3D::ResizeBuffers()
{
	RenderPath2D::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();


	// Render targets:
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtSSR);
		device->CreateTexture2D(&desc, nullptr, &rtMotionBlur);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_lineardepth;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtLinearDepth);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*getParticleDownSample());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*getParticleDownSample());
		device->CreateTexture2D(&desc, nullptr, &rtParticle);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x * 0.25f);
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y * 0.25f);
		device->CreateTexture2D(&desc, nullptr, &rtVolumetricLights);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_waterripple;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtWaterRipple);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.MipLevels = 5;
		rtSceneCopy.RequestIndependentShaderResourcesForMIPs(true);
		rtSceneCopy.RequestIndependentUnorderedAccessResourcesForMIPs(true);
		device->CreateTexture2D(&desc, nullptr, &rtSceneCopy);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x * getReflectionQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y * getReflectionQuality());
		device->CreateTexture2D(&desc, nullptr, &rtReflection);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtFinal[0]);
		device->CreateTexture2D(&desc, nullptr, &rtFinal[1]);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*0.5f);
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*0.5f);
		device->CreateTexture2D(&desc, nullptr, &rtDof[0]);
		device->CreateTexture2D(&desc, nullptr, &rtDof[1]);

		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtDof[2]);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_ssao;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*getSSAOQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*getSSAOQuality());
		device->CreateTexture2D(&desc, nullptr, &rtSSAO[0]);
		device->CreateTexture2D(&desc, nullptr, &rtSSAO[1]);
		device->CreateTexture2D(&desc, nullptr, &rtSSAO[2]);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleDesc.Count = getMSAASampleCount();
		device->CreateTexture2D(&desc, nullptr, &rtSun[0]);

		desc.SampleDesc.Count = 1;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*getLightShaftQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*getLightShaftQuality());
		device->CreateTexture2D(&desc, nullptr, &rtSun[1]);

		if (getMSAASampleCount() > 1)
		{
			desc.Width = wiRenderer::GetInternalResolution().x;
			desc.Height = wiRenderer::GetInternalResolution().y;
			desc.SampleDesc.Count = 1;
			device->CreateTexture2D(&desc, nullptr, &rtSun_resolved);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		desc.MipLevels = 5;
		rtBloom.RequestIndependentShaderResourcesForMIPs(true);
		rtBloom.RequestIndependentUnorderedAccessResourcesForMIPs(true);
		device->CreateTexture2D(&desc, nullptr, &rtBloom);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtTemporalAA[0]);
		device->CreateTexture2D(&desc, nullptr, &rtTemporalAA[1]);
	}

	// Depth buffers:
	{
		TextureDesc desc;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;

		desc.Format = wiRenderer::DSFormat_full;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.SampleDesc.Count = getMSAASampleCount();
		device->CreateTexture2D(&desc, nullptr, &depthBuffer);

		desc.Format = wiRenderer::DSFormat_full_alias;
		desc.BindFlags = BIND_SHADER_RESOURCE /*| BIND_UNORDERED_ACCESS*/;
		desc.SampleDesc.Count = 1;
		device->CreateTexture2D(&desc, nullptr, &depthCopy);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.Format = wiRenderer::DSFormat_small;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		device->CreateTexture2D(&desc, nullptr, &smallDepth);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.Format = wiRenderer::DSFormat_small;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x * getReflectionQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y * getReflectionQuality());
		device->CreateTexture2D(&desc, nullptr, &depthBuffer_reflection);
	}
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
	RenderPath2D::Update(dt);

	wiRenderer::UpdatePerFrameData(dt);
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
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->BindResource(CS, &depthCopy, TEXSLOT_DEPTH, threadID);
	wiRenderer::UpdateRenderData(threadID);
	
	ViewPort viewPort;
	viewPort.Width = (float)smallDepth.GetDesc().Width;
	viewPort.Height = (float)smallDepth.GetDesc().Height;
	device->BindViewports(1, &viewPort, threadID);
	device->BindRenderTargets(0, nullptr, &smallDepth, threadID);

	const GPUResource* dsv[] = { &smallDepth };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_DEPTH_READ, threadID);

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

		{
			GraphicsDevice* device = wiRenderer::GetDevice();

			const Texture2D* rts[] = { &rtReflection };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer_reflection, threadID);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[0], clear, threadID);
			device->ClearDepthStencil(&depthBuffer_reflection, CLEAR_DEPTH, 0, 0, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

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

			wiRenderer::DrawScene(wiRenderer::GetRefCamera(), false, threadID, RENDERPASS_TEXTURE, getHairParticlesReflectionEnabled(), false, getLayerMask());
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
void RenderPath3D::RenderSecondaryScene(const Texture2D& mainRT, const Texture2D& shadedSceneRT, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.enableHDR();

	device->EventBegin("Downsample Depth Buffer", threadID);
	{
		// Downsample the depth buffer for the occlusion culling phase...
		ViewPort viewPort;
		viewPort.Width = (float)smallDepth.GetDesc().Width;
		viewPort.Height = (float)smallDepth.GetDesc().Height;
		device->BindViewports(1, &viewPort, threadID);
		device->BindRenderTargets(0, nullptr, &smallDepth, threadID);
		// This depth buffer is not cleared because we don't have to (we overwrite it anyway because depthfunc is ALWAYS)

		GPUResource* dsv[] = { &smallDepth };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		fx.process.setDepthBufferDownsampling();
		wiImage::Draw(&depthCopy, fx, threadID);
		fx.process.clear();
	}
	device->EventEnd(threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}
	wiProfiler::BeginRange("Secondary Scene", wiProfiler::DOMAIN_GPU, threadID);

	if (getOutlineEnabled())
	{
		device->EventBegin("Outline", threadID);

		const Texture2D* rts[] = { &shadedSceneRT };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.process.setOutline(getOutlineThreshold(), getOutlineThickness(), getOutlineColor());
		wiImage::Draw(nullptr, fx, threadID);
		fx.process.clear();
		device->EventEnd(threadID);
	}

	XMVECTOR sunDirection = XMLoadFloat3(&wiRenderer::GetScene().weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, wiRenderer::GetCamera().GetAt())) > 0)
	{
		device->EventBegin("Light Shafts", threadID);
		device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

		// Render sun stencil cutout:
		{
			const Texture2D* rts[] = { &rtSun[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[0], clear, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			wiRenderer::DrawSun(threadID);
		}

		const Texture2D* sunSource = &rtSun[0];
		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(&rtSun_resolved, sunSource, threadID);
			sunSource = &rtSun_resolved;
		}

		// Radial blur on the sun:
		{
			const Texture2D* rts[] = { &rtSun[1] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[0], clear, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			wiImageParams fxs = fx;
			fxs.blendFlag = BLENDMODE_OPAQUE;
			XMVECTOR sunPos = XMVector3Project(sunDirection * 100000, 0, 0, 
				(float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 1.0f, 
				wiRenderer::GetCamera().GetProjection(), wiRenderer::GetCamera().GetView(), XMMatrixIdentity());
			{
				XMFLOAT2 sun;
				XMStoreFloat2(&sun, sunPos);
				fxs.process.setLightShaftCenter(sun);
				wiImage::Draw(sunSource, fxs, threadID);
			}
		}
		device->EventEnd(threadID);
	}

	if (getVolumeLightsEnabled())
	{
		const Texture2D* rts[] = { &rtVolumetricLights };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiRenderer::DrawVolumeLights(wiRenderer::GetCamera(), threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		const Texture2D* rts[] = { &rtParticle };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), false, threadID);
	}

	// todo: refactor water ripples and avoid clear if there is none!
	{
		const Texture2D* rts[] = { &rtWaterRipple };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiRenderer::DrawWaterRipples(threadID);
	}

	{
		device->EventBegin("Refraction Target", threadID);

		const Texture2D* rts[] = { &rtSceneCopy };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.quality = QUALITY_NEAREST;
		fx.enableFullScreen();
		wiImage::Draw(&shadedSceneRT, fx, threadID);
		device->EventEnd(threadID);
	}

	device->UnbindResources(TEXSLOT_GBUFFER0, 1, threadID);
	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
	if (wiRenderer::GetAdvancedRefractionsEnabled())
	{
		wiRenderer::GenerateMipChain(&rtSceneCopy, wiRenderer::MIPGENFILTER_GAUSSIAN, threadID);
	}
	{
		const Texture2D* rts[] = { &shadedSceneRT };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		RenderTransparentScene(rtSceneCopy, threadID);

		wiRenderer::DrawLightVisualizers(wiRenderer::GetCamera(), threadID);

		fx.enableFullScreen();

		if (getEmittedParticlesEnabled()) 
		{
			device->EventBegin("Contribute Emitters", threadID);
			fx.blendFlag = BLENDMODE_PREMULTIPLIED;
			wiImage::Draw(&rtParticle, fx, threadID);
			device->EventEnd(threadID);
		}

		if (getVolumeLightsEnabled())
		{
			device->EventBegin("Contribute Volumetric Lights", threadID);
			wiImage::Draw(&rtVolumetricLights, fx, threadID);
			device->EventEnd(threadID);
		}

		if (getLightShaftsEnabled()) 
		{
			device->EventBegin("Contribute LightShafts", threadID);
			fx.blendFlag = BLENDMODE_ADDITIVE;
			wiImage::Draw(&rtSun[1], fx, threadID);
			device->EventEnd(threadID);
		}

		if (getLensFlareEnabled())
		{
			wiRenderer::DrawLensFlares(wiRenderer::GetCamera(), threadID);
		}

		wiRenderer::DrawDebugWorld(wiRenderer::GetCamera(), threadID);
	}

	if (getEmittedParticlesEnabled())
	{
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		const Texture2D* rts[] = { &rtParticle };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), true, threadID);
	}

	wiProfiler::EndRange(threadID); // Secondary Scene
}
void RenderPath3D::RenderTransparentScene(const Texture2D& refractionRT, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiProfiler::BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	device->BindResource(PS, &refractionRT, TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), RENDERPASS_FORWARD, threadID, false, true, getLayerMask());

	wiProfiler::EndRange(threadID); // Transparent Scene
}
void RenderPath3D::RenderComposition(const Texture2D& shadedSceneRT, const Texture2D& mainRT0, const Texture2D& mainRT1, GRAPHICSTHREAD threadID)
{
	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();

	wiProfiler::BeginRange("Post Processing", wiProfiler::DOMAIN_GPU, threadID);

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	{
		device->EventBegin("Temporal AA Resolve", threadID);
		wiProfiler::BeginRange("Temporal AA Resolve", wiProfiler::DOMAIN_GPU, threadID);
		fx.enableHDR();
		fx.blendFlag = BLENDMODE_OPAQUE;
		int current = device->GetFrameCount() % 2 == 0 ? 0 : 1;
		int history = 1 - current;
		{
			const Texture2D* rts[] = { &rtTemporalAA[current] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			wiRenderer::BindGBufferTextures(&mainRT0, &mainRT1, nullptr, threadID);
			fx.disableFullScreen();
			fx.process.setTemporalAAResolve();
			fx.setMaskMap(&rtTemporalAA[history]);
			wiImage::Draw(&shadedSceneRT, fx, threadID);
			fx.process.clear();
		}
		device->UnbindResources(TEXSLOT_GBUFFER0, 1, threadID);
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		{
			const Texture2D* rts[] = { &shadedSceneRT };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.enableFullScreen();
			fx.quality = QUALITY_NEAREST;
			wiImage::Draw(&rtTemporalAA[current], fx, threadID);
			fx.disableFullScreen();
		}
		fx.disableHDR();
		wiProfiler::EndRange(threadID);
		device->EventEnd(threadID);
	}

	if (getBloomEnabled())
	{
		device->EventBegin("Bloom", threadID);
		fx.setMaskMap(nullptr);
		fx.process.clear();
		fx.disableFullScreen();
		fx.quality = QUALITY_LINEAR;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		// separate bright parts
		{
			const Texture2D* rts[] = { &rtBloom };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.setBloom(getBloomThreshold());
			wiImage::Draw(&shadedSceneRT, fx, threadID);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		}
		fx.process.clear();

		wiRenderer::GenerateMipChain(&rtBloom, wiRenderer::MIPGENFILTER_GAUSSIAN, threadID);
		// add to the scene
		{
			const Texture2D* rts[] = { &shadedSceneRT };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			// not full screen effect, because we draw specific mip levels, so some setup is required
			XMFLOAT2 siz = fx.siz;
			fx.siz = XMFLOAT2((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
			fx.blendFlag = BLENDMODE_ADDITIVE;
			//fx.quality = QUALITY_BICUBIC;
			fx.process.clear();
			fx.mipLevel = 1.5f;
			wiImage::Draw(&rtBloom, fx, threadID);
			fx.mipLevel = 3.5f;
			wiImage::Draw(&rtBloom, fx, threadID);
			fx.mipLevel = 5.5f;
			wiImage::Draw(&rtBloom, fx, threadID);
			fx.quality = QUALITY_LINEAR;
			fx.siz = siz;
		}

		device->EventEnd(threadID);
	}

	if (getMotionBlurEnabled()) 
	{
		device->EventBegin("Motion Blur", threadID);

		const Texture2D* rts[] = { &rtMotionBlur };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.process.setMotionBlur();
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.disableFullScreen();
		wiImage::Draw(&shadedSceneRT, fx, threadID);
		fx.process.clear();
		device->EventEnd(threadID);
	}


	{
		device->EventBegin("Tone Mapping", threadID);
		fx.disableHDR();
		fx.blendFlag = BLENDMODE_OPAQUE;

		const Texture2D* rts[] = { &rtFinal[0] };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.process.setToneMap(getExposure());
		fx.setDistortionMap(&rtParticle);
		if (getEyeAdaptionEnabled())
		{
			fx.setMaskMap(wiRenderer::GetLuminance(&shadedSceneRT, threadID));
		}
		else
		{
			fx.setMaskMap(wiTextureHelper::getColor(wiColor::Gray()));
		}
		if (getMotionBlurEnabled())
		{
			wiImage::Draw(&rtMotionBlur, fx, threadID);
		}
		else
		{
			wiImage::Draw(&shadedSceneRT, fx, threadID);
		}
		fx.process.clear();
		device->EventEnd(threadID);
	}

	Texture2D* rt0 = &rtFinal[0];
	Texture2D* rt1 = &rtFinal[1];
	if (getSharpenFilterEnabled())
	{
		const Texture2D* rts[] = { rt1 };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.process.setSharpen(getSharpenFilterAmount());
		wiImage::Draw(rt0, fx, threadID);
		fx.process.clear();
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		SwapPtr(rt0, rt1);
	}

	if (getDepthOfFieldEnabled())
	{
		device->EventBegin("Depth Of Field", threadID);
		// downsample + blur
		{
			const Texture2D* rts[] = { &rtDof[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.process.setBlur(XMFLOAT2(getDepthOfFieldStrength(), 0));
			wiImage::Draw(rt0, fx, threadID);
		}

		{
			const Texture2D* rts[] = { &rtDof[1] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.setBlur(XMFLOAT2(0, getDepthOfFieldStrength()));
			wiImage::Draw(&rtDof[0], fx, threadID);
			fx.process.clear();
		}

		// depth of field compose pass
		{
			const Texture2D* rts[] = { &rtDof[2] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.setDOF(getDepthOfFieldFocus());
			fx.setMaskMap(&rtDof[1]);
			wiImage::Draw(rt0, fx, threadID);
			fx.setMaskMap(nullptr);
			fx.process.clear();
		}
		device->EventEnd(threadID);
	}


	{
		device->EventBegin("FXAA", threadID);

		const Texture2D* rts[] = { rt1 };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		if (getFXAAEnabled())
		{
			fx.process.setFXAA();
		}
		else
		{
			fx.enableFullScreen();
		}
		if (getDepthOfFieldEnabled())
			wiImage::Draw(&rtDof[2], fx, threadID);
		else
			wiImage::Draw(rt0, fx, threadID);
		fx.process.clear();
		device->EventEnd(threadID);
	}


	wiProfiler::EndRange(threadID); // Post Processing 1
}
void RenderPath3D::RenderColorGradedComposition()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;
	fx.quality = QUALITY_LINEAR;

	if (getStereogramEnabled())
	{
		device->EventBegin("Stereogram", GRAPHICSTHREAD_IMMEDIATE);
		fx.disableFullScreen();
		fx.process.clear();
		fx.process.setStereogram();
		wiImage::Draw(wiTextureHelper::getRandom64x64(), fx, GRAPHICSTHREAD_IMMEDIATE);
		device->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
		return;
	}

	if (getColorGradingEnabled())
	{
		device->EventBegin("Color Graded Composition", GRAPHICSTHREAD_IMMEDIATE);
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
		device->EventBegin("Composition", GRAPHICSTHREAD_IMMEDIATE);
		fx.enableFullScreen();
	}

	if (getSharpenFilterEnabled())
	{
		wiImage::Draw(&rtFinal[0], fx, GRAPHICSTHREAD_IMMEDIATE);
	}
	else
	{
		wiImage::Draw(&rtFinal[1], fx, GRAPHICSTHREAD_IMMEDIATE);
	}
	device->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
}
