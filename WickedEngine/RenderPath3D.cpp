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
		device->SetName(&rtSSR, "rtSSR");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*getParticleDownSample());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*getParticleDownSample());
		device->CreateTexture2D(&desc, nullptr, &rtParticle);
		device->SetName(&rtParticle, "rtParticle");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x * 0.25f);
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y * 0.25f);
		device->CreateTexture2D(&desc, nullptr, &rtVolumetricLights);
		device->SetName(&rtVolumetricLights, "rtVolumetricLights");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_waterripple;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtWaterRipple);
		device->SetName(&rtWaterRipple, "rtWaterRipple");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.MipLevels = 8;
		rtSceneCopy.RequestIndependentShaderResourcesForMIPs(true);
		rtSceneCopy.RequestIndependentUnorderedAccessResourcesForMIPs(true);
		device->CreateTexture2D(&desc, nullptr, &rtSceneCopy);
		device->SetName(&rtSceneCopy, "rtSceneCopy");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x * getReflectionQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y * getReflectionQuality());
		device->CreateTexture2D(&desc, nullptr, &rtReflection);
		device->SetName(&rtReflection, "rtReflection");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x / 2);
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y / 2);
		device->CreateTexture2D(&desc, nullptr, &rtDof[0]);
		device->SetName(&rtDof[0], "rtDof[0]");
		device->CreateTexture2D(&desc, nullptr, &rtDof[1]);
		device->SetName(&rtDof[1], "rtDof[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_ssao;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*getSSAOQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*getSSAOQuality());
		device->CreateTexture2D(&desc, nullptr, &rtSSAO[0]);
		device->SetName(&rtSSAO[0], "rtSSAO[0]");
		device->CreateTexture2D(&desc, nullptr, &rtSSAO[1]);
		device->SetName(&rtSSAO[1], "rtSSAO[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleDesc.Count = getMSAASampleCount();
		device->CreateTexture2D(&desc, nullptr, &rtSun[0]);
		device->SetName(&rtSun[0], "rtSun[0]");

		desc.SampleDesc.Count = 1;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x*getLightShaftQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y*getLightShaftQuality());
		device->CreateTexture2D(&desc, nullptr, &rtSun[1]);
		device->SetName(&rtSun[1], "rtSun[1]");

		if (getMSAASampleCount() > 1)
		{
			desc.Width = wiRenderer::GetInternalResolution().x;
			desc.Height = wiRenderer::GetInternalResolution().y;
			desc.SampleDesc.Count = 1;
			device->CreateTexture2D(&desc, nullptr, &rtSun_resolved);
			device->SetName(&rtSun_resolved, "rtSun_resolved");
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
		device->SetName(&rtBloom, "rtBloom");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtTemporalAA[0]);
		device->SetName(&rtTemporalAA[0], "rtTemporalAA[0]");
		device->CreateTexture2D(&desc, nullptr, &rtTemporalAA[1]);
		device->SetName(&rtTemporalAA[1], "rtTemporalAA[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtPostprocess_HDR);
		device->SetName(&rtPostprocess_HDR, "rtPostprocess_HDR");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtPostprocess_LDR[0]);
		device->SetName(&rtPostprocess_LDR[0], "rtPostprocess_LDR[0]");
		device->CreateTexture2D(&desc, nullptr, &rtPostprocess_LDR[1]);
		device->SetName(&rtPostprocess_LDR[1], "rtPostprocess_LDR[1]");
	}

	// Depth buffers:
	{
		TextureDesc desc;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;

		desc.Format = wiRenderer::DSFormat_full_alias;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.SampleDesc.Count = getMSAASampleCount();
		device->CreateTexture2D(&desc, nullptr, &depthBuffer);
		device->SetName(&depthBuffer, "depthBuffer");

		if (getMSAASampleCount() > 1)
		{
			desc.Format = FORMAT_R32_FLOAT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		}
		else
		{
			desc.Format = wiRenderer::DSFormat_full_alias;
		}
		desc.SampleDesc.Count = 1;
		device->CreateTexture2D(&desc, nullptr, &depthBuffer_Copy);
		device->SetName(&depthBuffer_Copy, "depthBuffer_Copy");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_lineardepth;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtLinearDepth);
		device->SetName(&rtLinearDepth, "rtLinearDepth");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.Format = wiRenderer::DSFormat_small;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		device->CreateTexture2D(&desc, nullptr, &smallDepth);
		device->SetName(&smallDepth, "smallDepth");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.Format = wiRenderer::DSFormat_small;
		desc.Width = (UINT)(wiRenderer::GetInternalResolution().x * getReflectionQuality());
		desc.Height = (UINT)(wiRenderer::GetInternalResolution().y * getReflectionQuality());
		device->CreateTexture2D(&desc, nullptr, &depthBuffer_reflection);
		device->SetName(&depthBuffer_reflection, "depthBuffer_reflection");
	}
}

void RenderPath3D::Update(float dt)
{
	RenderPath2D::Update(dt);

	wiRenderer::UpdatePerFrameData(dt, getLayerMask());
}

void RenderPath3D::Compose(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_LINEAR;
	fx.enableFullScreen();

	device->EventBegin("Composition", cmd);
	wiImage::Draw(GetLastPostprocessRT(), fx, cmd);
	device->EventEnd(cmd);

	if (wiRenderer::GetDebugLightCulling())
	{
		wiImage::Draw((Texture2D*)wiRenderer::GetTexture(TEXTYPE_2D_DEBUGUAV), wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()), cmd);
	}

	RenderPath2D::Compose(cmd);
}

void RenderPath3D::RenderFrameSetUp(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->BindResource(CS, &depthBuffer_Copy, TEXSLOT_DEPTH, cmd);
	wiRenderer::UpdateRenderData(cmd);
	
	ViewPort viewPort;
	viewPort.Width = (float)smallDepth.GetDesc().Width;
	viewPort.Height = (float)smallDepth.GetDesc().Height;
	device->BindViewports(1, &viewPort, cmd);
	device->BindRenderTargets(0, nullptr, &smallDepth, cmd);

	const GPUResource* dsv[] = { &smallDepth };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_DEPTH_READ, cmd);

	wiRenderer::OcclusionCulling_Render(cmd);
}
void RenderPath3D::RenderReflections(CommandList cmd) const
{
	auto range = wiProfiler::BeginRangeGPU("Reflection rendering", cmd);

	if (wiRenderer::IsRequestedReflectionRendering())
	{
		wiRenderer::UpdateCameraCB(wiRenderer::GetRefCamera(), cmd);

		{
			GraphicsDevice* device = wiRenderer::GetDevice();

			const Texture2D* rts[] = { &rtReflection };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer_reflection, cmd);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[0], clear, cmd);
			device->ClearDepthStencil(&depthBuffer_reflection, CLEAR_DEPTH, 0, 0, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			// reverse clipping if underwater
			XMFLOAT4 water = wiRenderer::GetWaterPlane();
			float d = XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&water), wiRenderer::GetCamera().GetEye()));
			if (d < 0)
			{
				water.x *= -1;
				water.y *= -1;
				water.z *= -1;
			}

			wiRenderer::SetClipPlane(water, cmd);

			wiRenderer::DrawScene(wiRenderer::GetRefCamera(), false, cmd, RENDERPASS_TEXTURE, getHairParticlesReflectionEnabled(), false);
			wiRenderer::DrawSky(cmd);

			wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), cmd);
		}
	}

	wiProfiler::EndRange(range); // Reflection Rendering
}
void RenderPath3D::RenderShadows(CommandList cmd) const
{
	if (getShadowsEnabled())
	{
		wiRenderer::DrawForShadowMap(wiRenderer::GetCamera(), cmd, getLayerMask());
	}

	wiRenderer::VoxelRadiance(cmd);
}

void RenderPath3D::RenderLinearDepth(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	const Texture2D* rts[] = { &rtLinearDepth };
	device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

	ViewPort vp;
	vp.Width = (float)rts[0]->GetDesc().Width;
	vp.Height = (float)rts[0]->GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.sampleFlag = SAMPLEMODE_CLAMP;
	fx.quality = QUALITY_NEAREST;
	fx.process.setLinDepth();
	wiImage::Draw(&depthBuffer_Copy, fx, cmd);
	fx.process.clear();

	device->BindRenderTargets(0, nullptr, nullptr, cmd);
}
void RenderPath3D::RenderSSAO(CommandList cmd) const
{
	if (getSSAOEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

		device->UnbindResources(TEXSLOT_RENDERPATH_SSAO, 1, cmd);
		device->EventBegin("SSAO", cmd);
		{
			const Texture2D* rts[] = { &rtSSAO[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.setSSAO(getSSAORange(), getSSAOSampleCount());
			fx.setMaskMap(wiTextureHelper::getRandom64x64());
			fx.quality = QUALITY_LINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, cmd);
			fx.process.clear();
		}
		{
			const Texture2D* rts[] = { &rtSSAO[1] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.setBlur(XMFLOAT2(getSSAOBlur(), 0));
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(&rtSSAO[0], fx, cmd);
		}
		{
			const Texture2D* rts[] = { &rtSSAO[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.setBlur(XMFLOAT2(0, getSSAOBlur()));
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(&rtSSAO[1], fx, cmd);
			fx.process.clear();
		}
		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderSSR(const Texture2D& srcSceneRT, CommandList cmd) const
{
	if (getSSREnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

		device->UnbindResources(TEXSLOT_RENDERPATH_SSR, 1, cmd);
		device->EventBegin("SSR", cmd);
		{
			const Texture2D* rts[] = { &rtSSR };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.clear();
			fx.disableFullScreen();
			fx.process.setSSR();
			fx.setMaskMap(nullptr);
			wiImage::Draw(&srcSceneRT, fx, cmd);
			fx.process.clear();
		}
		device->EventEnd(cmd);
	}
}
void RenderPath3D::DownsampleDepthBuffer(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.enableHDR();

	device->EventBegin("Downsample Depth Buffer", cmd);
	{
		// Downsample the depth buffer for the occlusion culling phase...
		ViewPort viewPort;
		viewPort.Width = (float)smallDepth.GetDesc().Width;
		viewPort.Height = (float)smallDepth.GetDesc().Height;
		device->BindViewports(1, &viewPort, cmd);
		device->BindRenderTargets(0, nullptr, &smallDepth, cmd);
		// This depth buffer is not cleared because we don't have to (we overwrite it anyway because depthfunc is ALWAYS)

		const GPUResource* dsv[] = { &smallDepth };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, cmd);

		fx.process.setDepthBufferDownsampling();
		wiImage::Draw(&depthBuffer_Copy, fx, cmd);
		fx.process.clear();
	}
	device->EventEnd(cmd);
}
void RenderPath3D::RenderOutline(const Texture2D& dstSceneRT, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.enableHDR();

	if (getOutlineEnabled())
	{
		device->EventBegin("Outline", cmd);

		const Texture2D* rts[] = { &dstSceneRT };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		fx.process.setOutline(getOutlineThreshold(), getOutlineThickness(), getOutlineColor());
		wiImage::Draw(nullptr, fx, cmd);
		fx.process.clear();
		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderLightShafts(CommandList cmd) const
{
	XMVECTOR sunDirection = XMLoadFloat3(&wiSceneSystem::GetScene().weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, wiRenderer::GetCamera().GetAt())) > 0)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
		fx.enableHDR();

		device->EventBegin("Light Shafts", cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

		// Render sun stencil cutout:
		{
			const Texture2D* rts[] = { &rtSun[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, cmd);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[0], clear, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawSun(cmd);
		}

		const Texture2D* sunSource = &rtSun[0];
		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(&rtSun_resolved, sunSource, cmd);
			sunSource = &rtSun_resolved;
		}

		// Radial blur on the sun:
		{
			const Texture2D* rts[] = { &rtSun[1] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[0], clear, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiImageParams fxs = fx;
			fxs.blendFlag = BLENDMODE_OPAQUE;
			XMVECTOR sunPos = XMVector3Project(sunDirection * 100000, 0, 0,
				(float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 1.0f,
				wiRenderer::GetCamera().GetProjection(), wiRenderer::GetCamera().GetView(), XMMatrixIdentity());
			{
				XMFLOAT2 sun;
				XMStoreFloat2(&sun, sunPos);
				fxs.process.setLightShaftCenter(sun);
				wiImage::Draw(sunSource, fxs, cmd);
			}
		}
		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderVolumetrics(CommandList cmd) const
{
	if (getVolumeLightsEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		const Texture2D* rts[] = { &rtVolumetricLights };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, cmd);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawVolumeLights(wiRenderer::GetCamera(), cmd);
	}
}
void RenderPath3D::RenderParticles(bool isDistrortionPass, CommandList cmd) const
{
	if (getEmittedParticlesEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		const Texture2D* rts[] = { &rtParticle };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, cmd);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), isDistrortionPass, cmd);
	}
}
void RenderPath3D::RenderRefractionSource(const Texture2D& srcSceneRT, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.enableHDR();

	device->EventBegin("Refraction Source", cmd);

	const Texture2D* rts[] = { &rtSceneCopy };
	device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

	ViewPort vp;
	vp.Width = (float)rts[0]->GetDesc().Width;
	vp.Height = (float)rts[0]->GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_NEAREST;
	fx.enableFullScreen();
	wiImage::Draw(&srcSceneRT, fx, cmd);

	if (wiRenderer::GetAdvancedRefractionsEnabled())
	{
		wiRenderer::GenerateMipChain(&rtSceneCopy, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd);
	}
	device->EventEnd(cmd);
}
void RenderPath3D::RenderTransparents(const Texture2D& dstSceneRT, RENDERPASS renderPass, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	// Water ripple rendering:
	{
		// todo: refactor water ripples and avoid clear if there is none!
		const Texture2D* rts[] = { &rtWaterRipple };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, cmd);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawWaterRipples(cmd);
	}

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
	fx.enableHDR();

	device->UnbindResources(TEXSLOT_GBUFFER0, 1, cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

	const Texture2D* rts[] = { &dstSceneRT };
	device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, cmd);

	ViewPort vp;
	vp.Width = (float)rts[0]->GetDesc().Width;
	vp.Height = (float)rts[0]->GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	// Transparent scene
	{
		auto range = wiProfiler::BeginRangeGPU("Transparent Scene", cmd);

		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
		device->BindResource(PS, &rtSceneCopy, TEXSLOT_RENDERPATH_REFRACTION, cmd);
		device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERPATH_WATERRIPPLES, cmd);
		wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), renderPass, cmd, getHairParticlesEnabled(), true);

		wiProfiler::EndRange(range); // Transparent Scene
	}

	wiRenderer::DrawLightVisualizers(wiRenderer::GetCamera(), cmd);

	fx.enableFullScreen();

	if (getEmittedParticlesEnabled())
	{
		device->EventBegin("Contribute Emitters", cmd);
		fx.blendFlag = BLENDMODE_PREMULTIPLIED;
		wiImage::Draw(&rtParticle, fx, cmd);
		device->EventEnd(cmd);
	}

	if (getVolumeLightsEnabled())
	{
		device->EventBegin("Contribute Volumetric Lights", cmd);
		wiImage::Draw(&rtVolumetricLights, fx, cmd);
		device->EventEnd(cmd);
	}

	if (getLightShaftsEnabled())
	{
		device->EventBegin("Contribute LightShafts", cmd);
		fx.blendFlag = BLENDMODE_ADDITIVE;
		wiImage::Draw(&rtSun[1], fx, cmd);
		device->EventEnd(cmd);
	}

	if (getLensFlareEnabled())
	{
		wiRenderer::DrawLensFlares(wiRenderer::GetCamera(), cmd);
	}

	wiRenderer::DrawDebugWorld(wiRenderer::GetCamera(), cmd);

	device->BindRenderTargets(0, nullptr, nullptr, cmd);



	RenderParticles(true, cmd);
}
void RenderPath3D::TemporalAAResolve(const Texture2D& srcdstSceneRT, const Texture2D& srcGbuffer1, CommandList cmd) const
{
	if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

		wiRenderer::BindGBufferTextures(nullptr, &srcGbuffer1, nullptr, cmd);

		device->EventBegin("Temporal AA Resolve", cmd);
		auto range = wiProfiler::BeginRangeGPU("Temporal AA Resolve", cmd);
		fx.enableHDR();
		fx.blendFlag = BLENDMODE_OPAQUE;
		int current = device->GetFrameCount() % 2;
		int history = 1 - current;
		{
			const Texture2D* rts[] = { &rtTemporalAA[current] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.disableFullScreen();
			fx.process.setTemporalAAResolve();
			fx.setMaskMap(&rtTemporalAA[history]);
			wiImage::Draw(&srcdstSceneRT, fx, cmd);
			fx.process.clear();
		}
		device->UnbindResources(TEXSLOT_GBUFFER0, 1, cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		{
			const Texture2D* rts[] = { &srcdstSceneRT };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.enableFullScreen();
			fx.quality = QUALITY_NEAREST;
			wiImage::Draw(&rtTemporalAA[current], fx, cmd);
			fx.disableFullScreen();
		}
		fx.disableHDR();
		wiProfiler::EndRange(range);
		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderBloom(const Texture2D& srcdstSceneRT, CommandList cmd) const
{
	if (getBloomEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

		device->EventBegin("Bloom", cmd);
		fx.setMaskMap(nullptr);
		fx.process.clear();
		fx.disableFullScreen();
		fx.quality = QUALITY_LINEAR;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		// separate bright parts
		{
			const Texture2D* rts[] = { &rtBloom };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.setBloom(getBloomThreshold());
			wiImage::Draw(&srcdstSceneRT, fx, cmd);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}
		fx.process.clear();

		wiRenderer::GenerateMipChain(&rtBloom, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd);
		// add to the scene
		{
			fx.enableHDR();

			const Texture2D* rts[] = { &srcdstSceneRT };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			// not full screen effect, because we draw specific mip levels, so some setup is required
			XMFLOAT2 siz = fx.siz;
			fx.siz = XMFLOAT2((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
			fx.blendFlag = BLENDMODE_ADDITIVE;
			//fx.quality = QUALITY_BICUBIC;
			fx.process.clear();
			fx.mipLevel = 1.5f;
			wiImage::Draw(&rtBloom, fx, cmd);
			fx.mipLevel = 3.5f;
			wiImage::Draw(&rtBloom, fx, cmd);
			fx.mipLevel = 5.5f;
			wiImage::Draw(&rtBloom, fx, cmd);
			fx.quality = QUALITY_LINEAR;
			fx.siz = siz;
		}

		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderPostprocessChain(const Texture2D& srcSceneRT, const Texture2D& srcGbuffer1, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	const Texture2D* rt_read = &srcSceneRT;
	const Texture2D* rt_write = &rtPostprocess_HDR;

	// 1.) HDR post process chain
	{
		fx.enableHDR();

		if (getMotionBlurEnabled())
		{
			wiRenderer::BindGBufferTextures(nullptr, &srcGbuffer1, nullptr, cmd);

			device->EventBegin("Motion Blur", cmd);

			const Texture2D* rts[] = { rt_write };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.setMotionBlur();
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.disableFullScreen();
			wiImage::Draw(rt_read, fx, cmd);
			fx.process.clear();
			device->EventEnd(cmd);

			SwapPtr(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getDepthOfFieldEnabled())
		{
			device->EventBegin("Depth Of Field", cmd);
			// downsample + blur
			{
				const Texture2D* rts[] = { &rtDof[0] };
				device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

				ViewPort vp;
				vp.Width = (float)rts[0]->GetDesc().Width;
				vp.Height = (float)rts[0]->GetDesc().Height;
				device->BindViewports(1, &vp, cmd);

				fx.process.setBlur(XMFLOAT2(getDepthOfFieldStrength(), 0), fx.isHDREnabled());
				wiImage::Draw(rt_read, fx, cmd);

				device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
			}

			{
				const Texture2D* rts[] = { &rtDof[1] };
				device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

				ViewPort vp;
				vp.Width = (float)rts[0]->GetDesc().Width;
				vp.Height = (float)rts[0]->GetDesc().Height;
				device->BindViewports(1, &vp, cmd);

				fx.process.setBlur(XMFLOAT2(0, getDepthOfFieldStrength()), fx.isHDREnabled());
				wiImage::Draw(&rtDof[0], fx, cmd);
				fx.process.clear();

				device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
			}

			// depth of field compose pass
			{
				const Texture2D* rts[] = { rt_write };
				device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

				ViewPort vp;
				vp.Width = (float)rts[0]->GetDesc().Width;
				vp.Height = (float)rts[0]->GetDesc().Height;
				device->BindViewports(1, &vp, cmd);

				fx.process.setDOF(getDepthOfFieldFocus());
				fx.setMaskMap(&rtDof[1]);
				wiImage::Draw(rt_read, fx, cmd);
				fx.setMaskMap(nullptr);
				fx.process.clear();

				SwapPtr(rt_read, rt_write);
				device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
			}
			device->EventEnd(cmd);
		}

		fx.disableHDR();
	}

	// 2.) Tone mapping HDR -> LDR
	{
		rt_write = &rtPostprocess_LDR[0];

		device->EventBegin("Tone Mapping", cmd);
		fx.disableHDR();
		fx.blendFlag = BLENDMODE_OPAQUE;

		const Texture2D* rts[] = { rt_write };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		fx.process.setToneMap(getExposure());
		fx.setDistortionMap(&rtParticle);
		if (getEyeAdaptionEnabled())
		{
			fx.setMaskMap(wiRenderer::ComputeLuminance(&srcSceneRT, cmd));
		}
		else
		{
			fx.setMaskMap(wiTextureHelper::getColor(wiColor::Gray()));
		}

		wiImage::Draw(rt_read, fx, cmd);
		fx.process.clear();
		device->EventEnd(cmd);

		rt_read = rt_write;
		rt_write = &rtPostprocess_LDR[1];
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
	}

	// 3.) LDR post process chain
	{
		fx.disableHDR();

		if (getSharpenFilterEnabled())
		{
			device->EventBegin("Sharpen Filter", cmd);

			const Texture2D* rts[] = { rt_write };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.process.setSharpen(getSharpenFilterAmount());
			wiImage::Draw(rt_read, fx, cmd);
			fx.process.clear();
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);

			device->EventEnd(cmd);

			SwapPtr(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getColorGradingEnabled())
		{
			device->EventBegin("Color Grading", cmd);

			const Texture2D* rts[] = { rt_write };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			if (colorGradingTex != nullptr)
			{
				fx.process.setColorGrade();
				fx.setMaskMap(colorGradingTex);
			}
			else
			{
				fx.process.setColorGrade();
				fx.setMaskMap(wiTextureHelper::getColorGradeDefault());
			}
			wiImage::Draw(rt_read, fx, cmd);

			device->EventEnd(cmd);

			SwapPtr(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getFXAAEnabled())
		{
			device->EventBegin("FXAA", cmd);

			const Texture2D* rts[] = { rt_write };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			fx.process.setFXAA();
			wiImage::Draw(rt_read, fx, cmd);

			device->EventEnd(cmd);

			SwapPtr(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}
	}
}
