#include "stdafx.h"
#include "Example_ImGui.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#ifdef _WIN32
#include "ImGui/imgui_impl_win32.h"
#elif defined(SDL2)
#include "ImGui/imgui_impl_sdl.h"
#endif

#include <fstream>
#include <thread>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

Shader imguiVS;
Shader imguiPS;
Texture fontTexture;
Sampler sampler;
InputLayout	imguiInputLayout;
PipelineState imguiPSO;

struct ImGui_Impl_Data
{
};

static ImGui_Impl_Data* ImGui_Impl_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_Impl_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool ImGui_Impl_CreateDeviceObjects()
{
	auto* backendData = ImGui_Impl_GetBackendData();

	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Upload texture to graphics system
	TextureDesc textureDesc;
	textureDesc.width = width;
	textureDesc.height = height;
	textureDesc.mip_levels = 1;
	textureDesc.array_size = 1;
	textureDesc.format = Format::R8G8B8A8_UNORM;
	textureDesc.bind_flags = BindFlag::SHADER_RESOURCE;

	SubresourceData textureData;
	textureData.data_ptr = pixels;
	textureData.row_pitch = width * GetFormatStride(textureDesc.format);
	textureData.slice_pitch = textureData.row_pitch * height;

	wi::graphics::GetDevice()->CreateTexture(&textureDesc, &textureData, &fontTexture);

	SamplerDesc samplerDesc;
	samplerDesc.address_u = TextureAddressMode::WRAP;
	samplerDesc.address_v = TextureAddressMode::WRAP;
	samplerDesc.address_w = TextureAddressMode::WRAP;
	samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
	wi::graphics::GetDevice()->CreateSampler(&samplerDesc, &sampler);

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)&fontTexture);

	imguiInputLayout.elements =
	{
		{ "POSITION", 0, Format::R32G32_FLOAT, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, pos), InputClassification::PER_VERTEX_DATA },
		{ "TEXCOORD", 0, Format::R32G32_FLOAT, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, uv), InputClassification::PER_VERTEX_DATA },
		{ "COLOR", 0, Format::R8G8B8A8_UNORM, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, col), InputClassification::PER_VERTEX_DATA },
	};

	// Create pipeline
	PipelineStateDesc desc;
	desc.vs = &imguiVS;
	desc.ps = &imguiPS;
	desc.il = &imguiInputLayout;
	desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHREAD);
	desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
	desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
	desc.pt = PrimitiveTopology::TRIANGLELIST;
	wi::graphics::GetDevice()->CreatePipelineState(&desc, &imguiPSO);

	return true;
}

Example_ImGui::~Example_ImGui()
{
	// Cleanup
	//ImGui_ImplDX11_Shutdown();
#ifdef _WIN32
	ImGui_ImplWin32_Shutdown();
#elif defined(SDL2)
	ImGui_ImplSDL2_Shutdown();
#endif
	ImGui::DestroyContext();
}

void Example_ImGui::Initialize()
{
	// Compile shaders
	{
		auto shaderPath = wi::renderer::GetShaderSourcePath();
		wi::renderer::SetShaderSourcePath(wi::helper::GetCurrentPath() + "/");

		wi::renderer::LoadShader(ShaderStage::VS, imguiVS, "ImGuiVS.cso");
		wi::renderer::LoadShader(ShaderStage::PS, imguiPS, "ImGuiPS.cso");

		wi::renderer::SetShaderSourcePath(shaderPath);
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

#ifdef _WIN32
	ImGui_ImplWin32_Init(window);
#elif defined(SDL2)
	ImGui_ImplSDL2_InitForVulkan(window);
#endif

	IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_Impl_Data* bd = IM_NEW(ImGui_Impl_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "Wicked";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	Application::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;
	infoDisplay.heap_allocation_counter = true;

	renderer.init(canvas);
	renderer.Load();

	ActivatePath(&renderer);
}

void Example_ImGui::Compose(wi::graphics::CommandList cmd)
{
	Application::Compose(cmd);

	// Rendering
	ImGui::Render();

	auto drawData = ImGui::GetDrawData();

	if (!drawData || drawData->TotalVtxCount == 0)
	{
		return;
	}

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	auto* bd = ImGui_Impl_GetBackendData();

	GraphicsDevice* device = wi::graphics::GetDevice();

	// Get memory for vertex and index buffers
	const uint64_t vbSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
	const uint64_t ibSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
	auto vertexBufferAllocation = device->AllocateGPU(vbSize, cmd);
	auto indexBufferAllocation = device->AllocateGPU(ibSize, cmd);

	// Copy and convert all vertices into a single contiguous buffer
	ImDrawVert* vertexCPUMem = reinterpret_cast<ImDrawVert*>(vertexBufferAllocation.data);
	ImDrawIdx* indexCPUMem = reinterpret_cast<ImDrawIdx*>(indexBufferAllocation.data);
	for (int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; cmdListIdx++)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
		memcpy(vertexCPUMem, &drawList->VtxBuffer[0], drawList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(indexCPUMem, &drawList->IdxBuffer[0], drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vertexCPUMem += drawList->VtxBuffer.Size;
		indexCPUMem += drawList->IdxBuffer.Size;
	}

	// Setup orthographic projection matrix into our constant buffer
	struct ImGuiConstants
	{
		float   mvp[4][4];
	};

	{
		const float L = drawData->DisplayPos.x;
		const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float T = drawData->DisplayPos.y;
		const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

		//Matrix4x4::CreateOrthographicOffCenter(0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 0.0f, 1.0f, &constants.projectionMatrix);

		ImGuiConstants constants;

		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constants.mvp, mvp, sizeof(mvp));

		device->BindDynamicConstantBuffer(constants, 0, cmd);
	}

	const GPUBuffer* vbs[] = {
		&vertexBufferAllocation.buffer,
	};
	const uint32_t strides[] = {
		sizeof(ImDrawVert),
	};
	const uint64_t offsets[] = {
		vertexBufferAllocation.offset,
	};

	device->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
	device->BindIndexBuffer(&indexBufferAllocation.buffer, IndexBufferFormat::UINT16, indexBufferAllocation.offset, cmd);

	Viewport viewport;
	viewport.width = (float)fb_width;
	viewport.height = (float)fb_height;
	device->BindViewports(1, &viewport, cmd);

	device->BindPipelineState(&imguiPSO, cmd);

	device->BindSampler(&sampler, 0, cmd);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	//passEncoder->SetSampler(0, Sampler::LinearWrap());

	// Render command lists
	int32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	for (uint32_t cmdListIdx = 0; cmdListIdx < (uint32_t)drawData->CmdListsCount; ++cmdListIdx)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
		for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)drawList->CmdBuffer.size(); ++cmdIndex)
		{
			const ImDrawCmd* drawCmd = &drawList->CmdBuffer[cmdIndex];
			if (drawCmd->UserCallback)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
				{
				}
				else
				{
					drawCmd->UserCallback(drawList, drawCmd);
				}
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(drawCmd->ClipRect.x - clip_off.x, drawCmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(drawCmd->ClipRect.z - clip_off.x, drawCmd->ClipRect.w - clip_off.y);
				if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				Rect scissor;
				scissor.left = (int32_t)(clip_min.x);
				scissor.top = (int32_t)(clip_min.y);
				scissor.right = (int32_t)(clip_max.x);
				scissor.bottom = (int32_t)(clip_max.y);
				device->BindScissorRects(1, &scissor, cmd);

				const Texture* texture = (const Texture*)drawCmd->TextureId;
				device->BindResource(texture, 0, cmd);
				device->DrawIndexed(drawCmd->ElemCount, indexOffset, vertexOffset, cmd);
			}
			indexOffset += drawCmd->ElemCount;
		}
		vertexOffset += drawList->VtxBuffer.size();
	}

	//// Update and Render additional Platform Windows
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	//{
	//	ImGui::UpdatePlatformWindows();
	//	//ImGui::RenderPlatformWindowsDefault(NULL, (void*)g_pd3dCommandList);
	//}
}

void Example_ImGuiRenderer::ResizeLayout()
{
	RenderPath3D::ResizeLayout();

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	label.SetPos(XMFLOAT2(screenW / 2.f - label.scale.x / 2.f, screenH * 0.95f));
}

void Example_ImGuiRenderer::Render() const
{
	RenderPath3D::Render();
}

void Example_ImGuiRenderer::Load()
{
	setSSREnabled(false);
	setReflectionsEnabled(true);
	setFXAAEnabled(false);

	label.Create("Label1");
	label.SetText("Wicked Engine ImGui integration");
	label.font.params.h_align = wi::font::WIFALIGN_CENTER;
	label.SetSize(XMFLOAT2(240, 20));
	GetGUI().AddWidget(&label);

	// Reset all state that tests might have modified:
	wi::eventhandler::SetVSync(true);
	wi::renderer::SetToDrawGridHelper(false);
	wi::renderer::SetTemporalAAEnabled(true);
	wi::renderer::ClearWorld(wi::scene::GetScene());
	wi::scene::GetScene().weather = WeatherComponent();
	this->ClearSprites();
	this->ClearFonts();
	if (wi::lua::GetLuaState() != nullptr) {
		wi::lua::KillProcesses();
	}

	// Reset camera position:
	TransformComponent transform;
	transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
	transform.UpdateTransform();
	wi::scene::GetCamera().TransformCamera(transform);

	// Load model.
	wi::scene::LoadModel("../Content/models/teapot.wiscene");

	RenderPath3D::Load();
}

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void Example_ImGuiRenderer::Update(float dt)
{
	// Start the Dear ImGui frame
	auto* backendData = ImGui_Impl_GetBackendData();
	IM_ASSERT(backendData != NULL);

	if (!fontTexture.IsValid())
	{
		ImGui_Impl_CreateDeviceObjects();
	}

#ifdef _WIN32
	ImGui_ImplWin32_NewFrame();
#elif defined(SDL2)
	ImGui_ImplSDL2_NewFrame();
#endif
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	Scene& scene = wi::scene::GetScene();
	// teapot_material Base Base_mesh Top Top_mesh editorLight
	wi::ecs::Entity e_teapot_base = scene.Entity_FindByName("Base");
	wi::ecs::Entity e_teapot_top = scene.Entity_FindByName("Top");
	assert(e_teapot_base != wi::ecs::INVALID_ENTITY);
	assert(e_teapot_top != wi::ecs::INVALID_ENTITY);
	TransformComponent* transform_base = scene.transforms.GetComponent(e_teapot_base);
	TransformComponent* transform_top = scene.transforms.GetComponent(e_teapot_top);
	assert(transform_base != nullptr);
	assert(transform_top != nullptr);
	float rotation = dt;
	if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LEFT))
	{
		transform_base->Rotate(XMVectorSet(0, rotation, 0, 1));
		transform_top->Rotate(XMVectorSet(0, rotation, 0, 1));
	}
	else if (wi::input::Down(wi::input::KEYBOARD_BUTTON_RIGHT))
	{
		transform_base->Rotate(XMVectorSet(0, -rotation, 0, 1));
		transform_top->Rotate(XMVectorSet(0, -rotation, 0, 1));
	}

	RenderPath3D::Update(dt);
}
