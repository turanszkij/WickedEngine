#include "stdafx.h"
#include "Tests.h"
#include "wiProfiler.h"


Tests::Tests()
{
}
Tests::~Tests()
{
}

void Tests::Initialize()
{
	MainComponent::Initialize();

	wiRenderer::SHADERPATH = "../WickedEngine/shaders/";

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.cpuinfo = false;
	infoDisplay.resolution = true;

	wiProfiler::GetInstance().ENABLED = false;

	wiInitializer::InitializeComponents(
		wiInitializer::WICKEDENGINE_INITIALIZE_RENDERER
		| wiInitializer::WICKEDENGINE_INITIALIZE_IMAGE
		| wiInitializer::WICKEDENGINE_INITIALIZE_FONT
		| wiInitializer::WICKEDENGINE_INITIALIZE_SOUND
		| wiInitializer::WICKEDENGINE_INITIALIZE_MISC
	);

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
	testSelector->SetText("");
	testSelector->SetSize(XMFLOAT2(120, 20));
	testSelector->SetPos(XMFLOAT2(5, 60));
	testSelector->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	testSelector->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	testSelector->AddItem("HelloWorld");
	testSelector->AddItem("Model");
	testSelector->AddItem("Lua Script");
	testSelector->OnSelect([=](wiEventArgs args) {

		wiRenderer::CleanUpStaticTemp();
		this->clearSprites();
		wiLua::GetGlobal()->KillProcesses();

		switch (args.iValue)
		{
		case 0:
		{
			wiSprite* sprite = new wiSprite("HelloWorld.png");
			sprite->effects.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite->effects.siz = XMFLOAT2(200, 100);
			sprite->effects.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite->anim.rot = XM_PI / 400.0f;
			this->addSprite(sprite);
			break;
		}
		case 1:
			wiRenderer::LoadModel("../WickedEngine/models/Stormtrooper/", "Stormtrooper");
			break;
		case 2:
			wiLua::GetGlobal()->RunFile("test_script.lua");
			break;
		}

	});
	testSelector->SetSelected(0);
	GetGUI().AddWidget(testSelector);
}
TestsRenderer::~TestsRenderer()
{
}
