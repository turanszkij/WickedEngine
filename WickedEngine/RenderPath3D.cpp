#include "RenderPath3D.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "shaders/ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;

void RenderPath3D::ResizeBuffers()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();
	XMUINT2 internalResolution = GetInternalResolution();

	camera->CreatePerspective((float)internalResolution.x, (float)internalResolution.y, camera->zNearP, camera->zFarP);
	
	// Render targets:

	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		if (getMSAASampleCount() == 1)
		{
			desc.BindFlags |= BIND_UNORDERED_ACCESS;
		}
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		desc.SampleCount = getMSAASampleCount();

		desc.Format = FORMAT_R11G11B10_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_COLOR]);
		device->SetName(&rtGbuffer[GBUFFER_COLOR], "rtGbuffer[GBUFFER_COLOR]");

		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R8G8B8A8_UNORM;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_NORMAL_ROUGHNESS]);
		device->SetName(&rtGbuffer[GBUFFER_NORMAL_ROUGHNESS], "rtGbuffer[GBUFFER_NORMAL_ROUGHNESS]");

		desc.Format = FORMAT_R16G16_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_VELOCITY]);
		device->SetName(&rtGbuffer[GBUFFER_VELOCITY], "rtGbuffer[GBUFFER_VELOCITY]");

		if (getMSAASampleCount() > 1)
		{
			desc.SampleCount = 1;
			desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

			desc.Format = FORMAT_R11G11B10_FLOAT;
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_COLOR]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_COLOR], "rtGbuffer_resolved[GBUFFER_COLOR]");

			desc.Format = FORMAT_R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_NORMAL_ROUGHNESS]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_NORMAL_ROUGHNESS], "rtGbuffer_resolved[GBUFFER_NORMAL_ROUGHNESS]");

			desc.Format = FORMAT_R16G16_FLOAT;
			device->CreateTexture(&desc, nullptr, &rtGbuffer_resolved[GBUFFER_VELOCITY]);
			device->SetName(&rtGbuffer_resolved[GBUFFER_VELOCITY], "rtGbuffer_resolved[GBUFFER_VELOCITY]");

		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtSSR);
		device->SetName(&rtSSR, "rtSSR");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
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
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Width = internalResolution.x / 4;
		desc.Height = internalResolution.y / 4;
		device->CreateTexture(&desc, nullptr, &rtVolumetricLights[0]);
		device->SetName(&rtVolumetricLights[0], "rtVolumetricLights[0]");
		device->CreateTexture(&desc, nullptr, &rtVolumetricLights[1]);
		device->SetName(&rtVolumetricLights[1], "rtVolumetricLights[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R8G8_SNORM;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtWaterRipple);
		device->SetName(&rtWaterRipple, "rtWaterRipple");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_RENDER_TARGET;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = internalResolution.x / 2;
		desc.Height = internalResolution.y / 2;
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
		desc.Width = internalResolution.x / 4;
		desc.Height = internalResolution.y / 4;
		device->CreateTexture(&desc, nullptr, &rtReflection);
		device->SetName(&rtReflection, "rtReflection");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R8_UNORM;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtAO);
		device->SetName(&rtAO, "rtAO");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R32G32B32A32_UINT;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtShadow);
		device->SetName(&rtShadow, "rtShadow");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		desc.SampleCount = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtSun[0]);
		device->SetName(&rtSun[0], "rtSun[0]");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.SampleCount = 1;
		desc.Width = internalResolution.x / 2;
		desc.Height = internalResolution.y / 2;
		device->CreateTexture(&desc, nullptr, &rtSun[1]);
		device->SetName(&rtSun[1], "rtSun[1]");

		if (getMSAASampleCount() > 1)
		{
			desc.Width = internalResolution.x;
			desc.Height = internalResolution.y;
			desc.SampleCount = 1;
			device->CreateTexture(&desc, nullptr, &rtSun_resolved);
			device->SetName(&rtSun_resolved, "rtSun_resolved");
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtTemporalAA[0]);
		device->SetName(&rtTemporalAA[0], "rtTemporalAA[0]");
		device->CreateTexture(&desc, nullptr, &rtTemporalAA[1]);
		device->SetName(&rtTemporalAA[1], "rtTemporalAA[1]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtPostprocess_HDR);
		device->SetName(&rtPostprocess_HDR, "rtPostprocess_HDR");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = defaultTextureFormat;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
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
		desc.layout = IMAGE_LAYOUT_UNDEFINED;
		desc.BindFlags = BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R8_UINT;
		desc.Width = (internalResolution.x + tileSize - 1) / tileSize;
		desc.Height = (internalResolution.y + tileSize - 1) / tileSize;

		std::vector<uint8_t> data(desc.Width * desc.Height);
		uint8_t default_shadingrate;
		device->WriteShadingRateValue(SHADING_RATE_1X1, &default_shadingrate);
		std::fill(data.begin(), data.end(), default_shadingrate);

		SubresourceData initData;
		initData.pSysMem = data.data();
		initData.SysMemPitch = sizeof(uint8_t) * desc.Width;
		device->CreateTexture(&desc, &initData, &rtShadingRate);
		device->SetName(&rtShadingRate, "rtShadingRate");
	}

	// Depth buffers:
	{
		TextureDesc desc;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;

		desc.SampleCount = getMSAASampleCount();
		desc.layout = IMAGE_LAYOUT_DEPTHSTENCIL_READONLY;
		desc.Format = FORMAT_R32G8X24_TYPELESS;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Main);
		device->SetName(&depthBuffer_Main, "depthBuffer_Main");

		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		desc.Format = FORMAT_R32_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.SampleCount = 1;
		desc.MipLevels = 3;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy);
		device->SetName(&depthBuffer_Copy, "depthBuffer_Copy");
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy1);
		device->SetName(&depthBuffer_Copy1, "depthBuffer_Copy1");

		for (uint32_t i = 0; i < depthBuffer_Copy.desc.MipLevels; ++i)
		{
			int subresource = 0;
			subresource = device->CreateSubresource(&depthBuffer_Copy, SRV, 0, 1, i, 1);
			assert(subresource == i);
			subresource = device->CreateSubresource(&depthBuffer_Copy, UAV, 0, 1, i, 1);
			assert(subresource == i);
			subresource = device->CreateSubresource(&depthBuffer_Copy1, SRV, 0, 1, i, 1);
			assert(subresource == i);
			subresource = device->CreateSubresource(&depthBuffer_Copy1, UAV, 0, 1, i, 1);
			assert(subresource == i);
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R32_TYPELESS;
		desc.Width = internalResolution.x / 4;
		desc.Height = internalResolution.y / 4;
		desc.layout = IMAGE_LAYOUT_DEPTHSTENCIL_READONLY;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Reflection);
		device->SetName(&depthBuffer_Reflection, "depthBuffer_Reflection");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R32_FLOAT;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
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

	// Render passes:
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LOADOP_CLEAR,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL,
				IMAGE_LAYOUT_SHADER_RESOURCE
			)
		);
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_VELOCITY], RenderPassAttachment::LOADOP_DONTCARE));
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_VELOCITY)));
		}
		device->CreateRenderPass(&desc, &renderpass_depthprepass);

		desc.attachments.clear();
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_COLOR], RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_NORMAL_ROUGHNESS], RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_SHADER_RESOURCE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_COLOR)));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetGbuffer_Read(GBUFFER_NORMAL_ROUGHNESS)));
		}

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2))
		{
			desc.attachments.push_back(RenderPassAttachment::ShadingRateSource(&rtShadingRate, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_UNDEFINED));
		}

		device->CreateRenderPass(&desc, &renderpass_main);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGbuffer[GBUFFER_COLOR], RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtGbuffer_resolved[GBUFFER_COLOR]));
		}
		device->CreateRenderPass(&desc, &renderpass_transparent);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Reflection,
				RenderPassAttachment::LOADOP_CLEAR,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL,
				IMAGE_LAYOUT_SHADER_RESOURCE
			)
		);

		device->CreateRenderPass(&desc, &renderpass_reflection_depthprepass);
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
				RenderPassAttachment::LOADOP_LOAD, 
				RenderPassAttachment::STOREOP_DONTCARE,
				IMAGE_LAYOUT_SHADER_RESOURCE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		device->CreateRenderPass(&desc, &renderpass_reflection);
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
				&depthBuffer_Main,
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
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtVolumetricLights[0], RenderPassAttachment::LOADOP_CLEAR));

		device->CreateRenderPass(&desc, &renderpass_volumetriclight);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtParticleDistortion, RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
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


	// Other resources:
	{
		TextureDesc desc;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = FORMAT_R8G8B8A8_UNORM;
		desc.SampleCount = 1;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		device->CreateTexture(&desc, nullptr, &debugUAV);
		device->SetName(&debugUAV, "debugUAV");
	}
	wiRenderer::CreateTiledLightResources(tiledLightResources, internalResolution);
	wiRenderer::CreateLuminanceResources(luminanceResources, internalResolution);
	wiRenderer::CreateSSAOResources(ssaoResources, internalResolution);
	wiRenderer::CreateMSAOResources(msaoResources, internalResolution);
	wiRenderer::CreateSSRResources(ssrResources, internalResolution);
	wiRenderer::CreateScreenSpaceShadowResources(screenspaceshadowResources, internalResolution);
	wiRenderer::CreateDepthOfFieldResources(depthoffieldResources, internalResolution);
	wiRenderer::CreateMotionBlurResources(motionblurResources, internalResolution);
	wiRenderer::CreateVolumetricCloudResources(volumetriccloudResources, internalResolution);
	wiRenderer::CreateBloomResources(bloomResources, internalResolution);

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
	{
		wiRenderer::CreateRTAOResources(rtaoResources, internalResolution);
		wiRenderer::CreateRTReflectionResources(rtreflectionResources, internalResolution);
		wiRenderer::CreateRTShadowResources(rtshadowResources, internalResolution);
	}

	RenderPath2D::ResizeBuffers();
}

void RenderPath3D::PreUpdate()
{
	camera_previous = *camera;
}

void RenderPath3D::Update(float dt)
{
	RenderPath2D::Update(dt);

	if (getSceneUpdateEnabled())
	{
		scene->Update(dt * wiRenderer::GetGameSpeed());
	}

	// Frustum culling for main camera:
	visibility_main.layerMask = getLayerMask();
	visibility_main.scene = scene;
	visibility_main.camera = camera;
	visibility_main.flags = wiRenderer::Visibility::ALLOW_EVERYTHING;
	wiRenderer::UpdateVisibility(visibility_main);

	if (visibility_main.planar_reflection_visible)
	{
		// Frustum culling for planar reflections:
		camera_reflection = *camera;
		camera_reflection.jitter = XMFLOAT2(0, 0);
		camera_reflection.Reflect(visibility_main.reflectionPlane);
		visibility_reflection.layerMask = getLayerMask();
		visibility_reflection.scene = scene;
		visibility_reflection.camera = &camera_reflection;
		visibility_reflection.flags = wiRenderer::Visibility::ALLOW_OBJECTS;
		wiRenderer::UpdateVisibility(visibility_reflection);
	}

	wiRenderer::UpdatePerFrameData(*scene, visibility_main, frameCB, GetInternalResolution(), dt);

	if (wiRenderer::GetTemporalAAEnabled())
	{
		const XMFLOAT4& halton = wiMath::GetHaltonSequence(wiRenderer::GetDevice()->GetFrameCount() % 256);
		camera->jitter.x = (halton.x * 2 - 1) / (float)GetInternalResolution().x;
		camera->jitter.y = (halton.y * 2 - 1) / (float)GetInternalResolution().y;
	}
	else
	{
		camera->jitter = XMFLOAT2(0, 0);
	}
	camera->UpdateCamera();

	std::swap(depthBuffer_Copy, depthBuffer_Copy1);
}

void RenderPath3D::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;
	CommandList cmd;

	// Preparing the frame:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {
		RenderFrameSetUp(cmd);
		});

	static const uint32_t drawscene_flags =
		wiRenderer::DRAWSCENE_OPAQUE |
		wiRenderer::DRAWSCENE_HAIRPARTICLE |
		wiRenderer::DRAWSCENE_TESSELLATION |
		wiRenderer::DRAWSCENE_OCCLUSIONCULLING
		;
	static const uint32_t drawscene_flags_reflections =
		wiRenderer::DRAWSCENE_OPAQUE
		;

	// Depth prepass + Occlusion culling + AO:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::UpdateCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);

		device->RenderPassBegin(&renderpass_depthprepass, cmd);

		device->EventBegin("Opaque Z-prepass", cmd);
		auto range = wiProfiler::BeginRangeGPU("Z-Prepass", cmd);

		Viewport vp;
		vp.Width = (float)depthBuffer_Main.GetDesc().Width;
		vp.Height = (float)depthBuffer_Main.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);
		wiRenderer::DrawScene(visibility_main, RENDERPASS_PREPASS, cmd, drawscene_flags);
		wiRenderer::DrawSkyVelocity(cmd);

		wiProfiler::EndRange(range);
		device->EventEnd(cmd);

		if (getOcclusionCullingEnabled())
		{
			wiRenderer::OcclusionCulling_Render(*camera, visibility_main, cmd);
		}

		device->RenderPassEnd(cmd);

		// Create the top mip of depth pyramid from main depth buffer:
		if (getMSAASampleCount() > 1)
		{
			wiRenderer::ResolveMSAADepthBuffer(depthBuffer_Copy, depthBuffer_Main, cmd);
		}
		else
		{
			wiRenderer::CopyTexture2D(depthBuffer_Copy, 0, 0, 0, depthBuffer_Main, 0, cmd);
		}

		wiRenderer::Postprocess_DepthPyramid(depthBuffer_Copy, rtLinearDepth, cmd);

		RenderAO(cmd);

		if (wiRenderer::GetVariableRateShadingClassification() && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2))
		{
			wiRenderer::ComputeShadingRateClassification(
				GetGbuffer_Read(),
				rtLinearDepth,
				rtShadingRate,
				debugUAV,
				cmd
			);
		}

		if (getVolumetricCloudsEnabled())
		{
			wiRenderer::Postprocess_VolumetricClouds(
				volumetriccloudResources,
				rtLinearDepth,
				depthBuffer_Copy,
				cmd
			);
		}

		});

	// Planar reflections depth prepass + Light culling:
	if (visibility_main.IsRequestedPlanarReflections())
	{
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [cmd, this](wiJobArgs args) {

			GraphicsDevice* device = wiRenderer::GetDevice();

			wiRenderer::UpdateCameraCB(
				camera_reflection,
				camera_reflection,
				camera_reflection,
				cmd
			);

			device->EventBegin("Planar reflections Z-Prepass", cmd);
			auto range = wiProfiler::BeginRangeGPU("Planar Reflections Z-Prepass", cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer_Reflection.GetDesc().Width;
			vp.Height = (float)depthBuffer_Reflection.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->RenderPassBegin(&renderpass_reflection_depthprepass, cmd);

			wiRenderer::DrawScene(visibility_reflection, RENDERPASS_PREPASS, cmd, drawscene_flags_reflections);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Planar Reflections
			device->EventEnd(cmd);

			wiRenderer::ComputeTiledLightCulling(
				tiledLightResources,
				depthBuffer_Reflection,
				debugUAV,
				cmd
			);

			});
	}

	// Shadow maps:
	if (getShadowsEnabled())
	{
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {
			wiRenderer::DrawShadowmaps(visibility_main, cmd);
			});
	}

	// Updating textures:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [cmd, this](wiJobArgs args) {
		wiRenderer::BindCommonResources(cmd);
		wiRenderer::RefreshDecalAtlas(*scene, cmd);
		wiRenderer::RefreshLightmapAtlas(*scene, cmd);
		wiRenderer::RefreshEnvProbes(visibility_main, cmd);
		wiRenderer::RefreshImpostors(*scene, cmd);
		});

	// Voxel GI:
	if (wiRenderer::GetVoxelRadianceEnabled())
	{
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [cmd, this](wiJobArgs args) {
			wiRenderer::VoxelRadiance(visibility_main, cmd);
			});
	}

	// Planar reflections:
	if (visibility_main.IsRequestedPlanarReflections())
	{
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [cmd, this](wiJobArgs args) {

			GraphicsDevice* device = wiRenderer::GetDevice();

			wiRenderer::UpdateCameraCB(
				camera_reflection,
				camera_reflection,
				camera_reflection,
				cmd
			);

			device->EventBegin("Planar reflections", cmd);
			auto range = wiProfiler::BeginRangeGPU("Planar Reflections", cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer_Reflection.GetDesc().Width;
			vp.Height = (float)depthBuffer_Reflection.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->UnbindResources(TEXSLOT_DEPTH, 1, cmd);

			device->RenderPassBegin(&renderpass_reflection, cmd);

			device->BindResource(PS, &tiledLightResources.entityTiles_Opaque, TEXSLOT_RENDERPATH_ENTITYTILES, cmd);
			device->BindResource(PS, wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			device->BindResource(PS, wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_AO, cmd);
			device->BindResource(PS, wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
			device->BindResource(PS, wiTextureHelper::getUINT4(), TEXSLOT_RENDERPATH_RTSHADOW, cmd);
			wiRenderer::DrawScene(visibility_reflection, RENDERPASS_MAIN, cmd, drawscene_flags_reflections);
			wiRenderer::DrawSky(*scene, cmd);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Planar Reflections
			device->EventEnd(cmd);
			});
	}

	// Lighting effects:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::UpdateCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);

		{
			auto range = wiProfiler::BeginRangeGPU("Entity Culling", cmd);
			wiRenderer::ComputeTiledLightCulling(
				tiledLightResources,
				depthBuffer_Copy,
				debugUAV,
				cmd
			);
			wiProfiler::EndRange(range);
		}

		RenderSSR(cmd);

		if (wiRenderer::GetScreenSpaceShadowsEnabled())
		{
			wiRenderer::Postprocess_ScreenSpaceShadow(
				screenspaceshadowResources,
				depthBuffer_Copy,
				rtLinearDepth,
				tiledLightResources.entityTiles_Opaque,
				rtShadow,
				cmd,
				getScreenSpaceShadowRange(),
				getScreenSpaceShadowSampleCount()
			);
		}

		if (wiRenderer::GetRaytracedShadowsEnabled())
		{
			wiRenderer::Postprocess_RTShadow(
				rtshadowResources,
				*scene,
				depthBuffer_Copy,
				rtLinearDepth,
				depthBuffer_Copy1,
				tiledLightResources.entityTiles_Opaque,
				GetGbuffer_Read(),
				rtShadow,
				cmd
			);
		}

		});

	// Opaque scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("Opaque Scene", cmd);

		device->RenderPassBegin(&renderpass_main, cmd);

		auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

		Viewport vp;
		vp.Width = (float)depthBuffer_Main.GetDesc().Width;
		vp.Height = (float)depthBuffer_Main.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		if (wiRenderer::GetRaytracedShadowsEnabled() || wiRenderer::GetScreenSpaceShadowsEnabled())
		{
			device->BindResource(PS, &rtShadow, TEXSLOT_RENDERPATH_RTSHADOW, cmd);
		}
		else
		{
			device->BindResource(PS, wiTextureHelper::getUINT4(), TEXSLOT_RENDERPATH_RTSHADOW, cmd);
		}

		device->BindResource(PS, &tiledLightResources.entityTiles_Opaque, TEXSLOT_RENDERPATH_ENTITYTILES, cmd);
		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
		device->BindResource(PS, getAOEnabled() ? &rtAO : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_AO, cmd);
		device->BindResource(PS, getSSREnabled() || getRaytracedReflectionEnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
		wiRenderer::DrawScene(visibility_main, RENDERPASS_MAIN, cmd, drawscene_flags);
		wiRenderer::DrawSky(*scene, cmd);

		wiProfiler::EndRange(range); // Opaque Scene

		RenderOutline(cmd);

		// Upsample + Blend the volumetric clouds on top:
		if (getVolumetricCloudsEnabled())
		{
			device->EventBegin("Volumetric Clouds Upsample + Blend", cmd);
			wiRenderer::Postprocess_Upsample_Bilateral(
				volumetriccloudResources.texture_reproject[device->GetFrameCount() % 2],
				rtLinearDepth,
				*GetGbuffer_Read(GBUFFER_COLOR), // only desc is taken if pixel shader upsampling is used
				cmd,
				true // pixel shader upsampling
			);
			device->EventEnd(cmd);
		}

		device->RenderPassEnd(cmd);

		device->EventEnd(cmd);
		});

	// Transparents, post processes, etc:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::UpdateCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);
		wiRenderer::BindCommonResources(cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderSceneMIPChain(cmd);

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
		wiImage::Draw(&debugUAV, wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight()), cmd);
	}

	RenderPath2D::Compose(cmd);
}

void RenderPath3D::RenderFrameSetUp(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->BindResource(CS, &depthBuffer_Copy1, TEXSLOT_DEPTH, cmd);
	wiRenderer::UpdateRenderData(visibility_main, frameCB, cmd);

	wiRenderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
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
				ssaoResources,
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
				ssaoResources,
				*camera,
				rtLinearDepth,
				rtAO,
				cmd,
				getAOPower()
				);
			break;
		case AO_MSAO:
			wiRenderer::Postprocess_MSAO(
				msaoResources,
				*camera,
				rtLinearDepth,
				rtAO,
				cmd,
				getAOPower()
				);
			break;
		case AO_RTAO:
			wiRenderer::Postprocess_RTAO(
				rtaoResources,
				*scene,
				depthBuffer_Copy,
				rtLinearDepth,
				depthBuffer_Copy1,
				GetGbuffer_Read(),
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
			rtreflectionResources,
			*scene,
			depthBuffer_Copy,
			depthBuffer_Copy1,
			GetGbuffer_Read(),
			rtSSR, 
			cmd
		);
	}
	else if (getSSREnabled())
	{
		wiRenderer::Postprocess_SSR(
			ssrResources,
			rtSceneCopy, 
			depthBuffer_Copy, 
			rtLinearDepth,
			depthBuffer_Copy1,
			GetGbuffer_Read(),
			rtSSR, 
			cmd
		);
	}
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
	XMVECTOR sunDirection = XMLoadFloat3(&scene->weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, camera->GetAt())) > 0)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("Light Shafts", cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

		// Render sun stencil cutout:
		{
			device->RenderPassBegin(&renderpass_lightshafts, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer_Main.GetDesc().Width;
			vp.Height = (float)depthBuffer_Main.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawSun(cmd);

			device->RenderPassEnd(cmd);
		}

		// Radial blur on the sun:
		{
			XMVECTOR sunPos = XMVector3Project(sunDirection * 100000, 0, 0,
				1.0f, 1.0f, 0.1f, 1.0f,
				camera->GetProjection(), camera->GetView(), XMMatrixIdentity());
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
	if (getVolumeLightsEnabled() && visibility_main.IsRequestedVolumetricLights())
	{
		auto range = wiProfiler::BeginRangeGPU("Volumetric Lights", cmd);

		GraphicsDevice* device = wiRenderer::GetDevice();

		device->RenderPassBegin(&renderpass_volumetriclight, cmd);

		Viewport vp;
		vp.Width = (float)rtVolumetricLights[0].GetDesc().Width;
		vp.Height = (float)rtVolumetricLights[0].GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawVolumeLights(visibility_main, depthBuffer_Copy, cmd);

		device->RenderPassEnd(cmd);

		wiRenderer::Postprocess_Blur_Bilateral(
			rtVolumetricLights[0],
			rtLinearDepth,
			rtVolumetricLights[1],
			rtVolumetricLights[0],
			cmd
		);

		wiProfiler::EndRange(range);
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
	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(GetGbuffer_Read(GBUFFER_COLOR), fx, cmd);

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
	if(!scene->waterRipples.empty())
	{
		device->RenderPassBegin(&renderpass_waterripples, cmd);

		Viewport vp;
		vp.Width = (float)rtWaterRipple.GetDesc().Width;
		vp.Height = (float)rtWaterRipple.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawWaterRipples(visibility_main, cmd);

		device->RenderPassEnd(cmd);
	}

	device->UnbindResources(TEXSLOT_GBUFFER0, 1, cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, cmd);

	device->RenderPassBegin(&renderpass_transparent, cmd);

	Viewport vp;
	vp.Width = (float)depthBuffer_Main.GetDesc().Width;
	vp.Height = (float)depthBuffer_Main.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	// Transparent scene
	{
		auto range = wiProfiler::BeginRangeGPU("Transparent Scene", cmd);
		device->EventBegin("Transparent Scene", cmd);

		device->BindResource(PS, &tiledLightResources.entityTiles_Transparent, TEXSLOT_RENDERPATH_ENTITYTILES, cmd);
		device->BindResource(PS, &rtLinearDepth, TEXSLOT_LINEARDEPTH, cmd);
		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
		device->BindResource(PS, &rtSceneCopy, TEXSLOT_RENDERPATH_REFRACTION, cmd);
		device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERPATH_WATERRIPPLES, cmd);

		uint32_t drawscene_flags = 0;
		drawscene_flags |= wiRenderer::DRAWSCENE_TRANSPARENT;
		drawscene_flags |= wiRenderer::DRAWSCENE_OCCLUSIONCULLING;
		drawscene_flags |= wiRenderer::DRAWSCENE_HAIRPARTICLE;
		wiRenderer::DrawScene(visibility_main, RENDERPASS_MAIN, cmd, drawscene_flags);

		device->EventEnd(cmd);
		wiProfiler::EndRange(range); // Transparent Scene
	}

	wiRenderer::DrawLightVisualizers(visibility_main, cmd);

	wiRenderer::DrawSoftParticles(visibility_main, rtLinearDepth, false, cmd);

	if (getVolumeLightsEnabled() && visibility_main.IsRequestedVolumetricLights())
	{
		device->EventBegin("Contribute Volumetric Lights", cmd);
		wiRenderer::Postprocess_Upsample_Bilateral(rtVolumetricLights[0], rtLinearDepth, 
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
		wiRenderer::DrawLensFlares(visibility_main, depthBuffer_Copy, cmd);
	}

	wiRenderer::DrawDebugWorld(*scene, *camera, cmd);

	device->RenderPassEnd(cmd);

	// Distortion particles:
	{
		device->RenderPassBegin(&renderpass_particledistortion, cmd);

		Viewport vp;
		vp.Width = (float)rtParticleDistortion.GetDesc().Width;
		vp.Height = (float)rtParticleDistortion.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawSoftParticles(visibility_main, rtLinearDepth, true, cmd);

		device->RenderPassEnd(cmd);
	}
}
void RenderPath3D::RenderPostprocessChain(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture* rt_first = nullptr; // not ping-ponged with read / write
	const Texture* rt_read = GetGbuffer_Read(GBUFFER_COLOR);
	const Texture* rt_write = &rtPostprocess_HDR;

	// 1.) HDR post process chain
	{
		if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
		{
			GraphicsDevice* device = wiRenderer::GetDevice();

			int output = device->GetFrameCount() % 2;
			int history = 1 - output;
			wiRenderer::Postprocess_TemporalAA(
				*rt_read, rtTemporalAA[history], 
				rtLinearDepth,
				depthBuffer_Copy1,
				GetGbuffer_Read(),
				rtTemporalAA[output], 
				cmd
			);
			rt_first = &rtTemporalAA[output];
		}

		if (getDepthOfFieldEnabled() && camera->aperture_size > 0 && getDepthOfFieldStrength() > 0)
		{
			wiRenderer::Postprocess_DepthOfField(
				depthoffieldResources,
				rt_first == nullptr ? *rt_read : *rt_first,
				*rt_write,
				rtLinearDepth,
				cmd,
				getDepthOfFieldStrength()
			);
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getMotionBlurEnabled() && getMotionBlurStrength() > 0)
		{
			wiRenderer::Postprocess_MotionBlur(
				motionblurResources,
				rt_first == nullptr ? *rt_read : *rt_first,
				rtLinearDepth,
				GetGbuffer_Read(),
				*rt_write,
				cmd,
				getMotionBlurStrength()
			);
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}

		if (getBloomEnabled())
		{
			wiRenderer::Postprocess_Bloom(
				bloomResources,
				rt_first == nullptr ? *rt_read : *rt_first,
				*rt_write,
				cmd,
				getBloomThreshold()
			);
			rt_first = nullptr;

			std::swap(rt_read, rt_write);
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		}
	}

	// 2.) Tone mapping HDR -> LDR
	{
		rt_write = &rtPostprocess_LDR[0];

		wiRenderer::Postprocess_Tonemap(
			rt_first == nullptr ? *rt_read : *rt_first,
			*rt_write,
			cmd,
			getExposure(),
			getDitherEnabled(),
			getColorGradingEnabled() ? (scene->weather.colorGradingMap == nullptr ? nullptr : &scene->weather.colorGradingMap->texture) : nullptr,
			getMSAASampleCount() > 1 ? &rtParticleDistortion_Resolved : &rtParticleDistortion,
			getEyeAdaptionEnabled() ? wiRenderer::ComputeLuminance(luminanceResources, *GetGbuffer_Read(GBUFFER_COLOR), cmd, getEyeAdaptionRate()) : nullptr,
			getEyeAdaptionKey()
		);

		rt_first = nullptr;
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

