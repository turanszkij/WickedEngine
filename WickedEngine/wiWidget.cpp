#include "wiWidget.h"
#include "wiGUI.h"
#include "wiInputManager.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiColor.h"
#include "wiFont.h"
#include "wiMath.h"


wiWidget::wiWidget():Transform()
{
	state = IDLE;
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
	if (value.length() <= 0)
	{
		static unsigned long widgetID = 0;
		stringstream ss("");
		ss << "widget_" << widgetID;
		name = ss.str();
		widgetID++;
	}

	fastName = wiHashString(name);
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




wiButton::wiButton(const string& name)
{
	SetName(name);
	OnClick([](wiEventArgs args) {});
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE || state == FOCUS)
	{
		state = DEACTIVATING;
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	bool clicked = false;
	// click on the button
	if (pointerHitbox.intersects(hitBox))
	{
		state = FOCUS;

		if (wiInputManager::press(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			clicked = true;
		}
	}

	if (clicked)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		onClick(args);
		state = ACTIVE;
	}

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	wiColor color = wiColor::Ghost;
	switch (state)
	{
	case wiWidget::FOCUS:
	case wiWidget::DEACTIVATING:
		color = wiColor::Gray;
		break;
	case wiWidget::ACTIVE:
		color = wiColor::White;
		break;
	default:
		break;
	}

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f, 0, WIFALIGN_CENTER, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
}
void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}




wiLabel::wiLabel(const string& name)
{
	SetName(name);
}
wiLabel::~wiLabel()
{

}
void wiLabel::Update(wiGUI* gui)
{
	wiWidget::Update(gui);
}
void wiLabel::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	wiColor color = wiColor::Ghost;

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x, translation.y, 0, WIFALIGN_LEFT, WIFALIGN_TOP)).Draw(gui->GetGraphicsThread());
}




wiSlider::wiSlider(float start, float end, float defaultValue, float step, const string& name)
	:start(start), end(end), value(defaultValue), step(step)
{
	SetName(name);
	OnSlide([](wiEventArgs args) {});
}
wiSlider::~wiSlider()
{
}
void wiSlider::SetValue(float value)
{
	this->value = value;
}
float wiSlider::GetValue()
{
	return value;
}
void wiSlider::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	bool dragged = false;
	// click on the slider
	if (pointerHitbox.intersects(hitBox))
	{
		state = FOCUS;

		if (wiInputManager::down(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			dragged = true;
		}
	}

	// continue drag if already grabbed wheter it is intersecting or not
	if (state == ACTIVE)
	{
		if (wiInputManager::down(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			dragged = true;
		}
	}

	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		value = wiMath::InverseLerp(translation.x, translation.x + scale.x, args.clickPos.x);
		value = wiMath::Clamp(value, 0, 1);
		value = wiMath::Lerp(start, end, value);
		onSlide(args);
		state = ACTIVE;
	}
	else if(state != IDLE)
	{
		state = DEACTIVATING;
	}

}
void wiSlider::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	wiColor color = wiColor::Ghost;
	switch (state)
	{
	case wiWidget::FOCUS:
	case wiWidget::DEACTIVATING:
		color = wiColor::Gray;
		break;
	case wiWidget::ACTIVE:
		color = wiColor::White;
		break;
	default:
		break;
	}

	float headWidth = scale.x*0.05f;

	// trail
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x - headWidth*0.5f, translation.y + scale.y * 0.5f - scale.y*0.1f, scale.x + headWidth, scale.y * 0.2f), gui->GetGraphicsThread());
	// head
	float headPosX = wiMath::Lerp(translation.x, translation.x + scale.x, wiMath::Lerp(start, end, value));
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(headPosX - headWidth * 0.5f, translation.y, headWidth, scale.y), gui->GetGraphicsThread());

	// text
	wiFont(text, wiFontProps(translation.x - headWidth * 0.5f, translation.y + scale.y*0.5f, 0, WIFALIGN_RIGHT, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
	// value
	stringstream ss("");
	ss << value;
	wiFont(ss.str(), wiFontProps(translation.x + scale.x + headWidth * 0.5f, translation.y + scale.y*0.5f, 0, WIFALIGN_LEFT, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
}

void wiSlider::OnSlide(function<void(wiEventArgs args)> func)
{
	onSlide = move(func);
}
