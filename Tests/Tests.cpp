#include "stdafx.h"

using namespace wi::ecs;
using namespace wi::scene;

enum TEST_TYPE
{
	HELLOWORLD,
	MODEL,
	EMITTEDPARTICLE1,
	EMITTEDPARTICLE2,
	HAIRPARTICLE,
	LUASCRIPT,
	WATERTEST,
	SHADOWSTEST,
	PHYSICSTEST,
	CLOTHPHYSICSTEST,
	JOBSYSTEMTEST,
	FONTTEST,
	VOLUMETRICTEST,
	SPRITETEST,
	LIGHTMAPBAKETEST,
	NETWORKTEST,
	CONTROLLERTEST,
	INVERSEKINEMATICSTEST,
	INSTANCESTEST,
	CONTAINERPERF,
};

// Controller Test UI Data, info down below will be using Xbox Controller as reference
wi::SpriteFont 
	b_a, b_b, b_x, b_y, // Action Buttons: A, B, X, Y 
	b_du, b_dd, b_dl, b_dr, // Directional Buttons: Up, Down, Left, Right
	b_sta, b_bck, // Menu Buttons: Start, Back 
	b_lb, b_rb, // Shoulder Buttons: LB, RB
	b_ls, b_rs, // Stick Press Buttons: Left Stick, Right Stick
	t_l, t_r, // Trigger Inputs: LT, RT
	s_l, s_r, // Stick Axes: Left Stick, Right Stick
	i_led; // Controller LED status

void Tests::Initialize()
{
    wi::Application::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;
	infoDisplay.heap_allocation_counter = true;

	renderer.init(canvas);
	renderer.Load();

	ActivatePath(&renderer);
}

void TestsRenderer::ResizeLayout()
{
    RenderPath3D::ResizeLayout();

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	label.SetPos(XMFLOAT2(screenW / 2.f - label.scale.x / 2.f, screenH * 0.95f));
}
void TestsRenderer::Load()
{
	setSSREnabled(false);
	setReflectionsEnabled(true);
	setFXAAEnabled(false);

	wi::gui::GUI& gui = GetGUI();

	label.Create("Label1");
	label.SetText("Wicked Engine Test Framework");
	label.font.params.h_align = wi::font::WIFALIGN_CENTER;
	label.SetSize(XMFLOAT2(240,20));
	gui.AddWidget(&label);

	static wi::audio::Sound sound;
	static wi::audio::SoundInstance soundinstance;
	static wi::gui::Button audioTest;
	static wi::gui::Slider volume;
	static wi::gui::Slider direction;

	audioTest.Create("AudioTest");
	audioTest.SetText("Play Test Audio");
	audioTest.SetSize(XMFLOAT2(200, 20));
	audioTest.SetPos(XMFLOAT2(10, 110));
	audioTest.OnClick([&](wi::gui::EventArgs args) {
		static bool playing = false;

		if (!sound.IsValid())
		{
			wi::audio::CreateSound("../Content/models/water.wav", &sound);
			wi::audio::CreateSoundInstance(&sound, &soundinstance);
			wi::audio::SetVolume(volume.GetValue() / 100.0f, &soundinstance);
		}

		if (playing)
		{
			wi::audio::Stop(&soundinstance);
			audioTest.SetText("Play Test Audio");
		}
		else
		{
			wi::audio::Play(&soundinstance);
			audioTest.SetText("Stop Test Audio");
		}

		playing = !playing;
	});
	gui.AddWidget(&audioTest);


	volume.Create(0, 100, 50, 100, "Volume");
	volume.SetText("Volume: ");
	volume.SetSize(XMFLOAT2(120, 20));
	volume.SetPos(XMFLOAT2(65, 140));
	volume.OnSlide([](wi::gui::EventArgs args) {
		wi::audio::SetVolume(args.fValue / 100.0f, &soundinstance);
	});
	gui.AddWidget(&volume);

	direction.Create(-1, 1, 0, 10000, "Direction");
	direction.SetText("Direction: ");
	direction.SetSize(XMFLOAT2(120, 20));
	direction.SetPos(XMFLOAT2(65, 170));
	direction.OnSlide([](wi::gui::EventArgs args) {
		wi::audio::SoundInstance3D instance3D;
		instance3D.emitterPos = XMFLOAT3(args.fValue, 0, 0);
		instance3D.listenerPos = XMFLOAT3(0, 0, -0.1f);
		wi::audio::Update3D(&soundinstance, instance3D);
		});
	gui.AddWidget(&direction);

	testSelector.Create("TestSelector");
	testSelector.SetText("Demo: ");
	testSelector.SetSize(XMFLOAT2(140, 20));
	testSelector.SetPos(XMFLOAT2(50, 220));
	testSelector.AddItem("HelloWorld", HELLOWORLD);
	testSelector.AddItem("Model", MODEL);
	testSelector.AddItem("EmittedParticle 1", EMITTEDPARTICLE1);
	testSelector.AddItem("EmittedParticle 2", EMITTEDPARTICLE2);
	testSelector.AddItem("HairParticle", HAIRPARTICLE);
	testSelector.AddItem("Lua Script", LUASCRIPT);
	testSelector.AddItem("Water Test", WATERTEST);
	testSelector.AddItem("Shadows Test", SHADOWSTEST);
	testSelector.AddItem("Physics Test", PHYSICSTEST);
	testSelector.AddItem("Cloth Physics Test", CLOTHPHYSICSTEST);
	testSelector.AddItem("Job System Test", JOBSYSTEMTEST);
	testSelector.AddItem("Font Test", FONTTEST);
	testSelector.AddItem("Volumetric Test", VOLUMETRICTEST);
	testSelector.AddItem("Sprite Test", SPRITETEST);
	testSelector.AddItem("Lightmap Bake Test", LIGHTMAPBAKETEST);
	testSelector.AddItem("Network Test", NETWORKTEST);
	testSelector.AddItem("Controller Test", CONTROLLERTEST);
	testSelector.AddItem("Inverse Kinematics", INVERSEKINEMATICSTEST);
	testSelector.AddItem("65k Instances", INSTANCESTEST);
	testSelector.AddItem("Container perf", CONTAINERPERF);
	testSelector.SetMaxVisibleItemCount(10);
	testSelector.OnSelect([=](wi::gui::EventArgs args) {

		// Reset all state that tests might have modified:
		wi::eventhandler::SetVSync(true);
		wi::renderer::SetToDrawGridHelper(false);
		wi::renderer::SetTemporalAAEnabled(false);
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

		float screenW = GetLogicalWidth();
		float screenH = GetLogicalHeight();

		// Based on combobox selection, start the appropriate test:
		switch (args.userdata)
		{
		case HELLOWORLD:
		{
			// This will spawn a sprite with two textures. The first texture is a color texture and it will be animated.
			//	The second texture is a static image of "hello world" written on it
			//	Then add some animations to the sprite to get a nice wobbly and color changing effect.
			//	You can learn more in the Sprite test in RunSpriteTest() function
			static wi::Sprite sprite;
			sprite = wi::Sprite("images/movingtex.png", "images/HelloWorld.png");
			sprite.params.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite.params.siz = XMFLOAT2(200, 100);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.rot = XM_PI / 4.0f;
			sprite.anim.wobbleAnim.amount = XMFLOAT2(0.16f, 0.16f);
			sprite.anim.movingTexAnim.speedX = 0;
			sprite.anim.movingTexAnim.speedY = 3;
			AddSprite(&sprite);
			break;
		}
		case MODEL:
			wi::renderer::SetTemporalAAEnabled(true);
			wi::scene::LoadModel("../Content/models/teapot.wiscene");
			break;
		case EMITTEDPARTICLE1:
			wi::scene::LoadModel("../Content/models/emitter_smoke.wiscene");
			break;
		case EMITTEDPARTICLE2:
			wi::scene::LoadModel("../Content/models/emitter_skinned.wiscene");
			break;
		case HAIRPARTICLE:
			wi::scene::LoadModel("../Content/models/hairparticle_torus.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case LUASCRIPT:
			wi::renderer::SetToDrawGridHelper(true);
			wi::lua::RunFile("test_script.lua");
			break;
		case WATERTEST:
			wi::renderer::SetTemporalAAEnabled(true);
			wi::scene::LoadModel("../Content/models/water_test.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case SHADOWSTEST:
			wi::renderer::SetTemporalAAEnabled(true);
			wi::scene::LoadModel("../Content/models/shadows_test.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case PHYSICSTEST:
			wi::renderer::SetTemporalAAEnabled(true);
			wi::scene::LoadModel("../Content/models/physics_test.wiscene");
			break;
		case CLOTHPHYSICSTEST:
			wi::scene::LoadModel("../Content/models/cloth_test.wiscene", XMMatrixTranslation(0, 3, 4));
			break;
		case JOBSYSTEMTEST:
			RunJobSystemTest();
			break;
		case FONTTEST:
			RunFontTest();
			break;
		case VOLUMETRICTEST:
			wi::renderer::SetTemporalAAEnabled(true);
			wi::scene::LoadModel("../Content/models/volumetric_test.wiscene", XMMatrixTranslation(0, 0, 4));
			break;
		case SPRITETEST:
			RunSpriteTest();
			break;
		case LIGHTMAPBAKETEST:
			wi::eventhandler::SetVSync(false); // turn off vsync if we can to accelerate the baking
			wi::renderer::SetTemporalAAEnabled(true);
			wi::scene::LoadModel("../Content/models/lightmap_bake_test.wiscene", XMMatrixTranslation(0, 0, 4));
			break;
		case NETWORKTEST:
			RunNetworkTest();
			break;
		case CONTROLLERTEST:
		{
			float pos_center_x = screenW / 2, pos_center_y = screenH / 2;
			wi::font::Params baseparams(pos_center_x, pos_center_y, 20, wi::font::WIFALIGN_CENTER, wi::font::WIFALIGN_CENTER);
			baseparams.style = wi::font::AddFontStyle("yumin.ttf");

			std::string head_textbuild = "This test will check for Controller-1's inputs and it will be displayed down below.";
			head_textbuild += "\nPress Button 2 combined with one of the Trigger Buttons to set the controller's vibration.";
			head_textbuild += "\nStrength of the vibration are controlled by how hard the Trigger Buttons get pressed.";
			head_textbuild += "\nPress Button 3 to switch the controller's LED color, every press will bring different colors.";
			static wi::SpriteFont head(head_textbuild, baseparams);
			head.params.posY = pos_center_y-260.f;
			b_a = wi::SpriteFont("Button 2 (Xbox: A, PS: X)", baseparams);
			b_a.params.posY = pos_center_y-160.f;
			b_a.params.posX = pos_center_x-140.f;
			b_b = wi::SpriteFont("Button 3 (Xbox: B, PS: O)", baseparams);
			b_b.params.posY = b_a.params.posY+30.f;
			b_b.params.posX = b_a.params.posX;
			b_x = wi::SpriteFont("Button 1 (Xbox: X, PS: [])", baseparams);
			b_x.params.posY = b_b.params.posY+30.f;
			b_x.params.posX = b_a.params.posX;
			b_y = wi::SpriteFont("Button 4 (Xbox: Y, PS: △)", baseparams);
			b_y.params.posY = b_x.params.posY+30.f;
			b_y.params.posX = b_a.params.posX;
			b_bck = wi::SpriteFont("Button 9 (Xbox: Back, PS: Share)", baseparams);
			b_bck.params.posY = b_y.params.posY+30.f;
			b_bck.params.posX = b_a.params.posX;
			b_sta = wi::SpriteFont("Button 10 (Xbox: Start, PS: Option)", baseparams);
			b_sta.params.posY = b_bck.params.posY+30.f;
			b_sta.params.posX = b_a.params.posX;
			b_ls = wi::SpriteFont("Button 7 (Left Stick)", baseparams);
			b_ls.params.posY = b_sta.params.posY+30.f;
			b_ls.params.posX = b_a.params.posX;
			b_rs = wi::SpriteFont("Button 8 (Right Stick)", baseparams);
			b_rs.params.posY = b_ls.params.posY+30.f;
			b_rs.params.posX = b_a.params.posX;
			s_l = wi::SpriteFont("", baseparams);
			s_l.params.posY = b_rs.params.posY+30.f;
			s_l.params.posX = b_a.params.posX;
			s_r = wi::SpriteFont("", baseparams);
			s_r.params.posY = s_l.params.posY+30.f;
			s_r.params.posX = b_a.params.posX;

			b_du = wi::SpriteFont("Button Up", baseparams);
			b_du.params.posY = pos_center_y-160.f;
			b_du.params.posX = pos_center_x+140.f;
			b_dd = wi::SpriteFont("Button Down", baseparams);
			b_dd.params.posY = b_du.params.posY+30.f;
			b_dd.params.posX = b_du.params.posX;
			b_dl = wi::SpriteFont("Button Left", baseparams);
			b_dl.params.posY = b_dd.params.posY+30.f;
			b_dl.params.posX = b_du.params.posX;
			b_dr = wi::SpriteFont("Button Right", baseparams);
			b_dr.params.posY = b_dl.params.posY+30.f;
			b_dr.params.posX = b_du.params.posX;
			b_lb = wi::SpriteFont("Button 5 (Xbox: LB, PS: L1)", baseparams);
			b_lb.params.posY = b_dr.params.posY+30.f;
			b_lb.params.posX = b_du.params.posX;
			b_rb = wi::SpriteFont("Button 6 (Xbox: RB, PS: R1)", baseparams);
			b_rb.params.posY = b_lb.params.posY+30.f;
			b_rb.params.posX = b_du.params.posX;
			t_l = wi::SpriteFont("", baseparams);
			t_l.params.posY = b_rb.params.posY+30.f;
			t_l.params.posX = b_du.params.posX;
			t_r = wi::SpriteFont("", baseparams);
			t_r.params.posY = t_l.params.posY+30.f;
			t_r.params.posX = b_du.params.posX;

			i_led = wi::SpriteFont("", baseparams);
			i_led.params.posY = s_r.params.posY+30.f;

			AddFont(&head);

			AddFont(&b_a);
			AddFont(&b_b);
			AddFont(&b_x);
			AddFont(&b_y);
			AddFont(&b_sta);
			AddFont(&b_bck);
			AddFont(&b_ls);
			AddFont(&b_rs);
			AddFont(&s_l);
			AddFont(&s_r);
			AddFont(&b_du);
			AddFont(&b_dd);
			AddFont(&b_dl);
			AddFont(&b_dr);
			AddFont(&b_lb);
			AddFont(&b_rb);
			AddFont(&t_l);
			AddFont(&t_r);

			AddFont(&i_led);
		}
		break;
		case INVERSEKINEMATICSTEST:
		{
			Scene scene;
			LoadModel(scene, "../Content/scripts/character_controller/assets/character.wiscene", XMMatrixScaling(2, 2, 2));
			if (scene.humanoids.GetCount() == 0)
				break;

			HumanoidComponent& humanoid = scene.humanoids[0];
			humanoid.SetLookAtEnabled(false);
			ik_entity = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftHand];
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

			wi::scene::GetScene().Merge(scene); // add lodaded scene to global scene
		}
		break;

		case INSTANCESTEST:
		{
			wi::scene::LoadModel("../Content/models/cube.wiscene");
			wi::profiler::SetEnabled(true);
			Scene& scene = wi::scene::GetScene();
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

		case CONTAINERPERF:
			ContainerTest();
			break;

		default:
			assert(0);
			break;
		}

	});
	testSelector.SetSelected(0);
	gui.AddWidget(&testSelector);

	// Set a theme globally:
	{
		wi::Color theme_color_idle = wi::Color(100, 130, 150, 150);
		wi::Color theme_color_focus = wi::Color(100, 180, 200, 200);
		wi::Color dark_point = wi::Color(0, 0, 20, 200); // darker elements will lerp towards this
		wi::gui::Theme theme;
		theme.image.background = true;
		theme.image.blendFlag = wi::enums::BLENDMODE_OPAQUE;
		theme.font.color = wi::Color(160, 240, 250, 255);
		theme.shadow_color = wi::Color(100, 180, 200, 100);

		theme.tooltipImage = theme.image;
		theme.tooltipImage.color = theme_color_idle;
		theme.tooltipFont = theme.font;
		theme.tooltip_shadow_color = theme.shadow_color;

		wi::Color theme_color_active = wi::Color::White();
		wi::Color theme_color_deactivating = wi::Color::lerp(theme_color_focus, wi::Color::White(), 0.5f);

		gui.SetTheme(theme); // set basic params to all states

		// customize colors for specific states:
		gui.SetColor(theme_color_idle, wi::gui::IDLE);
		gui.SetColor(theme_color_focus, wi::gui::FOCUS);
		gui.SetColor(theme_color_active, wi::gui::ACTIVE);
		gui.SetColor(theme_color_deactivating, wi::gui::DEACTIVATING);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.7f), wi::gui::WIDGET_ID_WINDOW_BASE);

		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.75f), wi::gui::WIDGET_ID_SLIDER_BASE_IDLE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_FOCUS);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.85f), wi::gui::WIDGET_ID_SLIDER_BASE_ACTIVE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SLIDER_KNOB_FOCUS);
		gui.SetColor(theme_color_active, wi::gui::WIDGET_ID_SLIDER_KNOB_ACTIVE);
		gui.SetColor(theme_color_deactivating, wi::gui::WIDGET_ID_SLIDER_KNOB_DEACTIVATING);

		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.75f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_IDLE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_FOCUS);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.85f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_ACTIVE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_HOVER);
		gui.SetColor(theme_color_active, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);

		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_COMBO_DROPDOWN);
	}

    RenderPath3D::Load();
}
void TestsRenderer::Update(float dt)
{
	int selected = testSelector.GetSelected();
	uint64_t userdata = testSelector.GetItemUserData(selected);

	switch (userdata)
	{
    case MODEL:
    {
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
            transform_base->Rotate(XMVectorSet(0,rotation,0,1));
            transform_top->Rotate(XMVectorSet(0,rotation,0,1));
        }
        else if (wi::input::Down(wi::input::KEYBOARD_BUTTON_RIGHT))
        {
            transform_base->Rotate(XMVectorSet(0,-rotation,0,1));
            transform_top->Rotate(XMVectorSet(0,-rotation,0,1));
        }
    }
    break;
	case CONTROLLERTEST:
	{
		// Handle displays of all buttons here, the text will turn to white when the associated button are pressed
		b_a.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_2)) ? wi::Color::White() : wi::Color::Green();
		b_b.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_3)) ? wi::Color::White() : wi::Color::Red();
		b_x.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_1)) ? wi::Color::White() : wi::Color::Cyan();
		b_y.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_4)) ? wi::Color::White() : wi::Color::Yellow();
		b_sta.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_10)) ? wi::Color::White() : wi::Color::Gray();
		b_bck.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_9)) ? wi::Color::White() : wi::Color::Gray();
		b_ls.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_7)) ? wi::Color::White() : wi::Color::Gray();
		b_rs.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_8)) ? wi::Color::White() : wi::Color::Gray();
		b_du.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_UP)) ? wi::Color::White() : wi::Color::Gray();
		b_dd.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_DOWN)) ? wi::Color::White() : wi::Color::Gray();
		b_dl.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_LEFT)) ? wi::Color::White() : wi::Color::Gray();
		b_dr.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_RIGHT)) ? wi::Color::White() : wi::Color::Gray();
		b_lb.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_5)) ? wi::Color::White() : wi::Color::Gray();
		b_rb.params.color = (wi::input::Down(wi::input::GAMEPAD_BUTTON_6)) ? wi::Color::White() : wi::Color::Gray();

		// Handle displays of triggers and axes, will update the text according to the provided data of the controller
		static std::wstring text_head_axes[] = {L"L Trigger (PS: L2): ", L"R Trigger (PS: R2): ", L"L Stick: ", L"R Stick: "};
		t_l.text = text_head_axes[0]+std::to_wstring(wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_L).x);
		t_r.text = text_head_axes[1]+std::to_wstring(wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_R).x);
		s_l.text = text_head_axes[2]+
			std::to_wstring(wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_L).x)+L", "+
			std::to_wstring(wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_L).y);
		s_r.text = text_head_axes[3]+
			std::to_wstring(wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_R).x)+L", "+
			std::to_wstring(wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_R).y);

		// Handling controller's vibrations and LED color
		static float vibration_multipllier = 0.f;
		static int color_index_set = 0;
		static uint32_t color_sets[] = { wi::Color::Black(), wi::Color::Cyan(), wi::Color::Red(), wi::Color::Green(), wi::Color::Yellow()};
		static std::wstring color_sets_desc[] = {L"Controller LED: None", L"Controller LED: Cyan", L"Controller LED: Red", L"Controller LED: Green", L"Controller LED: Yellow"};

		if(wi::input::Down(wi::input::GAMEPAD_BUTTON_2)) vibration_multipllier = 1.f;
		else vibration_multipllier = 0.f;
		if(wi::input::Press(wi::input::GAMEPAD_BUTTON_3)){
			color_index_set++;
			if (color_index_set > 4) color_index_set = 0;
		}

		wi::input::ControllerFeedback feedback;
		feedback.led_color.rgba = color_sets[color_index_set];
		feedback.vibration_left = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_L).x*vibration_multipllier;
		feedback.vibration_right = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_R).x*vibration_multipllier;
		wi::input::SetControllerFeedback(feedback, 0);

		i_led.text = color_sets_desc[color_index_set];
		i_led.params.color = color_sets[color_index_set];
	}
	break;
	case INVERSEKINEMATICSTEST:
	{
		if (ik_entity != INVALID_ENTITY)
		{
			// Inverse kinematics test:
			Scene& scene = wi::scene::GetScene();
			InverseKinematicsComponent& ik = *scene.inverse_kinematics.GetComponent(ik_entity);
			TransformComponent& target = *scene.transforms.GetComponent(ik.target);

			// place ik target on a plane intersected by mouse ray:
			wi::primitive::Ray ray = wi::renderer::GetPickRay((long)wi::input::GetPointer().x, (long)wi::input::GetPointer().y, *this);
			XMVECTOR plane = XMVectorSet(0, 0, 1, 0.2f);
			XMVECTOR I = XMPlaneIntersectLine(plane, XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction) * 10000);
			target.ClearTransform();
			XMFLOAT3 _I;
			XMStoreFloat3(&_I, I);
			target.Translate(_I);
			target.UpdateTransform();

			// draw debug ik target position:
			wi::renderer::RenderablePoint pp;
			pp.position = target.GetPosition();
			pp.color = XMFLOAT4(0, 1, 1, 1);
			pp.size = 0.2f;
			wi::renderer::DrawPoint(pp);

			pp.position = scene.transforms.GetComponent(ik_entity)->GetPosition();
			pp.color = XMFLOAT4(1, 0, 0, 1);
			pp.size = 0.1f;
			wi::renderer::DrawPoint(pp);
		}
	}
	break;

	case INSTANCESTEST:
	{
		static wi::Timer timer;
		float sec = (float)timer.elapsed_seconds();
		wi::jobsystem::context ctx;
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene->transforms.GetCount(), 1024, [&](wi::jobsystem::JobArgs args) {
			TransformComponent& transform = scene->transforms[args.jobIndex];
			XMStoreFloat4x4(
				&transform.world,
				transform.GetLocalMatrix() * XMMatrixTranslation(0, std::sin(sec + 20 * (float)args.jobIndex / (float)scene->transforms.GetCount()) * 0.1f, 0)
			);
		});
		scene->materials[0].SetEmissiveColor(XMFLOAT4(1, 1, 1, 1));
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene->objects.GetCount(), 1024, [&](wi::jobsystem::JobArgs args) {
			ObjectComponent& object = scene->objects[args.jobIndex];
			float f = std::pow(std::sin(-sec * 2 + 4 * (float)args.jobIndex / (float)scene->objects.GetCount()) * 0.5f + 0.5f, 32.0f);
			object.emissiveColor = XMFLOAT4(0, 0.25f, 1, f * 3);
		});
		wi::jobsystem::Wait(ctx);
	}
	break;

	}

    RenderPath3D::Update(dt);
}

void TestsRenderer::RunJobSystemTest()
{
	wi::Timer timer;

	// This is created to be able to wait on the workload independently from other workload:
	wi::jobsystem::context ctx;

	// This will simulate going over a big dataset first in a simple loop, then with the Job System and compare timings
	uint32_t itemCount = 1000000;
	std::string ss;
	ss += "Job System performance test:\n";
	ss += "You can find out more in Tests.cpp, RunJobSystemTest() function.\n\n";

	ss += "wi::jobsystem was created with " + std::to_string(wi::jobsystem::GetThreadCount()) + " worker threads.\n\n";

	ss += "1) Execute() test:\n";

	// Serial test
	{
		timer.record();
		wi::helper::Spin(100);
		wi::helper::Spin(100);
		wi::helper::Spin(100);
		wi::helper::Spin(100);
		double time = timer.elapsed();
		ss += "Serial took " + std::to_string(time) + " milliseconds\n";
	}
	// Execute test
	{
		timer.record();
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args){ wi::helper::Spin(100); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args){ wi::helper::Spin(100); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args){ wi::helper::Spin(100); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args){ wi::helper::Spin(100); });
		wi::jobsystem::Wait(ctx);
		double time = timer.elapsed();
		ss += "wi::jobsystem::Execute() took " + std::to_string(time) + " milliseconds\n";
	}

	ss += "\n2) Dispatch() test:\n";

	// Simple loop test:
	{
		wi::vector<wi::scene::CameraComponent> dataSet(itemCount);
		timer.record();
		for (uint32_t i = 0; i < itemCount; ++i)
		{
			dataSet[i].UpdateCamera();
		}
		double time = timer.elapsed();
		ss += "Simple loop took " + std::to_string(time) + " milliseconds\n";
	}

	// Dispatch test:
	{
		wi::vector<wi::scene::CameraComponent> dataSet(itemCount);
		timer.record();
		wi::jobsystem::Dispatch(ctx, itemCount, 1000, [&](wi::jobsystem::JobArgs args) {
			dataSet[args.jobIndex].UpdateCamera();
		});
		wi::jobsystem::Wait(ctx);
		double time = timer.elapsed();
		ss += "wi::jobsystem::Dispatch() took " + std::to_string(time) + " milliseconds\n";
	}

	static wi::SpriteFont font;
	font = wi::SpriteFont(ss);
	font.params.posX = GetLogicalWidth() / 2;
	font.params.posY = GetLogicalHeight() / 2;
	font.params.h_align = wi::font::WIFALIGN_CENTER;
	font.params.v_align = wi::font::WIFALIGN_CENTER;
	font.params.size = 24;
	this->AddFont(&font);
}
void TestsRenderer::RunFontTest()
{
	static wi::SpriteFont font;
	static wi::SpriteFont font_upscaled;

	font.SetText("This is Liberation Sans, size 32 wi::font");
	font_upscaled.SetText("This is Liberation Sans, size 14 wi::font, but upscaled to 32");

	font.params.posX = GetLogicalWidth() / 2.0f;
	font.params.posY = GetLogicalHeight() / 6.0f;
	font.params.size = 32;

	font_upscaled.params = font.params;
	font_upscaled.params.posY += font.TextHeight();

	font.params.style = 0; // 0 = default font
	font_upscaled.params.style = 0; // 0 = default font
	font_upscaled.params.size = 14;
	font_upscaled.params.scaling = 32.0f / 14.0f;

	AddFont(&font);
	AddFont(&font_upscaled);

	static wi::SpriteFont font_aligned;
	font_aligned = font;
	font_aligned.params.posY += font.TextHeight() * 2;
	font_aligned.params.size = 38;
	font_aligned.params.shadowColor = wi::Color::Red();
	font_aligned.params.h_align = wi::font::WIFALIGN_CENTER;
	font_aligned.params.shadow_softness = 0.1f;
	font_aligned.params.shadow_bolden = 0.5f;
	font_aligned.SetText("Center aligned, red shadow, bigger");
	AddFont(&font_aligned);

	static wi::SpriteFont font_aligned2;
	font_aligned2 = font_aligned;
	font_aligned2.params.scaling = 0.6f;
	font_aligned2.params.posY += font_aligned.TextHeight();
	font_aligned2.params.shadowColor = wi::Color::Purple();
	font_aligned2.params.h_align = wi::font::WIFALIGN_RIGHT;
	font_aligned2.params.shadow_softness = 1;
	font_aligned2.params.shadow_offset_x = 2;
	font_aligned2.params.shadow_offset_y = 2;
	font_aligned2.SetText("Right aligned, downscaled, purple shadow with offset");
	AddFont(&font_aligned2);

	static wi::SpriteFont font_japanese;
	font_japanese = font_aligned2;
	font_japanese.params.scaling = 1;
	font_japanese.params.posY += font_aligned2.TextHeight();
	font_japanese.params.style = wi::font::AddFontStyle("yumin.ttf");
	font_japanese.params.shadowColor = wi::Color::Transparent();
	font_japanese.params.h_align = wi::font::WIFALIGN_CENTER;
	font_japanese.params.size = 34;
	font_japanese.SetText("ウィケッドエンジンです。よろしくお願いします。 Привет! (ʘ‿ʘ)");
	AddFont(&font_japanese);

	static wi::SpriteFont font_colored;
	font_colored.params.color = wi::Color::Cyan();
	font_colored.params.h_align = wi::font::WIFALIGN_CENTER;
	font_colored.params.v_align = wi::font::WIFALIGN_TOP;
	font_colored.params.size = 26;
	font_colored.params.posX = GetLogicalWidth() / 2;
	font_colored.params.posY = font_japanese.params.posY + font_japanese.TextHeight();
	font_colored.SetText("Colored font");
	AddFont(&font_colored);

	static wi::SpriteFont font_nosdf;
	font_nosdf.params.disableSDFRendering();
	font_nosdf.params.color = wi::Color::White();
	font_nosdf.params.h_align = wi::font::WIFALIGN_CENTER;
	font_nosdf.params.v_align = wi::font::WIFALIGN_TOP;
	font_nosdf.params.size = 24;
	font_nosdf.params.posX = GetLogicalWidth() / 2;
	font_nosdf.params.posY = font_colored.params.posY + font_colored.TextHeight();
	font_nosdf.SetText("SDF rendering disabled for this text.");
	AddFont(&font_nosdf);
}
void TestsRenderer::RunSpriteTest()
{
	const float step = 30;
	const float screenW = GetLogicalWidth();
	const float screenH = GetLogicalHeight();
	const XMFLOAT3 startPos = XMFLOAT3(screenW * 0.3f, screenH * 0.2f, 0);
	wi::image::Params params;
	params.pos = startPos;
	params.siz = XMFLOAT2(128, 128);
	params.pivot = XMFLOAT2(0.5f, 0.5f);
	params.quality = wi::image::QUALITY_LINEAR;
	params.sampleFlag = wi::image::SAMPLEMODE_CLAMP;
	params.blendFlag = wi::enums::BLENDMODE_ALPHA;

	// Info:
	{
		static wi::SpriteFont font("For more information, please see \nTests.cpp, RunSpriteTest() function.");
		font.params.posX = 10;
		font.params.posY = screenH / 2;
		AddFont(&font);
	}

	// Simple sprite, no animation:
	{
		static wi::Sprite sprite("../Content/logo_small.png");
		sprite.params = params;
		AddSprite(&sprite);

		static wi::SpriteFont font("No animation: ");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, fade animation:
	{
		static wi::Sprite sprite("../Content/logo_small.png");
		sprite.params = params;
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.fad = 1.2f; // (you can also do opacity animation with sprite.anim.opa)
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		static wi::SpriteFont font("Fade animation: ");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, wobble animation:
	{
		static wi::Sprite sprite("../Content/logo_small.png");
		sprite.params = params;
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.wobbleAnim.amount = XMFLOAT2(0.11f, 0.18f);
		sprite.anim.wobbleAnim.speed = 1.4f;
		AddSprite(&sprite);

		static wi::SpriteFont font("Wobble animation: ");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, rotate animation:
	{
		static wi::Sprite sprite("../Content/logo_small.png");
		sprite.params = params;
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.rot = 2.8f;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		static wi::SpriteFont font("Rotate animation: ");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, movingtex:
	{
		static wi::Sprite sprite("images/movingtex.png", "../Content/logo_small.png"); // first param is the texture we will display (and also scroll here). Second param is a mask texture
		// Don't overwrite all params for this, because we want to keep the mask...
		sprite.params.pos = params.pos;
		sprite.params.siz = params.siz;
		sprite.params.pivot = params.pivot;
		sprite.params.color = XMFLOAT4(2, 2, 2, 1); // increase brightness a bit
		sprite.params.sampleFlag = wi::image::SAMPLEMODE_MIRROR; // texcoords will be scrolled out of bounds, so set up a wrap mode other than clamp
		sprite.anim.movingTexAnim.speedX = 0;
		sprite.anim.movingTexAnim.speedY = 2; // scroll the texture vertically. This value is pixels/second. So because our texture here is 1x2 pixels, just scroll it once fully per second with a value of 2
		AddSprite(&sprite);

		static wi::SpriteFont font("MovingTex + mask: ");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x = startPos.x;
		params.pos.y += sprite.params.siz.y + step * 1.5f;
	}


	// Now the spritesheets:

	// Spritesheet, no anim:
	{
		static wi::Sprite sprite("images/spritesheet_grid.png");
		sprite.params = params; // nothing extra, just display the full spritesheet
		AddSprite(&sprite);

		static wi::SpriteFont font("Spritesheet: \n(without animation)");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, single line:
	{
		static wi::Sprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 4; // animate only a single line horizontally
		AddSprite(&sprite);

		static wi::SpriteFont font("single line anim: \n(4 frames)");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, single vertical line:
	{
		static wi::Sprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 4; // again, specify 4 total frames to loop...
		sprite.anim.drawRectAnim.horizontalFrameCount = 1; // ...but this time, limit the horizontal frame count. This way, we can get it to only animate vertically
		AddSprite(&sprite);

		static wi::SpriteFont font("single line: \n(4 vertical frames)");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, multiline:
	{
		static wi::Sprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 16; // all frames
		sprite.anim.drawRectAnim.horizontalFrameCount = 4; // all horizontal frames
		AddSprite(&sprite);

		static wi::SpriteFont font("multiline: \n(all 16 frames)");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, multiline, irregular:
	{
		static wi::Sprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 14; // NOT all frames, which makes it irregular, so the last line will not contain all horizontal frames
		sprite.anim.drawRectAnim.horizontalFrameCount = 4; // all horizontal frames
		AddSprite(&sprite);

		static wi::SpriteFont font("irregular multiline: \n(14 frames)");
		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x = startPos.x;
		params.pos.y += sprite.params.siz.y + step * 1.5f;
	}




	// And the nice ones:

	{
		static wi::Sprite sprite("images/fire_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192)); // set the draw rect (texture cutout). This will also be the first frame of the animation
		sprite.anim = wi::Sprite::Anim(); // reset animation state
		sprite.anim.drawRectAnim.frameCount = 20; // set the total frame count of the animation
		sprite.anim.drawRectAnim.horizontalFrameCount = 5; // this is a multiline spritesheet, so also set how many maximum frames there are in a line
		sprite.anim.drawRectAnim.frameRate = 40; // animation frames per second
		sprite.anim.repeatable = true; // looping
		AddSprite(&sprite);

		static wi::SpriteFont font("For the following spritesheets, credits belong to: https://mrbubblewand.wordpress.com/download/");
		font.params.v_align = wi::font::WIFALIGN_BOTTOM;
		font.params.posX = sprite.params.pos.x - sprite.params.siz.x * 0.5f;
		font.params.posY = sprite.params.pos.y - sprite.params.siz.y * 0.5f;
		AddFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wi::Sprite sprite("images/wind_002.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 30;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 30;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wi::Sprite sprite("images/water_003.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 50;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wi::Sprite sprite("images/earth_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wi::Sprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 20;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		AddSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wi::Sprite sprite("images/special_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wi::Sprite::Anim();
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
	static wi::SpriteFont font;

	wi::network::Connection connection;
	connection.ipaddress = { 127,0,0,1 }; // localhost
	connection.port = 12345; // just any random port really

	std::thread sender([&] {
		// Create sender socket:
		wi::network::Socket sock;
		wi::network::CreateSocket(&sock);

		// Create a text message:
		const char message[] = "Hi, this is a text message sent over the network!\nYou can find out more in Tests.cpp, RunNetworkTest() function.";
		const size_t message_size = sizeof(message);

		// First, send the text message size:
		wi::network::Send(&sock, &connection, &message_size, sizeof(message_size));

		// Then send the actual text message:
		wi::network::Send(&sock, &connection, message, message_size);

		});

	std::thread receiver([&] {
		// Create receiver socket:
		wi::network::Socket sock;
		wi::network::CreateSocket(&sock);

		// Listen on the port which the sender uses:
		wi::network::ListenPort(&sock, connection.port);
		
		// We can check for incoming messages with CanReceive(). A timeout value can be specified in microseconds
		//	to let the function block for some time, otherwise it returns imediately
		//	It is not necessary to use this, but the wi::network::Receive() will block until there is a message
		if (wi::network::CanReceive(&sock, 1000000))
		{
			// We know that we want a text message in this simple example, but we don't know the size.
			//	We also know that the sender sends the text size before the actual text, so first we will receive the text size:
			size_t message_size;
			wi::network::Connection sender_connection; // this will be filled out with the sender's address
			wi::network::Receive(&sock, &sender_connection, &message_size, sizeof(message_size));

			if (wi::network::CanReceive(&sock, 1000000))
			{
				// Once we know the text message length, we can receive the message itself:
				char* message = new char[message_size]; // allocate text buffer
				wi::network::Receive(&sock, &sender_connection, message, message_size);

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
	font.params.h_align = wi::font::WIFALIGN_CENTER;
	font.params.v_align = wi::font::WIFALIGN_CENTER;
	font.params.size = 24;
	AddFont(&font);
}
void TestsRenderer::ContainerTest()
{
	wi::Timer timer;

	const size_t elements = 1000000;
#define shuffle(i) (i * 345734667877) % 98787546343

	std::string ss = "Container test for " + std::to_string(elements) + " elements:\n";

#if WI_UNORDERED_MAP_TYPE
	std::unordered_map<size_t, size_t> std_map;
	{
		timer.record();
		for (size_t i = 0; i < elements; ++i)
		{
			std_map[shuffle(i)] = i;
		}
		ss += "\nstd::unordered_map insertion: " + std::to_string(timer.elapsed_milliseconds()) + " ms\n";

		timer.record();
		for (size_t i = 0; i < std_map.size(); ++i)
		{
			std_map[shuffle(i)] = 0;
		}
		ss += "std::unordered_map access: " + std::to_string(timer.elapsed_milliseconds()) + " ms";
	}

	wi::unordered_map<size_t, size_t> wi_map;
	{
		timer.record();
		for (size_t i = 0; i < elements; ++i)
		{
			wi_map[shuffle(i)] = i;
		}
		ss += "\nwi::unordered_map insertion: " + std::to_string(timer.elapsed_milliseconds()) + " ms\n";

		timer.record();
		for (size_t i = 0; i < wi_map.size(); ++i)
		{
			wi_map[shuffle(i)] = 0;
		}
		ss += "wi::unordered_map access: " + std::to_string(timer.elapsed_milliseconds()) + " ms\n";
	}
#else
	ss += "wi::unordered_map implementation uses std::unordered_map. There is nothing to test.";
#endif // WI_UNORDERED_MAP_TYPE

	ss += "\n";

#if WI_VECTOR_TYPE
	std::vector<CameraComponent> std_vector;
	{
		timer.record();
		for (size_t i = 0; i < elements; ++i)
		{
			std_vector.emplace_back();
		}
		ss += "\nstd::vector append: " + std::to_string(timer.elapsed_milliseconds()) + " ms\n";

		timer.record();
		for (auto& x : std_vector)
		{
			x.aperture_size = 8;
		}
		ss += "std::vector access: " + std::to_string(timer.elapsed_milliseconds()) + " ms";
	}

	wi::vector<CameraComponent> wi_vector;
	{
		timer.record();
		for (size_t i = 0; i < elements; ++i)
		{
			wi_vector.emplace_back();
		}
		ss += "\nwi::vector append: " + std::to_string(timer.elapsed_milliseconds()) + " ms\n";

		timer.record();
		for (auto& x : wi_vector)
		{
			x.aperture_size = 8;
		}
		ss += "wi::vector access: " + std::to_string(timer.elapsed_milliseconds()) + " ms\n";
	}
#else
	ss += "wi::vector implementation uses std::vector. There is nothing to test.";
#endif // WI_VECTOR_TYPE

	static wi::SpriteFont font;
	font = wi::SpriteFont(ss);
	font.params.posX = GetLogicalWidth() / 2;
	font.params.posY = GetLogicalHeight() / 2;
	font.params.h_align = wi::font::WIFALIGN_CENTER;
	font.params.v_align = wi::font::WIFALIGN_CENTER;
	font.params.size = 24;
	this->AddFont(&font);
}
