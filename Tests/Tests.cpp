#include "stdafx.h"
#include "Tests.h"

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
	infoDisplay.cpuinfo = false;
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


	wiComboBox* testSelector = new wiComboBox("TestSelector");
	testSelector->SetText("Demo: ");
	testSelector->SetSize(XMFLOAT2(120, 20));
	testSelector->SetPos(XMFLOAT2(50, 80));
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
	testSelector->SetMaxVisibleItemCount(100);
	testSelector->OnSelect([=](wiEventArgs args) {

		wiRenderer::SetToDrawGridHelper(false);
		wiRenderer::SetTemporalAAEnabled(false);
		wiRenderer::ClearWorld();
		wiRenderer::GetScene().weather = WeatherComponent();
		this->clearSprites();
		wiLua::GetGlobal()->KillProcesses();

		TransformComponent transform;
		transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
		transform.UpdateTransform();
		wiRenderer::GetCamera().TransformCamera(transform);

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
		}

	});
	testSelector->SetSelected(0);
	GetGUI().AddWidget(testSelector);


	wiButton* audioTest = new wiButton("AudioTest");
	audioTest->SetText("Play Test Audio");
	audioTest->SetSize(XMFLOAT2(180, 20));
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
