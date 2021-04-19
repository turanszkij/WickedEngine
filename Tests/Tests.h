#pragma once
#include "WickedEngine.h"


class TestsRenderer : public RenderPath3D
{
	wiLabel label;
	wiComboBox testSelector;
	wiECS::Entity ik_entity = wiECS::INVALID_ENTITY;
public:
	MainComponent* main = nullptr;

	void Load() override;
	void Update(const wiCanvas& canvas, float dt) override;
	void ResizeLayout(const wiCanvas& canvas) override;

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
	void RunNetworkTest();
};

class Tests : public MainComponent
{
	TestsRenderer renderer;
public:
	void Initialize() override;
};

