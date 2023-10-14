#include "wiRenderPath3D_PathTracing.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "wiProfiler.h"
#include "wiScene.h"
#include "wiBacklog.h"

#if __has_include("OpenImageDenoise/oidn.hpp")
#include "OpenImageDenoise/oidn.hpp"
#if OIDN_VERSION_MAJOR >= 2
#define OPEN_IMAGE_DENOISE
#pragma comment(lib,"OpenImageDenoise.lib")
// Also provide the required DLL files from OpenImageDenoise release near the exe!
#endif // OIDN_VERSION_MAJOR >= 2
#endif // __has_include("OpenImageDenoise/oidn.hpp")

using namespace wi::graphics;
using namespace wi::scene;


namespace wi
{
#ifdef OPEN_IMAGE_DENOISE
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
#endif // OPEN_IMAGE_DENOISE

	void RenderPath3D_PathTracing::ResizeBuffers()
	{

		GraphicsDevice* device = wi::graphics::GetDevice();

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

			{
				desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				desc.layout = ResourceState::UNORDERED_ACCESS;
				desc.format = Format::R32_FLOAT;
				device->CreateTexture(&desc, nullptr, &traceDepth);
				device->SetName(&traceDepth, "traceDepth");
				desc.format = Format::R8_UINT;
				device->CreateTexture(&desc, nullptr, &traceStencil);
				device->SetName(&traceStencil, "traceStencil");

				desc.layout = ResourceState::DEPTHSTENCIL;
				desc.bind_flags = BindFlag::DEPTH_STENCIL;
				desc.format = wi::renderer::format_depthbuffer_main;
				device->CreateTexture(&desc, nullptr, &depthBuffer_Main);
				device->SetName(&depthBuffer_Main, "depthBuffer_Main");
			}
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

		wi::renderer::CreateLuminanceResources(luminanceResources, internalResolution);
		wi::renderer::CreateBloomResources(bloomResources, internalResolution);

		// also reset accumulation buffer state:
		sam = -1;

		RenderPath2D::ResizeBuffers(); // we don't need to use any buffers from RenderPath3D, so skip those
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
			if (!denoiserResult.IsValid() && !wi::jobsystem::IsBusy(denoiserContext))
			{
				//wi::helper::saveTextureToFile(denoiserAlbedo, "C:/PROJECTS/WickedEngine/Editor/_albedo.png");
				//wi::helper::saveTextureToFile(denoiserNormal, "C:/PROJECTS/WickedEngine/Editor/_normal.png");

				texturedata_src.clear();
				texturedata_dst.clear();
				texturedata_albedo.clear();
				texturedata_normal.clear();

				if (wi::helper::saveTextureToMemory(traceResult, texturedata_src))
				{
					wi::helper::saveTextureToMemory(denoiserAlbedo, texturedata_albedo);
					wi::helper::saveTextureToMemory(denoiserNormal, texturedata_normal);

					texturedata_dst.resize(texturedata_src.size());

					wi::jobsystem::Execute(denoiserContext, [&](wi::jobsystem::JobArgs args) {

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

							oidn::BufferRef texturedata_src_buffer = device.newBuffer(texturedata_src.size());
							oidn::BufferRef texturedata_dst_buffer = device.newBuffer(texturedata_dst.size());
							oidn::BufferRef texturedata_albedo_buffer;
							oidn::BufferRef texturedata_normal_buffer;

							texturedata_src_buffer.write(0, texturedata_src.size(), texturedata_src.data());

							// Create a denoising filter
							oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
							filter.setImage("color", texturedata_src_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
							if (!texturedata_albedo.empty())
							{
								texturedata_albedo_buffer = device.newBuffer(texturedata_albedo.size());
								texturedata_albedo_buffer.write(0, texturedata_albedo.size(), texturedata_albedo.data());
								filter.setImage("albedo", texturedata_albedo_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
							}
							if (!texturedata_normal.empty())
							{
								texturedata_normal_buffer = device.newBuffer(texturedata_normal.size());
								texturedata_normal_buffer.write(0, texturedata_normal.size(), texturedata_normal.data());
								filter.setImage("normal", texturedata_normal_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
							}
							filter.setImage("output", texturedata_dst_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
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
								wi::backlog::post(std::string("[OpenImageDenoise error] ") + errorMessage);
							}
							else
							{
								texturedata_dst_buffer.read(0, texturedata_dst.size(), texturedata_dst.data());
							}
						}

						GraphicsDevice* device = wi::graphics::GetDevice();

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
		GraphicsDevice* device = wi::graphics::GetDevice();
		wi::jobsystem::context ctx;

		if (sam < target)
		{

			CommandList cmd_copypages;
			if (scene->terrains.GetCount() > 0)
			{
				cmd_copypages = device->BeginCommandList(QUEUE_COPY);
				wi::jobsystem::Execute(ctx, [this, cmd_copypages](wi::jobsystem::JobArgs args) {
					for (size_t i = 0; i < scene->terrains.GetCount(); ++i)
					{
						scene->terrains[i].CopyVirtualTexturePageStatusGPU(cmd_copypages);
					}
					});
			}

			// Setup:
			CommandList cmd = device->BeginCommandList();
			wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

				wi::renderer::BindCameraCB(
					*camera,
					camera_previous,
					camera_reflection,
					cmd
				);
				wi::renderer::UpdateRenderData(visibility_main, frameCB, cmd);

				if (scene->IsAccelerationStructureUpdateRequested())
				{
					wi::renderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
				}
				});

			// Main scene:
			cmd = device->BeginCommandList();
			if (cmd_copypages.IsValid())
			{
				device->WaitCommandList(cmd, cmd_copypages);
			}
			wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

				GraphicsDevice* device = wi::graphics::GetDevice();

				wi::renderer::BindCameraCB(
					*camera,
					*camera,
					*camera,
					cmd
				);
				wi::renderer::BindCommonResources(cmd);
				wi::renderer::UpdateRenderDataAsync(visibility_main, frameCB, cmd);

				if (scene->weather.IsRealisticSky())
				{
					wi::renderer::ComputeSkyAtmosphereTextures(cmd);
					wi::renderer::ComputeSkyAtmosphereSkyViewLut(cmd);
				}

				if (wi::renderer::GetRaytraceDebugBVHVisualizerEnabled())
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&traceResult, RenderPassImage::LoadOp::CLEAR)
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);

					Viewport vp;
					vp.width = (float)traceResult.GetDesc().width;
					vp.height = (float)traceResult.GetDesc().height;
					device->BindViewports(1, &vp, cmd);

					wi::renderer::RayTraceSceneBVH(*scene, cmd);

					device->RenderPassEnd(cmd);
				}
				else
				{
					auto range = wi::profiler::BeginRangeGPU("Traced Scene", cmd);

					wi::renderer::RayTraceScene(
						*scene,
						traceResult,
						sam,
						cmd,
						denoiserAlbedo.IsValid() ? &denoiserAlbedo : nullptr,
						denoiserNormal.IsValid() ? &denoiserNormal : nullptr,
						traceDepth.IsValid() ? &traceDepth : nullptr,
						traceStencil.IsValid() ? &traceStencil : nullptr,
						depthBuffer_Main.IsValid() ? &depthBuffer_Main : nullptr
					);

					wi::profiler::EndRange(range); // Traced Scene
				}

				if (depthBuffer_Main.IsValid())
				{
					RenderPassImage rp[] = {
						RenderPassImage::DepthStencil(&depthBuffer_Main, RenderPassImage::LoadOp::LOAD),
						RenderPassImage::RenderTarget(&traceResult, RenderPassImage::LoadOp::LOAD)
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&traceResult, RenderPassImage::LoadOp::LOAD)
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				wi::renderer::DrawSpritesAndFonts(*scene, *camera, false, cmd);
				device->RenderPassEnd(cmd);

				});

			if (scene->terrains.GetCount() > 0)
			{
				CommandList cmd_allocation_tilerequest = device->BeginCommandList(QUEUE_COMPUTE);
				device->WaitCommandList(cmd_allocation_tilerequest, cmd); // wait for opaque scene
				wi::jobsystem::Execute(ctx, [this, cmd_allocation_tilerequest](wi::jobsystem::JobArgs args) {
					for (size_t i = 0; i < scene->terrains.GetCount(); ++i)
					{
						scene->terrains[i].AllocateVirtualTextureTileRequestsGPU(cmd_allocation_tilerequest);
					}
					});

				CommandList cmd_writeback_tilerequest = device->BeginCommandList(QUEUE_COPY);
				device->WaitCommandList(cmd_writeback_tilerequest, cmd_allocation_tilerequest);
				wi::jobsystem::Execute(ctx, [this, cmd_writeback_tilerequest](wi::jobsystem::JobArgs args) {
					for (size_t i = 0; i < scene->terrains.GetCount(); ++i)
					{
						scene->terrains[i].WritebackTileRequestsGPU(cmd_writeback_tilerequest);
					}
					});
			}
		}

		// Tonemap etc:
		CommandList cmd = device->BeginCommandList();
		wi::jobsystem::Execute(ctx, [this, cmd](wi::jobsystem::JobArgs args) {

			GraphicsDevice* device = wi::graphics::GetDevice();

			wi::renderer::BindCameraCB(
				*camera,
				*camera,
				*camera,
				cmd
			);
			wi::renderer::BindCommonResources(cmd);

			Texture srcTex = denoiserResult.IsValid() && !wi::jobsystem::IsBusy(denoiserContext) ? denoiserResult : traceResult;

			if (getEyeAdaptionEnabled())
			{
				wi::renderer::ComputeLuminance(
					luminanceResources,
					srcTex,
					cmd,
					getEyeAdaptionRate(),
					getEyeAdaptionKey()
				);
			}
			if (getBloomEnabled())
			{
				wi::renderer::ComputeBloom(
					bloomResources,
					srcTex,
					cmd,
					getBloomThreshold(),
					getExposure(),
					getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr
				);
			}

			wi::renderer::Postprocess_Tonemap(
				srcTex,
				rtPostprocess,
				cmd,
				getExposure(),
				getBrightness(),
				getContrast(),
				getSaturation(),
				getDitherEnabled(),
				getColorGradingEnabled() ? (scene->weather.colorGradingMap.IsValid() ? &scene->weather.colorGradingMap.GetTexture() : nullptr) : nullptr,
				nullptr,
				getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr,
				getBloomEnabled() ? &bloomResources.texture_bloom : nullptr,
				colorspace,
				getTonemap()
			);
			lastPostprocessRT = &rtPostprocess;

			// GUI Background blurring:
			{
				auto range = wi::profiler::BeginRangeGPU("GUI Background Blur", cmd);
				device->EventBegin("GUI Background Blur", cmd);
				wi::renderer::Postprocess_Downsample4x(rtPostprocess, rtGUIBlurredBackground[0], cmd);
				wi::renderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
				wi::renderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd, -1, -1, true);
				device->EventEnd(cmd);
				wi::profiler::EndRange(range);
			}
			});

		RenderPath2D::Render();

		wi::jobsystem::Wait(ctx);
	}

	void RenderPath3D_PathTracing::Compose(CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("RenderPath3D_PathTracing::Compose", cmd);

		wi::renderer::BindCommonResources(cmd);

		wi::image::Params fx;
		fx.enableFullScreen();
		fx.blendFlag = wi::enums::BLENDMODE_OPAQUE;
		fx.quality = wi::image::QUALITY_LINEAR;
		wi::image::Draw(&rtPostprocess, fx, cmd);

		device->EventEnd(cmd);

		RenderPath2D::Compose(cmd);
	}

}
