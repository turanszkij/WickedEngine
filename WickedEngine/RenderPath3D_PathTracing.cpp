#include "RenderPath3D_PathTracing.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "shaders/ResourceMapping.h"
#include "wiProfiler.h"
#include "wiScene.h"

using namespace wiGraphics;
using namespace wiScene;


void RenderPath3D_PathTracing::ResizeBuffers()
{
	RenderPath2D::ResizeBuffers(); // we don't need to use any buffers from RenderPath3D, so skip those

	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();

	{
		TextureDesc desc;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
		desc.Format = FORMAT_R32G32B32A32_FLOAT;
		desc.Width = GetInternalResolution().x;
		desc.Height = GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &traceResult);
		device->SetName(&traceResult, "traceResult");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = defaultTextureFormat;
		desc.Width = GetInternalResolution().x;
		desc.Height = GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtPostprocess_LDR[0]);
		device->SetName(&rtPostprocess_LDR[0], "rtPostprocess_LDR[0]");


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

	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&traceResult, RenderPassAttachment::LOADOP_CLEAR));

		device->CreateRenderPass(&desc, &renderpass_debugbvh);
	}

	wiRenderer::CreateRayBuffers(rayBuffers, GetInternalResolution().x * GetInternalResolution().y);

	// also reset accumulation buffer state:
	sam = -1;
}

void RenderPath3D_PathTracing::Update(float dt)
{
	setOcclusionCullingEnabled(false);

	if (camera->IsDirty())
	{
		camera->SetDirty(false);
		sam = -1;
	}
	else
	{
		for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
		{
			const TransformComponent& transform = scene->transforms[i];

			if (transform.IsDirty())
			{
				sam = -1;
				break;
			}
		}

		if (sam >= 0)
		{
			for (size_t i = 0; i < scene->materials.GetCount(); ++i)
			{
				const MaterialComponent& material = scene->materials[i];

				if (material.IsDirty())
				{
					sam = -1;
					break;
				}
			}
		}
	}
	sam++;

	RenderPath3D::Update(dt);
}

void RenderPath3D_PathTracing::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;
	CommandList cmd;

	// Setup:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		wiRenderer::UpdateRenderData(visibility_main, frameCB, cmd);

		if (sam == 0)
		{
			wiRenderer::BuildSceneBVH(*scene, cmd);
		}
	});

	// Main scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::UpdateCameraCB(
			*camera,
			*camera,
			*camera,
			cmd
		);
		wiRenderer::BindCommonResources(cmd);

		if (wiRenderer::GetRaytraceDebugBVHVisualizerEnabled())
		{
			device->RenderPassBegin(&renderpass_debugbvh, cmd);

			Viewport vp;
			vp.Width = (float)traceResult.GetDesc().Width;
			vp.Height = (float)traceResult.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::RayTraceSceneBVH(cmd);

			device->RenderPassEnd(cmd);
		}
		else
		{
			auto range = wiProfiler::BeginRangeGPU("Traced Scene", cmd);

			wiRenderer::GenerateScreenRayBuffers(rayBuffers, *camera, GetInternalResolution().x, GetInternalResolution().y, cmd);
			wiRenderer::RayTraceScene(*scene, rayBuffers, &traceResult, sam, cmd);


			wiProfiler::EndRange(range); // Traced Scene
		}

		wiRenderer::Postprocess_Tonemap(
			traceResult,
			*wiTextureHelper::getColor(wiColor::Gray()),
			*wiTextureHelper::getBlack(),
			rtPostprocess_LDR[0],
			cmd,
			getExposure(),
			false,
			nullptr
		);

		// GUI Background blurring:
		{
			auto range = wiProfiler::BeginRangeGPU("GUI Background Blur", cmd);
			device->EventBegin("GUI Background Blur", cmd);
			wiRenderer::Postprocess_Downsample4x(rtPostprocess_LDR[0], rtGUIBlurredBackground[0], cmd);
			wiRenderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
			wiRenderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd);
			device->EventEnd(cmd);
			wiProfiler::EndRange(range);
		}
	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}

void RenderPath3D_PathTracing::Compose(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("RenderPath3D_PathTracing::Compose", cmd);

	wiRenderer::BindCommonResources(cmd);

	wiImageParams fx;
	fx.enableFullScreen();
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_LINEAR;
	wiImage::Draw(&rtPostprocess_LDR[0], fx, cmd);

	device->EventEnd(cmd);

	RenderPath2D::Compose(cmd);
}
