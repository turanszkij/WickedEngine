#include "wiRenderPath3D.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiProfiler.h"

using namespace wi::graphics;
using namespace wi::enums;

namespace wi
{

	void RenderPath3D::DeleteGPUResources()
	{
		rtMain = {};
		rtMain_render = {};
		rtPrimitiveID = {};
		rtPrimitiveID_render = {};
		rtVelocity = {};
		rtReflection = {};
		rtRaytracedDiffuse = {};
		rtSSR = {};
		rtSceneCopy = {};
		rtSceneCopy_tmp = {};
		rtWaterRipple = {};
		rtParticleDistortion = {};
		rtParticleDistortion_Resolved = {};
		rtVolumetricLights[0] = {};
		rtVolumetricLights[1] = {};
		rtBloom = {};
		rtBloom_tmp = {};
		rtAO = {};
		rtShadow = {};
		rtSun[0] = {};
		rtSun[1] = {};
		rtSun_resolved = {};
		rtGUIBlurredBackground[0] = {};
		rtGUIBlurredBackground[1] = {};
		rtGUIBlurredBackground[2] = {};
		rtShadingRate = {};
		rtFSR[0] = {};
		rtFSR[1] = {};
		rtOutlineSource = {};

		rtPostprocess = {};

		depthBuffer_Main = {};
		depthBuffer_Copy = {};
		depthBuffer_Copy1 = {};
		depthBuffer_Reflection = {};
		rtLinearDepth = {};

		renderpass_depthprepass = {};
		renderpass_main = {};
		renderpass_transparent = {};
		renderpass_reflection_depthprepass = {};
		renderpass_reflection = {};
		renderpass_lightshafts = {};
		renderpass_volumetriclight = {};
		renderpass_particledistortion = {};
		renderpass_waterripples = {};
		renderpass_outline_source = {};

		debugUAV = {};
		tiledLightResources = {};
		tiledLightResources_planarReflection = {};
		luminanceResources = {};
		ssaoResources = {};
		msaoResources = {};
		rtaoResources = {};
		rtdiffuseResources = {};
		rtreflectionResources = {};
		ssrResources = {};
		rtshadowResources = {};
		screenspaceshadowResources = {};
		depthoffieldResources = {};
		motionblurResources = {};
		volumetriccloudResources = {};
		volumetriccloudResources_reflection = {};
		bloomResources = {};
		surfelGIResources = {};
		temporalAAResources = {};
		visibilityResources = {};
	}

	void RenderPath3D::ResizeBuffers()
	{
		DeleteGPUResources();

		GraphicsDevice* device = wi::graphics::GetDevice();

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
			desc.format = Format::R32_UINT;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			desc.sample_count = 1;
			desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			device->CreateTexture(&desc, nullptr, &rtPrimitiveID);
			device->SetName(&rtPrimitiveID, "rtPrimitiveID");

			if (getMSAASampleCount() > 1)
			{
				desc.sample_count = getMSAASampleCount();
				desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
				device->CreateTexture(&desc, nullptr, &rtPrimitiveID_render);
				device->SetName(&rtPrimitiveID_render, "rtPrimitiveID_render");
			}
			else
			{
				rtPrimitiveID_render = rtPrimitiveID;
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
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R11G11B10_FLOAT;
			desc.width = internalResolution.x / 4;
			desc.height = internalResolution.y / 4;
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
		if (device->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2) &&
			wi::renderer::GetVariableRateShadingClassification())
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

		// Depth buffers:
		{
			TextureDesc desc;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;

			desc.sample_count = getMSAASampleCount();
			desc.layout = ResourceState::DEPTHSTENCIL_READONLY;
			desc.format = Format::D32_FLOAT_S8X24_UINT;
			desc.bind_flags = BindFlag::DEPTH_STENCIL;
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
					depthBuffer_Main,
					RenderPassAttachment::LoadOp::CLEAR,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			desc.attachments.push_back(
				RenderPassAttachment::RenderTarget(
					rtPrimitiveID_render,
					RenderPassAttachment::LoadOp::CLEAR,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::SHADER_RESOURCE_COMPUTE,
					ResourceState::RENDERTARGET,
					ResourceState::SHADER_RESOURCE_COMPUTE
				)
			);
			device->CreateRenderPass(&desc, &renderpass_depthprepass);

			desc.attachments.clear();
			desc.attachments.push_back(
				RenderPassAttachment::RenderTarget(
					rtMain_render,
					RenderPassAttachment::LoadOp::LOAD
				)
			);
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					depthBuffer_Main,
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			if (getMSAASampleCount() > 1)
			{
				desc.attachments.push_back(RenderPassAttachment::Resolve(rtMain));
			}

			if (device->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2) && rtShadingRate.IsValid())
			{
				desc.attachments.push_back(RenderPassAttachment::ShadingRateSource(rtShadingRate, ResourceState::UNORDERED_ACCESS, ResourceState::UNORDERED_ACCESS));
			}

			device->CreateRenderPass(&desc, &renderpass_main);
		}
		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(rtMain_render, RenderPassAttachment::LoadOp::LOAD));
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					depthBuffer_Main,
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			if (getMSAASampleCount() > 1)
			{
				desc.attachments.push_back(RenderPassAttachment::Resolve(rtMain));
			}
			device->CreateRenderPass(&desc, &renderpass_transparent);
		}
		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(rtParticleDistortion, RenderPassAttachment::LoadOp::CLEAR));
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					depthBuffer_Main,
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);

			if (getMSAASampleCount() > 1)
			{
				desc.attachments.push_back(RenderPassAttachment::Resolve(rtParticleDistortion_Resolved));
			}

			device->CreateRenderPass(&desc, &renderpass_particledistortion);
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
		wi::renderer::CreateVisibilityResources(visibilityResources, internalResolution);
		wi::renderer::CreateTiledLightResources(tiledLightResources, internalResolution);
		wi::renderer::CreateScreenSpaceShadowResources(screenspaceshadowResources, internalResolution);

		// These can trigger resource creations if needed:
		setAO(ao);
		setSSREnabled(ssrEnabled);
		setRaytracedReflectionsEnabled(raytracedReflectionsEnabled);
		setRaytracedDiffuseEnabled(raytracedDiffuseEnabled);
		setFSREnabled(fsrEnabled);
		setMotionBlurEnabled(motionBlurEnabled);
		setDepthOfFieldEnabled(depthOfFieldEnabled);
		setEyeAdaptionEnabled(eyeAdaptionEnabled);
		setReflectionsEnabled(reflectionsEnabled);
		setBloomEnabled(bloomEnabled);
		setVolumeLightsEnabled(volumeLightsEnabled);
		setLightShaftsEnabled(lightShaftsEnabled);
		setOutlineEnabled(outlineEnabled);

		RenderPath2D::ResizeBuffers();
	}

	void RenderPath3D::PreUpdate()
	{
		camera_previous = *camera;
		camera_reflection_previous = camera_reflection;
	}

	void RenderPath3D::Update(float dt)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		if (rtMain_render.desc.sample_count != msaaSampleCount)
		{
			ResizeBuffers();
		}

		RenderPath2D::Update(dt);

		const bool hw_raytrace = device->CheckCapability(GraphicsDeviceCapability::RAYTRACING);
		if (getSceneUpdateEnabled())
		{
			if (wi::renderer::GetSurfelGIEnabled() ||
				wi::renderer::GetDDGIEnabled() ||
				(hw_raytrace && wi::renderer::GetRaytracedShadowsEnabled()) ||
				(hw_raytrace && getAO() == AO_RTAO) ||
				(hw_raytrace && getRaytracedReflectionEnabled()) ||
				(hw_raytrace && getRaytracedDiffuseEnabled())
				)
			{
				scene->SetAccelerationStructureUpdateRequested(true);
			}

			scene->camera = *camera;
			scene->Update(dt * wi::renderer::GetGameSpeed());
		}

		// Frustum culling for main camera:
		visibility_main.layerMask = getLayerMask();
		visibility_main.scene = scene;
		visibility_main.camera = camera;
		visibility_main.flags = wi::renderer::Visibility::ALLOW_EVERYTHING;
		wi::renderer::UpdateVisibility(visibility_main);

		if (visibility_main.planar_reflection_visible)
		{
			// Frustum culling for planar reflections:
			camera_reflection = *camera;
			camera_reflection.jitter = XMFLOAT2(0, 0);
			camera_reflection.Reflect(visibility_main.reflectionPlane);
			visibility_reflection.layerMask = getLayerMask();
			visibility_reflection.scene = scene;
			visibility_reflection.camera = &camera_reflection;
			visibility_reflection.flags = wi::renderer::Visibility::ALLOW_OBJECTS;
			wi::renderer::UpdateVisibility(visibility_reflection);
		}

		XMUINT2 internalResolution = GetInternalResolution();

		wi::renderer::UpdatePerFrameData(
			*scene,
			visibility_main,
			frameCB,
			getSceneUpdateEnabled() ? dt : 0
		);

		if (wi::renderer::GetTemporalAAEnabled())
		{
			const XMFLOAT4& halton = wi::math::GetHaltonSequence(wi::graphics::GetDevice()->GetFrameCount() % 256);
			camera->jitter.x = (halton.x * 2 - 1) / (float)internalResolution.x;
			camera->jitter.y = (halton.y * 2 - 1) / (float)internalResolution.y;
			if (!temporalAAResources.IsValid())
			{
				wi::renderer::CreateTemporalAAResources(temporalAAResources, internalResolution);
			}
		}
		else
		{
			camera->jitter = XMFLOAT2(0, 0);
			temporalAAResources = {};
		}
		camera->UpdateCamera();

		if (getAO() != AO_RTAO)
		{
			rtaoResources.frame = 0;
		}
		if (!wi::renderer::GetRaytracedShadowsEnabled())
		{
			rtshadowResources.frame = 0;
		}
		if (!getSSREnabled() && !getRaytracedReflectionEnabled())
		{
			rtSSR = {};
		}
		if (!getRaytracedDiffuseEnabled())
		{
			rtRaytracedDiffuse = {};
		}
		if (getAO() == AO_DISABLED)
		{
			rtAO = {};
		}

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			if (!rtshadowResources.denoised.IsValid())
			{
				wi::renderer::CreateRTShadowResources(rtshadowResources, internalResolution);
			}
		}
		else
		{
			rtshadowResources = {};
		}

		if (scene->weather.IsVolumetricClouds())
		{
			if (!volumetriccloudResources.texture_cloudRender.IsValid())
			{
				wi::renderer::CreateVolumetricCloudResources(volumetriccloudResources, internalResolution);
			}
			if (getReflectionsEnabled() && depthBuffer_Reflection.IsValid())
			{
				if (!volumetriccloudResources_reflection.texture_cloudRender.IsValid())
				{
					wi::renderer::CreateVolumetricCloudResources(volumetriccloudResources_reflection, XMUINT2(depthBuffer_Reflection.desc.width, depthBuffer_Reflection.desc.height));
				}
			}
			else
			{
				volumetriccloudResources_reflection = {};
			}
		}

		if (!scene->waterRipples.empty())
		{
			if (!rtWaterRipple.IsValid())
			{
				TextureDesc desc;
				desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
				desc.format = Format::R16G16_FLOAT;
				desc.width = internalResolution.x / 8;
				desc.height = internalResolution.y / 8;
				device->CreateTexture(&desc, nullptr, &rtWaterRipple);
				device->SetName(&rtWaterRipple, "rtWaterRipple");
				{
					RenderPassDesc desc;
					desc.attachments.push_back(RenderPassAttachment::RenderTarget(rtWaterRipple, RenderPassAttachment::LoadOp::CLEAR));

					device->CreateRenderPass(&desc, &renderpass_waterripples);
				}
			}
		}
		else
		{
			rtWaterRipple = {};
			renderpass_waterripples = {};
		}

		if (wi::renderer::GetSurfelGIEnabled())
		{
			if (!surfelGIResources.result.IsValid())
			{
				wi::renderer::CreateSurfelGIResources(surfelGIResources, internalResolution);
			}
		}

		// Check whether velocity buffer is required:
		if (
			getMotionBlurEnabled() ||
			wi::renderer::GetTemporalAAEnabled() ||
			getSSREnabled() ||
			getRaytracedReflectionEnabled() ||
			getRaytracedDiffuseEnabled() ||
			wi::renderer::GetRaytracedShadowsEnabled() ||
			getAO() == AO::AO_RTAO ||
			wi::renderer::GetVariableRateShadingClassification()
			)
		{
			if (!rtVelocity.IsValid())
			{
				TextureDesc desc;
				desc.format = Format::R16G16_FLOAT;
				desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				desc.width = internalResolution.x;
				desc.height = internalResolution.y;
				desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
				device->CreateTexture(&desc, nullptr, &rtVelocity);
				device->SetName(&rtVelocity, "rtVelocity");
			}
		}
		else
		{
			rtVelocity = {};
		}

		// Check whether shadow mask is required:
		if (wi::renderer::GetScreenSpaceShadowsEnabled() || wi::renderer::GetRaytracedShadowsEnabled())
		{
			if (!rtShadow.IsValid())
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
		}
		else
		{
			rtShadow = {};
		}

		// Keep a copy of last frame's depth buffer for temporal disocclusion checks, so swap with current one every frame:
		std::swap(depthBuffer_Copy, depthBuffer_Copy1);

		visibilityResources.depthbuffer = &depthBuffer_Copy;
		visibilityResources.lineardepth = &rtLinearDepth;
		if (getMSAASampleCount() > 1)
		{
			visibilityResources.primitiveID_resolved = &rtPrimitiveID;
		}
		else
		{
			visibilityResources.primitiveID_resolved = nullptr;
		}

		camera->canvas.init(*this);
		camera->width = (float)internalResolution.x;
		camera->height = (float)internalResolution.y;
		camera->sample_count = depthBuffer_Main.desc.sample_count;
		camera->texture_primitiveID_index = device->GetDescriptorIndex(&rtPrimitiveID, SubresourceType::SRV);
		camera->texture_depth_index = device->GetDescriptorIndex(&depthBuffer_Copy, SubresourceType::SRV);
		camera->texture_lineardepth_index = device->GetDescriptorIndex(&rtLinearDepth, SubresourceType::SRV);
		camera->texture_velocity_index = device->GetDescriptorIndex(&rtVelocity, SubresourceType::SRV);
		camera->texture_normal_index = device->GetDescriptorIndex(&visibilityResources.texture_normals, SubresourceType::SRV);
		camera->texture_roughness_index = device->GetDescriptorIndex(&visibilityResources.texture_roughness, SubresourceType::SRV);
		camera->buffer_entitytiles_opaque_index = device->GetDescriptorIndex(&tiledLightResources.entityTiles_Opaque, SubresourceType::SRV);
		camera->buffer_entitytiles_transparent_index = device->GetDescriptorIndex(&tiledLightResources.entityTiles_Transparent, SubresourceType::SRV);
		camera->texture_reflection_index = device->GetDescriptorIndex(&rtReflection, SubresourceType::SRV);
		camera->texture_refraction_index = device->GetDescriptorIndex(&rtSceneCopy, SubresourceType::SRV);
		camera->texture_waterriples_index = device->GetDescriptorIndex(&rtWaterRipple, SubresourceType::SRV);
		camera->texture_ao_index = device->GetDescriptorIndex(&rtAO, SubresourceType::SRV);
		camera->texture_ssr_index = device->GetDescriptorIndex(&rtSSR, SubresourceType::SRV);
		camera->texture_rtshadow_index = device->GetDescriptorIndex(&rtShadow, SubresourceType::SRV);
		camera->texture_rtdiffuse_index = device->GetDescriptorIndex(&rtRaytracedDiffuse, SubresourceType::SRV);
		camera->texture_surfelgi_index = device->GetDescriptorIndex(&surfelGIResources.result, SubresourceType::SRV);

		camera_reflection.canvas.init(*this);
		camera_reflection.width = (float)depthBuffer_Reflection.desc.width;
		camera_reflection.height = (float)depthBuffer_Reflection.desc.height;
		camera_reflection.sample_count = depthBuffer_Reflection.desc.sample_count;
		camera_reflection.texture_primitiveID_index = -1;
		camera_reflection.texture_depth_index = device->GetDescriptorIndex(&depthBuffer_Reflection, SubresourceType::SRV);
		camera_reflection.texture_lineardepth_index = -1;
		camera_reflection.texture_velocity_index = -1;
		camera_reflection.texture_normal_index = -1;
		camera_reflection.texture_roughness_index = -1;
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
		GraphicsDevice* device = wi::graphics::GetDevice();
		wi::jobsystem::context ctx;

		// Preparing the frame:
		CommandList cmd = device->BeginCommandList();
		CommandList cmd_prepareframe = cmd;
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {
			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);
			wi::renderer::UpdateRenderData(visibility_main, frameCB, cmd);
			});

		//	async compute parallel with depth prepass
		cmd = device->BeginCommandList(QUEUE_COMPUTE);
		CommandList cmd_prepareframe_async = cmd;
		device->WaitCommandList(cmd, cmd_prepareframe);
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);
			wi::renderer::UpdateRenderDataAsync(visibility_main, frameCB, cmd);

			if (scene->IsAccelerationStructureUpdateRequested())
			{
				wi::renderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
			}

			if (wi::renderer::GetSurfelGIEnabled())
			{
				wi::renderer::SurfelGI(
					surfelGIResources,
					*scene,
					cmd,
					instanceInclusionMask_SurfelGI
				);
			}

			if (wi::renderer::GetDDGIEnabled())
			{
				wi::renderer::DDGI(
					*scene,
					cmd,
					instanceInclusionMask_DDGI
				);
			}

			});

		static const uint32_t drawscene_flags =
			wi::renderer::DRAWSCENE_OPAQUE |
			wi::renderer::DRAWSCENE_IMPOSTOR |
			wi::renderer::DRAWSCENE_HAIRPARTICLE |
			wi::renderer::DRAWSCENE_TESSELLATION |
			wi::renderer::DRAWSCENE_OCCLUSIONCULLING
			;
		static const uint32_t drawscene_flags_reflections =
			wi::renderer::DRAWSCENE_OPAQUE |
			wi::renderer::DRAWSCENE_IMPOSTOR
			;

		// Main camera depth prepass + occlusion culling:
		cmd = device->BeginCommandList();
		CommandList cmd_maincamera_prepass = cmd;
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

			GraphicsDevice* device = wi::graphics::GetDevice();

			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);

			wi::renderer::OcclusionCulling_Reset(visibility_main, cmd); // must be outside renderpass!

			device->RenderPassBegin(&renderpass_depthprepass, cmd);

			device->EventBegin("Opaque Z-prepass", cmd);
			auto range = wi::profiler::BeginRangeGPU("Z-Prepass", cmd);

			Viewport vp;
			vp.width = (float)depthBuffer_Main.GetDesc().width;
			vp.height = (float)depthBuffer_Main.GetDesc().height;
			device->BindViewports(1, &vp, cmd);
			wi::renderer::DrawScene(visibility_main, RENDERPASS_PREPASS, cmd, drawscene_flags);

			wi::profiler::EndRange(range);
			device->EventEnd(cmd);

			if (getOcclusionCullingEnabled())
			{
				wi::renderer::OcclusionCulling_Render(*camera, visibility_main, cmd);
			}

			device->RenderPassEnd(cmd);

			wi::renderer::OcclusionCulling_Resolve(visibility_main, cmd); // must be outside renderpass!

			});

		// Main camera compute effects:
		//	(async compute, parallel to "shadow maps" and "update textures",
		//	must finish before "main scene opaque color pass")
		cmd = device->BeginCommandList(QUEUE_COMPUTE);
		device->WaitCommandList(cmd, cmd_maincamera_prepass);
		CommandList cmd_maincamera_compute_effects = cmd;
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

			GraphicsDevice* device = wi::graphics::GetDevice();

			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);

			wi::renderer::Visibility_Prepare(
				visibilityResources,
				rtPrimitiveID_render,
				cmd
			);

			wi::renderer::ComputeTiledLightCulling(
				tiledLightResources,
				debugUAV,
				cmd
			);

			if (visibility_shading_in_compute)
			{
				wi::renderer::Visibility_Surface(
					visibilityResources,
					rtMain,
					cmd
				);
			}
			else if (
				getSSREnabled() ||
				getRaytracedReflectionEnabled() ||
				getRaytracedDiffuseEnabled() ||
				wi::renderer::GetScreenSpaceShadowsEnabled() ||
				wi::renderer::GetRaytracedShadowsEnabled()
				)
			{
				// These post effects require surface normals and/or roughness
				wi::renderer::Visibility_Surface_Reduced(
					visibilityResources,
					cmd
				);
			}

			if (rtVelocity.IsValid())
			{
				wi::renderer::Visibility_Velocity(
					visibilityResources,
					rtVelocity,
					cmd
				);
			}

			if (wi::renderer::GetSurfelGIEnabled())
			{
				wi::renderer::SurfelGI_Coverage(
					surfelGIResources,
					*scene,
					debugUAV,
					cmd
				);
			}

			RenderAO(cmd);

			if (wi::renderer::GetVariableRateShadingClassification() && device->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2))
			{
				wi::renderer::ComputeShadingRateClassification(
					rtShadingRate,
					debugUAV,
					cmd
				);
			}

			if (scene->weather.IsVolumetricClouds())
			{
				wi::renderer::Postprocess_VolumetricClouds(
					volumetriccloudResources,
					cmd,
					scene->weather.volumetricCloudsWeatherMap.IsValid() ? &scene->weather.volumetricCloudsWeatherMap.GetTexture() : nullptr
				);
			}

			RenderSSR(cmd);

			if (wi::renderer::GetScreenSpaceShadowsEnabled())
			{
				wi::renderer::Postprocess_ScreenSpaceShadow(
					screenspaceshadowResources,
					tiledLightResources.entityTiles_Opaque,
					rtShadow,
					cmd,
					getScreenSpaceShadowRange(),
					getScreenSpaceShadowSampleCount()
				);
			}

			if (wi::renderer::GetRaytracedShadowsEnabled())
			{
				wi::renderer::Postprocess_RTShadow(
					rtshadowResources,
					*scene,
					tiledLightResources.entityTiles_Opaque,
					rtShadow,
					cmd,
					instanceInclusionMask_RTShadow
				);
			}

			});

		// Shadow maps:
		if (getShadowsEnabled())
		{
			cmd = device->BeginCommandList();
			wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {
				wi::renderer::DrawShadowmaps(visibility_main, cmd);
				});
		}

		// Updating textures:
		cmd = device->BeginCommandList();
		device->WaitCommandList(cmd, cmd_prepareframe_async);
		wi::jobsystem::Execute(ctx, [cmd, this](wi::jobsystem::JobArgs args) {
			wi::renderer::BindCommonResources(cmd);
			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);
			wi::renderer::RefreshLightmaps(*scene, cmd, instanceInclusionMask_Lightmap);
			wi::renderer::RefreshEnvProbes(visibility_main, cmd);
			wi::renderer::RefreshImpostors(*scene, cmd);
			});

		// Voxel GI:
		if (wi::renderer::GetVoxelRadianceEnabled())
		{
			cmd = device->BeginCommandList();
			wi::jobsystem::Execute(ctx, [cmd, this](wi::jobsystem::JobArgs args) {
				wi::renderer::VoxelRadiance(visibility_main, cmd);
				});
		}

		if (visibility_main.IsRequestedPlanarReflections())
		{
			// Planar reflections depth prepass:
			cmd = device->BeginCommandList();
			wi::jobsystem::Execute(ctx, [cmd, this](wi::jobsystem::JobArgs args) {

				GraphicsDevice* device = wi::graphics::GetDevice();

				wi::renderer::BindCameraCB(
					camera_reflection,
					camera_reflection_previous,
					camera_reflection,
					cmd
				);

				device->EventBegin("Planar reflections Z-Prepass", cmd);
				auto range = wi::profiler::BeginRangeGPU("Planar Reflections Z-Prepass", cmd);

				Viewport vp;
				vp.width = (float)depthBuffer_Reflection.GetDesc().width;
				vp.height = (float)depthBuffer_Reflection.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				device->RenderPassBegin(&renderpass_reflection_depthprepass, cmd);

				wi::renderer::DrawScene(visibility_reflection, RENDERPASS_PREPASS, cmd, drawscene_flags_reflections);

				device->RenderPassEnd(cmd);

				if (scene->weather.IsVolumetricClouds())
				{
					wi::renderer::Postprocess_VolumetricClouds(
						volumetriccloudResources_reflection,
						cmd,
						scene->weather.volumetricCloudsWeatherMap.IsValid() ? &scene->weather.volumetricCloudsWeatherMap.GetTexture() : nullptr
					);
				}

				wi::profiler::EndRange(range); // Planar Reflections
				device->EventEnd(cmd);

				});

			// Planar reflections opaque color pass:
			cmd = device->BeginCommandList();
			wi::jobsystem::Execute(ctx, [cmd, this](wi::jobsystem::JobArgs args) {

				GraphicsDevice* device = wi::graphics::GetDevice();

				wi::renderer::BindCameraCB(
					camera_reflection,
					camera_reflection_previous,
					camera_reflection,
					cmd
				);

				wi::renderer::ComputeTiledLightCulling(
					tiledLightResources_planarReflection,
					Texture(),
					cmd
				);

				device->EventBegin("Planar reflections", cmd);
				auto range = wi::profiler::BeginRangeGPU("Planar Reflections", cmd);

				Viewport vp;
				vp.width = (float)depthBuffer_Reflection.GetDesc().width;
				vp.height = (float)depthBuffer_Reflection.GetDesc().height;
				device->BindViewports(1, &vp, cmd);


				device->RenderPassBegin(&renderpass_reflection, cmd);

				wi::renderer::DrawScene(visibility_reflection, RENDERPASS_MAIN, cmd, drawscene_flags_reflections);
				wi::renderer::DrawSky(*scene, cmd);

				// Blend the volumetric clouds on top:
				if (scene->weather.IsVolumetricClouds())
				{
					device->EventBegin("Volumetric Clouds Reflection Blend", cmd);
					wi::image::Params fx;
					fx.enableFullScreen();
					fx.blendFlag = BLENDMODE_PREMULTIPLIED;
					wi::image::Draw(&volumetriccloudResources_reflection.texture_reproject[device->GetFrameCount() % 2], fx, cmd);
					device->EventEnd(cmd);
				}

				device->RenderPassEnd(cmd);

				wi::profiler::EndRange(range); // Planar Reflections
				device->EventEnd(cmd);
				});
		}

		// Main camera opaque color pass:
		cmd = device->BeginCommandList();
		device->WaitCommandList(cmd, cmd_maincamera_compute_effects);
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

			GraphicsDevice* device = wi::graphics::GetDevice();
			device->EventBegin("Opaque Scene", cmd);

			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);

			// This can't run in "main camera compute effects" async compute,
			//	because it depends on shadow maps, and envmaps
			if (getRaytracedReflectionEnabled())
			{
				wi::renderer::Postprocess_RTReflection(
					rtreflectionResources,
					*scene,
					rtSSR,
					cmd,
					getRaytracedReflectionsRange(),
					getReflectionRoughnessCutoff(),
					instanceInclusionMask_RTReflection
				);
			}
			if (getRaytracedDiffuseEnabled())
			{
				wi::renderer::Postprocess_RTDiffuse(
					rtdiffuseResources,
					*scene,
					rtRaytracedDiffuse,
					cmd,
					getRaytracedDiffuseRange(),
					instanceInclusionMask_RTDiffuse
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

			if (wi::renderer::GetRaytracedShadowsEnabled() || wi::renderer::GetScreenSpaceShadowsEnabled())
			{
				GPUBarrier barrier = GPUBarrier::Image(&rtShadow, rtShadow.desc.layout, ResourceState::SHADER_RESOURCE);
				device->Barrier(&barrier, 1, cmd);
			}

			if (visibility_shading_in_compute)
			{
				wi::renderer::Visibility_Shade(
					visibilityResources,
					rtMain,
					cmd
				);
			}

			Viewport vp;
			vp.width = (float)depthBuffer_Main.GetDesc().width;
			vp.height = (float)depthBuffer_Main.GetDesc().height;
			device->BindViewports(1, &vp, cmd);


			if (getOutlineEnabled())
			{
				// Cut off outline source from linear depth:
				device->EventBegin("Outline Source", cmd);
				device->RenderPassBegin(&renderpass_outline_source, cmd);
				wi::image::Params params;
				params.enableFullScreen();
				params.stencilRefMode = wi::image::STENCILREFMODE_ENGINE;
				params.stencilComp = wi::image::STENCILMODE_EQUAL;
				params.stencilRef = wi::enums::STENCILREF_OUTLINE;
				wi::image::Draw(&rtLinearDepth, params, cmd);
				params.stencilRef = wi::enums::STENCILREF_CUSTOMSHADER_OUTLINE;
				wi::image::Draw(&rtLinearDepth, params, cmd);
				device->RenderPassEnd(cmd);
				device->EventEnd(cmd);
			}


			device->RenderPassBegin(&renderpass_main, cmd);

			if (visibility_shading_in_compute)
			{
				// In visibility compute shading, the impostors must still be drawn using rasterization:
				wi::renderer::DrawScene(visibility_main, RENDERPASS_MAIN, cmd, wi::renderer::DRAWSCENE_IMPOSTOR);
			}
			else
			{
				auto range = wi::profiler::BeginRangeGPU("Opaque Scene", cmd);
				wi::renderer::DrawScene(visibility_main, RENDERPASS_MAIN, cmd, drawscene_flags);
				wi::renderer::DrawSky(*scene, cmd);
				wi::profiler::EndRange(range); // Opaque Scene
			}

			RenderOutline(cmd);

			// Upsample + Blend the volumetric clouds on top:
			if (scene->weather.IsVolumetricClouds())
			{
				device->EventBegin("Volumetric Clouds Upsample + Blend", cmd);
				wi::renderer::Postprocess_Upsample_Bilateral(
					volumetriccloudResources.texture_reproject[device->GetFrameCount() % 2],
					rtLinearDepth,
					rtMain_render, // only desc is taken if pixel shader upsampling is used
					cmd,
					true // pixel shader upsampling
				);
				device->EventEnd(cmd);
			}

			device->RenderPassEnd(cmd);

			if (wi::renderer::GetRaytracedShadowsEnabled() || wi::renderer::GetScreenSpaceShadowsEnabled())
			{
				GPUBarrier barrier = GPUBarrier::Image(&rtShadow, ResourceState::SHADER_RESOURCE, rtShadow.desc.layout);
				device->Barrier(&barrier, 1, cmd);
			}

			device->EventEnd(cmd);
			});

		// Transparents, post processes, etc:
		cmd = device->BeginCommandList();
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

			GraphicsDevice* device = wi::graphics::GetDevice();

			wi::renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);
			wi::renderer::BindCommonResources(cmd);

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

		wi::jobsystem::Wait(ctx);
	}

	void RenderPath3D::Compose(CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		wi::image::Params fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.quality = wi::image::QUALITY_LINEAR;
		fx.enableFullScreen();

		device->EventBegin("Composition", cmd);
		wi::image::Draw(GetLastPostprocessRT(), fx, cmd);
		device->EventEnd(cmd);

		if (
			wi::renderer::GetDebugLightCulling() ||
			wi::renderer::GetVariableRateShadingClassificationDebug() ||
			wi::renderer::GetSurfelGIDebugEnabled()
			)
		{
			fx.enableFullScreen();
			fx.blendFlag = BLENDMODE_PREMULTIPLIED;
			wi::image::Draw(&debugUAV, fx, cmd);
		}

		RenderPath2D::Compose(cmd);
	}

	void RenderPath3D::Stop()
	{
		DeleteGPUResources();
	}

	void RenderPath3D::Start()
	{
		ResizeBuffers();
	}

	void RenderPath3D::RenderAO(CommandList cmd) const
	{

		if (getAOEnabled())
		{
			switch (getAO())
			{
			case AO_SSAO:
				wi::renderer::Postprocess_SSAO(
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
				wi::renderer::Postprocess_HBAO(
					ssaoResources,
					*camera,
					rtLinearDepth,
					rtAO,
					cmd,
					getAOPower()
				);
				break;
			case AO_MSAO:
				wi::renderer::Postprocess_MSAO(
					msaoResources,
					*camera,
					rtLinearDepth,
					rtAO,
					cmd,
					getAOPower()
				);
				break;
			case AO_RTAO:
				wi::renderer::Postprocess_RTAO(
					rtaoResources,
					*scene,
					rtAO,
					cmd,
					getAORange(),
					getAOPower(),
					instanceInclusionMask_RTAO
				);
				break;
			case AO_DISABLED:
				break;
			}
		}
	}
	void RenderPath3D::RenderSSR(CommandList cmd) const
	{
		if (getSSREnabled() && !getRaytracedReflectionEnabled())
		{
			wi::renderer::Postprocess_SSR(
				ssrResources,
				rtSceneCopy,
				rtSSR,
				cmd,
				getReflectionRoughnessCutoff()
			);
		}
	}
	void RenderPath3D::RenderOutline(CommandList cmd) const
	{
		if (getOutlineEnabled())
		{
			wi::renderer::Postprocess_Outline(
				rtOutlineSource,
				cmd,
				getOutlineThreshold(),
				getOutlineThickness(),
				getOutlineColor()
			);
		}
	}
	void RenderPath3D::RenderLightShafts(CommandList cmd) const
	{
		XMVECTOR sunDirection = XMLoadFloat3(&scene->weather.sunDirection);
		if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, camera->GetAt())) > 0)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();

			device->EventBegin("Light Shafts", cmd);

			// Render sun stencil cutout:
			{
				device->RenderPassBegin(&renderpass_lightshafts, cmd);

				Viewport vp;
				vp.width = (float)depthBuffer_Main.GetDesc().width;
				vp.height = (float)depthBuffer_Main.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				wi::renderer::DrawSun(cmd);

				if (scene->weather.IsVolumetricClouds())
				{
					device->EventBegin("Volumetric cloud occlusion mask", cmd);
					wi::image::Params fx;
					fx.enableFullScreen();
					fx.blendFlag = BLENDMODE_MULTIPLY;
					wi::image::Draw(&volumetriccloudResources.texture_cloudMask, fx, cmd);
					device->EventEnd(cmd);
				}

				device->RenderPassEnd(cmd);
			}

			// Radial blur on the sun:
			{
				XMVECTOR sunPos = XMVector3Project(camera->GetEye() + sunDirection * camera->zFarP, 0, 0,
					1.0f, 1.0f, 0.1f, 1.0f,
					camera->GetProjection(), camera->GetView(), XMMatrixIdentity());
				{
					XMFLOAT2 sun;
					XMStoreFloat2(&sun, sunPos);
					wi::renderer::Postprocess_LightShafts(
						renderpass_lightshafts.desc.attachments.back().texture,
						rtSun[1],
						cmd,
						sun,
						getLightShaftsStrength()
					);
				}
			}
			device->EventEnd(cmd);
		}
	}
	void RenderPath3D::RenderVolumetrics(CommandList cmd) const
	{
		if (getVolumeLightsEnabled() && visibility_main.IsRequestedVolumetricLights())
		{
			auto range = wi::profiler::BeginRangeGPU("Volumetric Lights", cmd);

			GraphicsDevice* device = wi::graphics::GetDevice();

			device->RenderPassBegin(&renderpass_volumetriclight, cmd);

			Viewport vp;
			vp.width = (float)rtVolumetricLights[0].GetDesc().width;
			vp.height = (float)rtVolumetricLights[0].GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			wi::renderer::DrawVolumeLights(visibility_main, cmd);

			device->RenderPassEnd(cmd);

			wi::renderer::Postprocess_Blur_Bilateral(
				rtVolumetricLights[0],
				rtLinearDepth,
				rtVolumetricLights[1],
				rtVolumetricLights[0],
				cmd
			);

			wi::profiler::EndRange(range);
		}
	}
	void RenderPath3D::RenderSceneMIPChain(CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		auto range = wi::profiler::BeginRangeGPU("Scene MIP Chain", cmd);
		device->EventBegin("RenderSceneMIPChain", cmd);

		wi::renderer::Postprocess_Downsample4x(rtMain, rtSceneCopy, cmd);

		wi::renderer::MIPGEN_OPTIONS mipopt;
		mipopt.gaussian_temp = &rtSceneCopy_tmp;
		wi::renderer::GenerateMipChain(rtSceneCopy, wi::renderer::MIPGENFILTER_GAUSSIAN, cmd, mipopt);

		device->EventEnd(cmd);
		wi::profiler::EndRange(range);
	}
	void RenderPath3D::RenderTransparents(CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		// Water ripple rendering:
		if (!scene->waterRipples.empty())
		{
			device->RenderPassBegin(&renderpass_waterripples, cmd);
			Viewport vp;
			vp.width = (float)rtWaterRipple.GetDesc().width;
			vp.height = (float)rtWaterRipple.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			wi::renderer::DrawWaterRipples(visibility_main, cmd);
			device->RenderPassEnd(cmd);
		}

		device->RenderPassBegin(&renderpass_transparent, cmd);

		Viewport vp;
		vp.width = (float)depthBuffer_Main.GetDesc().width;
		vp.height = (float)depthBuffer_Main.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		// Transparent scene
		{
			auto range = wi::profiler::BeginRangeGPU("Transparent Scene", cmd);
			device->EventBegin("Transparent Scene", cmd);

			uint32_t drawscene_flags = 0;
			drawscene_flags |= wi::renderer::DRAWSCENE_TRANSPARENT;
			drawscene_flags |= wi::renderer::DRAWSCENE_OCCLUSIONCULLING;
			drawscene_flags |= wi::renderer::DRAWSCENE_HAIRPARTICLE;
			drawscene_flags |= wi::renderer::DRAWSCENE_TESSELLATION;
			wi::renderer::DrawScene(visibility_main, RENDERPASS_MAIN, cmd, drawscene_flags);

			device->EventEnd(cmd);
			wi::profiler::EndRange(range); // Transparent Scene
		}

		wi::renderer::DrawLightVisualizers(visibility_main, cmd);

		wi::renderer::DrawSoftParticles(visibility_main, rtLinearDepth, false, cmd);

		if (getVolumeLightsEnabled() && visibility_main.IsRequestedVolumetricLights())
		{
			device->EventBegin("Contribute Volumetric Lights", cmd);
			wi::renderer::Postprocess_Upsample_Bilateral(rtVolumetricLights[0], rtLinearDepth,
				renderpass_transparent.desc.attachments[0].texture, cmd, true, 1.5f);
			device->EventEnd(cmd);
		}

		XMVECTOR sunDirection = XMLoadFloat3(&scene->weather.sunDirection);
		if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, camera->GetAt())) > 0)
		{
			device->EventBegin("Contribute LightShafts", cmd);
			wi::image::Params fx;
			fx.enableFullScreen();
			fx.blendFlag = BLENDMODE_ADDITIVE;
			wi::image::Draw(&rtSun[1], fx, cmd);
			device->EventEnd(cmd);
		}

		if (getLensFlareEnabled())
		{
			wi::renderer::DrawLensFlares(
				visibility_main,
				cmd,
				scene->weather.IsVolumetricClouds() ? &volumetriccloudResources.texture_cloudMask : nullptr
			);
		}

		wi::renderer::DrawDebugWorld(*scene, *camera, *this, cmd);

		device->RenderPassEnd(cmd);

		// Distortion particles:
		{
			device->RenderPassBegin(&renderpass_particledistortion, cmd);

			Viewport vp;
			vp.width = (float)rtParticleDistortion.GetDesc().width;
			vp.height = (float)rtParticleDistortion.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			wi::renderer::DrawSoftParticles(visibility_main, rtLinearDepth, true, cmd);

			device->RenderPassEnd(cmd);
		}
	}
	void RenderPath3D::RenderPostprocessChain(CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		const Texture* rt_first = nullptr; // not ping-ponged with read / write
		const Texture* rt_read = &rtMain;
		const Texture* rt_write = &rtPostprocess;

		// 1.) HDR post process chain
		{
			if (wi::renderer::GetTemporalAAEnabled() && !wi::renderer::GetTemporalAADebugEnabled())
			{
				wi::renderer::Postprocess_TemporalAA(
					temporalAAResources,
					*rt_read,
					cmd
				);
				rt_first = temporalAAResources.GetCurrent();
			}

			if (scene->weather.IsOceanEnabled())
			{
				wi::renderer::Postprocess_Underwater(
					rt_first == nullptr ? *rt_read : *rt_first,
					*rt_write,
					cmd
				);

				rt_first = nullptr;
				std::swap(rt_read, rt_write);
			}

			if (getDepthOfFieldEnabled() && camera->aperture_size > 0 && getDepthOfFieldStrength() > 0)
			{
				wi::renderer::Postprocess_DepthOfField(
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
				wi::renderer::Postprocess_MotionBlur(
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
				wi::renderer::ComputeLuminance(
					luminanceResources,
					rt_first == nullptr ? *rt_read : *rt_first,
					cmd,
					getEyeAdaptionRate(),
					getEyeAdaptionKey()
				);
			}
			if (getBloomEnabled())
			{
				wi::renderer::ComputeBloom(
					bloomResources,
					rt_first == nullptr ? *rt_read : *rt_first,
					cmd,
					getBloomThreshold(),
					getExposure(),
					getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr
				);
			}

			wi::renderer::Postprocess_Tonemap(
				rt_first == nullptr ? *rt_read : *rt_first,
				*rt_write,
				cmd,
				getExposure(),
				getBrightness(),
				getContrast(),
				getSaturation(),
				getDitherEnabled(),
				getColorGradingEnabled() ? (scene->weather.colorGradingMap.IsValid() ? &scene->weather.colorGradingMap.GetTexture() : nullptr) : nullptr,
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
				wi::renderer::Postprocess_Sharpen(*rt_read, *rt_write, cmd, getSharpenFilterAmount());

				std::swap(rt_read, rt_write);
			}

			if (getFXAAEnabled())
			{
				wi::renderer::Postprocess_FXAA(*rt_read, *rt_write, cmd);

				std::swap(rt_read, rt_write);
			}

			if (getChromaticAberrationEnabled())
			{
				wi::renderer::Postprocess_Chromatic_Aberration(*rt_read, *rt_write, cmd, getChromaticAberrationAmount());

				std::swap(rt_read, rt_write);
			}

			lastPostprocessRT = rt_read;

			// GUI Background blurring:
			{
				auto range = wi::profiler::BeginRangeGPU("GUI Background Blur", cmd);
				device->EventBegin("GUI Background Blur", cmd);
				wi::renderer::Postprocess_Downsample4x(*rt_read, rtGUIBlurredBackground[0], cmd);
				wi::renderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
				wi::renderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd, -1, -1, true);
				device->EventEnd(cmd);
				wi::profiler::EndRange(range);
			}

			if (rtFSR[0].IsValid() && getFSREnabled())
			{
				wi::renderer::Postprocess_FSR(*rt_read, rtFSR[1], rtFSR[0], cmd, getFSRSharpness());
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
		if (internalResolution.x == 0 || internalResolution.y == 0)
			return;

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
			wi::renderer::CreateSSAOResources(ssaoResources, internalResolution);
			break;
		case RenderPath3D::AO_MSAO:
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			wi::renderer::CreateMSAOResources(msaoResources, internalResolution);
			break;
		case RenderPath3D::AO_RTAO:
			desc.width = internalResolution.x / 2;
			desc.height = internalResolution.y / 2;
			wi::renderer::CreateRTAOResources(rtaoResources, internalResolution);
			break;
		default:
			break;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		device->CreateTexture(&desc, nullptr, &rtAO);
		device->SetName(&rtAO, "rtAO");
	}
	void RenderPath3D::setSSREnabled(bool value)
	{
		ssrEnabled = value;

		if (value)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

			TextureDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R16G16B16A16_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			device->CreateTexture(&desc, nullptr, &rtSSR);
			device->SetName(&rtSSR, "rtSSR");

			wi::renderer::CreateSSRResources(ssrResources, internalResolution);
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
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

			TextureDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R16G16B16A16_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &rtSSR);
			device->SetName(&rtSSR, "rtSSR");

			wi::renderer::CreateRTReflectionResources(rtreflectionResources, internalResolution);
		}
		else
		{
			rtreflectionResources = {};
		}
	}
	void RenderPath3D::setRaytracedDiffuseEnabled(bool value)
	{
		raytracedDiffuseEnabled = value;

		if (value)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

			TextureDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R16G16B16A16_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &rtRaytracedDiffuse);
			device->SetName(&rtRaytracedDiffuse, "rtRaytracedDiffuse");

			wi::renderer::CreateRTDiffuseResources(rtdiffuseResources, internalResolution);
		}
		else
		{
			rtRaytracedDiffuse = {};
			rtdiffuseResources = {};
		}
	}
	void RenderPath3D::setFSREnabled(bool value)
	{
		fsrEnabled = value;

		if (resolutionScale < 1.0f && fsrEnabled)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			if (GetPhysicalWidth() == 0 || GetPhysicalHeight() == 0)
				return;

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
	void RenderPath3D::setMotionBlurEnabled(bool value)
	{
		motionBlurEnabled = value;

		if (value)
		{
			wi::renderer::CreateMotionBlurResources(motionblurResources, GetInternalResolution());
		}
		else
		{
			motionblurResources = {};
		}
	}
	void RenderPath3D::setDepthOfFieldEnabled(bool value)
	{
		depthOfFieldEnabled = value;

		if (value)
		{
			wi::renderer::CreateDepthOfFieldResources(depthoffieldResources, GetInternalResolution());
		}
		else
		{
			depthoffieldResources = {};
		}
	}
	void RenderPath3D::setEyeAdaptionEnabled(bool value)
	{
		eyeAdaptionEnabled = value;

		if (value)
		{
			wi::renderer::CreateLuminanceResources(luminanceResources, GetInternalResolution());
		}
		else
		{
			luminanceResources = {};
		}
	}
	void RenderPath3D::setReflectionsEnabled(bool value)
	{
		reflectionsEnabled = value;

		if (value)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

			TextureDesc desc;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R11G11B10_FLOAT;
			desc.width = internalResolution.x / 4;
			desc.height = internalResolution.y / 4;
			device->CreateTexture(&desc, nullptr, &rtReflection);
			device->SetName(&rtReflection, "rtReflection");

			desc.bind_flags = BindFlag::DEPTH_STENCIL | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R32_TYPELESS;
			desc.width = internalResolution.x / 4;
			desc.height = internalResolution.y / 4;
			desc.layout = ResourceState::DEPTHSTENCIL_READONLY;
			device->CreateTexture(&desc, nullptr, &depthBuffer_Reflection);
			device->SetName(&depthBuffer_Reflection, "depthBuffer_Reflection");

			{
				RenderPassDesc desc;
				desc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						depthBuffer_Reflection,
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
						rtReflection,
						RenderPassAttachment::LoadOp::DONTCARE,
						RenderPassAttachment::StoreOp::STORE,
						ResourceState::SHADER_RESOURCE,
						ResourceState::RENDERTARGET,
						ResourceState::SHADER_RESOURCE
					)
				);
				desc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						depthBuffer_Reflection,
						RenderPassAttachment::LoadOp::LOAD,
						RenderPassAttachment::StoreOp::DONTCARE,
						ResourceState::SHADER_RESOURCE,
						ResourceState::DEPTHSTENCIL_READONLY,
						ResourceState::DEPTHSTENCIL_READONLY
					)
				);

				device->CreateRenderPass(&desc, &renderpass_reflection);
			}

			wi::renderer::CreateTiledLightResources(tiledLightResources_planarReflection, XMUINT2(depthBuffer_Reflection.desc.width, depthBuffer_Reflection.desc.height));
		}
		else
		{
			rtReflection = {};
			depthBuffer_Reflection = {};
			renderpass_reflection_depthprepass = {};
			renderpass_reflection = {};
			tiledLightResources_planarReflection = {};
		}
	}
	void RenderPath3D::setBloomEnabled(bool value)
	{
		bloomEnabled = value;

		if (value)
		{
			wi::renderer::CreateBloomResources(bloomResources, GetInternalResolution());
		}
		else
		{
			bloomResources = {};
		}
	}
	void RenderPath3D::setVolumeLightsEnabled(bool value)
	{
		volumeLightsEnabled = value;

		if (value)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

			TextureDesc desc;
			desc.format = Format::R16G16B16A16_FLOAT;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.width = internalResolution.x / 4;
			desc.height = internalResolution.y / 4;
			device->CreateTexture(&desc, nullptr, &rtVolumetricLights[0]);
			device->SetName(&rtVolumetricLights[0], "rtVolumetricLights[0]");
			device->CreateTexture(&desc, nullptr, &rtVolumetricLights[1]);
			device->SetName(&rtVolumetricLights[1], "rtVolumetricLights[1]");
			{
				RenderPassDesc desc;
				desc.attachments.push_back(RenderPassAttachment::RenderTarget(rtVolumetricLights[0], RenderPassAttachment::LoadOp::CLEAR));

				device->CreateRenderPass(&desc, &renderpass_volumetriclight);
			}
		}
		else
		{
			rtVolumetricLights[0] = {};
			rtVolumetricLights[1] = {};
			renderpass_volumetriclight = {};
		}
	}
	void RenderPath3D::setLightShaftsEnabled(bool value)
	{
		lightShaftsEnabled = value;

		if (value)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

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

			{
				RenderPassDesc desc;
				desc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						depthBuffer_Main,
						RenderPassAttachment::LoadOp::LOAD,
						RenderPassAttachment::StoreOp::STORE,
						ResourceState::DEPTHSTENCIL_READONLY,
						ResourceState::DEPTHSTENCIL_READONLY,
						ResourceState::DEPTHSTENCIL_READONLY
					)
				);
				desc.attachments.push_back(RenderPassAttachment::RenderTarget(rtSun[0], RenderPassAttachment::LoadOp::CLEAR));
				if (getMSAASampleCount() > 1)
				{
					desc.attachments.back().storeop = RenderPassAttachment::StoreOp::DONTCARE;
					desc.attachments.push_back(RenderPassAttachment::Resolve(rtSun_resolved));
				}

				device->CreateRenderPass(&desc, &renderpass_lightshafts);
			}
		}
		else
		{
			rtSun[0] = {};
			rtSun[1] = {};
			rtSun_resolved = {};
			renderpass_lightshafts = {};
		}
	}
	void RenderPath3D::setOutlineEnabled(bool value)
	{
		outlineEnabled = value;

		if (value)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			XMUINT2 internalResolution = GetInternalResolution();
			if (internalResolution.x == 0 || internalResolution.y == 0)
				return;

			TextureDesc desc;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R32_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &rtOutlineSource);
			device->SetName(&rtOutlineSource, "rtOutlineSource");

			{
				RenderPassDesc desc;
				desc.attachments.push_back(RenderPassAttachment::RenderTarget(rtOutlineSource, RenderPassAttachment::LoadOp::CLEAR));
				desc.attachments.push_back(RenderPassAttachment::DepthStencil(depthBuffer_Main, RenderPassAttachment::LoadOp::LOAD));

				device->CreateRenderPass(&desc, &renderpass_outline_source);
			}
		}
		else
		{
			rtOutlineSource = {};
			renderpass_outline_source = {};
		}
	}

}
