#pragma once
#include "WickedEngine.h"


class TestsRenderer : public wi::RenderPath3D
{
	wi::widget::Label label;
	wi::widget::ComboBox testSelector;
	wi::ecs::Entity ik_entity = wi::ecs::INVALID_ENTITY;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
	void RunNetworkTest();
	void RunUnorderedMapTest();
};

class Tests : public wi::Application
{
	TestsRenderer renderer;
public:
	void Initialize() override;
};

