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
	GraphicsDevice* device = wiGraphics::GetDevice();

	XMUINT2 internalResolution = GetInternalResolution();

	camera->CreatePerspective((float)internalResolution.x, (float)internalResolution.y, camera->zNearP, camera->zFarP);
	
	// Render targets:

	{
		TextureDesc desc;
		desc.format = Format::R11G11B10_FLOAT;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		desc.sample_count = 1;

		device->CreateTexture(&desc, nullptr, &rtMain);
		device->SetName(&rtMain, "rtMain");

		if (getMSAASampleCount() > 1)
		{
			desc.sample_count = getMSAASampleCount();
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;

			device->CreateTexture(&desc, nullptr, &rtMain_render);
			device->SetName(&rtMain_render, "rtMain_render");
		}
		else
		{
			rtMain_render = rtMain;
		}
	}
	{
		TextureDesc desc;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
		desc.sample_count = 1;

		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R32G32_UINT;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_PRIMITIVEID]);
		device->SetName(&rtGbuffer[GBUFFER_PRIMITIVEID], "rtGbuffer[GBUFFER_PRIMITIVEID]");

		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R16G16_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtGbuffer[GBUFFER_VELOCITY]);
		device->SetName(&rtGbuffer[GBUFFER_VELOCITY], "rtGbuffer[GBUFFER_VELOCITY]");

		if (getMSAASampleCount() > 1)
		{
			desc = rtGbuffer[GBUFFER_PRIMITIVEID].desc;
			desc.sample_count = getMSAASampleCount();
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;

			device->CreateTexture(&desc, nullptr, &rtPrimitiveID_render);
			device->SetName(&rtPrimitiveID_render, "rtPrimitiveID_render");
		}
		else
		{
			rtPrimitiveID_render = rtGbuffer[GBUFFER_PRIMITIVEID];
		}
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		desc.format = Format::R16G16_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		desc.sample_count = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtParticleDistortion);
		device->SetName(&rtParticleDistortion, "rtParticleDistortion");
		if (getMSAASampleCount() > 1)
		{
			desc.sample_count = 1;
			device->CreateTexture(&desc, nullptr, &rtParticleDistortion_Resolved);
			device->SetName(&rtParticleDistortion_Resolved, "rtParticleDistortion_Resolved");
		}
	}
	{
		TextureDesc desc;
		desc.format = Format::R16G16B16A16_FLOAT;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.width = internalResolution.x / 4;
		desc.height = internalResolution.y / 4;
		device->CreateTexture(&desc, nullptr, &rtVolumetricLights[0]);
		device->SetName(&rtVolumetricLights[0], "rtVolumetricLights[0]");
		device->CreateTexture(&desc, nullptr, &rtVolumetricLights[1]);
		device->SetName(&rtVolumetricLights[1], "rtVolumetricLights[1]");
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		desc.format = Format::R16G16_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtWaterRipple);
		device->SetName(&rtWaterRipple, "rtWaterRipple");
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS | BindFlag::RENDER_TARGET;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		desc.mip_levels = std::min(8u, (uint32_t)std::log2(std::max(desc.width, desc.height)));
		device->CreateTexture(&desc, nullptr, &rtSceneCopy);
		device->SetName(&rtSceneCopy, "rtSceneCopy");
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &rtSceneCopy_tmp);
		device->SetName(&rtSceneCopy_tmp, "rtSceneCopy_tmp");

		for (uint32_t i = 0; i < rtSceneCopy.GetDesc().mip_levels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtSceneCopy, SubresourceType::SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy_tmp, SubresourceType::SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtSceneCopy_tmp, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x / 4;
		desc.height = internalResolution.y / 4;
		device->CreateTexture(&desc, nullptr, &rtReflection);
		device->SetName(&rtReflection, "rtReflection");
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		desc.sample_count = getMSAASampleCount();
		device->CreateTexture(&desc, nullptr, &rtSun[0]);
		device->SetName(&rtSun[0], "rtSun[0]");

		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.sample_count = 1;
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		device->CreateTexture(&desc, nullptr, &rtSun[1]);
		device->SetName(&rtSun[1], "rtSun[1]");

		if (getMSAASampleCount() > 1)
		{
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			desc.sample_count = 1;
			device->CreateTexture(&desc, nullptr, &rtSun_resolved);
			device->SetName(&rtSun_resolved, "rtSun_resolved");
		}
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtTemporalAA[0]);
		device->SetName(&rtTemporalAA[0], "rtTemporalAA[0]");
		device->CreateTexture(&desc, nullptr, &rtTemporalAA[1]);
		device->SetName(&rtTemporalAA[1], "rtTemporalAA[1]");
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtPostprocess);
		device->SetName(&rtPostprocess, "rtPostprocess");
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R10G10B10A2_UNORM;
		desc.width = internalResolution.x / 4;
		desc.height = internalResolution.y / 4;
		desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[0]);
		device->SetName(&rtGUIBlurredBackground[0], "rtGUIBlurredBackground[0]");

		desc.width /= 4;
		desc.height /= 4;
		device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[1]);
		device->SetName(&rtGUIBlurredBackground[1], "rtGUIBlurredBackground[1]");
		device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[2]);
		device->SetName(&rtGUIBlurredBackground[2], "rtGUIBlurredBackground[2]");
	}
	if(device->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2) &&
		wiRenderer::GetVariableRateShadingClassification())
	{
		uint32_t tileSize = device->GetVariableRateShadingTileSize();

		TextureDesc desc;
		desc.layout = ResourceState::UNORDERED_ACCESS;
		desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADING_RATE;
		desc.format = Format::R8_UINT;
		desc.width = (internalResolution.x + tileSize - 1) / tileSize;
		desc.height = (internalResolution.y + tileSize - 1) / tileSize;

		device->CreateTexture(&desc, nullptr, &rtShadingRate);
		device->SetName(&rtShadingRate, "rtShadingRate");
	}
	rtAO = {};
	rtShadow = {};
	rtSSR = {};

	// Depth buffers:
	{
		TextureDesc desc;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;

		desc.sample_count = getMSAASampleCount();
		desc.layout = ResourceState::DEPTHSTENCIL_READONLY;
		desc.format = Format::R32G8X24_TYPELESS;
		desc.bind_flags = BindFlag::DEPTH_STENCIL | BindFlag::SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Main);
		device->SetName(&depthBuffer_Main, "depthBuffer_Main");

		desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
		desc.format = Format::R32_FLOAT;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.sample_count = 1;
		desc.mip_levels = 5;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy);
		device->SetName(&depthBuffer_Copy, "depthBuffer_Copy");
		device->CreateTexture(&desc, nullptr, &depthBuffer_Copy1);
		device->SetName(&depthBuffer_Copy1, "depthBuffer_Copy1");

		for (uint32_t i = 0; i < depthBuffer_Copy.desc.mip_levels; ++i)
		{
			int subresource = 0;
			subresource = device->CreateSubresource(&depthBuffer_Copy, SubresourceType::SRV, 0, 1, i, 1);
			assert(subresource == i);
			subresource = device->CreateSubresource(&depthBuffer_Copy, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource == i);
			subresource = device->CreateSubresource(&depthBuffer_Copy1, SubresourceType::SRV, 0, 1, i, 1);
			assert(subresource == i);
			subresource = device->CreateSubresource(&depthBuffer_Copy1, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource == i);
		}
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::DEPTH_STENCIL | BindFlag::SHADER_RESOURCE;
		desc.format = Format::R32_TYPELESS;
		desc.width = internalResolution.x / 4;
		desc.height = internalResolution.y / 4;
		desc.layout = ResourceState::DEPTHSTENCIL_READONLY;
		device->CreateTexture(&desc, nullptr, &depthBuffer_Reflection);
		device->SetName(&depthBuffer_Reflection, "depthBuffer_Reflection");
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R32_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		desc.mip_levels = 5;
		desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
		device->CreateTexture(&desc, nullptr, &rtLinearDepth);
		device->SetName(&rtLinearDepth, "rtLinearDepth");

		for (uint32_t i = 0; i < desc.mip_levels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&rtLinearDepth, SubresourceType::SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&rtLinearDepth, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}

	// Render passes:
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LoadOp::CLEAR,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL,
				ResourceState::SHADER_RESOURCE
			)
		);
		desc.attachments.push_back(
			RenderPassAttachment::RenderTarget(
				&rtPrimitiveID_render,
				RenderPassAttachment::LoadOp::DONTCARE,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::SHADER_RESOURCE_COMPUTE,
				ResourceState::RENDERTARGET,
				ResourceState::SHADER_RESOURCE_COMPUTE
			)
		);
		device->CreateRenderPass(&desc, &renderpass_depthprepass);

		desc.attachments.clear();
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtMain_render, RenderPassAttachment::LoadOp::DONTCARE));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LoadOp::LOAD,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::SHADER_RESOURCE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtMain));
		}

		if (device->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2) && rtShadingRate.IsValid())
		{
			desc.attachments.push_back(RenderPassAttachment::ShadingRateSource(&rtShadingRate, ResourceState::UNORDERED_ACCESS, ResourceState::UNORDERED_ACCESS));
		}

		device->CreateRenderPass(&desc, &renderpass_main);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtMain_render, RenderPassAttachment::LoadOp::LOAD));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LoadOp::LOAD,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL,
				ResourceState::DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtMain));
		}
		device->CreateRenderPass(&desc, &renderpass_transparent);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Reflection,
				RenderPassAttachment::LoadOp::CLEAR,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL,
				ResourceState::SHADER_RESOURCE
			)
		);

		device->CreateRenderPass(&desc, &renderpass_reflection_depthprepass);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::RenderTarget(
				&rtReflection,
				RenderPassAttachment::LoadOp::DONTCARE,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::SHADER_RESOURCE,
				ResourceState::RENDERTARGET,
				ResourceState::SHADER_RESOURCE
			)
		);
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Reflection, 
				RenderPassAttachment::LoadOp::LOAD, 
				RenderPassAttachment::StoreOp::DONTCARE,
				ResourceState::SHADER_RESOURCE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY
			)
		);

		device->CreateRenderPass(&desc, &renderpass_reflection);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtSceneCopy, RenderPassAttachment::LoadOp::DONTCARE));

		device->CreateRenderPass(&desc, &renderpass_downsamplescene);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LoadOp::LOAD,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY
			)
		);
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtSun[0], RenderPassAttachment::LoadOp::CLEAR));
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.back().storeop = RenderPassAttachment::StoreOp::DONTCARE;
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtSun_resolved));
		}

		device->CreateRenderPass(&desc, &renderpass_lightshafts);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtVolumetricLights[0], RenderPassAttachment::LoadOp::CLEAR));

		device->CreateRenderPass(&desc, &renderpass_volumetriclight);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtParticleDistortion, RenderPassAttachment::LoadOp::CLEAR));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer_Main,
				RenderPassAttachment::LoadOp::LOAD,
				RenderPassAttachment::StoreOp::STORE,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY,
				ResourceState::DEPTHSTENCIL_READONLY
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
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtWaterRipple, RenderPassAttachment::LoadOp::CLEAR));

		device->CreateRenderPass(&desc, &renderpass_waterripples);
	}


	// Other resources:
	{
		TextureDesc desc;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		desc.mip_levels = 1;
		desc.array_size = 1;
		desc.format = Format::R8G8B8A8_UNORM;
		desc.sample_count = 1;
		desc.usage = Usage::DEFAULT;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;

		device->CreateTexture(&desc, nullptr, &debugUAV);
		device->SetName(&debugUAV, "debugUAV");
	}
	wiRenderer::CreateTiledLightResources(tiledLightResources, internalResolution);
	wiRenderer::CreateTiledLightResources(tiledLightResources_planarReflection, XMUINT2(depthBuffer_Reflection.desc.width, depthBuffer_Reflection.desc.height));
	wiRenderer::CreateLuminanceResources(luminanceResources, internalResolution);
	wiRenderer::CreateScreenSpaceShadowResources(screenspaceshadowResources, internalResolution);
	wiRenderer::CreateDepthOfFieldResources(depthoffieldResources, internalResolution);
	wiRenderer::CreateMotionBlurResources(motionblurResources, internalResolution);
	wiRenderer::CreateVolumetricCloudResources(volumetriccloudResources, internalResolution);
	wiRenderer::CreateVolumetricCloudResources(volumetriccloudResources_reflection, XMUINT2(depthBuffer_Reflection.desc.width, depthBuffer_Reflection.desc.height));
	wiRenderer::CreateBloomResources(bloomResources, internalResolution);
	wiRenderer::CreateSurfelGIResources(surfelGIResources, internalResolution);

	if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
	{
		wiRenderer::CreateRTShadowResources(rtshadowResources, internalResolution);
	}

	setAO(ao);
	setSSREnabled(ssrEnabled);
	setRaytracedReflectionsEnabled(raytracedReflectionsEnabled);
	setFSREnabled(fsrEnabled);

	RenderPath2D::ResizeBuffers();
}

void RenderPath3D::PreUpdate()
{
	camera_previous = *camera;
	camera_reflection_previous = camera_reflection;
}

void RenderPath3D::Update(float dt)
{
	if (rtMain_render.desc.sample_count != msaaSampleCount)
	{
		ResizeBuffers();
	}

	RenderPath2D::Update(dt);

	if (getSceneUpdateEnabled())
	{
		if (wiRenderer::GetSurfelGIEnabled() ||
			wiRenderer::GetRaytracedShadowsEnabled() ||
			getAO() == AO_RTAO ||
			getRaytracedReflectionEnabled())
		{
			scene->SetAccelerationStructureUpdateRequested(true);
		}

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

	XMUINT2 internalResolution = GetInternalResolution();

	wiRenderer::UpdatePerFrameData(
		*scene,
		visibility_main,
		frameCB,
		getSceneUpdateEnabled() ? dt : 0
	);

	if (wiRenderer::GetTemporalAAEnabled())
	{
		const XMFLOAT4& halton = wiMath::GetHaltonSequence(wiGraphics::GetDevice()->GetFrameCount() % 256);
		camera->jitter.x = (halton.x * 2 - 1) / (float)internalResolution.x;
		camera->jitter.y = (halton.y * 2 - 1) / (float)internalResolution.y;
	}
	else
	{
		camera->jitter = XMFLOAT2(0, 0);
	}
	camera->UpdateCamera();

	if (getAO() != AO_RTAO)
	{
		rtaoResources.frame = 0;
	}
	if (!wiRenderer::GetRaytracedShadowsEnabled())
	{
		rtshadowResources.frame = 0;
	}
	if (!getSSREnabled() && !getRaytracedReflectionEnabled())
	{
		rtSSR = {};
	}
	if (getAO() == AO_DISABLED)
	{
		rtAO = {};
	}
	if (!wiRenderer::GetScreenSpaceShadowsEnabled() && !wiRenderer::GetRaytracedShadowsEnabled())
	{
		rtShadow = {};
	}

	GraphicsDevice* device = wiGraphics::GetDevice();

	if((wiRenderer::GetScreenSpaceShadowsEnabled() || wiRenderer::GetRaytracedShadowsEnabled()) && !rtShadow.IsValid())
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R32G32B32A32_UINT;
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
		device->CreateTexture(&desc, nullptr, &rtShadow);
		device->SetName(&rtShadow, "rtShadow");
	}

	std::swap(depthBuffer_Copy, depthBuffer_Copy1);

	camera->canvas = *this;
	camera->width = (float)internalResolution.x;
	camera->height = (float)internalResolution.y;
	camera->texture_depth_index = device->GetDescriptorIndex(&depthBuffer_Copy, SubresourceType::SRV);
	camera->texture_lineardepth_index = device->GetDescriptorIndex(&rtLinearDepth, SubresourceType::SRV);
	camera->texture_gbuffer0_index = device->GetDescriptorIndex(&rtGbuffer[GBUFFER_PRIMITIVEID], SubresourceType::SRV);
	camera->texture_gbuffer1_index = device->GetDescriptorIndex(&rtGbuffer[GBUFFER_VELOCITY], SubresourceType::SRV);
	camera->buffer_entitytiles_opaque_index = device->GetDescriptorIndex(&tiledLightResources.entityTiles_Opaque, SubresourceType::SRV);
	camera->buffer_entitytiles_transparent_index = device->GetDescriptorIndex(&tiledLightResources.entityTiles_Transparent, SubresourceType::SRV);
	camera->texture_reflection_index = device->GetDescriptorIndex(&rtReflection, SubresourceType::SRV);
	camera->texture_refraction_index = device->GetDescriptorIndex(&rtSceneCopy, SubresourceType::SRV);
	camera->texture_waterriples_index = device->GetDescriptorIndex(&rtWaterRipple, SubresourceType::SRV);
	camera->texture_ao_index = device->GetDescriptorIndex(&rtAO, SubresourceType::SRV);
	camera->texture_ssr_index = device->GetDescriptorIndex(&rtSSR, SubresourceType::SRV);
	camera->texture_rtshadow_index = device->GetDescriptorIndex(&rtShadow, SubresourceType::SRV);
	camera->texture_surfelgi_index = device->GetDescriptorIndex(&surfelGIResources.result, SubresourceType::SRV);

	camera_reflection.canvas = *this;
	camera_reflection.width = (float)depthBuffer_Reflection.desc.width;
	camera_reflection.height = (float)depthBuffer_Reflection.desc.height;
	camera_reflection.texture_depth_index = device->GetDescriptorIndex(&depthBuffer_Reflection, SubresourceType::SRV);
	camera_reflection.texture_lineardepth_index = -1;
	camera_reflection.texture_gbuffer0_index = -1;
	camera_reflection.texture_gbuffer1_index = -1;
	camera_reflection.buffer_entitytiles_opaque_index = device->GetDescriptorIndex(&tiledLightResources_planarReflection.entityTiles_Opaque, SubresourceType::SRV);
	camera_reflection.buffer_entitytiles_transparent_index = device->GetDescriptorIndex(&tiledLightResources_planarReflection.entityTiles_Transparent, SubresourceType::SRV);
	camera_reflection.texture_reflection_index = -1;
	camera_reflection.texture_refraction_index = -1;
	camera_reflection.texture_waterriples_index = -1;
	camera_reflection.texture_ao_index = -1;
	camera_reflection.texture_ssr_index = -1;
	camera_reflection.texture_rtshadow_index = -1;
	camera_reflection.texture_surfelgi_index = -1;
}

void RenderPath3D::Render() const
{
	GraphicsDevice* device = wiGraphics::GetDevice();
	wiJobSystem::context ctx;

	// Preparing the frame:
	CommandList cmd = device->BeginCommandList();
	CommandList cmd_prepareframe = cmd;
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {
		wiRenderer::BindCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);
		wiRenderer::UpdateRenderData(visibility_main, frameCB, cmd);
		});

	//	async compute parallel with depth prepass
	cmd = device->BeginCommandList(QUEUE_COMPUTE);
	device->WaitCommandList(cmd, cmd_prepareframe);
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiGraphics::GetDevice();

		wiRenderer::BindCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);
		wiRenderer::UpdateRenderDataAsync(visibility_main, frameCB, cmd);

		if (scene->IsAccelerationStructureUpdateRequested())
		{
			wiRenderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
		}

		if (wiRenderer::GetSurfelGIEnabled())
		{
			wiRenderer::SurfelGI(
				surfelGIResources,
				*scene,
				cmd
			);
		}

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

	// Main camera depth prepass + occlusion culling:
	cmd = device->BeginCommandList();
	CommandList cmd_maincamera_prepass = cmd;
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiGraphics::GetDevice();

		wiRenderer::BindCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);

		wiRenderer::OcclusionCulling_Reset(visibility_main, cmd); // must be outside renderpass!

		device->RenderPassBegin(&renderpass_depthprepass, cmd);

		device->EventBegin("Opaque Z-prepass", cmd);
		auto range = wiProfiler::BeginRangeGPU("Z-Prepass", cmd);

		Viewport vp;
		vp.width = (float)depthBuffer_Main.GetDesc().width;
		vp.height = (float)depthBuffer_Main.GetDesc().height;
		device->BindViewports(1, &vp, cmd);
		wiRenderer::DrawScene(visibility_main, RENDERPASS_PREPASS, cmd, drawscene_flags);

		wiProfiler::EndRange(range);
		device->EventEnd(cmd);

		if (getOcclusionCullingEnabled())
		{
			wiRenderer::OcclusionCulling_Render(*camera, visibility_main, cmd);
		}

		device->RenderPassEnd(cmd);

		wiRenderer::OcclusionCulling_Resolve(visibility_main, cmd); // must be outside renderpass!

		});

	// Main camera compute effects:
	//	(async compute, parallel to "shadow maps" and "update textures",
	//	must finish before "main scene opaque color pass")
	cmd = device->BeginCommandList(QUEUE_COMPUTE);
	device->WaitCommandList(cmd, cmd_maincamera_prepass);
	CommandList cmd_maincamera_compute_effects = cmd;
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiGraphics::GetDevice();

		wiRenderer::BindCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);

		wiRenderer::VisibilityResolve(
			depthBuffer_Main,
			rtPrimitiveID_render,
			rtGbuffer,
			depthBuffer_Copy,
			rtLinearDepth,
			cmd
		);

		if (wiRenderer::GetSurfelGIEnabled())
		{
			wiRenderer::SurfelGI_Coverage(
				surfelGIResources,
				*scene,
				debugUAV,
				cmd
			);
		}

		RenderAO(cmd);

		if (wiRenderer::GetVariableRateShadingClassification() && device->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2))
		{
			wiRenderer::ComputeShadingRateClassification(
				rtShadingRate,
				debugUAV,
				cmd
			);
		}

		if (scene->weather.IsVolumetricClouds())
		{
			wiRenderer::Postprocess_VolumetricClouds(
				volumetriccloudResources,
				cmd
			);
		}

		{
			auto range = wiProfiler::BeginRangeGPU("Entity Culling", cmd);
			wiRenderer::ComputeTiledLightCulling(
				tiledLightResources,
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
				tiledLightResources.entityTiles_Opaque,
				rtShadow,
				cmd
			);
		}

		});

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
		wiRenderer::BindCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);
		wiRenderer::RefreshLightmaps(*scene, cmd);
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

	if (visibility_main.IsRequestedPlanarReflections())
	{
		// Planar reflections depth prepass:
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [cmd, this](wiJobArgs args) {

			GraphicsDevice* device = wiGraphics::GetDevice();

			wiRenderer::BindCameraCB(
				camera_reflection,
				camera_reflection_previous,
				camera_reflection,
				cmd
			);

			device->EventBegin("Planar reflections Z-Prepass", cmd);
			auto range = wiProfiler::BeginRangeGPU("Planar Reflections Z-Prepass", cmd);

			Viewport vp;
			vp.width = (float)depthBuffer_Reflection.GetDesc().width;
			vp.height = (float)depthBuffer_Reflection.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			device->RenderPassBegin(&renderpass_reflection_depthprepass, cmd);

			wiRenderer::DrawScene(visibility_reflection, RENDERPASS_PREPASS, cmd, drawscene_flags_reflections);

			device->RenderPassEnd(cmd);

			if (scene->weather.IsVolumetricClouds())
			{
				wiRenderer::Postprocess_VolumetricClouds(
					volumetriccloudResources_reflection,
					cmd
				);
			}

			wiProfiler::EndRange(range); // Planar Reflections
			device->EventEnd(cmd);

			});

		// Planar reflections opaque color pass:
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [cmd, this](wiJobArgs args) {

			GraphicsDevice* device = wiGraphics::GetDevice();

			wiRenderer::BindCameraCB(
				camera_reflection,
				camera_reflection_previous,
				camera_reflection,
				cmd
			);

			wiRenderer::ComputeTiledLightCulling(
				tiledLightResources_planarReflection,
				Texture(),
				cmd
			);

			device->EventBegin("Planar reflections", cmd);
			auto range = wiProfiler::BeginRangeGPU("Planar Reflections", cmd);

			Viewport vp;
			vp.width = (float)depthBuffer_Reflection.GetDesc().width;
			vp.height = (float)depthBuffer_Reflection.GetDesc().height;
			device->BindViewports(1, &vp, cmd);


			device->RenderPassBegin(&renderpass_reflection, cmd);

			wiRenderer::DrawScene(visibility_reflection, RENDERPASS_MAIN, cmd, drawscene_flags_reflections);
			wiRenderer::DrawSky(*scene, cmd);

			// Blend the volumetric clouds on top:
			if (scene->weather.IsVolumetricClouds())
			{
				device->EventBegin("Volumetric Clouds Reflection Blend", cmd);
				wiImageParams fx;
				fx.enableFullScreen();
				wiImage::Draw(&volumetriccloudResources_reflection.texture_temporal[device->GetFrameCount() % 2], fx, cmd);
				device->EventEnd(cmd);
			}

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Planar Reflections
			device->EventEnd(cmd);
			});
	}

	// Main camera opaque color pass:
	cmd = device->BeginCommandList();
	device->WaitCommandList(cmd, cmd_maincamera_compute_effects);
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiGraphics::GetDevice();
		device->EventBegin("Opaque Scene", cmd);

		wiRenderer::BindCameraCB(
			*camera,
			camera_previous,
			camera_reflection,
			cmd
		);

		// This can't run in "main camera compute effects" async compute,
		//	because it depends on shadow maps, and envmaps
		if (getRaytracedReflectionEnabled())
		{
			wiRenderer::Postprocess_RTReflection(
				rtreflectionResources,
				*scene,
				rtSSR,
				cmd
			);
		}

		// Depth buffers were created on COMPUTE queue, so make them available for pixel shaders here:
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&rtLinearDepth, rtLinearDepth.desc.layout, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Image(&depthBuffer_Copy, depthBuffer_Copy.desc.layout, ResourceState::SHADER_RESOURCE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

		Viewport vp;
		vp.width = (float)depthBuffer_Main.GetDesc().width;
		vp.height = (float)depthBuffer_Main.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		if (wiRenderer::GetRaytracedShadowsEnabled() || wiRenderer::GetScreenSpaceShadowsEnabled())
		{
			GPUBarrier barrier = GPUBarrier::Image(&rtShadow, rtShadow.desc.layout, ResourceState::SHADER_RESOURCE);
			device->Barrier(&barrier, 1, cmd);
		}

		device->RenderPassBegin(&renderpass_main, cmd);

		wiRenderer::DrawScene(visibility_main, RENDERPASS_MAIN, cmd, drawscene_flags);
		wiRenderer::DrawSky(*scene, cmd);

		wiProfiler::EndRange(range); // Opaque Scene

		RenderOutline(cmd);

		// Upsample + Blend the volumetric clouds on top:
		if (scene->weather.IsVolumetricClouds())
		{
			device->EventBegin("Volumetric Clouds Upsample + Blend", cmd);
			wiRenderer::Postprocess_Upsample_Bilateral(
				volumetriccloudResources.texture_temporal[device->GetFrameCount() % 2],
				rtLinearDepth,
				rtMain_render, // only desc is taken if pixel shader upsampling is used
				cmd,
				true // pixel shader upsampling
			);
			device->EventEnd(cmd);
		}

		device->RenderPassEnd(cmd);

		if (wiRenderer::GetRaytracedShadowsEnabled() || wiRenderer::GetScreenSpaceShadowsEnabled())
		{
			GPUBarrier barrier = GPUBarrier::Image(&rtShadow, ResourceState::SHADER_RESOURCE, rtShadow.desc.layout);
			device->Barrier(&barrier, 1, cmd);
		}

		device->EventEnd(cmd);
		});

	// Transparents, post processes, etc:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiGraphics::GetDevice();

		wiRenderer::BindCameraCB(
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

		// Depth buffers expect a non-pixel shader resource state as they are generated on compute queue:
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&rtLinearDepth, ResourceState::SHADER_RESOURCE, rtLinearDepth.desc.layout),
				GPUBarrier::Image(&depthBuffer_Copy, ResourceState::SHADER_RESOURCE, depthBuffer_Copy.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}

void RenderPath3D::Compose(CommandList cmd) const
{
	GraphicsDevice* device = wiGraphics::GetDevice();

	wiImageParams fx;
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_LINEAR;
	fx.enableFullScreen();

	device->EventBegin("Composition", cmd);
	wiImage::Draw(GetLastPostprocessRT(), fx, cmd);
	device->EventEnd(cmd);

	if (
		wiRenderer::GetDebugLightCulling() ||
		wiRenderer::GetVariableRateShadingClassificationDebug() ||
		wiRenderer::GetSurfelGIDebugEnabled()
		)
	{
		fx.enableFullScreen();
		fx.blendFlag = BLENDMODE_PREMULTIPLIED;
		wiImage::Draw(&debugUAV, fx, cmd);
	}

	RenderPath2D::Compose(cmd);
}

void RenderPath3D::RenderAO(CommandList cmd) const
{

	if (getAOEnabled())
	{
		switch (getAO())
		{
		case AO_SSAO:
			wiRenderer::Postprocess_SSAO(
				ssaoResources,
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
				rtAO,
				cmd,
				getAORange(),
				getAOPower()
			);
			break;
		}
	}
}
void RenderPath3D::RenderSSR(CommandList cmd) const
{
	if (getSSREnabled() && !getRaytracedReflectionEnabled())
	{
		wiRenderer::Postprocess_SSR(
			ssrResources,
			rtSceneCopy,
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
		GraphicsDevice* device = wiGraphics::GetDevice();

		device->EventBegin("Light Shafts", cmd);

		// Render sun stencil cutout:
		{
			device->RenderPassBegin(&renderpass_lightshafts, cmd);

			Viewport vp;
			vp.width = (float)depthBuffer_Main.GetDesc().width;
			vp.height = (float)depthBuffer_Main.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawSun(cmd);

			if (scene->weather.IsVolumetricClouds())
			{
				device->EventBegin("Volumetric cloud occlusion mask", cmd);
				wiImageParams fx;
				fx.enableFullScreen();
				fx.blendFlag = BLENDMODE_MULTIPLY;
				wiImage::Draw(&volumetriccloudResources.texture_cloudMask, fx, cmd);
				device->EventEnd(cmd);
			}

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

		GraphicsDevice* device = wiGraphics::GetDevice();

		device->RenderPassBegin(&renderpass_volumetriclight, cmd);

		Viewport vp;
		vp.width = (float)rtVolumetricLights[0].GetDesc().width;
		vp.height = (float)rtVolumetricLights[0].GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawVolumeLights(visibility_main, cmd);

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
	GraphicsDevice* device = wiGraphics::GetDevice();

	auto range = wiProfiler::BeginRangeGPU("Scene MIP Chain", cmd);
	device->EventBegin("RenderSceneMIPChain", cmd);

	device->RenderPassBegin(&renderpass_downsamplescene, cmd);

	Viewport vp;
	vp.width = (float)rtSceneCopy.GetDesc().width;
	vp.height = (float)rtSceneCopy.GetDesc().height;
	device->BindViewports(1, &vp, cmd);

	wiImageParams fx;
	fx.enableFullScreen();
	fx.sampleFlag = SAMPLEMODE_CLAMP;
	fx.quality = QUALITY_LINEAR;
	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(&rtMain, fx, cmd);

	device->RenderPassEnd(cmd);

	wiRenderer::MIPGEN_OPTIONS mipopt;
	mipopt.gaussian_temp = &rtSceneCopy_tmp;
	wiRenderer::GenerateMipChain(rtSceneCopy, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd, mipopt);

	device->EventEnd(cmd);
	wiProfiler::EndRange(range);
}
void RenderPath3D::RenderTransparents(CommandList cmd) const
{
	GraphicsDevice* device = wiGraphics::GetDevice();

	// Water ripple rendering:
	if(!scene->waterRipples.empty())
	{
		device->RenderPassBegin(&renderpass_waterripples, cmd);

		Viewport vp;
		vp.width = (float)rtWaterRipple.GetDesc().width;
		vp.height = (float)rtWaterRipple.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawWaterRipples(visibility_main, cmd);

		device->RenderPassEnd(cmd);
	}


	device->RenderPassBegin(&renderpass_transparent, cmd);

	Viewport vp;
	vp.width = (float)depthBuffer_Main.GetDesc().width;
	vp.height = (float)depthBuffer_Main.GetDesc().height;
	device->BindViewports(1, &vp, cmd);

	// Transparent scene
	{
		auto range = wiProfiler::BeginRangeGPU("Transparent Scene", cmd);
		device->EventBegin("Transparent Scene", cmd);

		uint32_t drawscene_flags = 0;
		drawscene_flags |= wiRenderer::DRAWSCENE_TRANSPARENT;
		drawscene_flags |= wiRenderer::DRAWSCENE_OCCLUSIONCULLING;
		drawscene_flags |= wiRenderer::DRAWSCENE_HAIRPARTICLE;
		drawscene_flags |= wiRenderer::DRAWSCENE_TESSELLATION;
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

	XMVECTOR sunDirection = XMLoadFloat3(&scene->weather.sunDirection);
	if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, camera->GetAt())) > 0)
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
		wiRenderer::DrawLensFlares(
			visibility_main,
			cmd,
			scene->weather.IsVolumetricClouds() ? &volumetriccloudResources.texture_cloudMask : nullptr
		);
	}

	wiRenderer::DrawDebugWorld(*scene, *camera, *this, cmd);

	device->RenderPassEnd(cmd);

	// Distortion particles:
	{
		device->RenderPassBegin(&renderpass_particledistortion, cmd);

		Viewport vp;
		vp.width = (float)rtParticleDistortion.GetDesc().width;
		vp.height = (float)rtParticleDistortion.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::DrawSoftParticles(visibility_main, rtLinearDepth, true, cmd);

		device->RenderPassEnd(cmd);
	}
}
void RenderPath3D::RenderPostprocessChain(CommandList cmd) const
{
	GraphicsDevice* device = wiGraphics::GetDevice();

	const Texture* rt_first = nullptr; // not ping-ponged with read / write
	const Texture* rt_read = &rtMain;
	const Texture* rt_write = &rtPostprocess;

	// 1.) HDR post process chain
	{
		if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
		{
			GraphicsDevice* device = wiGraphics::GetDevice();

			int output = device->GetFrameCount() % 2;
			int history = 1 - output;
			wiRenderer::Postprocess_TemporalAA(
				*rt_read,
				rtTemporalAA[history],
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
				cmd,
				getDepthOfFieldStrength()
			);

			rt_first = nullptr;
			std::swap(rt_read, rt_write);
		}

		if (getMotionBlurEnabled() && getMotionBlurStrength() > 0)
		{
			wiRenderer::Postprocess_MotionBlur(
				motionblurResources,
				rt_first == nullptr ? *rt_read : *rt_first,
				*rt_write,
				cmd,
				getMotionBlurStrength()
			);

			rt_first = nullptr;
			std::swap(rt_read, rt_write);
		}
	}

	// 2.) Tone mapping HDR -> LDR
	{
		// Bloom and eye adaption is not part of post process "chain",
		//	because they will be applied to the screen in tonemap
		if (getEyeAdaptionEnabled())
		{
			wiRenderer::ComputeLuminance(
				luminanceResources,
				rt_first == nullptr ? *rt_read : *rt_first,
				cmd,
				getEyeAdaptionRate(),
				getEyeAdaptionKey()
				);
		}
		if (getBloomEnabled())
		{
			wiRenderer::ComputeBloom(
				bloomResources,
				rt_first == nullptr ? *rt_read : *rt_first,
				cmd,
				getBloomThreshold(),
				getExposure(),
				getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr
			);
		}

		wiRenderer::Postprocess_Tonemap(
			rt_first == nullptr ? *rt_read : *rt_first,
			*rt_write,
			cmd,
			getExposure(),
			getDitherEnabled(),
			getColorGradingEnabled() ? (scene->weather.colorGradingMap == nullptr ? nullptr : &scene->weather.colorGradingMap->texture) : nullptr,
			getMSAASampleCount() > 1 ? &rtParticleDistortion_Resolved : &rtParticleDistortion,
			getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr,
			getBloomEnabled() ? &bloomResources.texture_bloom : nullptr,
			colorspace
		);

		rt_first = nullptr;
		std::swap(rt_read, rt_write);
	}

	// 3.) LDR post process chain
	{
		if (getSharpenFilterEnabled())
		{
			wiRenderer::Postprocess_Sharpen(*rt_read, *rt_write, cmd, getSharpenFilterAmount());

			std::swap(rt_read, rt_write);
		}

		if (getFXAAEnabled())
		{
			wiRenderer::Postprocess_FXAA(*rt_read, *rt_write, cmd);

			std::swap(rt_read, rt_write);
		}

		if (getChromaticAberrationEnabled())
		{
			wiRenderer::Postprocess_Chromatic_Aberration(*rt_read, *rt_write, cmd, getChromaticAberrationAmount());

			std::swap(rt_read, rt_write);
		}

		lastPostprocessRT = rt_read;

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

		if (rtFSR[0].IsValid() && getFSREnabled())
		{
			wiRenderer::Postprocess_FSR(*rt_read, rtFSR[1], rtFSR[0], cmd, getFSRSharpness());
			lastPostprocessRT = &rtFSR[0];
		}
	}
}

void RenderPath3D::setAO(AO value)
{
	ao = value;

	rtAO = {};
	ssaoResources = {};
	msaoResources = {};
	rtaoResources = {};

	if (ao == AO_DISABLED)
	{
		return;
	}

	XMUINT2 internalResolution = GetInternalResolution();

	TextureDesc desc;
	desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
	desc.format = Format::R8_UNORM;
	desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;

	switch (ao)
	{
	case RenderPath3D::AO_SSAO:
	case RenderPath3D::AO_HBAO:
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		wiRenderer::CreateSSAOResources(ssaoResources, internalResolution);
		break;
	case RenderPath3D::AO_MSAO:
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		wiRenderer::CreateMSAOResources(msaoResources, internalResolution);
		break;
	case RenderPath3D::AO_RTAO:
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		wiRenderer::CreateRTAOResources(rtaoResources, internalResolution);
		break;
	default:
		break;
	}

	GraphicsDevice* device = wiGraphics::GetDevice();
	device->CreateTexture(&desc, nullptr, &rtAO);
	device->SetName(&rtAO, "rtAO");
}

void RenderPath3D::setSSREnabled(bool value)
{
	ssrEnabled = value;

	if (value)
	{
		GraphicsDevice* device = wiGraphics::GetDevice();
		XMUINT2 internalResolution = GetInternalResolution();

		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R16G16B16A16_FLOAT;
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		device->CreateTexture(&desc, nullptr, &rtSSR);
		device->SetName(&rtSSR, "rtSSR");

		wiRenderer::CreateSSRResources(ssrResources, internalResolution);
	}
	else
	{
		ssrResources = {};
	}
}

void RenderPath3D::setRaytracedReflectionsEnabled(bool value)
{
	raytracedReflectionsEnabled = value;

	if (value)
	{
		GraphicsDevice* device = wiGraphics::GetDevice();
		XMUINT2 internalResolution = GetInternalResolution();

		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x / 2;
		desc.height = internalResolution.y / 2;
		device->CreateTexture(&desc, nullptr, &rtSSR);
		device->SetName(&rtSSR, "rtSSR");

		wiRenderer::CreateRTReflectionResources(rtreflectionResources, internalResolution);
	}
	else
	{
		rtreflectionResources = {};
	}
}

void RenderPath3D::setFSREnabled(bool value)
{
	fsrEnabled = value;

	if (resolutionScale < 1.0f && fsrEnabled)
	{
		GraphicsDevice* device = wiGraphics::GetDevice();

		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = rtPostprocess.desc.format;
		desc.width = GetPhysicalWidth();
		desc.height = GetPhysicalHeight();
		device->CreateTexture(&desc, nullptr, &rtFSR[0]);
		device->SetName(&rtFSR[0], "rtFSR[0]");
		device->CreateTexture(&desc, nullptr, &rtFSR[1]);
		device->SetName(&rtFSR[1], "rtFSR[1]");
	}
	else
	{
		rtFSR[0] = {};
		rtFSR[1] = {};
	}
}
