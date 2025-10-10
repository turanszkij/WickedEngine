#pragma once
class EditorComponent;

class ProfilerWidget : public wi::gui::Widget
{
	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
};

class ProfilerWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	ProfilerWidget profilerWidget;
	EditorComponent* editor = nullptr;

	void Update(const wi::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};
