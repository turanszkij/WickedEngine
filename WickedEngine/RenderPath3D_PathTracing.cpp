#include "RenderPath3D_PathTracing.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"
#include "wiSceneSystem.h"

using namespace wiGraphics;
using namespace wiSceneSystem;


void RenderPath3D_PathTracing::ResizeBuffers()
{
	RenderPath2D::ResizeBuffers(); // we don't need to use any buffers from RenderPath3D, so skip those

	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();

	{
		TextureDesc desc;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
		desc.Format = FORMAT_R32G32B32A32_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &traceResult);
		device->SetName(&traceResult, "traceResult");
	}

	// also reset accumulation buffer state:
	sam = -1;
}

void RenderPath3D_PathTracing::Update(float dt)
{
	const Scene& scene = wiSceneSystem::GetScene();

	if (wiRenderer::GetCamera().IsDirty())
	{
		wiRenderer::GetCamera().SetDirty(false);
		sam = -1;
	}
	else
	{
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			const TransformComponent& transform = scene.transforms[i];

			if (transform.IsDirty())
			{
				sam = -1;
				break;
			}
		}

		if (sam >= 0)
		{
			for (size_t i = 0; i < scene.materials.GetCount(); ++i)
			{
				const MaterialComponent& material = scene.materials[i];

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
	wiJobSystem::Execute(ctx, [this, cmd] {

		wiRenderer::UpdateRenderData(cmd);

		if (sam == 0)
		{
			wiRenderer::BuildSceneBVH(cmd);
		}
	});

	// Main scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::BindCommonResources(cmd);

		if (wiRenderer::GetRaytraceDebugBVHVisualizerEnabled())
		{
			const Texture2D* rts[] = { &traceResult };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, cmd);
			float clear[] = { 0,0,0,1 };
			device->ClearRenderTarget(rts[0], clear, cmd);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::RayTraceSceneBVH(cmd);
		}
		else
		{
			auto range = wiProfiler::BeginRangeGPU("Traced Scene", cmd);

			wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

			wiRenderer::RayBuffers* rayBuffers = wiRenderer::GenerateScreenRayBuffers(wiRenderer::GetCamera(), cmd);
			wiRenderer::RayTraceScene(rayBuffers, &traceResult, sam, cmd);


			wiProfiler::EndRange(range); // Traced Scene
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

	wiImageParams fx((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_LINEAR;
	fx.process.setToneMap(getExposure());
	fx.setDistortionMap(wiTextureHelper::getBlack()); // tonemap shader uses signed distortion mask, so black = no distortion
	fx.setMaskMap(wiTextureHelper::getColor(wiColor::Gray()));
	
	wiImage::Draw(&traceResult, fx, cmd);


	device->EventEnd(cmd);

	RenderPath2D::Compose(cmd);
}
