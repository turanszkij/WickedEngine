#include "RenderPath3D_PathTracing.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "shaders/ResourceMapping.h"
#include "wiProfiler.h"
#include "wiScene.h"
#include "wiBackLog.h"

#if __has_include("OpenImageDenoise/oidn.hpp")
#define OPEN_IMAGE_DENOISE
#include "OpenImageDenoise/oidn.hpp"
#pragma comment(lib,"OpenImageDenoise.lib")
#pragma comment(lib,"tbb.lib")
// Also provide OpenImageDenoise.dll and tbb.dll near the exe!
bool DenoiserCallback(void* userPtr, double n)
{
	auto renderpath = (RenderPath3D_PathTracing*)userPtr;
	if (renderpath->getProgress() < 1)
	{
		renderpath->denoiserProgress = 0;
		return false;
	}
	renderpath->denoiserProgress = (float)n;
	return true;
}
bool RenderPath3D_PathTracing::isDenoiserAvailable() const { return true; }
#else
bool RenderPath3D_PathTracing::isDenoiserAvailable() const { return false; }
#endif


using namespace wiGraphics;
using namespace wiScene;


void RenderPath3D_PathTracing::ResizeBuffers()
{
	RenderPath2D::ResizeBuffers(); // we don't need to use any buffers from RenderPath3D, so skip those

	GraphicsDevice* device = wiRenderer::GetDevice();

	XMUINT2 internalResolution = GetInternalResolution();

	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET;
		desc.format = Format::R32G32B32A32_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &traceResult);
		device->SetName(&traceResult, "traceResult");

#ifdef OPEN_IMAGE_DENOISE
		desc.bind_flags = BindFlag::UNORDERED_ACCESS;
		desc.layout = ResourceState::UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &denoiserAlbedo);
		device->SetName(&denoiserAlbedo, "denoiserAlbedo");
		device->CreateTexture(&desc, nullptr, &denoiserNormal);
		device->SetName(&denoiserNormal, "denoiserNormal");
#endif // OPEN_IMAGE_DENOISE
	}
	{
		TextureDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.format = Format::R11G11B10_FLOAT;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;
		device->CreateTexture(&desc, nullptr, &rtPostprocess);
		device->SetName(&rtPostprocess, "rtPostprocess");


		desc.width /= 4;
		desc.height /= 4;
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

	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&traceResult, RenderPassAttachment::LoadOp::CLEAR));

		device->CreateRenderPass(&desc, &renderpass_debugbvh);
	}

	wiRenderer::CreateLuminanceResources(luminanceResources, internalResolution);
	wiRenderer::CreateBloomResources(bloomResources, internalResolution);

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

	if (sam > target)
	{
		sam = target;
	}
	if (target < sam)
	{
		resetProgress();
	}

	scene->SetAccelerationStructureUpdateRequested(sam == 0);
	setSceneUpdateEnabled(sam == 0);

	RenderPath3D::Update(dt);


#ifdef OPEN_IMAGE_DENOISE
	if (sam == target)
	{
		if (!denoiserResult.IsValid() && !wiJobSystem::IsBusy(denoiserContext))
		{
			//wiHelper::saveTextureToFile(denoiserAlbedo, "C:/PROJECTS/WickedEngine/Editor/_albedo.png");
			//wiHelper::saveTextureToFile(denoiserNormal, "C:/PROJECTS/WickedEngine/Editor/_normal.png");

			texturedata_src.clear();
			texturedata_dst.clear();
			texturedata_albedo.clear();
			texturedata_normal.clear();

			if (wiHelper::saveTextureToMemory(traceResult, texturedata_src))
			{
				wiHelper::saveTextureToMemory(denoiserAlbedo, texturedata_albedo);
				wiHelper::saveTextureToMemory(denoiserNormal, texturedata_normal);

				texturedata_dst.resize(texturedata_src.size());

				wiJobSystem::Execute(denoiserContext, [&](wiJobArgs args) {

					size_t width = (size_t)traceResult.desc.width;
					size_t height = (size_t)traceResult.desc.height;
					{
						// https://github.com/OpenImageDenoise/oidn#c11-api-example

						// Create an Intel Open Image Denoise device
						static oidn::DeviceRef device = oidn::newDevice();
						static bool init = false;
						if (!init)
						{
							device.commit();
							init = true;
						}

						// Create a denoising filter
						oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
						filter.setImage("color", texturedata_src.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
						if (!texturedata_albedo.empty())
						{
							filter.setImage("albedo", texturedata_albedo.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
						}
						if (!texturedata_normal.empty())
						{
							filter.setImage("normal", texturedata_normal.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
						}
						filter.setImage("output", texturedata_dst.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
						filter.set("hdr", true); // image is HDR
						//filter.set("cleanAux", true);
						filter.commit();

						denoiserProgress = 0;
						filter.setProgressMonitorFunction(DenoiserCallback, this);

						// Filter the image
						filter.execute();

						// Check for errors
						const char* errorMessage;
						auto error = device.getError(errorMessage);
						if (error != oidn::Error::None && error != oidn::Error::Cancelled)
						{
							wiBackLog::post((std::string("[OpenImageDenoise error] ") + errorMessage).c_str());
						}
					}

					GraphicsDevice* device = wiRenderer::GetDevice();

					TextureDesc desc;
					desc.width = (uint32_t)width;
					desc.height = (uint32_t)height;
					desc.bind_flags = BindFlag::SHADER_RESOURCE;
					desc.format = Format::R32G32B32A32_FLOAT;

					SubresourceData initdata;
					initdata.data_ptr = texturedata_dst.data();
					initdata.row_pitch = uint32_t(sizeof(XMFLOAT4) * width);
					device->CreateTexture(&desc, &initdata, &denoiserResult);

					});
			}
		}
	}
	else
	{
		denoiserResult = Texture();
		denoiserProgress = 0;
	}
#endif // OPEN_IMAGE_DENOISE
}

void RenderPath3D_PathTracing::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;

	if (sam < target)
	{
		// Setup:
		CommandList cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

			wiRenderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);
			wiRenderer::UpdateRenderData(visibility_main, frameCB, cmd);
			wiRenderer::UpdateRenderDataAsync(visibility_main, frameCB, cmd);

			if (scene->IsAccelerationStructureUpdateRequested())
			{
				wiRenderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
			}
			});

		// Main scene:
		cmd = device->BeginCommandList();
		wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

			GraphicsDevice* device = wiRenderer::GetDevice();

			wiRenderer::BindCameraCB(
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
				vp.width = (float)traceResult.GetDesc().width;
				vp.height = (float)traceResult.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				wiRenderer::RayTraceSceneBVH(*scene, cmd);

				device->RenderPassEnd(cmd);
			}
			else
			{
				auto range = wiProfiler::BeginRangeGPU("Traced Scene", device, cmd);

				wiRenderer::RayTraceScene(
					*scene,
					traceResult,
					sam,
					cmd,
					denoiserAlbedo.IsValid() ? &denoiserAlbedo : nullptr,
					denoiserNormal.IsValid() ? &denoiserNormal : nullptr
				);


				wiProfiler::EndRange(range); // Traced Scene
			}

			});
	}

	// Tonemap etc:
	CommandList cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::BindCameraCB(
			*camera,
			*camera,
			*camera,
			cmd
		);
		wiRenderer::BindCommonResources(cmd);

		Texture srcTex = denoiserResult.IsValid() && !wiJobSystem::IsBusy(denoiserContext) ? denoiserResult : traceResult;

		if (getEyeAdaptionEnabled())
		{
			wiRenderer::ComputeLuminance(
				luminanceResources,
				srcTex,
				cmd,
				getEyeAdaptionRate(),
				getEyeAdaptionKey()
			);
		}
		if (getBloomEnabled())
		{
			wiRenderer::ComputeBloom(
				bloomResources,
				srcTex,
				cmd,
				getBloomThreshold(),
				getExposure(),
				getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr
			);
		}

		wiRenderer::Postprocess_Tonemap(
			srcTex,
			rtPostprocess,
			cmd,
			getExposure(),
			getDitherEnabled(),
			getColorGradingEnabled() ? (scene->weather.colorGradingMap == nullptr ? nullptr : &scene->weather.colorGradingMap->texture) : nullptr,
			nullptr,
			getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr,
			getBloomEnabled() ? &bloomResources.texture_bloom : nullptr,
			colorspace
		);
		lastPostprocessRT = &rtPostprocess;

		// GUI Background blurring:
		{
			auto range = wiProfiler::BeginRangeGPU("GUI Background Blur", device, cmd);
			device->EventBegin("GUI Background Blur", cmd);
			wiRenderer::Postprocess_Downsample4x(rtPostprocess, rtGUIBlurredBackground[0], cmd);
			wiRenderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
			wiRenderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd, -1, -1, true);
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
	wiImage::Draw(&rtPostprocess, fx, cmd);

	device->EventEnd(cmd);

	RenderPath2D::Compose(cmd);
}
