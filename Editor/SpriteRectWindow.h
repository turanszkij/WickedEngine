#pragma once
class EditorComponent;

class SpriteRectWindow : public wi::gui::Window
{
	std::function<void()> onAccepted;
public:
	void Create(EditorComponent* editor);

	wi::gui::Button spriteButton;
	wi::gui::Button okButton;

	wi::Sprite sprite;
	XMFLOAT2 dragStartUV = XMFLOAT2(0, 0);
	XMFLOAT2 dragEndUV = XMFLOAT2(0, 0);
	XMFLOAT4 muladd = XMFLOAT4(1, 1, 0, 0);

	void SetSprite(const wi::Sprite& sprite);
	void ResetSelection();
	void OnAccepted(std::function<void()> cb);

	void ResizeLayout() override;
	void Update(const wi::Canvas& canvas, float dt) override;
	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
};
