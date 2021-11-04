#pragma once
#include "WickedEngine.h"


class Example_ImGuiRenderer : public RenderPath3D
{
	wiLabel label;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;
	void Render() const override;
};

class Example_ImGui : public MainComponent
{
	Example_ImGuiRenderer renderer;

public:
	~Example_ImGui() override;
	void Initialize() override;
	void Compose(wiGraphics::CommandList cmd) override;
};

