#pragma once
#include "WickedEngine.h"


class TestsRenderer : public RenderPath3D_Deferred
{
	wiComboBox* testSelector = nullptr;
	wiECS::Entity ik_entity = wiECS::INVALID_ENTITY;
public:
	void Load() override;
	void Update(float dt) override;

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

