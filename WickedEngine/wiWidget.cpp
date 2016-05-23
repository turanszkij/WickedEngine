#include "wiWidget.h"
#include "wiGUI.h"
#include "wiInputManager.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiColor.h"
#include "wiFont.h"


wiWidget::wiWidget():Transform()
{
}


wiWidget::~wiWidget()
{
}

void wiWidget::Update(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");
	Transform::UpdateTransform();
}

wiHashString wiWidget::GetName()
{
	return fastName;
}

void wiWidget::SetName(const string& value)
{
	name = value;
	fastName = wiHashString(value);
}

string wiWidget::GetText()
{
	return name;
}

void wiWidget::SetText(const string& value)
{
	text = value;
}

void wiWidget::SetPos(const XMFLOAT2& value)
{
	Transform::translation_rest.x = value.x;
	Transform::translation_rest.y = value.y;
}

void wiWidget::SetSize(const XMFLOAT2& value)
{
	Transform::scale_rest.x = value.x;
	Transform::scale_rest.y = value.y;
}




wiButton::wiButton(const string& newName)
{
	SetName(newName);
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	if (pointerHitbox.intersects(hitBox))
	{
		if (wiInputManager::press(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			wiEventArgs args;
			args.clickPos = pointerHitbox.pos;
			onClick(args);
		}
	}

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(wiColor(127,127,127,127))
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());
	wiFont(text, wiFontProps(translation.x, translation.y)).Draw(gui->GetGraphicsThread());
}

void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}