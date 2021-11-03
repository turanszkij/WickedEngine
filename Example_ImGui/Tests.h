#pragma once
#include "WickedEngine.h"


class TestsRenderer : public RenderPath3D
{
	wiLabel label;
	wiECS::Entity ik_entity = wiECS::INVALID_ENTITY;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;
	void Render() const override;
};

class Tests : public MainComponent
{
	TestsRenderer renderer;

public:
	~Tests() override;
	void Initialize() override;
	void Compose(wiGraphics::CommandList cmd) override;
};

