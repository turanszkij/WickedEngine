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
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
		desc.MipLevels = 5;
		device->CreateTexture2D(&desc, nullptr, &rtSSR);
		device->SetName(&rtSSR, "rtSSR");

		for (UINT i = 0; i < rtSSR.GetDesc().MipLevels; ++i)
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
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtParticle);
		device->SetName(&rtParticle, "rtParticle");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
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
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.MipLevels = 8;
		device->CreateTexture2D(&desc, nullptr, &rtSceneCopy);
		device->SetName(&rtSceneCopy, "rtSceneCopy");

		for (UINT i = 0; i < rtSceneCopy.GetDesc().MipLevels; ++i)
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
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtReflection);
		device->SetName(&rtReflection, "rtReflection");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
		device->CreateTexture2D(&desc, nullptr, &rtDof[0]);
		device->SetName(&rtDof[0], "rtDof[0]");
		device->CreateTexture2D(&desc, nullptr, &rtDof[1]);
		device->SetName(&rtDof[1], "rtDof[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_ssao;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
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

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.SampleDesc.Count = 1;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
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
		device->CreateTexture2D(&desc, nullptr, &rtBloom);
		device->SetName(&rtBloom, "rtBloom");

		for (UINT i = 0; i < rtBloom.GetDesc().MipLevels; ++i)
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
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtPostprocess_HDR);
		device->SetName(&rtPostprocess_HDR, "rtPostprocess_HDR");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
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
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
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

	// Render passes:
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&smallDepth,-1 };

		device->CreateRenderPass(&desc, &renderpass_occlusionculling);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_DONTCARE,&rtReflection,-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_CLEAR,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_reflection);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_DONTCARE,&smallDepth,-1 };

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
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtParticle,-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_particles);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtWaterRipple,-1 };

		device->CreateRenderPass(&desc, &renderpass_waterripples);
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

	wiImageParams fx;
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

	device->RenderPassBegin(&renderpass_occlusionculling, cmd);
	
	ViewPort viewPort;
	viewPort.Width = (float)smallDepth.GetDesc().Width;
	viewPort.Height = (float)smallDepth.GetDesc().Height;
	device->BindViewports(1, &viewPort, cmd);

	const GPUResource* dsv[] = { &smallDepth };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_DEPTH_READ, cmd);

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

		device->RenderPassBegin(&renderpass_reflection, cmd);

		ViewPort vp;
		vp.Width = (float)depthBuffer.GetDesc().Width;
		vp.Height = (float)depthBuffer.GetDesc().Height;
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
	wiRenderer::Postprocess_Lineardepth(depthBuffer_Copy, rtLinearDepth, cmd);
}
void RenderPath3D::RenderSSAO(CommandList cmd) const
{
	if (getSSAOEnabled())
	{
		wiRenderer::Postprocess_SSAO(
			depthBuffer_Copy, 
			rtLinearDepth, 
			rtSSAO[1],
			rtSSAO[0],
			cmd,
			getSSAORange(),
			getSSAOSampleCount(),
			getSSAOBlur()
		);
	}
}
void RenderPath3D::RenderSSR(const Texture2D& srcSceneRT, const wiGraphics::Texture2D& gbuffer1, CommandList cmd) const
{
	if (getSSREnabled())
	{
		wiRenderer::Postprocess_SSR(srcSceneRT, depthBuffer_Copy, rtLinearDepth, gbuffer1, rtSSR, cmd);
	}
}
void RenderPath3D::DownsampleDepthBuffer(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->RenderPassBegin(&renderpass_downsampledepthbuffer, cmd);

	ViewPort viewPort;
	viewPort.Width = (float)smallDepth.GetDesc().Width;
	viewPort.Height = (float)smallDepth.GetDesc().Height;
	device->BindViewports(1, &viewPort, cmd);

	const GPUResource* dsv[] = { &smallDepth };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, cmd);

	wiRenderer::DownsampleDepthBuffer(depthBuffer_Copy, cmd);

	device->RenderPassEnd(cmd);
}
void RenderPath3D::RenderOutline(const Texture2D& dstSceneRT, CommandList cmd) const
{
	if (getOutlineEnabled())
	{
		wiRenderer::Postprocess_Outline(rtLinearDepth, dstSceneRT, cmd, getOutlineThreshold(), getOutlineThickness(), getOutlineColor());
	}
}
void RenderPath3D::RenderLightShafts(CommandList cmd) const
{
	XMVECTOR sunDirection = XMLoadFloat3(&wiSceneSystem::GetScene().weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, wiRenderer::GetCamera().GetAt())) > 0)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("Light Shafts", cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

		// Render sun stencil cutout:
		{
			device->RenderPassBegin(&renderpass_lightshafts, cmd);

			ViewPort vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawSun(cmd);

			device->RenderPassEnd(cmd);
		}

		const Texture2D* sunSource = &rtSun[0];
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

		ViewPort vp;
		vp.Width = (float)rtVolumetricLights.GetDesc().Width;
		vp.Height = (float)rtVolumetricLights.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawVolumeLights(wiRenderer::GetCamera(), depthBuffer_Copy, cmd);

		device->RenderPassEnd(cmd);
	}
}
void RenderPath3D::RenderParticles(bool isDistrortionPass, CommandList cmd) const
{
	if (getEmittedParticlesEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->RenderPassBegin(&renderpass_particles, cmd);

		ViewPort vp;
		vp.Width = (float)rtParticle.GetDesc().Width;
		vp.Height = (float)rtParticle.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), rtLinearDepth, isDistrortionPass, cmd);

		device->RenderPassEnd(cmd);
	}
}
void RenderPath3D::RenderRefractionSource(const Texture2D& srcSceneRT, CommandList cmd) const
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

		ViewPort vp;
		vp.Width = (float)rtWaterRipple.GetDesc().Width;
		vp.Height = (float)rtWaterRipple.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawWaterRipples(cmd);

		device->RenderPassEnd(cmd);
	}

	device->UnbindResources(TEXSLOT_GBUFFER0, 1, cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

	device->RenderPassBegin(&renderpass_transparent, cmd);

	ViewPort vp;
	vp.Width = (float)renderpass_transparent.desc.attachments[0].texture->GetDesc().Width;
	vp.Height = (float)renderpass_transparent.desc.attachments[0].texture->GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	// Transparent scene
	{
		auto range = wiProfiler::BeginRangeGPU("Transparent Scene", cmd);

		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
		device->BindResource(PS, &rtSceneCopy, TEXSLOT_RENDERPATH_REFRACTION, cmd);
		device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERPATH_WATERRIPPLES, cmd);
		wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), rtLinearDepth, renderPass, cmd, getHairParticlesEnabled(), true);

		wiProfiler::EndRange(range); // Transparent Scene
	}

	wiRenderer::DrawLightVisualizers(wiRenderer::GetCamera(), cmd);

	wiImageParams fx;
	fx.enableFullScreen();
	fx.enableHDR();

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
		wiRenderer::DrawLensFlares(wiRenderer::GetCamera(), depthBuffer_Copy, cmd);
	}

	wiRenderer::DrawDebugWorld(wiRenderer::GetCamera(), cmd);

	device->RenderPassEnd(cmd);


	RenderParticles(true, cmd);
}
void RenderPath3D::TemporalAAResolve(const Texture2D& srcdstSceneRT, const Texture2D& srcGbuffer1, CommandList cmd) const
{
	if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		int output = device->GetFrameCount() % 2;
		int history = 1 - output;
		wiRenderer::Postprocess_TemporalAA(srcdstSceneRT, rtTemporalAA[history], srcGbuffer1, rtTemporalAA[output], cmd);
		wiRenderer::CopyTexture2D(srcdstSceneRT, 0, 0, 0, rtTemporalAA[output], 0, cmd); // todo: remove and do better ping pong

		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderBloom(const RenderPass& renderpass_bloom, CommandList cmd) const
{
	if (getBloomEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("Bloom", cmd);
		auto range = wiProfiler::BeginRangeGPU("Bloom", cmd);

		wiRenderer::Postprocess_BloomSeparate(*renderpass_bloom.desc.attachments[0].texture, rtBloom, cmd, getBloomThreshold());

		wiRenderer::GenerateMipChain(rtBloom, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd);

		// add to the scene
		{
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);

			device->RenderPassBegin(&renderpass_bloom, cmd);

			ViewPort vp;
			vp.Width = (float)renderpass_bloom.desc.attachments[0].texture->GetDesc().Width;
			vp.Height = (float)renderpass_bloom.desc.attachments[0].texture->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiImageParams fx;
			fx.enableHDR();
			fx.enableFullScreen();
			fx.blendFlag = BLENDMODE_ADDITIVE;

			fx.mipLevel = 1.5f;
			wiImage::Draw(&rtBloom, fx, cmd);
			fx.mipLevel = 3.5f;
			wiImage::Draw(&rtBloom, fx, cmd);
			fx.mipLevel = 5.5f;
			wiImage::Draw(&rtBloom, fx, cmd);
			fx.quality = QUALITY_LINEAR;

			device->RenderPassEnd(cmd);
		}

		wiProfiler::EndRange(range);
		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderPostprocessChain(const Texture2D& srcSceneRT, const Texture2D& srcGbuffer1, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture2D* rt_read = &srcSceneRT;
	const Texture2D* rt_write = &rtPostprocess_HDR;

	// 1.) HDR post process chain
	{
		if (getMotionBlurEnabled())
		{
			wiRenderer::Postprocess_MotionBlur(*rt_read, srcGbuffer1, *rt_write, cmd);

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getDepthOfFieldEnabled())
		{
			device->EventBegin("Depth Of Field", cmd);

			wiRenderer::Postprocess_Blur_Gaussian(*rt_read, rtDof[0], rtDof[1], cmd, getDepthOfFieldStrength(), getDepthOfFieldStrength());

			// depth of field compose pass
			{
				wiRenderer::Postprocess_DepthOfField(*rt_read, rtDof[1], *rt_write, rtLinearDepth, cmd, getDepthOfFieldFocus());

				std::swap(rt_read, rt_write);
				device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
			}
			device->EventEnd(cmd);
		}
	}

	// 2.) Tone mapping HDR -> LDR
	{
		rt_write = &rtPostprocess_LDR[0];

		wiRenderer::Postprocess_Tonemap(
			*rt_read,
			getEyeAdaptionEnabled() ? *wiRenderer::ComputeLuminance(srcSceneRT, cmd) : *wiTextureHelper::getColor(wiColor::Gray()),
			rtParticle,
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
				colorGradingTex != nullptr ? *colorGradingTex : *wiTextureHelper::getColorGradeDefault(), 
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
