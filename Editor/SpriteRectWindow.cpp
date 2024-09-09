#include "stdafx.h"
#include "Editor.h"
#include "SpriteRectWindow.h"

void SpriteRectWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Select an area within the sprite...");
	SetLocalizationEnabled(false);

	SetSize(XMFLOAT2(300, 300));

	spriteButton.Create("");
	spriteButton.OnDragStart([=](wi::gui::EventArgs args) {
		XMFLOAT2 pos = spriteButton.GetPos();
		XMFLOAT2 size = spriteButton.GetSize();
		dragStartUV = wi::math::InverseLerp(pos, XMFLOAT2(pos.x + size.x, pos.y + size.y), args.clickPos);
		dragStartUV.x = saturate(dragStartUV.x);
		dragStartUV.y = saturate(dragStartUV.y);
		dragEndUV = dragStartUV;
	});
	spriteButton.OnDrag([=](wi::gui::EventArgs args) {
		XMFLOAT2 pos = spriteButton.GetPos();
		XMFLOAT2 size = spriteButton.GetSize();
		dragEndUV = wi::math::InverseLerp(pos, XMFLOAT2(pos.x + size.x, pos.y + size.y), args.clickPos);
		dragEndUV.x = saturate(dragEndUV.x);
		dragEndUV.y = saturate(dragEndUV.y);
	});
	spriteButton.OnDragEnd([=](wi::gui::EventArgs args) {
		XMFLOAT2 pos = spriteButton.GetPos();
		XMFLOAT2 size = spriteButton.GetSize();
		dragEndUV = wi::math::InverseLerp(pos, XMFLOAT2(pos.x + size.x, pos.y + size.y), args.clickPos);
		dragEndUV.x = saturate(dragEndUV.x);
		dragEndUV.y = saturate(dragEndUV.y);

		muladd = XMFLOAT4(
			saturate(std::abs(dragEndUV.x - dragStartUV.x)),
			saturate(std::abs(dragEndUV.y - dragStartUV.y)),
			saturate(std::min(dragStartUV.x, dragEndUV.x)),
			saturate(std::min(dragStartUV.y, dragEndUV.y))
		);
	});
	AddWidget(&spriteButton);

	okButton.Create("OK");
	okButton.OnClick([=](wi::gui::EventArgs args) {

		if (onAccepted != nullptr)
			onAccepted();

		SetVisible(false);
	});
	AddWidget(&okButton);

	SetPos(XMFLOAT2(200, 100));
}

void SpriteRectWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	XMFLOAT2 size = GetWidgetAreaSize();

	spriteButton.SetSize(XMFLOAT2(size.x, size.y - okButton.GetSize().y * 2 - 1));
	spriteButton.SetPos(XMFLOAT2(0, 0));

	okButton.SetSize(XMFLOAT2(size.x, okButton.GetSize().y));
	okButton.SetPos(XMFLOAT2(0, spriteButton.GetSize().y));
}
void SpriteRectWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::gui::Window::Update(canvas, dt);

	if (!IsVisible())
		return;

	if (GetPointerHitbox().intersects(spriteButton.hitBox))
	{
		wi::input::SetCursor(wi::input::CURSOR_CROSS);
	}
}
void SpriteRectWindow::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	wi::gui::Window::Render(canvas, cmd);

	if (!IsVisible())
		return;

	ApplyScissor(canvas, this->scissorRect, cmd);

	XMFLOAT2 pos = spriteButton.GetPos();
	XMFLOAT2 size = spriteButton.GetSize();
	XMFLOAT2 end = XMFLOAT2(pos.x + size.x, pos.y + size.y);

	XMFLOAT2 rectstart = wi::math::Lerp(pos, end, dragStartUV);
	XMFLOAT2 rectend = wi::math::Lerp(pos, end, dragEndUV);

	wi::image::Params params;
	params.pos = XMFLOAT3(rectstart.x, rectstart.y, 0);
	params.siz = XMFLOAT2(rectend.x - rectstart.x, rectend.y - rectstart.y);
	params.color = wi::Color(255, 255, 255, 80);
	params.sampleFlag = wi::image::SAMPLEMODE_WRAP;
	wi::image::Draw(nullptr, params, cmd);
}

void SpriteRectWindow::SetSprite(const wi::Sprite& sprite)
{
	for (auto& x : spriteButton.sprites)
	{
		x = sprite;
	}
}
void SpriteRectWindow::ResetSelection()
{
	dragStartUV = XMFLOAT2(0, 0);
	dragEndUV = XMFLOAT2(0, 0);
	muladd = XMFLOAT4(1, 1, 0, 0);
}
void SpriteRectWindow::OnAccepted(std::function<void()> cb)
{
	onAccepted = cb;
}
