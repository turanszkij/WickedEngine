#include "MainComponent.h"
#include "RenderPath.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiInput.h"
#include "wiBackLog.h"
#include "MainComponent_BindLua.h"
#include "wiVersion.h"
#include "wiEnums.h"
#include "wiTextureHelper.h"
#include "wiProfiler.h"
#include "wiInitializer.h"
#include "wiStartupArguments.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiEvent.h"

#include "wiGraphicsDevice_DX12.h"
#include "wiGraphicsDevice_Vulkan.h"

#include <sstream>
#include <algorithm>
#include <new>
#include <cstdlib>
#include <atomic>

std::atomic<uint32_t> number_of_heap_allocations{ 0 };

using namespace wiGraphics;

void MainComponent::Initialize()
{
	if (initialized)
	{
		return;
	}
	initialized = true;

	wiInitializer::InitializeComponentsAsync();
}

void MainComponent::ActivatePath(RenderPath* component, float fadeSeconds, wiColor fadeColor)
{
	if (component != nullptr)
	{
		component->init(canvas);
	}

	// Fade manager will activate on fadeout
	fadeManager.Clear();
	fadeManager.Start(fadeSeconds, fadeColor, [this, component]() {

		if (GetActivePath() != nullptr)
		{
			GetActivePath()->Stop();
		}

		if (component != nullptr)
		{
			component->Start();
		}
		activePath = component;
	});

	fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
}

void MainComponent::Run()
{

	if (!initialized)
	{
		// Initialize in a lazy way, so the user application doesn't have to call this explicitly
		Initialize();
		initialized = true;
	}
	if (!wiInitializer::IsInitializeFinished())
	{
		// Until engine is not loaded, present initialization screen...
		CommandList cmd = graphicsDevice->BeginCommandList();
		graphicsDevice->RenderPassBegin(&swapChain, cmd);
		wiImage::SetCanvas(canvas, cmd);
		wiFont::SetCanvas(canvas, cmd);
		Viewport viewport;
		viewport.width = (float)swapChain.desc.width;
		viewport.height = (float)swapChain.desc.height;
		graphicsDevice->BindViewports(1, &viewport, cmd);
		wiFontParams params;
		params.posX = 5.f;
		params.posY = 5.f;
		std::string text = wiBackLog::getText();
		float textheight = wiFont::textHeight(text, params);
		float screenheight = canvas.GetLogicalHeight();
		if (textheight > screenheight)
		{
			params.posY = screenheight - textheight;
		}
		wiFont::Draw(text, params, cmd);
		graphicsDevice->RenderPassEnd(cmd);
		graphicsDevice->SubmitCommandLists();
		return;
	}

	static bool startup_script = false;
	if (!startup_script)
	{
		startup_script = true;
		wiLua::RegisterObject(MainComponent_BindLua::className, "main", new MainComponent_BindLua(this));
		wiLua::RunFile("startup.lua");
	}

	if (!is_window_active)
	{
		// If the application is not active, disable Update loops:
		deltaTimeAccumulator = 0;
		return;
	}

	wiProfiler::BeginFrame();

	deltaTime = float(std::max(0.0, timer.elapsed() / 1000.0));
	timer.record();

	// Wake up the events that need to be executed on the main thread, in thread safe manner:
	wiEvent::FireEvent(SYSTEM_EVENT_THREAD_SAFE_POINT, 0);

	const float dt = framerate_lock ? (1.0f / targetFrameRate) : deltaTime;

	fadeManager.Update(dt);

	ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->colorspace = colorspace;
		GetActivePath()->init(canvas);
		GetActivePath()->PreUpdate();
	}

	// Fixed time update:
	auto range = wiProfiler::BeginRangeCPU("Fixed Update");
	{
		if (frameskip)
		{
			deltaTimeAccumulator += dt;
			if (deltaTimeAccumulator > 10)
			{
				// application probably lost control, fixed update would take too long
				deltaTimeAccumulator = 0;
			}

			const float targetFrameRateInv = 1.0f / targetFrameRate;
			while (deltaTimeAccumulator >= targetFrameRateInv)
			{
				FixedUpdate();
				deltaTimeAccumulator -= targetFrameRateInv;
			}
		}
		else
		{
			FixedUpdate();
		}
	}
	wiProfiler::EndRange(range); // Fixed Update

	// Variable-timed update:
	Update(dt);

	Render();

	wiInput::Update(window);

	// Begin final compositing:
	CommandList cmd = graphicsDevice->BeginCommandList();
	wiImage::SetCanvas(canvas, cmd);
	wiFont::SetCanvas(canvas, cmd);
	Viewport viewport;
	viewport.width = (float)swapChain.desc.width;
	viewport.height = (float)swapChain.desc.height;
	graphicsDevice->BindViewports(1, &viewport, cmd);

	bool colorspace_conversion_required = colorspace == ColorSpace::HDR10_ST2084;
	if (colorspace_conversion_required)
	{
		// In HDR10, we perform the compositing in a custom linear color space render target
		graphicsDevice->RenderPassBegin(&renderpass, cmd);
	}
	else
	{
		// If swapchain is SRGB or Linear HDR, it can be used for blending
		//	- If it is SRGB, the render path will ensure tonemapping to SDR
		//	- If it is Linear HDR, we can blend trivially in linear space
		graphicsDevice->RenderPassBegin(&swapChain, cmd);
	}
	Compose(cmd);
	graphicsDevice->RenderPassEnd(cmd);

	if (colorspace_conversion_required)
	{
		// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
		graphicsDevice->RenderPassBegin(&swapChain, cmd);
		wiImageParams fx;
		fx.enableFullScreen();
		fx.enableHDR10OutputMapping();
		wiImage::Draw(&rendertarget, fx, cmd);
		graphicsDevice->RenderPassEnd(cmd);
	}

	wiProfiler::EndFrame(cmd);
	graphicsDevice->SubmitCommandLists();
}

void MainComponent::Update(float dt)
{
	auto range = wiProfiler::BeginRangeCPU("Update");

	wiLua::SetDeltaTime(double(dt));
	wiLua::Update();

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Update(dt);
		GetActivePath()->PostUpdate();
	}

	wiProfiler::EndRange(range); // Update
}

void MainComponent::FixedUpdate()
{
	wiBackLog::Update(canvas);
	wiLua::FixedUpdate();

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->FixedUpdate();
	}
}

void MainComponent::Render()
{
	auto range = wiProfiler::BeginRangeCPU("Render");

	wiLua::Render();

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Render();
	}

	wiProfiler::EndRange(range); // Render
}

void MainComponent::Compose(CommandList cmd)
{
	auto range = wiProfiler::BeginRangeCPU("Compose");

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Compose(cmd);
	}

	if (fadeManager.IsActive())
	{
		// display fade rect
		static wiImageParams fx;
		fx.siz.x = canvas.GetLogicalWidth();
		fx.siz.y = canvas.GetLogicalHeight();
		fx.opacity = fadeManager.opacity;
		wiImage::Draw(wiTextureHelper::getColor(fadeManager.color), fx, cmd);
	}

	// Draw the information display
	if (infoDisplay.active)
	{
		std::stringstream ss("");
		if (infoDisplay.watermark)
		{
			ss << "Wicked Engine " << wiVersion::GetVersionString() << " ";

#if defined(_ARM)
			ss << "[ARM]";
#elif defined(_WIN64)
			ss << "[64-bit]";
#elif defined(_WIN32)
			ss << "[32-bit]";
#endif

#ifdef PLATFORM_UWP
			ss << "[UWP]";
#endif

#ifdef WICKEDENGINE_BUILD_DX12
			if (dynamic_cast<GraphicsDevice_DX12*>(graphicsDevice.get()))
			{
				ss << "[DX12]";
			}
#endif
#ifdef WICKEDENGINE_BUILD_VULKAN
			if (dynamic_cast<GraphicsDevice_Vulkan*>(graphicsDevice.get()))
			{
				ss << "[Vulkan]";
			}
#endif

#ifdef _DEBUG
			ss << "[DEBUG]";
#endif
			if (graphicsDevice->IsDebugDevice())
			{
				ss << "[debugdevice]";
			}
			ss << std::endl;
		}
		if (infoDisplay.resolution)
		{
			ss << "Resolution: " << canvas.GetPhysicalWidth() << " x " << canvas.GetPhysicalHeight() << " (" << canvas.GetDPI() << " dpi)" << std::endl;
		}
		if (infoDisplay.logical_size)
		{
			ss << "Logical Size: " << canvas.GetLogicalWidth() << " x " << canvas.GetLogicalHeight() << std::endl;
		}
		if (infoDisplay.colorspace)
		{
			ss << "Color Space: ";
			ColorSpace colorSpace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
			switch (colorSpace)
			{
			default:
			case wiGraphics::ColorSpace::SRGB:
				ss << "sRGB";
				break;
			case wiGraphics::ColorSpace::HDR10_ST2084:
				ss << "ST.2084 (HDR10)";
				break;
			case wiGraphics::ColorSpace::HDR_LINEAR:
				ss << "Linear (HDR)";
				break;
			}
			ss << std::endl;
		}
		if (infoDisplay.fpsinfo)
		{
			deltatimes[fps_avg_counter++ % arraysize(deltatimes)] = deltaTime;
			float displaydeltatime = deltaTime;
			if (fps_avg_counter > arraysize(deltatimes))
			{
				float avg_time = 0;
				for (int i = 0; i < arraysize(deltatimes); ++i)
				{
					avg_time += deltatimes[i];
				}
				displaydeltatime = avg_time / arraysize(deltatimes);
			}

			ss.precision(2);
			ss << std::fixed << 1.0f / displaydeltatime << " FPS" << std::endl;
		}
		if (infoDisplay.heap_allocation_counter)
		{
			ss << "Heap allocations per frame: " << number_of_heap_allocations.load() << std::endl;
			number_of_heap_allocations.store(0);
		}
		if (infoDisplay.pipeline_count)
		{
			ss << "Graphics pipelines active: " << graphicsDevice->GetActivePipelineCount() << std::endl;
		}

#ifdef _DEBUG
		ss << "Warning: This is a [DEBUG] build, performance will be slow!" << std::endl;
#endif
		if (graphicsDevice->IsDebugDevice())
		{
			ss << "Warning: Graphics is in [debugdevice] mode, performance will be slow!" << std::endl;
		}

		ss.precision(2);
		wiFont::Draw(ss.str(), wiFontParams(4, 4, infoDisplay.size, WIFALIGN_LEFT, WIFALIGN_TOP, wiColor(255,255,255,255), wiColor(0,0,0,255)), cmd);

		if (infoDisplay.colorgrading_helper)
		{
			wiImage::Draw(wiTextureHelper::getColorGradeDefault(), wiImageParams(0, 0, 256.0f / canvas.GetDPIScaling(), 16.0f / canvas.GetDPIScaling()), cmd);
		}
	}

	wiProfiler::DrawData(canvas, 4, 120, cmd);

	wiBackLog::Draw(canvas, cmd);

	wiProfiler::EndRange(range); // Compose
}

void MainComponent::SetWindow(wiPlatform::window_type window, bool fullscreen)
{
	this->window = window;

	// User can also create a graphics device if custom logic is desired, but they must do before this function!
	if (graphicsDevice == nullptr)
	{
		bool debugdevice = wiStartupArguments::HasArgument("debugdevice");
		bool gpuvalidation = wiStartupArguments::HasArgument("gpuvalidation");

		bool use_dx12 = wiStartupArguments::HasArgument("dx12");
		bool use_vulkan = wiStartupArguments::HasArgument("vulkan");

#ifndef WICKEDENGINE_BUILD_DX12
		if (use_dx12) {
			wiHelper::messageBox("The engine was built without DX12 support!", "Error");
			use_dx12 = false;
		}
#endif
#ifndef WICKEDENGINE_BUILD_VULKAN
		if (use_vulkan) {
			wiHelper::messageBox("The engine was built without Vulkan support!", "Error");
			use_vulkan = false;
		}
#endif

		if (!use_dx12 && !use_vulkan)
		{
#if defined(WICKEDENGINE_BUILD_DX12)
			use_dx12 = true;
#elif defined(WICKEDENGINE_BUILD_VULKAN)
			use_vulkan = true;
#else
			wiBackLog::post("No rendering backend is enabled! Please enable at least one so we can use it as default");
			assert(false);
#endif
		}
		assert(use_dx12 || use_vulkan);

		if (use_vulkan)
		{
#ifdef WICKEDENGINE_BUILD_VULKAN
			wiRenderer::SetShaderPath(wiRenderer::GetShaderPath() + "spirv/");
			graphicsDevice = std::make_unique<GraphicsDevice_Vulkan>(window, debugdevice);
#endif
		}
		else if (use_dx12)
		{
#ifdef WICKEDENGINE_BUILD_DX12
			wiRenderer::SetShaderPath(wiRenderer::GetShaderPath() + "hlsl6/");
			graphicsDevice = std::make_unique<GraphicsDevice_DX12>(debugdevice, gpuvalidation);
#endif
		}
	}
	wiGraphics::GetDevice() = graphicsDevice.get();

	canvas.init(window);

	SwapChainDesc desc;
	if (swapChain.IsValid())
	{
		// it will only resize, but keep format and other settings
		desc = swapChain.desc;
	}
	else
	{
		// initialize for the first time
		desc.buffer_count = 3;
		desc.format = Format::R10G10B10A2_UNORM;
	}
	desc.width = canvas.GetPhysicalWidth();
	desc.height = canvas.GetPhysicalHeight();
	desc.allow_hdr = allow_hdr;
	bool success = graphicsDevice->CreateSwapChain(&desc, window, &swapChain);
	assert(success);

	swapChainVsyncChangeEvent = wiEvent::Subscribe(SYSTEM_EVENT_SET_VSYNC, [this](uint64_t userdata) {
		SwapChainDesc desc = swapChain.desc;
		desc.vsync = userdata != 0;
		bool success = graphicsDevice->CreateSwapChain(&desc, nullptr, &swapChain);
		assert(success);
		});

	if (graphicsDevice->GetSwapChainColorSpace(&swapChain) == ColorSpace::HDR10_ST2084)
	{
		TextureDesc desc;
		desc.width = swapChain.desc.width;
		desc.height = swapChain.desc.height;
		desc.format = Format::R11G11B10_FLOAT;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		bool success = graphicsDevice->CreateTexture(&desc, nullptr, &rendertarget);
		assert(success);
		graphicsDevice->SetName(&rendertarget, "MainComponent::rendertarget");

		RenderPassDesc renderpassdesc;
		renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&rendertarget, RenderPassAttachment::LoadOp::CLEAR));
		success = graphicsDevice->CreateRenderPass(&renderpassdesc, &renderpass);
		assert(success);
	}
}


// Heap alloc replacements are used to count heap allocations:
//	It is good practice to reduce the amount of heap allocations that happen during the frame,
//	so keep an eye on the info display of the engine while MainComponent::InfoDisplayer::heap_allocation_counter is enabled

void* operator new(std::size_t size) {
	number_of_heap_allocations.fetch_add(1);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size) {
	number_of_heap_allocations.fetch_add(1);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size, const std::nothrow_t&) throw() {
	number_of_heap_allocations.fetch_add(1);
	return malloc(size);
}
void* operator new(std::size_t size, const std::nothrow_t&) throw() {
	number_of_heap_allocations.fetch_add(1);
	return malloc(size);
}
void operator delete(void* ptr) throw() { free(ptr); }
void operator delete (void* ptr, const std::nothrow_t&) throw() { free(ptr); }
void operator delete[](void* ptr) throw() { free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) throw() { free(ptr); }
