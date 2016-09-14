#include "wiWidget.h"
#include "wiGUI.h"
#include "wiInputManager.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiFont.h"
#include "wiMath.h"
#include "wiHelper.h"

using namespace wiGraphicsTypes;


wiWidget::wiWidget():Transform()
{
	SetFontScaling(0.5f);
	state = IDLE;
	enabled = true;
	visible = true;
	colors[IDLE] = wiColor::Booger;
	colors[FOCUS] = wiColor::Gray;
	colors[ACTIVE] = wiColor::White;
	colors[DEACTIVATING] = wiColor::Gray;
	scissorRect.bottom = 0;
	scissorRect.left = 0;
	scissorRect.right = 0;
	scissorRect.top = 0;
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
	if (state == WIDGETSTATE_COUNT)
	{
		for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
		{
			colors[i] = color;
		}
	}
	else
	{
		colors[state] = color;
	}
}
wiColor wiWidget::GetColor()
{
	wiColor retVal = colors[GetState()];
	if (!IsEnabled()) {
		retVal = wiColor::lerp(wiColor::Transparent, retVal, 0.5f);
	}
	return retVal;
}
float wiWidget::GetScaledFontSize()
{
	return GetFontScaling() * min(scale.x, scale.y);
}
void wiWidget::SetFontScaling(float val)
{
	fontScaling = val;
}
float wiWidget::GetFontScaling()
{
	return fontScaling;
}
void wiWidget::SetScissorRect(const wiGraphicsTypes::Rect& rect)
{
	scissorRect = rect;
	if(scissorRect.bottom>0)
		scissorRect.bottom -= 1;
	if (scissorRect.left>0)
		scissorRect.left += 1;
	if (scissorRect.right>0)
		scissorRect.right -= 1;
	if (scissorRect.top>0)
		scissorRect.top += 1;
}


wiButton::wiButton(const string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	OnDragStart([](wiEventArgs args) {});
	OnDrag([](wiEventArgs args) {});
	OnDragEnd([](wiEventArgs args) {});
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

		if (pointerHitbox.intersects(hitBox))
		{
			// Click occurs when the button is released within the bounds
			onClick(args);
		}

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
			onDrag(args);
		}
	}

	if (clicked)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		dragStart = args.clickPos;
		args.startPos = dragStart;
		onDragStart(args);
		gui->ActivateWidget(this);
	}

	prevPos.x = pointerHitbox.pos.x;
	prevPos.y = pointerHitbox.pos.y;

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());



	scissorRect.bottom = (LONG)(translation.y + scale.y);
	scissorRect.left = (LONG)(translation.x);
	scissorRect.right = (LONG)(translation.x + scale.x);
	scissorRect.top = (LONG)(translation.y);
	wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f, GetScaledFontSize(), WIFALIGN_CENTER, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread(), true);

}
void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}
void wiButton::OnDragStart(function<void(wiEventArgs args)> func)
{
	onDragStart = move(func);
}
void wiButton::OnDrag(function<void(wiEventArgs args)> func)
{
	onDrag = move(func);
}
void wiButton::OnDragEnd(function<void(wiEventArgs args)> func)
{
	onDragEnd = move(func);
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

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());


	scissorRect.bottom = (LONG)(translation.y + scale.y);
	scissorRect.left = (LONG)(translation.x);
	scissorRect.right = (LONG)(translation.x + scale.x);
	scissorRect.top = (LONG)(translation.y);
	wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps(translation.x, translation.y, GetScaledFontSize(), WIFALIGN_LEFT, WIFALIGN_TOP)).Draw(gui->GetGraphicsThread(), true);
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

	float headWidth = scale.x*0.05f;

	hitBox.pos.x = Transform::translation.x - headWidth*0.5f;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x + headWidth;
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

	float headWidth = scale.x*0.05f;

	// trail
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x - headWidth*0.5f, translation.y + scale.y * 0.5f - scale.y*0.1f, scale.x + headWidth, scale.y * 0.2f), gui->GetGraphicsThread());
	// head
	float headPosX = wiMath::Lerp(translation.x, translation.x + scale.x, wiMath::Clamp(wiMath::InverseLerp(start, end, value), 0, 1));
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(headPosX - headWidth * 0.5f, translation.y, headWidth, scale.y), gui->GetGraphicsThread());

	if (parent != nullptr)
	{
		wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	}
	// text
	wiFont(text, wiFontProps(translation.x - headWidth * 0.5f, translation.y + scale.y*0.5f, GetScaledFontSize(), WIFALIGN_RIGHT, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread(), parent != nullptr);
	// value
	stringstream ss("");
	ss << value;
	wiFont(ss.str(), wiFontProps(translation.x + scale.x + headWidth * 0.5f, translation.y + scale.y*0.5f, GetScaledFontSize(), WIFALIGN_LEFT, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread(), parent != nullptr);
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

	if (parent != nullptr)
	{
		wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	}
	wiFont(text, wiFontProps(translation.x, translation.y + scale.y*0.5f, GetScaledFontSize(), WIFALIGN_RIGHT, WIFALIGN_CENTER)).Draw(gui->GetGraphicsThread(), parent != nullptr);
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




wiWindow::wiWindow(wiGUI* gui, const string& name) :wiWidget()
, gui(gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(640, 480));

	// Add controls

	SAFE_INIT(closeButton);
	SAFE_INIT(moveDragger);
	SAFE_INIT(resizeDragger_BottomRight);
	SAFE_INIT(resizeDragger_UpperLeft);

	static const float controlSize = 20.0f;


	// Add a grabber onto the title bar
	moveDragger = new wiButton(name + "_move_dragger");
	moveDragger->SetText("");
	moveDragger->SetSize(XMFLOAT2(scale.x - controlSize * 3, controlSize));
	moveDragger->SetPos(XMFLOAT2(controlSize, 0));
	moveDragger->OnDrag([this](wiEventArgs args) {
		this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
	});
	gui->AddWidget(moveDragger);
	moveDragger->attachTo(this);

	// Add close button to the top right corner
	closeButton = new wiButton(name + "_close_button");
	closeButton->SetText("x");
	closeButton->SetSize(XMFLOAT2(controlSize, controlSize));
	closeButton->SetPos(XMFLOAT2(translation.x + scale.x - controlSize, translation.y));
	closeButton->OnClick([this](wiEventArgs args) {
		this->SetVisible(false);
	});
	gui->AddWidget(closeButton);
	closeButton->attachTo(this);

	// Add minimize button to the top right corner
	minimizeButton = new wiButton(name + "_minimize_button");
	minimizeButton->SetText("-");
	minimizeButton->SetSize(XMFLOAT2(controlSize, controlSize));
	minimizeButton->SetPos(XMFLOAT2(translation.x + scale.x - controlSize*2, translation.y));
	minimizeButton->OnClick([this](wiEventArgs args) {
		this->SetMinimized(!this->IsMinimized());
	});
	gui->AddWidget(minimizeButton);
	minimizeButton->attachTo(this);

	// Add a resizer control to the upperleft corner
	resizeDragger_UpperLeft = new wiButton(name + "_resize_dragger_upper_left");
	resizeDragger_UpperLeft->SetText("");
	resizeDragger_UpperLeft->SetSize(XMFLOAT2(controlSize, controlSize));
	resizeDragger_UpperLeft->SetPos(XMFLOAT2(0, 0));
	resizeDragger_UpperLeft->OnDrag([this](wiEventArgs args) {
		XMFLOAT2 scaleDiff;
		scaleDiff.x = (scale.x - args.deltaPos.x) / scale.x;
		scaleDiff.y = (scale.y - args.deltaPos.y) / scale.y;
		this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
		this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
	});
	gui->AddWidget(resizeDragger_UpperLeft);
	resizeDragger_UpperLeft->attachTo(this);

	// Add a resizer control to the bottom right corner
	resizeDragger_BottomRight = new wiButton(name + "_resize_dragger_bottom_right");
	resizeDragger_BottomRight->SetText("");
	resizeDragger_BottomRight->SetSize(XMFLOAT2(controlSize, controlSize));
	resizeDragger_BottomRight->SetPos(XMFLOAT2(translation.x + scale.x - controlSize, translation.y + scale.y - controlSize));
	resizeDragger_BottomRight->OnDrag([this](wiEventArgs args) {
		XMFLOAT2 scaleDiff;
		scaleDiff.x = (scale.x + args.deltaPos.x) / scale.x;
		scaleDiff.y = (scale.y + args.deltaPos.y) / scale.y;
		this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
	});
	gui->AddWidget(resizeDragger_BottomRight);
	resizeDragger_BottomRight->attachTo(this);


	SetEnabled(true);
	SetVisible(true);
	SetMinimized(false);
}
wiWindow::~wiWindow()
{
	SAFE_DELETE(closeButton);
	SAFE_DELETE(moveDragger);
	SAFE_DELETE(resizeDragger_BottomRight);
	SAFE_DELETE(resizeDragger_UpperLeft);
}
void wiWindow::AddWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	widget->SetEnabled(this->IsEnabled());
	widget->SetVisible(this->IsVisible());
	gui->AddWidget(widget);
	widget->attachTo(this);

	childrenWidgets.push_back(widget);
}
void wiWindow::RemoveWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	gui->RemoveWidget(widget);
	widget->detach();

	childrenWidgets.remove(widget);
}
void wiWindow::RemoveWidgets(bool alsoDelete)
{
	assert(gui != nullptr && "Ivalid GUI!");

	for (auto& x : childrenWidgets)
	{
		x->detach();
		gui->RemoveWidget(x);
		if (alsoDelete)
		{
			SAFE_DELETE(x);
		}
	}

	childrenWidgets.clear();
}
void wiWindow::Update(wiGUI* gui)
{
	wiWidget::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	for (auto& x : childrenWidgets)
	{
		x->SetScissorRect(scissorRect);
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

	// body
	if (!IsMinimized())
	{
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
			, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());
	}


	scissorRect.bottom = (LONG)(translation.y + scale.y);
	scissorRect.left = (LONG)(translation.x);
	scissorRect.right = (LONG)(translation.x + scale.x);
	scissorRect.top = (LONG)(translation.y);
	wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps(translation.x, translation.y, moveDragger->scale.y, WIFALIGN_LEFT, WIFALIGN_TOP)).Draw(gui->GetGraphicsThread(),true);
}
void wiWindow::SetVisible(bool value)
{
	wiWidget::SetVisible(value);
	SetMinimized(!value);
	if (closeButton != nullptr)
	{
		closeButton->SetVisible(value);
	}
	if (minimizeButton != nullptr)
	{
		minimizeButton->SetVisible(value);
	}
	if (moveDragger != nullptr)
	{
		moveDragger->SetVisible(value);
	}
	if (resizeDragger_BottomRight != nullptr)
	{
		resizeDragger_BottomRight->SetVisible(value);
	}
	if (resizeDragger_UpperLeft != nullptr)
	{
		resizeDragger_UpperLeft->SetVisible(value);
	}
	for (auto& x : childrenWidgets)
	{
		x->SetVisible(value);
	}
}
void wiWindow::SetEnabled(bool value)
{
	wiWidget::SetEnabled(value);
	for (auto& x : childrenWidgets)
	{
		x->SetEnabled(value);
	}
}
void wiWindow::SetMinimized(bool value)
{
	minimized = value;

	if (resizeDragger_BottomRight != nullptr)
	{
		resizeDragger_BottomRight->SetVisible(!value);
	}
	for (auto& x : childrenWidgets)
	{
		x->SetVisible(!value);
	}
}
bool wiWindow::IsMinimized()
{
	return minimized;
}




wiColorPicker::wiColorPicker(wiGUI* gui, const string& name) :wiWindow(gui, name)
{
	SetSize(XMFLOAT2(300, 300));
	SetColor(wiColor::Ghost);
	RemoveWidget(resizeDragger_BottomRight);
	RemoveWidget(resizeDragger_UpperLeft);

	hue_picker = XMFLOAT2(0, 0);
	saturation_picker = XMFLOAT2(0, 0);
	saturation_picker_barycentric = XMFLOAT3(0, 1, 0);
	hue_color = XMFLOAT4(1, 0, 0, 1);
	final_color = XMFLOAT4(1, 1, 1, 1);
	angle = 0;
}
wiColorPicker::~wiColorPicker()
{
}
static const float __colorpicker_center = 120;
static const float __colorpicker_radius_triangle = 75;
static const float __colorpicker_radius = 80;
static const float __colorpicker_width = 16;
void wiColorPicker::Update(wiGUI* gui)
{
	wiWindow::Update(gui);

	if (!IsEnabled())
	{
		return;
	}

	//if (!gui->IsWidgetDisabled(this))
	//{
	//	return;
	//}

	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	XMFLOAT2 center = XMFLOAT2(translation.x + __colorpicker_center, translation.y + __colorpicker_center);
	XMFLOAT4 pointer4 = wiInputManager::GetInstance()->getpointer();
	XMFLOAT2 pointer = XMFLOAT2(pointer4.x, pointer4.y);
	float distance = wiMath::Distance(center, pointer);
	bool hover_hue = (distance > __colorpicker_radius) && (distance < __colorpicker_radius + __colorpicker_width);

	float distTri = 0;
	XMFLOAT4 A, B, C;
	wiMath::ConstructTriangleEquilateral(__colorpicker_radius_triangle, A, B, C);
	XMVECTOR _A = XMLoadFloat4(&A);
	XMVECTOR _B = XMLoadFloat4(&B);
	XMVECTOR _C = XMLoadFloat4(&C);
	XMMATRIX _triTransform = XMMatrixRotationZ(-angle) * XMMatrixTranslation(center.x, center.y, 0);
	_A = XMVector4Transform(_A, _triTransform);
	_B = XMVector4Transform(_B, _triTransform);
	_C = XMVector4Transform(_C, _triTransform);
	XMVECTOR O = XMVectorSet(pointer.x, pointer.y, 0, 0);
	XMVECTOR D = XMVectorSet(0, 0, 1, 0);
	bool hover_saturation = TriangleTests::Intersects(O, D, _A, _B, _C, distTri);

	if (hover_hue && state==IDLE)
	{
		state = FOCUS;
		huefocus = true;
	}
	else if (hover_saturation && state == IDLE)
	{
		state = FOCUS;
		huefocus = false;
	}
	else if (state == IDLE)
	{
		huefocus = false;
	}

	bool dragged = false;

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == ACTIVE)
		{
			// continue drag if already grabbed wheter it is intersecting or not
			dragged = true;
		}
	}

	dragged = dragged && !gui->IsWidgetDisabled(this);
	if (huefocus && dragged)
	{
		//hue pick
		angle = wiMath::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(__colorpicker_radius, 0));
		XMFLOAT3 color3 = wiMath::HueToRGB(angle / XM_2PI);
		hue_color = XMFLOAT4(color3.x, color3.y, color3.z, 1);
		gui->ActivateWidget(this);
	}
	else if (!huefocus && dragged)
	{
		// saturation pick
		wiMath::GetBarycentric(O, _A, _B, _C, saturation_picker_barycentric.x, saturation_picker_barycentric.y, saturation_picker_barycentric.z, true);
		gui->ActivateWidget(this);
	}
	else if(state != IDLE)
	{
		gui->DeactivateWidget(this);
	}

	float r = __colorpicker_radius + __colorpicker_width*0.5f;
	hue_picker = XMFLOAT2(center.x + r*cos(angle), center.y + r*-sin(angle));
	XMStoreFloat2(&saturation_picker, _A*saturation_picker_barycentric.x + _B*saturation_picker_barycentric.y + _C*saturation_picker_barycentric.z);
	XMStoreFloat4(&final_color, XMLoadFloat4(&hue_color)*saturation_picker_barycentric.x + XMVectorSet(1, 1, 1, 1)*saturation_picker_barycentric.y + XMVectorSet(0, 0, 0, 1)*saturation_picker_barycentric.z);

	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointer;
		args.fValue = angle;
		args.color = final_color;
		onColorChanged(args);
	}
}
void wiColorPicker::Render(wiGUI* gui)
{
	wiWindow::Render(gui);


	if (!IsVisible() || IsMinimized())
	{
		return;
	}
	GRAPHICSTHREAD threadID = gui->GetGraphicsThread();

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
	static wiGraphicsTypes::GPUBuffer vb_saturation;
	static wiGraphicsTypes::GPUBuffer vb_hue;
	static wiGraphicsTypes::GPUBuffer vb_picker;
	static wiGraphicsTypes::GPUBuffer vb_preview;

	static bool buffersComplete = false;
	if (!buffersComplete)
	{
		HRESULT hr = S_OK;
		// saturation
		{
			static vector<Vertex> vertices(0);
			if (vb_saturation.IsValid() && !vertices.empty())
			{
				vertices[0].col = hue_color;
				wiRenderer::GetDevice()->UpdateBuffer(&vb_saturation, vertices.data(), threadID, vb_saturation.GetDesc().ByteWidth);
			}
			else
			{
				vertices.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,0,0,1) });	// hue
				vertices.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,1,1,1) });	// white
				vertices.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(0,0,0,1) });	// black
				wiMath::ConstructTriangleEquilateral(__colorpicker_radius_triangle, vertices[0].pos, vertices[1].pos, vertices[2].pos);

				GPUBufferDesc desc = { 0 };
				desc.BindFlags = BIND_VERTEX_BUFFER;
				desc.ByteWidth = vertices.size() * sizeof(Vertex);
				desc.CPUAccessFlags = CPU_ACCESS_WRITE;
				desc.MiscFlags = 0;
				desc.StructureByteStride = 0;
				desc.Usage = USAGE_DYNAMIC;
				SubresourceData data = { 0 };
				data.pSysMem = vertices.data();
				hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_saturation);
			}
		}
		// hue
		{
			vector<Vertex> vertices(0);
			for (float i = 0; i <= 100; i += 1.0f)
			{
				float p = i / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				XMFLOAT3 rgb = wiMath::HueToRGB(p);
				vertices.push_back({ XMFLOAT4(__colorpicker_radius * x, __colorpicker_radius * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
				vertices.push_back({ XMFLOAT4((__colorpicker_radius + __colorpicker_width) * x, (__colorpicker_radius + __colorpicker_width) * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
			}

			GPUBufferDesc desc = { 0 };
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = vertices.size() * sizeof(Vertex);
			desc.CPUAccessFlags = CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_DYNAMIC;
			SubresourceData data = { 0 };
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_hue);
		}
		// picker
		{
			float _radius = 3;
			float _width = 3;
			vector<Vertex> vertices(0);
			for (float i = 0; i <= 100; i += 1.0f)
			{
				float p = i / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				XMFLOAT3 rgb = wiMath::HueToRGB(p);
				vertices.push_back({ XMFLOAT4(_radius * x, _radius * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
				vertices.push_back({ XMFLOAT4((_radius + _width) * x, (_radius + _width) * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
			}

			GPUBufferDesc desc = { 0 };
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = vertices.size() * sizeof(Vertex);
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data = { 0 };
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_picker);
		}
		// preview
		{
			float _width = 20;

			vector<Vertex> vertices(0);
			vertices.push_back({ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(-_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });

			GPUBufferDesc desc = { 0 };
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = vertices.size() * sizeof(Vertex);
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data = { 0 };
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_preview);
		}

	}

	XMMATRIX __cam = XMMatrixOrthographicOffCenterLH(0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0, -1, 1);

	wiRenderer::GetDevice()->BindConstantBufferVS(wiRenderer::constantBuffers[CBTYPE_MISC], CBSLOT_RENDERER_MISC, threadID);
	wiRenderer::GetDevice()->BindRasterizerState(wiRenderer::rasterizers[RSTYPE_DOUBLESIDED], threadID);
	wiRenderer::GetDevice()->BindBlendState(wiRenderer::blendStates[BSTYPE_OPAQUE], threadID);
	wiRenderer::GetDevice()->BindDepthStencilState(wiRenderer::depthStencils[DSSTYPE_XRAY], 0, threadID);
	wiRenderer::GetDevice()->BindVertexLayout(wiRenderer::vertexLayouts[VLTYPE_LINE], threadID);
	wiRenderer::GetDevice()->BindVS(wiRenderer::vertexShaders[VSTYPE_LINE], threadID);
	wiRenderer::GetDevice()->BindPS(wiRenderer::pixelShaders[PSTYPE_LINE], threadID);
	wiRenderer::GetDevice()->BindPrimitiveTopology(TRIANGLESTRIP, threadID);

	// render saturation triangle
	wiRenderer::MiscCB cb;
	cb.mTransform = XMMatrixTranspose(
		XMMatrixRotationZ(-angle) *
		XMMatrixTranslation(translation.x + __colorpicker_center, translation.y + __colorpicker_center, 0) *
		__cam
	);
	cb.mColor = XMFLOAT4(1, 1, 1, 1);
	wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(&vb_saturation, 0, sizeof(Vertex), threadID);
	wiRenderer::GetDevice()->Draw(vb_saturation.GetDesc().ByteWidth / sizeof(Vertex), threadID);

	// render hue circle
	cb.mTransform = XMMatrixTranspose(
		XMMatrixTranslation(translation.x + __colorpicker_center, translation.y + __colorpicker_center, 0) *
		__cam
	);
	cb.mColor = XMFLOAT4(1, 1, 1, 1);
	wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(&vb_hue, 0, sizeof(Vertex), threadID);
	wiRenderer::GetDevice()->Draw(vb_hue.GetDesc().ByteWidth / sizeof(Vertex), threadID);

	// render hue picker
	cb.mTransform = XMMatrixTranspose(
		XMMatrixTranslation(hue_picker.x, hue_picker.y, 0) *
		__cam
	);
	cb.mColor = XMFLOAT4(0, 0, 0, 1);
	wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(&vb_picker, 0, sizeof(Vertex), threadID);
	wiRenderer::GetDevice()->Draw(vb_picker.GetDesc().ByteWidth / sizeof(Vertex), threadID);

	// render saturation picker
	cb.mTransform = XMMatrixTranspose(
		XMMatrixTranslation(saturation_picker.x, saturation_picker.y, 0) *
		__cam
	);
	cb.mColor = XMFLOAT4(0, 0, 0, 1);
	wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(&vb_picker, 0, sizeof(Vertex), threadID);
	wiRenderer::GetDevice()->Draw(vb_picker.GetDesc().ByteWidth / sizeof(Vertex), threadID);

	// render preview
	cb.mTransform = XMMatrixTranspose(
		XMMatrixTranslation(translation.x + 260, translation.y + 40, 0) *
		__cam
	);
	cb.mColor = GetColor();
	wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
	wiRenderer::GetDevice()->BindVertexBuffer(&vb_preview, 0, sizeof(Vertex), threadID);
	wiRenderer::GetDevice()->Draw(vb_preview.GetDesc().ByteWidth / sizeof(Vertex), threadID);
	
}
XMFLOAT4 wiColorPicker::GetColor()
{
	return final_color;
}
void wiColorPicker::OnColorChanged(function<void(wiEventArgs args)> func)
{
	onColorChanged = move(func);
}
