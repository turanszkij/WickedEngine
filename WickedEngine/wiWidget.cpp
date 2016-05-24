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

	state = IDLE;
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

wiWidget::WIDGETSTATE wiWidget::GetState()
{
	return state;
}




wiButton::wiButton(const string& newName)
{
	SetName(newName);
	state = IDLE;
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
		state = FOCUS;
		if (wiInputManager::press(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			wiEventArgs args;
			args.clickPos = pointerHitbox.pos;
			onClick(args);
			state = ACTIVE;
		}
	}

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	wiColor buttonColor = wiColor(127, 127, 127, 127);
	if (state == FOCUS)
	{
		buttonColor = wiColor::Gray;
	}
	else if (state == ACTIVE)
	{
		buttonColor = wiColor::White;
	}

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(buttonColor)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f, 0, WIFALIGN_CENTER, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
}

void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}