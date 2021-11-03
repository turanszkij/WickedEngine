#include "stdafx.h"
#include "Tests.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#ifdef _WIN32
#include "ImGui/imgui_impl_win32.h"
#endif

#include <string>
#include <sstream>
#include <fstream>
#include <thread>

using namespace wiECS;
using namespace wiGraphics;
using namespace wiScene;

Shader imguiVS;
Shader imguiPS;
Texture fontTexture;
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
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = FORMAT_R8G8B8A8_UNORM;
	textureDesc.BindFlags = BIND_SHADER_RESOURCE;

	SubresourceData textureData;
	textureData.pData = pixels;
	textureData.rowPitch = width * GetFormatStride(textureDesc.Format);
	textureData.slicePitch = textureData.rowPitch * height;

	wiRenderer::GetDevice()->CreateTexture(&textureDesc, &textureData, &fontTexture);

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)&fontTexture);

	imguiInputLayout.elements =
	{
		{ "POSITION", 0, FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, pos), INPUT_PER_VERTEX_DATA },
		{ "TEXCOORD", 0, FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, uv), INPUT_PER_VERTEX_DATA },
		{ "COLOR", 0, FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), INPUT_PER_VERTEX_DATA },
	};

	// Create pipeline
	PipelineStateDesc desc;
	desc.vs = &imguiVS;
	desc.ps = &imguiPS;
	desc.il = &imguiInputLayout;
	desc.dss = wiRenderer::GetDepthStencilState(DSSTYPE_DEPTHREAD);
	desc.rs = wiRenderer::GetRasterizerState(RSTYPE_DOUBLESIDED);
	desc.bs = wiRenderer::GetBlendState(BSTYPE_TRANSPARENT);
	desc.pt = TRIANGLELIST;
	wiRenderer::GetDevice()->CreatePipelineState(&desc, &imguiPSO);

	return true;
}

Tests::~Tests()
{
	// Cleanup
	//ImGui_ImplDX11_Shutdown();
#ifdef _WIN32
	ImGui_ImplWin32_Shutdown();
#endif
	ImGui::DestroyContext();
}

void Tests::Initialize()
{
	// Compile shaders
	{
		wiShaderCompiler::Initialize();

		auto shaderPath = wiRenderer::GetShaderSourcePath();
		wiRenderer::SetShaderSourcePath(wiHelper::GetCurrentPath() + "/");

		wiRenderer::LoadShader(VS, imguiVS, "ImGuiVS.cso");
		wiRenderer::LoadShader(PS, imguiPS, "ImGuiPS.cso");

		wiRenderer::SetShaderSourcePath(shaderPath);
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
#endif

	IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_Impl_Data* bd = IM_NEW(ImGui_Impl_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "Wicked";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	MainComponent::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;
	infoDisplay.heap_allocation_counter = true;

	renderer.init(canvas);
	renderer.Load();

	ActivatePath(&renderer);
}

void Tests::Compose(wiGraphics::CommandList cmd)
{
	MainComponent::Compose(cmd);

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

	GraphicsDevice* device = wiRenderer::GetDevice();

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
	device->BindIndexBuffer(&indexBufferAllocation.buffer, INDEXFORMAT_16BIT, indexBufferAllocation.offset, cmd);

	Viewport viewport;
	viewport.Width = (float)fb_width;
	viewport.Height = (float)fb_height;
	device->BindViewports(1, &viewport, cmd);

	device->BindPipelineState(&imguiPSO, cmd);

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

void TestsRenderer::ResizeLayout()
{
	RenderPath3D::ResizeLayout();

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	label.SetPos(XMFLOAT2(screenW / 2.f - label.scale.x / 2.f, screenH * 0.95f));
}

void TestsRenderer::Render() const
{
	RenderPath3D::Render();
}

void TestsRenderer::Load()
{
	setSSREnabled(false);
	setReflectionsEnabled(true);
	setFXAAEnabled(false);

	label.Create("Label1");
	label.SetText("Wicked Engine ImGui integration");
	label.font.params.h_align = WIFALIGN_CENTER;
	label.SetSize(XMFLOAT2(240, 20));
	GetGUI().AddWidget(&label);

	testSelector.Create("TestSelector");
	testSelector.SetText("Demo: ");
	testSelector.SetSize(XMFLOAT2(140, 20));
	testSelector.SetPos(XMFLOAT2(50, 200));
	testSelector.SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	testSelector.SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	testSelector.AddItem("HelloWorld");
	testSelector.AddItem("Model");
	testSelector.AddItem("EmittedParticle 1");
	testSelector.AddItem("EmittedParticle 2");
	testSelector.AddItem("HairParticle");
	testSelector.AddItem("Lua Script");
	testSelector.AddItem("Water Test");
	testSelector.AddItem("Shadows Test");
	testSelector.AddItem("Physics Test");
	testSelector.AddItem("Cloth Physics Test");
	testSelector.AddItem("Job System Test");
	testSelector.AddItem("Font Test");
	testSelector.AddItem("Volumetric Test");
	testSelector.AddItem("Sprite Test");
	testSelector.AddItem("Lightmap Bake Test");
	testSelector.AddItem("Network Test");
	testSelector.AddItem("Controller Test");
	testSelector.AddItem("Inverse Kinematics");
	testSelector.AddItem("65k Instances");
	testSelector.SetMaxVisibleItemCount(10);
	testSelector.OnSelect([=](wiEventArgs args) {

		// Reset all state that tests might have modified:
		wiEvent::SetVSync(true);
		wiRenderer::SetToDrawGridHelper(false);
		wiRenderer::SetTemporalAAEnabled(false);
		wiRenderer::ClearWorld(wiScene::GetScene());
		wiScene::GetScene().weather = WeatherComponent();
		this->ClearSprites();
		this->ClearFonts();
		if (wiLua::GetLuaState() != nullptr) {
			wiLua::KillProcesses();
		}

		// Reset camera position:
		TransformComponent transform;
		transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
		transform.UpdateTransform();
		wiScene::GetCamera().TransformCamera(transform);

		float screenW = GetLogicalWidth();
		float screenH = GetLogicalHeight();

		// Based on combobox selection, start the appropriate test:
		switch (args.iValue)
		{
			case 0:
			{
				// This will spawn a sprite with two textures. The first texture is a color texture and it will be animated.
				//	The second texture is a static image of "hello world" written on it
				//	Then add some animations to the sprite to get a nice wobbly and color changing effect.
				//	You can learn more in the Sprite test in RunSpriteTest() function
				static wiSprite sprite;
				sprite = wiSprite("images/movingtex.png", "images/HelloWorld.png");
				sprite.params.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
				sprite.params.siz = XMFLOAT2(200, 100);
				sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
				sprite.anim.rot = XM_PI / 4.0f;
				sprite.anim.wobbleAnim.amount = XMFLOAT2(0.16f, 0.16f);
				sprite.anim.movingTexAnim.speedX = 0;
				sprite.anim.movingTexAnim.speedY = 3;
				this->AddSprite(&sprite);
				break;
			}
			case 1:
				wiRenderer::SetTemporalAAEnabled(true);
				wiScene::LoadModel("../Content/models/teapot.wiscene");
				break;
			case 2:
				wiScene::LoadModel("../Content/models/emitter_smoke.wiscene");
				break;
			case 3:
				wiScene::LoadModel("../Content/models/emitter_skinned.wiscene");
				break;
			case 4:
				wiScene::LoadModel("../Content/models/hairparticle_torus.wiscene", XMMatrixTranslation(0, 1, 0));
				break;
			case 5:
				wiRenderer::SetToDrawGridHelper(true);
				wiLua::RunFile("test_script.lua");
				break;
			case 6:
				wiRenderer::SetTemporalAAEnabled(true);
				wiScene::LoadModel("../Content/models/water_test.wiscene", XMMatrixTranslation(0, 1, 0));
				break;
			case 7:
				wiRenderer::SetTemporalAAEnabled(true);
				wiScene::LoadModel("../Content/models/shadows_test.wiscene", XMMatrixTranslation(0, 1, 0));
				break;
			case 8:
				wiRenderer::SetTemporalAAEnabled(true);
				wiScene::LoadModel("../Content/models/physics_test.wiscene");
				break;
			case 9:
				wiScene::LoadModel("../Content/models/cloth_test.wiscene", XMMatrixTranslation(0, 3, 4));
				break;
			case 10:
				RunJobSystemTest();
				break;
			case 11:
				RunFontTest();
				break;
			case 12:
				wiRenderer::SetTemporalAAEnabled(true);
				wiScene::LoadModel("../Content/models/volumetric_test.wiscene", XMMatrixTranslation(0, 0, 4));
				break;
			case 13:
				RunSpriteTest();
				break;
			case 14:
				wiEvent::SetVSync(false); // turn off vsync if we can to accelerate the baking
				wiRenderer::SetTemporalAAEnabled(true);
				wiScene::LoadModel("../Content/models/lightmap_bake_test.wiscene", XMMatrixTranslation(0, 0, 4));
				break;
			case 15:
				RunNetworkTest();
				break;
			case 16:
			{
				static wiSpriteFont font("This test plays a vibration on the first controller's left motor (if device supports it) \n and changes the LED to a random color (if device supports it)");
				font.params.h_align = WIFALIGN_CENTER;
				font.params.v_align = WIFALIGN_CENTER;
				font.params.size = 20;
				font.params.posX = screenW / 2;
				font.params.posY = screenH / 2;
				AddFont(&font);

				wiInput::ControllerFeedback feedback;
				feedback.led_color.rgba = wiRandom::getRandom(0xFFFFFF);
				feedback.vibration_left = 0.9f;
				wiInput::SetControllerFeedback(feedback, 0);
			}
			break;
			case 17:
			{
				Scene scene;
				LoadModel(scene, "../Content/models/girl.wiscene", XMMatrixScaling(0.7f, 0.7f, 0.7f));

				ik_entity = scene.Entity_FindByName("mano_L"); // hand bone in girl.wiscene
				if (ik_entity != INVALID_ENTITY)
				{
					InverseKinematicsComponent& ik = scene.inverse_kinematics.Create(ik_entity);
					ik.chain_length = 2; // lower and upper arm included (two parents in hierarchy of hand)
					ik.iteration_count = 5; // precision of ik simulation
					ik.target = CreateEntity();
					scene.transforms.Create(ik.target);
				}

				// Play walk animation:
				Entity walk = scene.Entity_FindByName("walk");
				AnimationComponent* walk_anim = scene.animations.GetComponent(walk);
				if (walk_anim != nullptr)
				{
					walk_anim->Play();
				}

				// Add some nice weather, not just black:
				auto& weather = scene.weathers.Create(CreateEntity());
				weather.ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
				weather.horizon = XMFLOAT3(0.38f, 0.38f, 0.38f);
				weather.zenith = XMFLOAT3(0.42f, 0.42f, 0.42f);
				weather.cloudiness = 0.75f;

				wiScene::GetScene().Merge(scene); // add lodaded scene to global scene
			}
			break;

			case 18:
			{
				wiScene::LoadModel("../Content/models/cube.wiscene");
				wiProfiler::SetEnabled(true);
				Scene& scene = wiScene::GetScene();
				scene.Entity_CreateLight("testlight", XMFLOAT3(0, 2, -4), XMFLOAT3(1, 1, 1), 4, 10);
				Entity cubeentity = scene.Entity_FindByName("Cube");
				const float scale = 0.06f;
				for (int x = 0; x < 32; ++x)
				{
					for (int y = 0; y < 32; ++y)
					{
						for (int z = 0; z < 64; ++z)
						{
							Entity entity = scene.Entity_Duplicate(cubeentity);
							TransformComponent* transform = scene.transforms.GetComponent(entity);
							transform->Scale(XMFLOAT3(scale, scale, scale));
							transform->Translate(XMFLOAT3(-5.5f + 11 * float(x) / 32.f, -0.5f + 5 * y / 32.f, float(z) * 0.5f));
						}
					}
				}
				scene.Entity_Remove(cubeentity);
			}
			break;

			default:
				assert(0);
				break;
		}

		});
	testSelector.SetSelected(0);
	GetGUI().AddWidget(&testSelector);

	RenderPath3D::Load();
}

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void TestsRenderer::Update(float dt)
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

	switch (testSelector.GetSelected())
	{
		case 1:
		{
			Scene& scene = wiScene::GetScene();
			// teapot_material Base Base_mesh Top Top_mesh editorLight
			wiECS::Entity e_teapot_base = scene.Entity_FindByName("Base");
			wiECS::Entity e_teapot_top = scene.Entity_FindByName("Top");
			assert(e_teapot_base != wiECS::INVALID_ENTITY);
			assert(e_teapot_top != wiECS::INVALID_ENTITY);
			TransformComponent* transform_base = scene.transforms.GetComponent(e_teapot_base);
			TransformComponent* transform_top = scene.transforms.GetComponent(e_teapot_top);
			assert(transform_base != nullptr);
			assert(transform_top != nullptr);
			float rotation = dt;
			if (wiInput::Down(wiInput::KEYBOARD_BUTTON_LEFT))
			{
				transform_base->Rotate(XMVectorSet(0, rotation, 0, 1));
				transform_top->Rotate(XMVectorSet(0, rotation, 0, 1));
			}
			else if (wiInput::Down(wiInput::KEYBOARD_BUTTON_RIGHT))
			{
				transform_base->Rotate(XMVectorSet(0, -rotation, 0, 1));
				transform_top->Rotate(XMVectorSet(0, -rotation, 0, 1));
			}
		}
		break;
		case 17:
		{
			if (ik_entity != INVALID_ENTITY)
			{
				// Inverse kinematics test:
				Scene& scene = wiScene::GetScene();
				InverseKinematicsComponent& ik = *scene.inverse_kinematics.GetComponent(ik_entity);
				TransformComponent& target = *scene.transforms.GetComponent(ik.target);

				// place ik target on a plane intersected by mouse ray:
				RAY ray = wiRenderer::GetPickRay((long)wiInput::GetPointer().x, (long)wiInput::GetPointer().y, *this);
				XMVECTOR plane = XMVectorSet(0, 0, 1, 0.2f);
				XMVECTOR I = XMPlaneIntersectLine(plane, XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction) * 10000);
				target.ClearTransform();
				XMFLOAT3 _I;
				XMStoreFloat3(&_I, I);
				target.Translate(_I);
				target.UpdateTransform();

				// draw debug ik target position:
				wiRenderer::RenderablePoint pp;
				pp.position = target.GetPosition();
				pp.color = XMFLOAT4(0, 1, 1, 1);
				pp.size = 0.2f;
				wiRenderer::DrawPoint(pp);

				pp.position = scene.transforms.GetComponent(ik_entity)->GetPosition();
				pp.color = XMFLOAT4(1, 0, 0, 1);
				pp.size = 0.1f;
				wiRenderer::DrawPoint(pp);
			}
		}
		break;

		case 18:
		{
			static wiTimer timer;
			float sec = (float)timer.elapsed_seconds();
			wiJobSystem::context ctx;
			wiJobSystem::Dispatch(ctx, (uint32_t)scene->transforms.GetCount(), 1024, [&](wiJobArgs args) {
				TransformComponent& transform = scene->transforms[args.jobIndex];
				XMStoreFloat4x4(
					&transform.world,
					transform.GetLocalMatrix() * XMMatrixTranslation(0, std::sin(sec + 20 * (float)args.jobIndex / (float)scene->transforms.GetCount()) * 0.1f, 0)
				);
				});
			scene->materials[0].SetEmissiveColor(XMFLOAT4(1, 1, 1, 1));
			wiJobSystem::Dispatch(ctx, (uint32_t)scene->objects.GetCount(), 1024, [&](wiJobArgs args) {
				ObjectComponent& object = scene->objects[args.jobIndex];
				float f = std::pow(std::sin(-sec * 2 + 4 * (float)args.jobIndex / (float)scene->objects.GetCount()) * 0.5f + 0.5f, 32.0f);
				object.emissiveColor = XMFLOAT4(0, 0.25f, 1, f * 3);
				});
			wiJobSystem::Wait(ctx);
		}
		break;

	}

	RenderPath3D::Update(dt);
}

void TestsRenderer::RunJobSystemTest()
{
	wiTimer timer;

	// This is created to be able to wait on the workload independently from other workload:
	wiJobSystem::context ctx;

	// This will simulate going over a big dataset first in a simple loop, then with the Job System and compare timings
	uint32_t itemCount = 1000000;
	std::stringstream ss("");
	ss << "Job System performance test:" << std::endl;
	ss << "You can find out more in Tests.cpp, RunJobSystemTest() function." << std::endl << std::endl;

	ss << "wiJobSystem was created with " << wiJobSystem::GetThreadCount() << " worker threads." << std::endl << std::endl;

	ss << "1) Execute() test:" << std::endl;

	// Serial test
	{
		timer.record();
		wiHelper::Spin(100);
		wiHelper::Spin(100);
		wiHelper::Spin(100);
		wiHelper::Spin(100);
		double time = timer.elapsed();
		ss << "Serial took " << time << " milliseconds" << std::endl;
	}
	// Execute test
	{
		timer.record();
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiHelper::Spin(100); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiHelper::Spin(100); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiHelper::Spin(100); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiHelper::Spin(100); });
		wiJobSystem::Wait(ctx);
		double time = timer.elapsed();
		ss << "wiJobSystem::Execute() took " << time << " milliseconds" << std::endl;
	}

	ss << std::endl;
	ss << "2) Dispatch() test:" << std::endl;

	// Simple loop test:
	{
		std::vector<wiScene::CameraComponent> dataSet(itemCount);
		timer.record();
		for (uint32_t i = 0; i < itemCount; ++i)
		{
			dataSet[i].UpdateCamera();
		}
		double time = timer.elapsed();
		ss << "Simple loop took " << time << " milliseconds" << std::endl;
	}

	// Dispatch test:
	{
		std::vector<wiScene::CameraComponent> dataSet(itemCount);
		timer.record();
		wiJobSystem::Dispatch(ctx, itemCount, 1000, [&](wiJobArgs args) {
			dataSet[args.jobIndex].UpdateCamera();
			});
		wiJobSystem::Wait(ctx);
		double time = timer.elapsed();
		ss << "wiJobSystem::Dispatch() took " << time << " milliseconds" << std::endl;
	}

	static wiSpriteFont font;
	font = wiSpriteFont(ss.str());
	font.params.posX = GetLogicalWidth() / 2;
	font.params.posY = GetLogicalHeight() / 2;
	font.params.h_align = WIFALIGN_CENTER;
	font.params.v_align = WIFALIGN_CENTER;
	font.params.size = 24;
	this->AddFont(&font);
}
void TestsRenderer::RunFontTest()
{
	static wiSpriteFont font;
	static wiSpriteFont font_upscaled;

	font.SetText("This is Arial, size 32 wiFont");
	font_upscaled.SetText("This is Arial, size 14 wiFont, but upscaled to 32");

	font.params.posX = GetLogicalWidth() / 2.0f;
	font.params.posY = GetLogicalHeight() / 6.0f;
	font.params.size = 32;

	font_upscaled.params = font.params;
	font_upscaled.params.posY += font.textHeight();

	font.params.style = 0; // 0 = default font
	font_upscaled.params.style = 0; // 0 = default font
	font_upscaled.params.size = 14;
	font_upscaled.params.scaling = 32.0f / 14.0f;

	AddFont(&font);
	AddFont(&font_upscaled);

	static wiSpriteFont font_aligned;
	font_aligned = font;
	font_aligned.params.posY += font.textHeight() * 2;
	font_aligned.params.size = 38;
	font_aligned.params.shadowColor = wiColor::Red();
	font_aligned.params.h_align = WIFALIGN_CENTER;
	font_aligned.SetText("Center aligned, red shadow, bigger");
	AddFont(&font_aligned);

	static wiSpriteFont font_aligned2;
	font_aligned2 = font_aligned;
	font_aligned2.params.posY += font_aligned.textHeight();
	font_aligned2.params.shadowColor = wiColor::Purple();
	font_aligned2.params.h_align = WIFALIGN_RIGHT;
	font_aligned2.SetText("Right aligned, purple shadow");
	AddFont(&font_aligned2);

	std::stringstream ss("");
	std::ifstream file("font_test.txt");
	if (file.is_open())
	{
		while (!file.eof())
		{
			std::string s;
			file >> s;
			ss << s << "\t";
		}
	}
	static wiSpriteFont font_japanese;
	font_japanese = font_aligned2;
	font_japanese.params.posY += font_aligned2.textHeight();
	font_japanese.params.style = wiFont::AddFontStyle("yumin.ttf");
	font_japanese.params.shadowColor = wiColor::Transparent();
	font_japanese.params.h_align = WIFALIGN_CENTER;
	font_japanese.params.size = 34;
	font_japanese.SetText(ss.str());
	AddFont(&font_japanese);

	static wiSpriteFont font_colored;
	font_colored.params.color = wiColor::Cyan();
	font_colored.params.h_align = WIFALIGN_CENTER;
	font_colored.params.v_align = WIFALIGN_TOP;
	font_colored.params.size = 26;
	font_colored.params.posX = GetLogicalWidth() / 2;
	font_colored.params.posY = font_japanese.params.posY + font_japanese.textHeight();
	font_colored.SetText("Colored font");
	AddFont(&font_colored);
}
void TestsRenderer::RunSpriteTest()
{
	const float step = 30;
	const float screenW = GetLogicalWidth();
	const float screenH = GetLogicalHeight();
	const XMFLOAT3 startPos = XMFLOAT3(screenW * 0.3f, screenH * 0.2f, 0);
	wiImageParams params;
	params.pos = startPos;
	params.siz = XMFLOAT2(128, 128);
	params.pivot = XMFLOAT2(0.5f, 0.5f);
	params.quality = QUALITY_LINEAR;
	params.sampleFlag = SAMPLEMODE_CLAMP;
	params.blendFlag = BLENDMODE_ALPHA;

	// Info:
	{
		static wiSpriteFont font("For more information, please see \nTests.cpp, RunSpriteTest() function.");
		font.params.posX = 10;
		font.params.posY = screenH / 2;
		AddFont(&font);
	}

	// Simple sprite, no animation:
	{
		static wiSprite sprite("../Content/logo_small.png");
		sprite.params = params;
		AddSprite(&sprite);

		static wiSpriteFont font("No animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, fade animation:
	{
		static wiSprite sprite("../Content/logo_small.png");
		sprite.params = params;
		sprite.anim = wiSprite::Anim();
		sprite.anim.fad = 1.2f; // (you can also do opacity animation with sprite.anim.opa)
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		static wiSpriteFont font("Fade animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, wobble animation:
	{
		static wiSprite sprite("../Content/logo_small.png");
		sprite.params = params;
		sprite.anim = wiSprite::Anim();
		sprite.anim.wobbleAnim.amount = XMFLOAT2(0.11f, 0.18f);
		sprite.anim.wobbleAnim.speed = 1.4f;
		AddSprite(&sprite);

		static wiSpriteFont font("Wobble animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, rotate animation:
	{
		static wiSprite sprite("../Content/logo_small.png");
		sprite.params = params;
		sprite.anim = wiSprite::Anim();
		sprite.anim.rot = 2.8f;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		static wiSpriteFont font("Rotate animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, movingtex:
	{
		static wiSprite sprite("images/movingtex.png", "../Content/logo_small.png"); // first param is the texture we will display (and also scroll here). Second param is a mask texture
		// Don't overwrite all params for this, because we want to keep the mask...
		sprite.params.pos = params.pos;
		sprite.params.siz = params.siz;
		sprite.params.pivot = params.pivot;
		sprite.params.color = XMFLOAT4(2, 2, 2, 1); // increase brightness a bit
		sprite.params.sampleFlag = SAMPLEMODE_MIRROR; // texcoords will be scrolled out of bounds, so set up a wrap mode other than clamp
		sprite.anim.movingTexAnim.speedX = 0;
		sprite.anim.movingTexAnim.speedY = 2; // scroll the texture vertically. This value is pixels/second. So because our texture here is 1x2 pixels, just scroll it once fully per second with a value of 2
		AddSprite(&sprite);

		static wiSpriteFont font("MovingTex + mask: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x = startPos.x;
		params.pos.y += sprite.params.siz.y + step * 1.5f;
	}


	// Now the spritesheets:

	// Spritesheet, no anim:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params; // nothing extra, just display the full spritesheet
		AddSprite(&sprite);

		static wiSpriteFont font("Spritesheet: \n(without animation)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, single line:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 4; // animate only a single line horizontally
		AddSprite(&sprite);

		static wiSpriteFont font("single line anim: \n(4 frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, single vertical line:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 4; // again, specify 4 total frames to loop...
		sprite.anim.drawRectAnim.horizontalFrameCount = 1; // ...but this time, limit the horizontal frame count. This way, we can get it to only animate vertically
		AddSprite(&sprite);

		static wiSpriteFont font("single line: \n(4 vertical frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, multiline:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 16; // all frames
		sprite.anim.drawRectAnim.horizontalFrameCount = 4; // all horizontal frames
		AddSprite(&sprite);

		static wiSpriteFont font("multiline: \n(all 16 frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, multiline, irregular:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 14; // NOT all frames, which makes it irregular, so the last line will not contain all horizontal frames
		sprite.anim.drawRectAnim.horizontalFrameCount = 4; // all horizontal frames
		AddSprite(&sprite);

		static wiSpriteFont font("irregular multiline: \n(14 frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x = startPos.x;
		params.pos.y += sprite.params.siz.y + step * 1.5f;
	}




	// And the nice ones:

	{
		static wiSprite sprite("images/fire_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192)); // set the draw rect (texture cutout). This will also be the first frame of the animation
		sprite.anim = wiSprite::Anim(); // reset animation state
		sprite.anim.drawRectAnim.frameCount = 20; // set the total frame count of the animation
		sprite.anim.drawRectAnim.horizontalFrameCount = 5; // this is a multiline spritesheet, so also set how many maximum frames there are in a line
		sprite.anim.drawRectAnim.frameRate = 40; // animation frames per second
		sprite.anim.repeatable = true; // looping
		AddSprite(&sprite);

		static wiSpriteFont font("For the following spritesheets, credits belong to: https://mrbubblewand.wordpress.com/download/");
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x - sprite.params.siz.x * 0.5f;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/wind_002.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 30;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 30;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/water_003.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 50;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/earth_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 20;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/special_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 40;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

}
void TestsRenderer::RunNetworkTest()
{
	static wiSpriteFont font;

	wiNetwork::Connection connection;
	connection.ipaddress = { 127,0,0,1 }; // localhost
	connection.port = 12345; // just any random port really

	std::thread sender([&] {
		// Create sender socket:
		wiNetwork::Socket sock;
		wiNetwork::CreateSocket(&sock);

		// Create a text message:
		const char message[] = "Hi, this is a text message sent over the network!\nYou can find out more in Tests.cpp, RunNetworkTest() function.";
		const size_t message_size = sizeof(message);

		// First, send the text message size:
		wiNetwork::Send(&sock, &connection, &message_size, sizeof(message_size));

		// Then send the actual text message:
		wiNetwork::Send(&sock, &connection, message, message_size);

		});

	std::thread receiver([&] {
		// Create receiver socket:
		wiNetwork::Socket sock;
		wiNetwork::CreateSocket(&sock);

		// Listen on the port which the sender uses:
		wiNetwork::ListenPort(&sock, connection.port);

		// We can check for incoming messages with CanReceive(). A timeout value can be specified in microseconds
		//	to let the function block for some time, otherwise it returns imediately
		//	It is not necessary to use this, but the wiNetwork::Receive() will block until there is a message
		if (wiNetwork::CanReceive(&sock, 1000000))
		{
			// We know that we want a text message in this simple example, but we don't know the size.
			//	We also know that the sender sends the text size before the actual text, so first we will receive the text size:
			size_t message_size;
			wiNetwork::Connection sender_connection; // this will be filled out with the sender's address
			wiNetwork::Receive(&sock, &sender_connection, &message_size, sizeof(message_size));

			if (wiNetwork::CanReceive(&sock, 1000000))
			{
				// Once we know the text message length, we can receive the message itself:
				char* message = new char[message_size]; // allocate text buffer
				wiNetwork::Receive(&sock, &sender_connection, message, message_size);

				// Write the message to the screen:
				font.SetText(message);

				// delete the message:
				delete[] message;
			}
			else
			{
				font.SetText("Failed to receive the message in time");
			}
		}
		else
		{
			font.SetText("Failed to receive the message length in time");
		}

		});

	sender.join();
	receiver.join();

	font.params.posX = GetLogicalWidth() / 2;
	font.params.posY = GetLogicalHeight() / 2;
	font.params.h_align = WIFALIGN_CENTER;
	font.params.v_align = WIFALIGN_CENTER;
	font.params.size = 24;
	AddFont(&font);
}
