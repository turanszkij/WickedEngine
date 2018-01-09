#include "stdafx.h"
#include "Tests.h"


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
	wiRenderer::SHADERPATH = "../WickedEngine/shaders/";
	wiFont::FONTPATH = "../WickedEngine/fonts/"; // search for fonts elsewhere
	MainComponent::Initialize();

	wiRenderer::physicsEngine = new wiBULLET;

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.cpuinfo = false;
	infoDisplay.resolution = true;

	activateComponent(new TestsRenderer);
}


TestsRenderer::TestsRenderer()
{
	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	wiRenderer::getCamera()->Translate(XMFLOAT3(0, 2.f, -4.5f));

	wiLabel* label = new wiLabel("Label1");
	label->SetText("Wicked Engine Test Framework");
	label->SetSize(XMFLOAT2(200,15));
	label->SetPos(XMFLOAT2(screenW / 2.f - label->scale.x / 2.f, screenH*0.95f));
	GetGUI().AddWidget(label);


	wiComboBox* testSelector = new wiComboBox("TestSelector");
	testSelector->SetText("Demo: ");
	testSelector->SetSize(XMFLOAT2(100, 20));
	testSelector->SetPos(XMFLOAT2(50, 80));
	testSelector->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	testSelector->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	testSelector->AddItem("HelloWorld");
	testSelector->AddItem("Model");
	testSelector->AddItem("Lua Script");
	testSelector->AddItem("Soft Body");
	testSelector->AddItem("Emitter");
	testSelector->OnSelect([=](wiEventArgs args) {

		wiRenderer::ClearWorld();
		this->clearSprites();
		wiLua::GetGlobal()->KillProcesses();

		switch (args.iValue)
		{
		case 0:
		{
			wiSprite* sprite = new wiSprite("images/HelloWorld.png");
			sprite->effects.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite->effects.siz = XMFLOAT2(200, 100);
			sprite->effects.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite->anim.rot = XM_PI / 400.0f;
			this->addSprite(sprite);
			break;
		}
		case 1:
			wiRenderer::LoadModel("../models/Stormtrooper/Stormtrooper.wimf");
			break;
		case 2:
			wiLua::GetGlobal()->RunFile("test_script.lua");
			break;
		case 3:
			wiRenderer::LoadModel("../models/SoftBody/flag.wimf")->Translate(XMFLOAT3(0, -1, 2));
			break;
		case 4:
			wiRenderer::LoadModel("../models/Emitter/emitter.wimf")->Translate(XMFLOAT3(0, 2, 2));
			break;
		}

	});
	testSelector->SetSelected(0);
	GetGUI().AddWidget(testSelector);


	wiButton* audioTest = new wiButton("AudioTest");
	audioTest->SetText("Play Test Audio");
	audioTest->SetSize(XMFLOAT2(161, 20));
	audioTest->SetPos(XMFLOAT2(10, 110));
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
	volume->SetPos(XMFLOAT2(65, 140));
	volume->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	volume->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	volume->OnSlide([](wiEventArgs args) {
		wiMusic::SetVolume(args.fValue / 100.0f);
	});
	GetGUI().AddWidget(volume);

}
TestsRenderer::~TestsRenderer()
{
}
