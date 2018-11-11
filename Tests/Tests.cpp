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
			sprite.effects.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite.effects.siz = XMFLOAT2(200, 100);
			sprite.effects.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.rot = XM_PI / 400.0f;
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
		wiJobSystem::Dispatch(itemCount, 1000, [&](JobDispatchArgs args) {
			dataSet[args.jobIndex].UpdateCamera();
		});
		wiJobSystem::Wait();
		double time = timer.elapsed();
		ss << "wiJobSystem::Dispatch() took " << time << " milliseconds" << std::endl;
	}

	static wiFont font;
	font = wiFont(ss.str());
	font.props.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font.props.posY = wiRenderer::GetDevice()->GetScreenHeight() / 2;
	font.props.h_align = WIFALIGN_CENTER;
	font.props.v_align = WIFALIGN_CENTER;
	font.props.size = 24;
	this->addFont(&font);
}
void TestsRenderer::RunFontTest()
{
	static wiFont font;
	static wiFont font_upscaled;

	font.SetText("This is Arial, size 32 wiFont");
	font_upscaled.SetText("This is Arial, size 14 wiFont, but upscaled to 32");

	font.props.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font.props.posY = wiRenderer::GetDevice()->GetScreenHeight() / 6;

	font_upscaled.props = font.props;
	font_upscaled.props.posY += font.textHeight() + 10;

	font.style = wiFont::AddFontStyle(wiFont::GetFontPath() + "arial.ttf", 32);
	font_upscaled.style = wiFont::AddFontStyle(wiFont::GetFontPath() + "arial.ttf", 14);
	font_upscaled.props.size = 32; // upscale

	addFont(&font);
	addFont(&font_upscaled);

	static wiFont font_aligned;
	font_aligned = font;
	font_aligned.props.posY += font.textHeight() * 2;
	font_aligned.props.size = 38;
	font_aligned.props.shadowColor = wiColor::Red;
	font_aligned.props.h_align = WIFALIGN_CENTER;
	font_aligned.SetText("Center aligned, red shadow, bigger");
	addFont(&font_aligned);

	static wiFont font_aligned2;
	font_aligned2 = font_aligned;
	font_aligned2.props.posY += font_aligned.textHeight() + 10;
	font_aligned2.props.shadowColor = wiColor::Purple;
	font_aligned2.props.h_align = WIFALIGN_RIGHT;
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
	font_japanese.props.posY += font_aligned2.textHeight();
	font_japanese.style = wiFont::AddFontStyle("yumin.ttf", 34);
	font_japanese.props.shadowColor = wiColor::Transparent;
	font_japanese.props.h_align = WIFALIGN_CENTER;
	font_japanese.props.size = -1; // no scaling, it will use 34 (that was specified with AddFontStyle second argument)
	font_japanese.SetText(ss.str());
	addFont(&font_japanese);
}
