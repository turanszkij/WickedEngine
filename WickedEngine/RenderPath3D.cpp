#include "RenderPath3D.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiScene.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;

void RenderPath3D::ResizeBuffers()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();


	// Render targets:
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
		desc.MipLevels = 5;
		device->CreateTexture(&desc, nullptr, &rtSSR);
		device->SetName(&rtSSR, "rtSSR");

		for (uint32_t i = 0; i < rtSSR.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtSSR, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSSR, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleCount = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtParticleDistortion);
		device->SetName(&rtParticleDistortion, "rtParticleDistortion");
		if (getMSAASampleCount() > 1)
		{
			desc.SampleCount = 1;
			device->CreateTexture(&desc, nullptr, &rtParticleDistortion_Resolved);
			device->SetName(&rtParticleDistortion_Resolved, "rtParticleDistortion_Resolved");
		}
	}
	{
		TextureDesc desc;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		device->CreateTexture(&desc, nullptr, &rtVolumetricLights);
		device->SetName(&rtVolumetricLights, "rtVolumetricLights");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R8G8B8A8_SNORM;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtWaterRipple);
		device->SetName(&rtWaterRipple, "rtWaterRipple");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.MipLevels = 8;
		device->CreateTexture(&desc, nullptr, &rtSceneCopy);
		device->SetName(&rtSceneCopy, "rtSceneCopy");

		for (uint32_t i = 0; i < rtSceneCopy.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtSceneCopy, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &rtReflection);
		device->SetName(&rtReflection, "rtReflection");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R8_UNORM;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
		device->CreateTexture(&desc, nullptr, &rtSSAO[0]);
		device->SetName(&rtSSAO[0], "rtSSAO[0]");
		device->CreateTexture(&desc, nullptr, &rtSSAO[1]);
		device->SetName(&rtSSAO[1], "rtSSAO[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleCount = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtSun[0]);
		device->SetName(&rtSun[0], "rtSun[0]");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.SampleCount = 1;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
		device->CreateTexture(&desc, nullptr, &rtSun[1]);
		device->SetName(&rtSun[1], "rtSun[1]");

		if (getMSAASampleCount() > 1)
		{
			desc.Width = wiRenderer::GetInternalResolution().x;
			desc.Height = wiRenderer::GetInternalResolution().y;
			desc.SampleCount = 1;
			device->CreateTexture(&desc, nullptr, &rtSun_resolved);
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
		device->CreateTexture(&desc, nullptr, &rtBloom);
		device->SetName(&rtBloom, "rtBloom");

		for (uint32_t i = 0; i < rtBloom.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtBloom, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtBloom, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtTemporalAA[0]);
		device->SetName(&rtTemporalAA[0], "rtTemporalAA[0]");
		device->CreateTexture(&desc, nullptr, &rtTemporalAA[1]);
		device->SetName(&rtTemporalAA[1], "rtTemporalAA[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtPostprocess_HDR);
		device->SetName(&rtPostprocess_HDR, "rtPostprocess_HDR");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtPostprocess_LDR[0]);
		device->SetName(&rtPostprocess_LDR[0], "rtPostprocess_LDR[0]");
		device->CreateTexture(&desc, nullptr, &rtPostprocess_LDR[1]);
		device->SetName(&rtPostprocess_LDR[1], "rtPostprocess_LDR[1]");
	}

	// Depth buffers:
	{
		TextureDesc desc;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;

		desc.Format = FORMAT_R32G8X24_TYPELESS;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.SampleCount = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &depthBuffer);
		device->SetName(&depthBuffer, "depthBuffer");

		if (getMSAASampleCount() > 1)
		{
			desc.Format = FORMAT_R32_FLOAT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		}
		else
		{
			desc.Format = FORMAT_R32G8X24_TYPELESS;
		}
		desc.SampleCount = 1;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy);
		device->SetName(&depthBuffer_Copy, "depthBuffer_Copy");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.Format = FORMAT_D16_UNORM;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		desc.layout = IMAGE_LAYOUT_DEPTHSTENCIL;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Reflection);
		device->SetName(&depthBuffer_Reflection, "depthBuffer_Reflection");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R16_UNORM;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtLinearDepth);
		device->SetName(&rtLinearDepth, "rtLinearDepth");

		desc.Width = (desc.Width + 1) / 2;
		desc.Height = (desc.Height + 1) / 2;
		desc.MipLevels = 5;
		desc.Format = FORMAT_R16G16_UNORM;
		device->CreateTexture(&desc, nullptr, &rtLinearDepth_minmax);
		device->SetName(&rtLinearDepth_minmax, "rtLinearDepth_minmax");

		for (uint32_t i = 0; i < desc.MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtLinearDepth_minmax, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtLinearDepth_minmax, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.Format = FORMAT_D16_UNORM;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		desc.layout = IMAGE_LAYOUT_DEPTHSTENCIL_READONLY;
		device->CreateTexture(&desc, nullptr, &smallDepth);
		device->SetName(&smallDepth, "smallDepth");
	}

	// Render passes:
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&smallDepth,-1,RenderPassAttachment::STOREOP_DONTCARE,IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,IMAGE_LAYOUT_DEPTHSTENCIL };

		device->CreateRenderPass(&desc, &renderpass_occlusionculling);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_DONTCARE,&rtReflection,-1,RenderPassAttachment::STOREOP_STORE,IMAGE_LAYOUT_RENDERTARGET,IMAGE_LAYOUT_SHADER_RESOURCE };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_CLEAR,&depthBuffer_Reflection,-1,RenderPassAttachment::STOREOP_DONTCARE,IMAGE_LAYOUT_DEPTHSTENCIL,IMAGE_LAYOUT_DEPTHSTENCIL };

		device->CreateRenderPass(&desc, &renderpass_reflection);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_DONTCARE,&smallDepth,-1,RenderPassAttachment::STOREOP_STORE,IMAGE_LAYOUT_DEPTHSTENCIL,IMAGE_LAYOUT_DEPTHSTENCIL_READONLY };

		device->CreateRenderPass(&desc, &renderpass_downsampledepthbuffer);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtSun[0],-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_lightshafts);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtVolumetricLights,-1 };

		device->CreateRenderPass(&desc, &renderpass_volumetriclight);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtParticleDistortion,-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1,RenderPassAttachment::STOREOP_DONTCARE };

		device->CreateRenderPass(&desc, &renderpass_particledistortion);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtWaterRipple,-1 };

		device->CreateRenderPass(&desc, &renderpass_waterripples);
	}

	RenderPath2D::ResizeBuffers();
}

void RenderPath3D::Update(float dt)
{
	RenderPath2D::Update(dt);

	wiRenderer::UpdatePerFrameData(dt, getLayerMask());
}

void RenderPath3D::Compose(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx;
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_LINEAR;
	fx.enableFullScreen();

	device->EventBegin("Composition", cmd);
	wiImage::Draw(GetLastPostprocessRT(), fx, cmd);
	device->EventEnd(cmd);

	if (wiRenderer::GetDebugLightCulling())
	{
		wiImage::Draw((Texture*)wiRenderer::GetTexture(TEXTYPE_2D_DEBUGUAV), wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()), cmd);
	}

	RenderPath2D::Compose(cmd);
}

void RenderPath3D::RenderFrameSetUp(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->BindResource(CS, &depthBuffer_Copy, TEXSLOT_DEPTH, cmd);
	wiRenderer::UpdateRenderData(cmd);
	
	Viewport viewport;
	viewport.Width = (float)smallDepth.GetDesc().Width;
	viewport.Height = (float)smallDepth.GetDesc().Height;
	device->BindViewports(1, &viewport, cmd);

	device->RenderPassBegin(&renderpass_occlusionculling, cmd);

	wiRenderer::OcclusionCulling_Render(cmd);

	device->RenderPassEnd(cmd);
}
void RenderPath3D::RenderReflections(CommandList cmd) const
{
	auto range = wiProfiler::BeginRangeGPU("Reflection rendering", cmd);

	if (wiRenderer::IsRequestedReflectionRendering())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::UpdateCameraCB(wiRenderer::GetRefCamera(), cmd);

		Viewport vp;
		vp.Width = (float)depthBuffer_Reflection.GetDesc().Width;
		vp.Height = (float)depthBuffer_Reflection.GetDesc().Height;
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

		device->Barrier(&GPUBarrier::Image(&rtReflection, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_RENDERTARGET), 1, cmd);

		device->RenderPassBegin(&renderpass_reflection, cmd);

		wiRenderer::DrawScene(wiRenderer::GetRefCamera(), false, cmd, RENDERPASS_TEXTURE, false, false);
		wiRenderer::DrawSky(cmd);

		wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), cmd);

		device->RenderPassEnd(cmd);
	}

	wiProfiler::EndRange(range); // Reflection Rendering
}
void RenderPath3D::RenderShadows(CommandList cmd) const
{
	if (getShadowsEnabled())
	{
		wiRenderer::DrawShadowmaps(wiRenderer::GetCamera(), cmd, getLayerMask());
	}

	wiRenderer::VoxelRadiance(cmd);
}

void RenderPath3D::RenderLinearDepth(CommandList cmd) const
{
	wiRenderer::Postprocess_Lineardepth(depthBuffer_Copy, rtLinearDepth, rtLinearDepth_minmax, cmd);
}
void RenderPath3D::RenderSSAO(CommandList cmd) const
{
	if (getSSAOEnabled())
	{
		wiRenderer::Postprocess_SSAO(
			depthBuffer_Copy, 
			rtLinearDepth,
			rtLinearDepth_minmax,
			rtSSAO[1],
			rtSSAO[0],
			cmd,
			getSSAORange(),
			getSSAOSampleCount(),
			getSSAOBlur(),
			getSSAOPower()
		);
	}
}
void RenderPath3D::RenderSSR(const Texture& srcSceneRT, const wiGraphics::Texture& gbuffer1, CommandList cmd) const
{
	if (getSSREnabled())
	{
		wiRenderer::Postprocess_SSR(srcSceneRT, depthBuffer_Copy, rtLinearDepth_minmax, gbuffer1, rtSSR, cmd);
	}
}
void RenderPath3D::DownsampleDepthBuffer(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	Viewport viewport;
	viewport.Width = (float)smallDepth.GetDesc().Width;
	viewport.Height = (float)smallDepth.GetDesc().Height;
	device->BindViewports(1, &viewport, cmd);

	device->RenderPassBegin(&renderpass_downsampledepthbuffer, cmd);

	wiRenderer::DownsampleDepthBuffer(depthBuffer_Copy, cmd);

	device->RenderPassEnd(cmd);
}
void RenderPath3D::RenderOutline(const Texture& dstSceneRT, CommandList cmd) const
{
	if (getOutlineEnabled())
	{
		wiRenderer::Postprocess_Outline(rtLinearDepth, dstSceneRT, cmd, getOutlineThreshold(), getOutlineThickness(), getOutlineColor());
	}
}
void RenderPath3D::RenderLightShafts(CommandList cmd) const
{
	XMVECTOR sunDirection = XMLoadFloat3(&wiScene::GetScene().weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, wiRenderer::GetCamera().GetAt())) > 0)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("Light Shafts", cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

		// Render sun stencil cutout:
		{
			device->RenderPassBegin(&renderpass_lightshafts, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawSun(cmd);

			device->RenderPassEnd(cmd);
		}

		const Texture* sunSource = &rtSun[0];
		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(&rtSun_resolved, sunSource, cmd);
			sunSource = &rtSun_resolved;
		}

		// Radial blur on the sun:
		{
			XMVECTOR sunPos = XMVector3Project(sunDirection * 100000, 0, 0,
				1.0f, 1.0f, 0.1f, 1.0f,
				wiRenderer::GetCamera().GetProjection(), wiRenderer::GetCamera().GetView(), XMMatrixIdentity());
			{
				XMFLOAT2 sun;
				XMStoreFloat2(&sun, sunPos);
				wiRenderer::Postprocess_LightShafts(*sunSource, rtSun[1], cmd, sun);
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

		device->RenderPassBegin(&renderpass_volumetriclight, cmd);

		Viewport vp;
		vp.Width = (float)rtVolumetricLights.GetDesc().Width;
		vp.Height = (float)rtVolumetricLights.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawVolumeLights(wiRenderer::GetCamera(), depthBuffer_Copy, cmd);

		device->RenderPassEnd(cmd);
	}
}
void RenderPath3D::RenderRefractionSource(const Texture& srcSceneRT, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Refraction Source", cmd);

	wiRenderer::CopyTexture2D(rtSceneCopy, 0, 0, 0, srcSceneRT, 0, cmd);

	if (wiRenderer::GetAdvancedRefractionsEnabled())
	{
		wiRenderer::GenerateMipChain(rtSceneCopy, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd);
	}
	device->EventEnd(cmd);
}
void RenderPath3D::RenderTransparents(const RenderPass& renderpass_transparent, RENDERPASS renderPass, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	// Water ripple rendering:
	{
		// todo: refactor water ripples and avoid clear if there is none!
		device->RenderPassBegin(&renderpass_waterripples, cmd);

		Viewport vp;
		vp.Width = (float)rtWaterRipple.GetDesc().Width;
		vp.Height = (float)rtWaterRipple.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawWaterRipples(cmd);

		device->RenderPassEnd(cmd);
	}

	device->UnbindResources(TEXSLOT_GBUFFER0, 1, cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

	device->RenderPassBegin(&renderpass_transparent, cmd);

	Viewport vp;
	vp.Width = (float)renderpass_transparent.desc.attachments[0].texture->GetDesc().Width;
	vp.Height = (float)renderpass_transparent.desc.attachments[0].texture->GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	// Transparent scene
	{
		auto range = wiProfiler::BeginRangeGPU("Transparent Scene", cmd);

		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
		device->BindResource(PS, &rtSceneCopy, TEXSLOT_RENDERPATH_REFRACTION, cmd);
		device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERPATH_WATERRIPPLES, cmd);
		wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), rtLinearDepth, renderPass, cmd, true, true);

		wiProfiler::EndRange(range); // Transparent Scene
	}

	wiRenderer::DrawLightVisualizers(wiRenderer::GetCamera(), cmd);

	wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), rtLinearDepth, false, cmd);

	wiImageParams fx;
	fx.enableFullScreen();

	if (getVolumeLightsEnabled())
	{
		device->EventBegin("Contribute Volumetric Lights", cmd);
		wiRenderer::Postprocess_Upsample_Bilateral(rtVolumetricLights, rtLinearDepth, rtLinearDepth_minmax, 
			*renderpass_transparent.desc.attachments[0].texture, cmd, true, 1.5f);
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
		wiRenderer::DrawLensFlares(wiRenderer::GetCamera(), depthBuffer_Copy, cmd);
	}

	wiRenderer::DrawDebugWorld(wiRenderer::GetCamera(), cmd);

	device->RenderPassEnd(cmd);

	// Distortion particles:
	{
		device->RenderPassBegin(&renderpass_particledistortion, cmd);

		Viewport vp;
		vp.Width = (float)rtParticleDistortion.GetDesc().Width;
		vp.Height = (float)rtParticleDistortion.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), rtLinearDepth, true, cmd);

		device->RenderPassEnd(cmd);

		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(&rtParticleDistortion_Resolved, &rtParticleDistortion, cmd);
		}
	}
}
void RenderPath3D::RenderPostprocessChain(const Texture& srcSceneRT, const Texture& srcGbuffer1, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture* rt_first = nullptr; // not ping-ponged with read / write
	const Texture* rt_read = &srcSceneRT;
	const Texture* rt_write = &rtPostprocess_HDR;

	// 1.) HDR post process chain
	{
		if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
		{
			GraphicsDevice* device = wiRenderer::GetDevice();

			int output = device->GetFrameCount() % 2;
			int history = 1 - output;
			wiRenderer::Postprocess_TemporalAA(*rt_read, rtTemporalAA[history], srcGbuffer1, rtLinearDepth, rtTemporalAA[output], cmd);
			rt_first = &rtTemporalAA[output];
		}

		if (getDepthOfFieldEnabled())
		{
			wiRenderer::Postprocess_DepthOfField(rt_first == nullptr ? *rt_read : *rt_first, *rt_write, rtLinearDepth, rtLinearDepth_minmax, cmd, getDepthOfFieldFocus(), getDepthOfFieldStrength(), getDepthOfFieldAspect());
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getMotionBlurEnabled())
		{
			wiRenderer::Postprocess_MotionBlur(rt_first == nullptr ? *rt_read : *rt_first, srcGbuffer1, rtLinearDepth, *rt_write, cmd, getMotionBlurStrength());
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getBloomEnabled())
		{
			wiRenderer::Postprocess_Bloom(rt_first == nullptr ? *rt_read : *rt_first, rtBloom, *rt_write, cmd, getBloomThreshold());
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}
	}

	// 2.) Tone mapping HDR -> LDR
	{
		rt_write = &rtPostprocess_LDR[0];

		wiRenderer::Postprocess_Tonemap(
			*rt_read,
			getEyeAdaptionEnabled() ? *wiRenderer::ComputeLuminance(srcSceneRT, cmd) : *wiTextureHelper::getColor(wiColor::Gray()),
			getMSAASampleCount() > 1 ? rtParticleDistortion_Resolved : rtParticleDistortion,
			*rt_write,
			cmd,
			getExposure()
		);

		rt_read = rt_write;
		rt_write = &rtPostprocess_LDR[1];
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
	}

	// 3.) LDR post process chain
	{
		if (getSharpenFilterEnabled())
		{
			wiRenderer::Postprocess_Sharpen(*rt_read, *rt_write, cmd, getSharpenFilterAmount());

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getColorGradingEnabled())
		{
			wiRenderer::Postprocess_Colorgrade(
				*rt_read, 
				colorGradingTex != nullptr ? *colorGradingTex->texture : *wiTextureHelper::getColorGradeDefault(), 
				*rt_write, 
				cmd
			);

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getFXAAEnabled())
		{
			wiRenderer::Postprocess_FXAA(*rt_read, *rt_write, cmd);

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getChromaticAberrationEnabled())
		{
			wiRenderer::Postprocess_Chromatic_Aberration(*rt_read, *rt_write, cmd, getChromaticAberrationAmount());

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}
	}
}
