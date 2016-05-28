#include "wiWidget.h"
#include "wiGUI.h"
#include "wiInputManager.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiFont.h"
#include "wiMath.h"


wiWidget::wiWidget():Transform()
{
	state = IDLE;
	enabled = true;
	visible = true;
	colors[IDLE] = wiColor::Ghost;
	colors[FOCUS] = wiColor::Gray;
	colors[ACTIVE] = wiColor::White;
	colors[DEACTIVATING] = wiColor::Gray;
}
wiWidget::~wiWidget()
{
}
void wiWidget::Update(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	// Only do the updatetransform if it has no parent because if it has, its transform
	// will be updated down the chain anyway
	if (Transform::parent == nullptr)
	{
		Transform::UpdateTransform();
	}
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
	Transform::UpdateTransform();
}
void wiWidget::SetSize(const XMFLOAT2& value)
{
	Transform::scale_rest.x = value.x;
	Transform::scale_rest.y = value.y;
	Transform::UpdateTransform();
}
wiWidget::WIDGETSTATE wiWidget::GetState()
{
	return state;
}
void wiWidget::SetEnabled(bool val) 
{ 
	enabled = val; 
}
bool wiWidget::IsEnabled() 
{ 
	return enabled && visible; 
}
void wiWidget::SetVisible(bool val)
{ 
	visible = val;
}
bool wiWidget::IsVisible() 
{ 
	return visible;
}
void wiWidget::Activate()
{
	state = ACTIVE;
}
void wiWidget::Deactivate()
{
	state = DEACTIVATING;
}
void wiWidget::SetColor(const wiColor& color, WIDGETSTATE state)
{
	colors[state] = color;
}
wiColor wiWidget::GetColor()
{
	return colors[GetState()];
}


wiButton::wiButton(const string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::GetInstance()->getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	if (clicked)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		onClick(args);
		gui->ActivateWidget(this);
	}

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (!IsEnabled())
	{
		color = wiColor::lerp(wiColor::Transparent, color, 0.25f);
	}

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f, 0, WIFALIGN_CENTER, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
}
void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}




wiLabel::wiLabel(const string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(100, 20));
}
wiLabel::~wiLabel()
{

}
void wiLabel::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiLabel::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (!IsEnabled())
	{
		color = wiColor::lerp(wiColor::Transparent, color, 0.25f);
	}

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x, translation.y, 0, WIFALIGN_LEFT, WIFALIGN_TOP)).Draw(gui->GetGraphicsThread());
}




wiSlider::wiSlider(float start, float end, float defaultValue, float step, const string& name) :wiWidget()
	,start(start), end(end), value(defaultValue), step(max(step, 1))
{
	SetName(name);
	SetText(fastName.GetString());
	OnSlide([](wiEventArgs args) {});
	SetSize(XMFLOAT2(200, 40));
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

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::GetInstance()->getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	bool dragged = false;

	if (pointerHitbox.intersects(hitBox))
	{
		// hover the slider
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}

	if(wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == ACTIVE)
		{
			// continue drag if already grabbed wheter it is intersecting or not
			dragged = true;
		}
	}

	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		value = wiMath::InverseLerp(translation.x, translation.x + scale.x, args.clickPos.x);
		value = wiMath::Clamp(value, 0, 1);
		value *= step;
		value = floorf(value);
		value /= step;
		value = wiMath::Lerp(start, end, value);
		args.fValue = value;
		onSlide(args);
		gui->ActivateWidget(this);
	}
	else if(state != IDLE)
	{
		gui->DeactivateWidget(this);
	}

}
void wiSlider::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (!IsEnabled())
	{
		color = wiColor::lerp(wiColor::Transparent, color, 0.25f);
	}

	float headWidth = scale.x*0.05f;

	// trail
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x - headWidth*0.5f, translation.y + scale.y * 0.5f - scale.y*0.1f, scale.x + headWidth, scale.y * 0.2f), gui->GetGraphicsThread());
	// head
	float headPosX = wiMath::Lerp(translation.x, translation.x + scale.x, wiMath::Clamp(wiMath::InverseLerp(start, end, value), 0, 1));
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





wiCheckBox::wiCheckBox(const string& name) :wiWidget()
	,checked(false)
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	SetSize(XMFLOAT2(20,20));
}
wiCheckBox::~wiCheckBox()
{

}
void wiCheckBox::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::GetInstance()->getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	if (clicked)
	{
		SetCheck(!GetCheck());
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		args.bValue = GetCheck();
		onClick(args);
		gui->ActivateWidget(this);
	}

}
void wiCheckBox::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (!IsEnabled())
	{
		color = wiColor::lerp(wiColor::Transparent, color, 0.25f);
	}

	// control
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	// check
	if (GetCheck())
	{
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(wiColor::lerp(color, wiColor::White, 0.8f))
			, wiImageEffects(translation.x + scale.x*0.25f, translation.y + scale.y*0.25f, scale.x*0.5f, scale.y*0.5f)
			, gui->GetGraphicsThread());
	}

	wiFont(text, wiFontProps(translation.x, translation.y + scale.y*0.5f, 0, WIFALIGN_RIGHT, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
}
void wiCheckBox::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}
void wiCheckBox::SetCheck(bool value)
{
	checked = value;
}
bool wiCheckBox::GetCheck()
{
	return checked;
}





wiDragger::wiDragger(const string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnDragStart([](wiEventArgs args) {});
	OnDrag([](wiEventArgs args) {});
	OnDragEnd([](wiEventArgs args) {});
	SetSize(XMFLOAT2(30, 30));
}
wiDragger::~wiDragger()
{

}
void wiDragger::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	XMFLOAT4 pointerPos = wiInputManager::GetInstance()->getpointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointerPos.x, pointerPos.y), XMFLOAT2(1, 1));

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		onDragEnd(args);

		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);

			wiEventArgs args;
			args.clickPos = pointerHitbox.pos;
			XMFLOAT3 posDelta;
			posDelta.x = pointerHitbox.pos.x - prevPos.x;
			posDelta.y = pointerHitbox.pos.y - prevPos.y;
			posDelta.z = 0;
			args.deltaPos = XMFLOAT2(posDelta.x, posDelta.y);
			Transform* savedParent = Transform::parent;
			Transform::detach();
			Transform::Translate(posDelta);
			Transform::attachTo(savedParent);
			onDrag(args);
		}
	}

	if (clicked)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;

		// just clicked
		dragStart = args.clickPos;
		args.startPos = dragStart;
		onDragStart(args);

		gui->ActivateWidget(this);
	}

	prevPos.x = pointerHitbox.pos.x;
	prevPos.y = pointerHitbox.pos.y;

}
void wiDragger::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (!IsEnabled())
	{
		color = wiColor::lerp(wiColor::Transparent, color, 0.25f);
	}

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f, 0, WIFALIGN_CENTER, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread());
}
void wiDragger::OnDragStart(function<void(wiEventArgs args)> func)
{
	onDragStart = move(func);
}
void wiDragger::OnDrag(function<void(wiEventArgs args)> func)
{
	onDrag = move(func);
}
void wiDragger::OnDragEnd(function<void(wiEventArgs args)> func)
{
	onDragEnd = move(func);
}






wiWindow::wiWindow(wiGUI* gui, const string& name) :wiWidget()
	, gui(gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(640, 480));

	SAFE_INIT(closeButton);
	SAFE_INIT(moveDragger);
	SAFE_INIT(resizeDragger);
}
wiWindow::~wiWindow()
{
	SAFE_DELETE(closeButton);
	SAFE_DELETE(moveDragger);
	SAFE_DELETE(resizeDragger);
}
void wiWindow::AddWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	gui->AddWidget(widget);
	widget->attachTo(this);

	children.push_back(widget);
}
void wiWindow::RemoveWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	gui->RemoveWidget(widget);
	widget->detach();

	children.remove(widget);
}
void wiWindow::AddControls()
{
	if (moveDragger == nullptr)
	{
		moveDragger = new wiDragger(name + "move_dragger");
		moveDragger->SetText("");
		moveDragger->SetSize(XMFLOAT2(scale.x - 20, 20));
		moveDragger->SetPos(XMFLOAT2(0, 0));
		moveDragger->OnDragStart([this](wiEventArgs args) {

		});
		moveDragger->OnDrag([this](wiEventArgs args) {
			this->moveDragger->detach();
			this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
			this->moveDragger->attachTo(this);
		});
		moveDragger->OnDragEnd([this](wiEventArgs args) {

		});
		gui->AddWidget(moveDragger);
		moveDragger->attachTo(this);
	}

	if(closeButton==nullptr)
	{
		closeButton = new wiButton(name + "_close_button");
		closeButton->SetText("x");
		closeButton->SetSize(XMFLOAT2(20, 20));
		closeButton->SetPos(XMFLOAT2(translation.x + scale.x - 20, translation.y));
		closeButton->OnClick([this](wiEventArgs args) {
			this->SetVisible(false);
		});
		gui->AddWidget(closeButton);
		closeButton->attachTo(this);
	}

	if (resizeDragger == nullptr)
	{
		resizeDragger = new wiDragger(name + "resize_dragger");
		resizeDragger->SetText("");
		resizeDragger->SetSize(XMFLOAT2(20, 20));
		resizeDragger->SetPos(XMFLOAT2(translation.x + scale.x - 20, translation.y + scale.y - 20));
		resizeDragger->OnDragStart([this](wiEventArgs args) {

		});
		resizeDragger->OnDrag([this](wiEventArgs args) {
			XMFLOAT2 scaleDiff;
			scaleDiff.x = (scale.x + args.deltaPos.x) / scale.x;
			scaleDiff.y = (scale.y + args.deltaPos.y) / scale.y;
			this->resizeDragger->detach();
			this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 0));
			this->resizeDragger->attachTo(this);
		});
		resizeDragger->OnDragEnd([this](wiEventArgs args) {

		});
		gui->AddWidget(resizeDragger);
		resizeDragger->attachTo(this);
	}
}
void wiWindow::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiWindow::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (!IsEnabled())
	{
		color = wiColor::lerp(wiColor::Transparent, color, 0.25f);
	}

	// body
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	wiFont(text, wiFontProps(translation.x, translation.y, 0, WIFALIGN_LEFT, WIFALIGN_TOP)).Draw(gui->GetGraphicsThread());
}
void wiWindow::SetVisible(bool value)
{
	wiWidget::SetVisible(value);
	if (closeButton != nullptr)
	{
		closeButton->SetVisible(value);
	}
	if (moveDragger != nullptr)
	{
		moveDragger->SetVisible(value);
	}
	if (resizeDragger != nullptr)
	{
		resizeDragger->SetVisible(value);
	}
	for (auto& x : children)
	{
		x->SetVisible(value);
	}
}
