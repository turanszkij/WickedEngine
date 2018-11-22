#include "stdafx.h"
#include "Tests.h"

#include <string>
#include <sstream>
#include <fstream>

using namespace wiSceneSystem;

Tests::Tests()
{
}
Tests::~Tests()
{
}

void Tests::Initialize()
{
	// Call this before Maincomponent::Initialize if you want to load shaders from an other directory!
	// otherwise, shaders will be loaded from the working directory
	wiRenderer::GetShaderPath() = "../WickedEngine/shaders/";
	wiFont::GetFontPath() = "../WickedEngine/fonts/"; // search for fonts elsewhere
	MainComponent::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;

	ActivatePath(new TestsRenderer);
}


TestsRenderer::TestsRenderer()
{
	setSSREnabled(false);
	setSSAOEnabled(false);
	setReflectionsEnabled(true);
	setFXAAEnabled(false);

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	wiLabel* label = new wiLabel("Label1");
	label->SetText("Wicked Engine Test Framework");
	label->SetSize(XMFLOAT2(200,15));
	label->SetPos(XMFLOAT2(screenW / 2.f - label->scale.x / 2.f, screenH*0.95f));
	GetGUI().AddWidget(label);


	wiButton* audioTest = new wiButton("AudioTest");
	audioTest->SetText("Play Test Audio");
	audioTest->SetSize(XMFLOAT2(180, 20));
	audioTest->SetPos(XMFLOAT2(10, 80));
	audioTest->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	audioTest->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	audioTest->OnClick([=](wiEventArgs args) {
		static wiMusic music("sound/music.wav");
		static bool playing = false;

		if (playing)
		{
			music.Stop();
			audioTest->SetText("Play Test Audio");
		}
		else
		{
			music.Play();
			audioTest->SetText("Stop Test Audio");
		}

		playing = !playing;
	});
	GetGUI().AddWidget(audioTest);


	wiSlider* volume = new wiSlider(0, 100, 50, 100, "Volume");
	volume->SetText("Volume: ");
	volume->SetSize(XMFLOAT2(85, 20));
	volume->SetPos(XMFLOAT2(65, 110));
	volume->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	volume->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	volume->OnSlide([](wiEventArgs args) {
		wiMusic::SetVolume(args.fValue / 100.0f);
	});
	GetGUI().AddWidget(volume);


	wiComboBox* testSelector = new wiComboBox("TestSelector");
	testSelector->SetText("Demo: ");
	testSelector->SetSize(XMFLOAT2(120, 20));
	testSelector->SetPos(XMFLOAT2(50, 140));
	testSelector->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	testSelector->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	testSelector->AddItem("HelloWorld");
	testSelector->AddItem("Model");
	testSelector->AddItem("EmittedParticle 1");
	testSelector->AddItem("EmittedParticle 2");
	testSelector->AddItem("HairParticle");
	testSelector->AddItem("Lua Script");
	testSelector->AddItem("Water Test");
	testSelector->AddItem("Shadows Test");
	testSelector->AddItem("Physics Test");
	testSelector->AddItem("Cloth Physics Test");
	testSelector->AddItem("Job System Test");
	testSelector->AddItem("Font Test");
	testSelector->AddItem("Volumetric Test");
	testSelector->AddItem("Sprite Test");
	testSelector->SetMaxVisibleItemCount(100);
	testSelector->OnSelect([=](wiEventArgs args) {

		// Reset all state that tests might have modified:
		wiRenderer::SetToDrawGridHelper(false);
		wiRenderer::SetTemporalAAEnabled(false);
		wiRenderer::ClearWorld();
		wiRenderer::GetScene().weather = WeatherComponent();
		this->clearSprites();
		this->clearFonts();
		wiLua::GetGlobal()->KillProcesses();

		// Reset camera position:
		TransformComponent transform;
		transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
		transform.UpdateTransform();
		wiRenderer::GetCamera().TransformCamera(transform);

		// Based on combobox selection, start the appropriate test:
		switch (args.iValue)
		{
		case 0:
		{
			static wiSprite sprite;
			sprite = wiSprite("images/HelloWorld.png");
			sprite.params.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite.params.siz = XMFLOAT2(200, 100);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.rot = XM_PI / 4.0f;
			this->addSprite(&sprite);
			break;
		}
		case 1:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/teapot.wiscene");
			break;
		case 2:
			wiRenderer::LoadModel("../models/emitter_smoke.wiscene");
			break;
		case 3:
			wiRenderer::LoadModel("../models/emitter_skinned.wiscene");
			break;
		case 4:
			wiRenderer::LoadModel("../models/hairparticle_torus.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case 5:
			wiRenderer::SetToDrawGridHelper(true);
			wiLua::GetGlobal()->RunFile("test_script.lua");
			break;
		case 6:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/water_test.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case 7:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/shadows_test.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case 8:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/physics_test.wiscene");
			break;
		case 9:
			wiRenderer::LoadModel("../models/cloth_test.wiscene", XMMatrixTranslation(0, 3, 4));
			break;
		case 10:
			RunJobSystemTest();
			break;
		case 11:
			RunFontTest();
			break;
		case 12:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/volumetric_test.wiscene", XMMatrixTranslation(0, 0, 4));
			break;
		case 13:
			RunSpriteTest();
			break;
		default:
			assert(0);
			break;
		}

	});
	testSelector->SetSelected(0);
	GetGUI().AddWidget(testSelector);

}
TestsRenderer::~TestsRenderer()
{
}


void TestsRenderer::RunJobSystemTest()
{
	// This will simulate going over a big dataset first in a simple loop, then with the Job System and compare timings
	uint32_t itemCount = 1000000;
	std::stringstream ss("");
	ss << "Job System performance test:" << std::endl;
	ss << "You can find out more in Tests.cpp, RunJobSystemTest() function." << std::endl;
	ss << "The simple loop should take longer to execute if the job system is implemented correctly." << std::endl;
	ss << "Result:" << std::endl;
	ss << std::endl;

	wiTimer timer;

	// Simple loop test:
	{
		std::vector<wiSceneSystem::CameraComponent> dataSet(itemCount);
		timer.record();
		for (uint32_t i = 0; i < itemCount; ++i)
		{
			dataSet[i].UpdateCamera();
		}
		double time = timer.elapsed();
		ss << "Simple loop took " << time << " milliseconds" << std::endl;
	}

	// Job system test:
	{
		std::vector<wiSceneSystem::CameraComponent> dataSet(itemCount);
		timer.record();
		wiJobSystem::Dispatch(itemCount, 1000, [&](wiJobDispatchArgs args) {
			dataSet[args.jobIndex].UpdateCamera();
		});
		wiJobSystem::Wait();
		double time = timer.elapsed();
		ss << "wiJobSystem::Dispatch() took " << time << " milliseconds" << std::endl;
	}

	static wiFont font;
	font = wiFont(ss.str());
	font.params.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font.params.posY = wiRenderer::GetDevice()->GetScreenHeight() / 2;
	font.params.h_align = WIFALIGN_CENTER;
	font.params.v_align = WIFALIGN_CENTER;
	font.params.size = 24;
	this->addFont(&font);
}
void TestsRenderer::RunFontTest()
{
	static wiFont font;
	static wiFont font_upscaled;
	int arial = wiFont::AddFontStyle(wiFont::GetFontPath() + "arial.ttf");

	font.SetText("This is Arial, size 32 wiFont");
	font_upscaled.SetText("This is Arial, size 14 wiFont, but upscaled to 32");

	font.params.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font.params.posY = wiRenderer::GetDevice()->GetScreenHeight() / 6;
	font.params.size = 32;

	font_upscaled.params = font.params;
	font_upscaled.params.posY += font.textHeight() + 10;

	font.style = arial;
	font_upscaled.style = arial;
	font_upscaled.params.size = 14;
	font_upscaled.params.scaling = 32.0f / 14.0f;

	addFont(&font);
	addFont(&font_upscaled);

	static wiFont font_aligned;
	font_aligned = font;
	font_aligned.params.posY += font.textHeight() * 2;
	font_aligned.params.size = 38;
	font_aligned.params.shadowColor = wiColor::Red();
	font_aligned.params.h_align = WIFALIGN_CENTER;
	font_aligned.SetText("Center aligned, red shadow, bigger");
	addFont(&font_aligned);

	static wiFont font_aligned2;
	font_aligned2 = font_aligned;
	font_aligned2.params.posY += font_aligned.textHeight() + 10;
	font_aligned2.params.shadowColor = wiColor::Purple();
	font_aligned2.params.h_align = WIFALIGN_RIGHT;
	font_aligned2.SetText("Right aligned, purple shadow");
	addFont(&font_aligned2);

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
	static wiFont font_japanese;
	font_japanese = font_aligned2;
	font_japanese.params.posY += font_aligned2.textHeight();
	font_japanese.style = wiFont::AddFontStyle("yumin.ttf");
	font_japanese.params.shadowColor = wiColor::Transparent();
	font_japanese.params.h_align = WIFALIGN_CENTER;
	font_japanese.params.size = 34;
	font_japanese.SetText(ss.str());
	addFont(&font_japanese);

	static wiFont font_colored;
	font_colored.params.color = wiColor::Cyan();
	font_colored.params.h_align = WIFALIGN_CENTER;
	font_colored.params.v_align = WIFALIGN_TOP;
	font_colored.params.size = 26;
	font_colored.params.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font_colored.params.posY = font_japanese.params.posY + font_japanese.textHeight();
	font_colored.SetText("Colored font");
	addFont(&font_colored);
}
void TestsRenderer::RunSpriteTest()
{
	static wiSprite sprite("images/fire_001.png");
	sprite.params.pos = XMFLOAT3(wiRenderer::GetDevice()->GetScreenWidth() * 0.5f, wiRenderer::GetDevice()->GetScreenHeight() * 0.5f, 0);
	sprite.params.siz = XMFLOAT2(200, 200); // size on screen
	sprite.params.pivot = XMFLOAT2(0.5f, 0.5f); // center pivot
	sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192)); // set the draw rect (texture cutout). This will also be the first frame of the animation
	sprite.anim = wiSprite::Anim(); // reset animation state
	sprite.anim.drawRectAnim.frameCount = 20; // set the total frame count of the animation
	sprite.anim.drawRectAnim.horizontalFrameCount = 5; // this is a multiline spritesheet, so also set how many maximum frames there are in a line
	sprite.anim.drawRectAnim.frameRate = 40; // animation frames per second
	sprite.anim.repeatable = true; // looping
	addSprite(&sprite);
}
