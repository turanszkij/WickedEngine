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
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		if (getMSAASampleCount() == 1)
		{
			desc.BindFlags |= BIND_UNORDERED_ACCESS;
		}
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleCount = getMSAASampleCount();

		desc.Format = FORMAT_R8G8B8A8_UNORM;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_ALBEDO_ROUGHNESS]);
		device->SetName(&rtGbuffer[GBUFFER_ALBEDO_ROUGHNESS], "rtGbuffer[GBUFFER_ALBEDO_ROUGHNESS]");

		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_NORMAL_VELOCITY]);
		device->SetName(&rtGbuffer[GBUFFER_NORMAL_VELOCITY], "rtGbuffer[GBUFFER_NORMAL_VELOCITY]");

		desc.Format = FORMAT_R11G11B10_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_LIGHTBUFFER_DIFFUSE]);
		device->SetName(&rtGbuffer[GBUFFER_LIGHTBUFFER_DIFFUSE], "rtGbuffer[GBUFFER_LIGHTBUFFER_DIFFUSE]");
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_LIGHTBUFFER_SPECULAR]);
		device->SetName(&rtGbuffer[GBUFFER_LIGHTBUFFER_SPECULAR], "rtGbuffer[GBUFFER_LIGHTBUFFER_SPECULAR]");

		if (getMSAASampleCount() > 1)
		{
			desc.SampleCount = 1;
			desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

			desc.Format = FORMAT_R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_ALBEDO_ROUGHNESS]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_ALBEDO_ROUGHNESS], "rtGbuffer_resolved[GBUFFER_ALBEDO_ROUGHNESS]");

			desc.Format = FORMAT_R16G16B16A16_FLOAT;
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_NORMAL_VELOCITY]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_NORMAL_VELOCITY], "rtGbuffer_resolved[GBUFFER_NORMAL_VELOCITY]");

			desc.Format = FORMAT_R11G11B10_FLOAT;
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_DIFFUSE]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_DIFFUSE], "rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_DIFFUSE]");
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_SPECULAR]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_SPECULAR], "rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_SPECULAR]");
		}

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
		{
			desc.BindFlags |= BIND_UNORDERED_ACCESS;
			desc.Format = FORMAT_R11G11B10_FLOAT;
			desc.SampleCount = 1;

			device->CreateTexture(&desc, nullptr, &rtDiffuseTemporal[0]);
			device->SetName(&rtDiffuseTemporal[0], "rtDiffuseTemporal[0]");
			device->CreateTexture(&desc, nullptr, &rtDiffuseTemporal[1]);
			device->SetName(&rtDiffuseTemporal[1], "rtDiffuseTemporal[1]");

			device->CreateTexture(&desc, nullptr, &rtSpecularTemporal[0]);
			device->SetName(&rtSpecularTemporal[0], "rtSpecularTemporal[0]");
			device->CreateTexture(&desc, nullptr, &rtSpecularTemporal[1]);
			device->SetName(&rtSpecularTemporal[1], "rtSpecularTemporal[1]");
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		if (getMSAASampleCount() == 1)
		{
			desc.BindFlags |= BIND_UNORDERED_ACCESS;
		}
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleCount = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtDeferred);
		device->SetName(&rtDeferred, "rtDeferred");

		if (getMSAASampleCount() > 1)
		{
			desc.SampleCount = 1;
			desc.BindFlags |= BIND_UNORDERED_ACCESS;

			device->CreateTexture(&desc, nullptr, &rtDeferred_resolved);
			device->SetName(&rtDeferred_resolved, "rtDeferred_resolved");
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleCount = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtSSS[0]);
		device->SetName(&rtSSS[0], "rtSSS[0]");
		device->CreateTexture(&desc, nullptr, &rtSSS[1]);
		device->SetName(&rtSSS[1], "rtSSS[1]");

		if (getMSAASampleCount() > 1)
		{
			desc.SampleCount = 1;

			device->CreateTexture(&desc, nullptr, &rtSSS_resolved);
			device->SetName(&rtSSS_resolved, "rtSSS_resolved");
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtSSR);
		device->SetName(&rtSSR, "rtSSR");
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
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_RENDER_TARGET;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x / 2;
		desc.Height = wiRenderer::GetInternalResolution().y / 2;
		desc.MipLevels = std::min(8u, (uint32_t)std::log2(std::max(desc.Width, desc.Height)));
		device->CreateTexture(&desc, nullptr, &rtSceneCopy);
		device->SetName(&rtSceneCopy, "rtSceneCopy");
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &rtSceneCopy_tmp);
		device->SetName(&rtSceneCopy_tmp, "rtSceneCopy_tmp");

		for (uint32_t i = 0; i < rtSceneCopy.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtSceneCopy, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy_tmp, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy_tmp, UAV, 0, 1, i, 1);
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
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtAO);
		device->SetName(&rtAO, "rtAO");
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
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x / 4;
		desc.Height = wiRenderer::GetInternalResolution().y / 4;
		desc.MipLevels = std::min(5u, (uint32_t)std::log2(std::max(desc.Width, desc.Height)));
		device->CreateTexture(&desc, nullptr, &rtBloom);
		device->SetName(&rtBloom, "rtBloom");
		device->CreateTexture(&desc, nullptr, &rtBloom_tmp);
		device->SetName(&rtBloom_tmp, "rtBloom_tmp");

		for (uint32_t i = 0; i < rtBloom.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtBloom, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtBloom_tmp, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtBloom, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtBloom_tmp, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R11G11B10_FLOAT;
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
		desc.Format = FORMAT_R11G11B10_FLOAT;
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

		desc.Width /= 4;
		desc.Height /= 4;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[0]);
		device->SetName(&rtGUIBlurredBackground[0], "rtGUIBlurredBackground[0]");

		desc.Width /= 4;
		desc.Height /= 4;
		device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[1]);
		device->SetName(&rtGUIBlurredBackground[1], "rtGUIBlurredBackground[1]");
		device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[2]);
		device->SetName(&rtGUIBlurredBackground[2], "rtGUIBlurredBackground[2]");
	}

	if(device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2))
	{
		uint32_t tileSize = device->GetVariableRateShadingTileSize();

		TextureDesc desc;
		desc.BindFlags = BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R8_UINT;
		desc.Width = (wiRenderer::GetInternalResolution().x + tileSize - 1) / tileSize;
		desc.Height = (wiRenderer::GetInternalResolution().y + tileSize - 1) / tileSize;
		device->CreateTexture(&desc, nullptr, &rtShadingRate);
		device->SetName(&rtShadingRate, "rtShadingRate");
	}

	// Depth buffers:
	{
		TextureDesc desc;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;

		desc.Format = FORMAT_R32G8X24_TYPELESS;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.SampleCount = getMSAASampleCount();
		desc.layout = IMAGE_LAYOUT_DEPTHSTENCIL_READONLY;
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
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy);
		device->SetName(&depthBuffer_Copy, "depthBuffer_Copy");
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy1);
		device->SetName(&depthBuffer_Copy1, "depthBuffer_Copy1");
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
		desc.Format = FORMAT_R32_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.MipLevels = 6;
		device->CreateTexture(&desc, nullptr, &rtLinearDepth);
		device->SetName(&rtLinearDepth, "rtLinearDepth");

		for (uint32_t i = 0; i < desc.MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtLinearDepth, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtLinearDepth, UAV, 0, 1, i, 1);
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
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_CLEAR,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		device->CreateRenderPass(&desc, &renderpass_depthprepass);

		desc.attachments.clear();
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_ALBEDO_ROUGHNESS], RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_NORMAL_VELOCITY], RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_LIGHTBUFFER_DIFFUSE], RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_LIGHTBUFFER_SPECULAR], RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_ALBEDO_ROUGHNESS)));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_NORMAL_VELOCITY)));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_LIGHTBUFFER_DIFFUSE)));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_LIGHTBUFFER_SPECULAR)));
		}
		device->CreateRenderPass(&desc, &renderpass_main);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtDeferred, RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetDeferred_Read()));
		}
		device->CreateRenderPass(&desc, &renderpass_transparent);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtDeferred, RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_NORMAL_VELOCITY], RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetDeferred_Read()));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_NORMAL_VELOCITY)));
		}
		device->CreateRenderPass(&desc, &renderpass_deferredcomposition);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_LIGHTBUFFER_DIFFUSE], RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtGbuffer_resolved[GBUFFER_LIGHTBUFFER_DIFFUSE]));
		}
		device->CreateRenderPass(&desc, &renderpass_SSS[0]);

		if (getMSAASampleCount() > 1)
		{
			desc.attachments.back() = RenderPassAttachment::Resolve(&rtSSS_resolved);
		}
		desc.attachments[0].texture = &rtSSS[0];
		device->CreateRenderPass(&desc, &renderpass_SSS[1]);

		desc.attachments[0].texture = &rtSSS[1];
		device->CreateRenderPass(&desc, &renderpass_SSS[2]);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&smallDepth,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_DONTCARE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		device->CreateRenderPass(&desc, &renderpass_occlusionculling);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::RenderTarget(
				&rtReflection,
				RenderPassAttachment::LOADOP_DONTCARE,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_SHADER_RESOURCE,
				IMAGE_LAYOUT_RENDERTARGET,
				IMAGE_LAYOUT_SHADER_RESOURCE
			)
		);
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Reflection, 
				RenderPassAttachment::LOADOP_CLEAR, 
				RenderPassAttachment::STOREOP_DONTCARE, 
				IMAGE_LAYOUT_DEPTHSTENCIL, 
				IMAGE_LAYOUT_DEPTHSTENCIL, 
				IMAGE_LAYOUT_DEPTHSTENCIL
			)
		);

		device->CreateRenderPass(&desc, &renderpass_reflection);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&smallDepth,
				RenderPassAttachment::LOADOP_DONTCARE,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		device->CreateRenderPass(&desc, &renderpass_downsampledepthbuffer);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtSceneCopy, RenderPassAttachment::LOADOP_DONTCARE));

		device->CreateRenderPass(&desc, &renderpass_downsamplescene);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtSun[0], RenderPassAttachment::LOADOP_CLEAR));
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.back().storeop = RenderPassAttachment::STOREOP_DONTCARE;
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtSun_resolved));
		}

		device->CreateRenderPass(&desc, &renderpass_lightshafts);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtVolumetricLights, RenderPassAttachment::LOADOP_CLEAR));

		device->CreateRenderPass(&desc, &renderpass_volumetriclight);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtParticleDistortion, RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtParticleDistortion_Resolved));
		}

		device->CreateRenderPass(&desc, &renderpass_particledistortion);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtWaterRipple, RenderPassAttachment::LOADOP_CLEAR));

		device->CreateRenderPass(&desc, &renderpass_waterripples);
	}

	RenderPath2D::ResizeBuffers();
}

void RenderPath3D::Update(float dt)
{
	RenderPath2D::Update(dt);

	wiRenderer::UpdatePerFrameData(dt, getLayerMask());

	std::swap(depthBuffer_Copy, depthBuffer_Copy1);
}

void RenderPath3D::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;
	CommandList cmd;

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) { RenderFrameSetUp(cmd); });

	if (getShadowsEnabled() && !wiRenderer::GetRaytracedShadowsEnabled())
	{
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {
			wiRenderer::DrawShadowmaps(wiRenderer::GetCamera(), cmd, getLayerMask());
			});
	}

	if (wiRenderer::GetVoxelRadianceEnabled())
	{
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [cmd](wiJobArgs args) {
			wiRenderer::VoxelRadiance(cmd);
			});
	}

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [cmd](wiJobArgs args) {
		wiRenderer::BindCommonResources(cmd);
		wiRenderer::RefreshDecalAtlas(cmd);
		wiRenderer::RefreshLightmapAtlas(cmd);
		wiRenderer::RefreshEnvProbes(cmd);
		wiRenderer::RefreshImpostors(cmd);
		});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) { RenderReflections(cmd); });


	static const uint32_t drawscene_flags =
		wiRenderer::DRAWSCENE_OPAQUE |
		wiRenderer::DRAWSCENE_HAIRPARTICLE |
		wiRenderer::DRAWSCENE_TESSELLATION |
		wiRenderer::DRAWSCENE_OCCLUSIONCULLING
		;

	// Main scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		// depth prepass
		{
			auto range = wiProfiler::BeginRangeGPU("Z-Prepass", cmd);

			device->RenderPassBegin(&renderpass_depthprepass, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawScene(wiRenderer::GetCamera(), RENDERPASS_DEPTHONLY, cmd, drawscene_flags);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range);
		}

		if (getMSAASampleCount() > 1)
		{
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY, IMAGE_LAYOUT_SHADER_RESOURCE),
					GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_GENERAL)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			wiRenderer::ResolveMSAADepthBuffer(depthBuffer_Copy, depthBuffer, cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY),
					GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_RESOURCE)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}
		}
		else
		{
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY, IMAGE_LAYOUT_COPY_SRC),
					GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_COPY_DST)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->CopyResource(&depthBuffer_Copy, &depthBuffer, cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_COPY_SRC, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY),
					GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_COPY_DST, IMAGE_LAYOUT_SHADER_RESOURCE)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}
		}

		RenderLinearDepth(cmd);

		RenderAO(cmd);
		});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::ComputeTiledLightCulling(
			depthBuffer_Copy,
			cmd
		);

		if (wiRenderer::GetVariableRateShadingClassification() && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2))
		{
			wiRenderer::ComputeShadingRateClassification(GetGbuffer_Read(), rtLinearDepth, rtShadingRate, cmd);
			device->BindShadingRate(SHADING_RATE_1X1, cmd);
			device->BindShadingRateImage(&rtShadingRate, cmd);
		}

		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);

		// Opaque scene:
		{
			auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

			device->RenderPassBegin(&renderpass_main, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			device->BindResource(PS, getAOEnabled() ? &rtAO : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_AO, cmd);
			device->BindResource(PS, getSSREnabled() || getRaytracedReflectionEnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), RENDERPASS_MAIN, cmd, drawscene_flags);
			
			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Opaque Scene
		}

		device->BindShadingRateImage(nullptr, cmd);
		});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);
		wiRenderer::BindCommonResources(cmd);

		RenderSSS(cmd);

		RenderDeferredComposition(cmd);

		DownsampleDepthBuffer(cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderSceneMIPChain(cmd);

		RenderSSR(cmd);

		RenderTransparents(cmd);

		RenderPostprocessChain(cmd);
		});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
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

	if (wiRenderer::GetDebugLightCulling() || wiRenderer::GetVariableRateShadingClassificationDebug())
	{
		wiImage::Draw((Texture*)wiRenderer::GetTexture(TEXTYPE_2D_DEBUGUAV), wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()), cmd);
	}

	RenderPath2D::Compose(cmd);
}

void RenderPath3D::RenderFrameSetUp(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->BindResource(CS, &depthBuffer_Copy1, TEXSLOT_DEPTH, cmd);
	wiRenderer::UpdateRenderData(cmd);

	if (getAO() == AO_RTAO || wiRenderer::GetRaytracedShadowsEnabled() || getRaytracedReflectionEnabled())
	{
		wiRenderer::UpdateRaytracingAccelerationStructures(cmd);
	}
	
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

		device->RenderPassBegin(&renderpass_reflection, cmd);

		wiRenderer::DrawScene(wiRenderer::GetRefCamera(), RENDERPASS_TEXTURE, cmd);
		wiRenderer::DrawSky(cmd);

		wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), cmd);

		device->RenderPassEnd(cmd);
	}

	wiProfiler::EndRange(range); // Reflection Rendering
}

void RenderPath3D::RenderSSS(CommandList cmd) const
{
	if (getSSSEnabled() && getSSSBlurAmount() > 0)
	{
		wiRenderer::Postprocess_SSS(
			rtLinearDepth,
			GetGbuffer_Read(),
			renderpass_SSS[0],
			renderpass_SSS[1],
			renderpass_SSS[2],
			cmd,
			getSSSBlurAmount()
		);
	}
}
void RenderPath3D::RenderDeferredComposition(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	auto range = wiProfiler::BeginRangeGPU("Sky and Composition", cmd);

	if (wiRenderer::GetRaytracedShadowsEnabled() && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		int output = device->GetFrameCount() % 2;
		int history = 1 - output;

		wiRenderer::Postprocess_Denoise(
			*GetGbuffer_Read(GBUFFER_LIGHTBUFFER_DIFFUSE),
			rtDiffuseTemporal[history],
			rtDiffuseTemporal[output],
			*GetGbuffer_Read(GBUFFER_NORMAL_VELOCITY),
			rtLinearDepth,
			depthBuffer_Copy1,
			cmd
		);
		wiRenderer::Postprocess_Denoise(
			*GetGbuffer_Read(GBUFFER_LIGHTBUFFER_SPECULAR),
			rtSpecularTemporal[history],
			rtSpecularTemporal[output],
			*GetGbuffer_Read(GBUFFER_NORMAL_VELOCITY),
			rtLinearDepth,
			depthBuffer_Copy1,
			cmd
		);
	}

	device->RenderPassBegin(&renderpass_deferredcomposition, cmd);

	Viewport vp;
	vp.Width = (float)depthBuffer.GetDesc().Width;
	vp.Height = (float)depthBuffer.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	wiRenderer::DeferredComposition(
		GetGbuffer_Read(),
		depthBuffer_Copy,
		cmd
	);

	wiRenderer::DrawSky(cmd);

	RenderOutline(cmd);

	device->RenderPassEnd(cmd);

	wiProfiler::EndRange(range);
}
void RenderPath3D::RenderLinearDepth(CommandList cmd) const
{
	wiRenderer::Postprocess_Lineardepth(depthBuffer_Copy, rtLinearDepth, cmd);
}
void RenderPath3D::RenderAO(CommandList cmd) const
{
	wiRenderer::GetDevice()->UnbindResources(TEXSLOT_RENDERPATH_AO, 1, cmd);

	if (getAOEnabled())
	{
		switch (getAO())
		{
		case AO_SSAO:
			wiRenderer::Postprocess_SSAO(
				depthBuffer_Copy,
				rtLinearDepth,
				rtAO,
				cmd,
				getAORange(),
				getAOSampleCount(),
				getAOPower()
				);
			break;
		case AO_HBAO:
			wiRenderer::Postprocess_HBAO(
				rtLinearDepth,
				rtAO,
				cmd,
				getAOPower()
				);
			break;
		case AO_MSAO:
			wiRenderer::Postprocess_MSAO(
				rtLinearDepth,
				rtAO,
				cmd,
				getAOPower()
				);
			break;
		case AO_RTAO:
			wiRenderer::Postprocess_RTAO(
				depthBuffer_Copy,
				rtLinearDepth,
				depthBuffer_Copy1,
				rtAO,
				cmd,
				getAORange(),
				getAOSampleCount(),
				getAOPower()
			);
			break;
		}
	}
}
void RenderPath3D::RenderSSR(CommandList cmd) const
{
	if (getRaytracedReflectionEnabled())
	{
		wiRenderer::Postprocess_RTReflection(
			depthBuffer_Copy, 
			GetGbuffer_Read(),
			rtSSR, 
			cmd
		);
	}
	else if (getSSREnabled())
	{
		wiRenderer::Postprocess_SSR(
			rtSceneCopy, 
			depthBuffer_Copy, 
			rtLinearDepth,
			GetGbuffer_Read(),
			rtSSR, 
			cmd
		);
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
void RenderPath3D::RenderOutline(CommandList cmd) const
{
	if (getOutlineEnabled())
	{
		wiRenderer::Postprocess_Outline(rtLinearDepth, cmd, getOutlineThreshold(), getOutlineThickness(), getOutlineColor());
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

		// Radial blur on the sun:
		{
			XMVECTOR sunPos = XMVector3Project(sunDirection * 100000, 0, 0,
				1.0f, 1.0f, 0.1f, 1.0f,
				wiRenderer::GetCamera().GetProjection(), wiRenderer::GetCamera().GetView(), XMMatrixIdentity());
			{
				XMFLOAT2 sun;
				XMStoreFloat2(&sun, sunPos);
				wiRenderer::Postprocess_LightShafts(*renderpass_lightshafts.desc.attachments.back().texture, rtSun[1], cmd, sun);
			}
		}
		device->EventEnd(cmd);
	}
}
void RenderPath3D::RenderVolumetrics(CommandList cmd) const
{
	if (getVolumeLightsEnabled() && wiRenderer::IsRequestedVolumetricLightRendering())
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
void RenderPath3D::RenderSceneMIPChain(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	auto range = wiProfiler::BeginRangeGPU("Scene MIP Chain", cmd);
	device->EventBegin("RenderSceneMIPChain", cmd);

	device->RenderPassBegin(&renderpass_downsamplescene, cmd);

	Viewport vp;
	vp.Width = (float)rtSceneCopy.GetDesc().Width;
	vp.Height = (float)rtSceneCopy.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	wiImageParams fx;
	fx.enableFullScreen();
	fx.sampleFlag = SAMPLEMODE_CLAMP;
	fx.quality = QUALITY_LINEAR;
	wiImage::Draw(GetDeferred_Read(), fx, cmd);

	device->RenderPassEnd(cmd);

	wiRenderer::MIPGEN_OPTIONS mipopt;
	mipopt.gaussian_temp = &rtSceneCopy_tmp;
	wiRenderer::GenerateMipChain(rtSceneCopy, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd, mipopt);

	device->EventEnd(cmd);
	wiProfiler::EndRange(range);
}
void RenderPath3D::RenderTransparents(CommandList cmd) const
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

		device->BindResource(PS, &rtLinearDepth, TEXSLOT_LINEARDEPTH, cmd);
		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
		device->BindResource(PS, &rtSceneCopy, TEXSLOT_RENDERPATH_REFRACTION, cmd);
		device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERPATH_WATERRIPPLES, cmd);

		uint32_t drawscene_flags = 0;
		drawscene_flags |= wiRenderer::DRAWSCENE_TRANSPARENT;
		drawscene_flags |= wiRenderer::DRAWSCENE_OCCLUSIONCULLING;
		drawscene_flags |= wiRenderer::DRAWSCENE_HAIRPARTICLE;
		wiRenderer::DrawScene(wiRenderer::GetCamera(), RENDERPASS_MAIN, cmd, drawscene_flags);

		wiProfiler::EndRange(range); // Transparent Scene
	}

	wiRenderer::DrawLightVisualizers(wiRenderer::GetCamera(), cmd);

	{
		auto range = wiProfiler::BeginRangeGPU("EmittedParticles - Render", cmd);
		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), rtLinearDepth, false, cmd);
		wiProfiler::EndRange(range);
	}

	if (getVolumeLightsEnabled() && wiRenderer::IsRequestedVolumetricLightRendering())
	{
		device->EventBegin("Contribute Volumetric Lights", cmd);
		wiRenderer::Postprocess_Upsample_Bilateral(rtVolumetricLights, rtLinearDepth, 
			*renderpass_transparent.desc.attachments[0].texture, cmd, true, 1.5f);
		device->EventEnd(cmd);
	}

	if (getLightShaftsEnabled())
	{
		device->EventBegin("Contribute LightShafts", cmd);
		wiImageParams fx;
		fx.enableFullScreen();
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
		auto range = wiProfiler::BeginRangeGPU("EmittedParticles - Render (Distortion)", cmd);
		device->RenderPassBegin(&renderpass_particledistortion, cmd);

		Viewport vp;
		vp.Width = (float)rtParticleDistortion.GetDesc().Width;
		vp.Height = (float)rtParticleDistortion.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawSoftParticles(wiRenderer::GetCamera(), rtLinearDepth, true, cmd);

		device->RenderPassEnd(cmd);
		wiProfiler::EndRange(range);
	}
}
void RenderPath3D::RenderPostprocessChain(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture* rt_first = nullptr; // not ping-ponged with read / write
	const Texture* rt_read = GetDeferred_Read();
	const Texture* rt_write = &rtPostprocess_HDR;

	// 1.) HDR post process chain
	{
		if (getVolumetricCloudsEnabled())
		{
			const Texture* lightShaftTemp = nullptr;

			wiRenderer::Postprocess_VolumetricClouds(*rt_read, *rt_write, *lightShaftTemp, rtLinearDepth, depthBuffer_Copy, cmd);
			
			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
		{
			GraphicsDevice* device = wiRenderer::GetDevice();

			int output = device->GetFrameCount() % 2;
			int history = 1 - output;
			wiRenderer::Postprocess_TemporalAA(
				*rt_read, rtTemporalAA[history], 
				*GetGbuffer_Read(GBUFFER_NORMAL_VELOCITY), 
				rtLinearDepth,
				depthBuffer_Copy1,
				rtTemporalAA[output], 
				cmd
			);
			rt_first = &rtTemporalAA[output];
		}

		if (getDepthOfFieldEnabled())
		{
			wiRenderer::Postprocess_DepthOfField(rt_first == nullptr ? *rt_read : *rt_first, *rt_write, rtLinearDepth, cmd, getDepthOfFieldFocus(), getDepthOfFieldStrength(), getDepthOfFieldAspect());
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getMotionBlurEnabled())
		{
			wiRenderer::Postprocess_MotionBlur(rt_first == nullptr ? *rt_read : *rt_first, *GetGbuffer_Read(GBUFFER_NORMAL_VELOCITY), rtLinearDepth, *rt_write, cmd, getMotionBlurStrength());
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getBloomEnabled())
		{
			wiRenderer::Postprocess_Bloom(rt_first == nullptr ? *rt_read : *rt_first, rtBloom, rtBloom_tmp, *rt_write, cmd, getBloomThreshold());
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
			getEyeAdaptionEnabled() ? *wiRenderer::ComputeLuminance(*GetDeferred_Read(), cmd) : *wiTextureHelper::getColor(wiColor::Gray()),
			getMSAASampleCount() > 1 ? rtParticleDistortion_Resolved : rtParticleDistortion,
			*rt_write,
			cmd,
			getExposure(),
			getDitherEnabled(),
			getColorGradingEnabled() ? (colorGradingTex != nullptr ? colorGradingTex->texture : wiTextureHelper::getColorGradeDefault()) : nullptr
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

		// GUI Background blurring:
		{
			auto range = wiProfiler::BeginRangeGPU("GUI Background Blur", cmd);
			device->EventBegin("GUI Background Blur", cmd);
			wiRenderer::Postprocess_Downsample4x(*rt_read, rtGUIBlurredBackground[0], cmd);
			wiRenderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
			wiRenderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd, -1,-1, true);
			device->EventEnd(cmd);
			wiProfiler::EndRange(range);
		}
	}
}
