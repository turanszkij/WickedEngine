#pragma once

class TestsRenderer : public wi::RenderPath3D
{
	wi::gui::Label label;
	wi::gui::ComboBox testSelector;
	wi::ecs::Entity ik_entity = wi::ecs::INVALID_ENTITY;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
	void RunNetworkTest();
	void ContainerTest();
};

class Tests : public wi::Application
{
	TestsRenderer renderer;
public:
	void Initialize() override;
};

