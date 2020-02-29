#include "wiWidget.h"
#include "wiGUI.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiMath.h"
#include "wiHelper.h"
#include "wiInput.h"
#include "wiRenderer.h"
#include "ShaderInterop_Renderer.h"

#include <DirectXCollision.h>

#include <sstream>

using namespace std;
using namespace wiGraphics;
using namespace wiScene;


static wiGraphics::PipelineState PSO_colored;

void wiWidget::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));
	hitBox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));

	if (IsEnabled() && GetState() != WIDGETSTATE::ACTIVE && !tooltip.empty() && pointerHitbox.intersects(hitBox))
	{
		tooltipTimer++;
	}
	else
	{
		tooltipTimer = 0;
	}

	UpdateTransform();

	if (parent != nullptr)
	{
		this->UpdateTransform_Parented(*parent, world_parent_bind);
	}

	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
	XMStoreFloat3(&translation, T);
	XMStoreFloat3(&scale, S);

	scissorRect.bottom = (int32_t)(translation.y + scale.y);
	scissorRect.left = (int32_t)(translation.x);
	scissorRect.right = (int32_t)(translation.x + scale.x);
	scissorRect.top = (int32_t)(translation.y);
}
void wiWidget::RenderTooltip(const wiGUI* gui, CommandList cmd) const
{
	if (!IsVisible())
	{
		return;
	}

	assert(gui != nullptr && "Ivalid GUI!");

	if (tooltipTimer > 25)
	{
		XMFLOAT2 tooltipPos = XMFLOAT2(gui->pointerpos.x, gui->pointerpos.y);
		if (tooltipPos.y > wiRenderer::GetDevice()->GetScreenHeight()*0.8f)
		{
			tooltipPos.y -= 30;
		}
		else
		{
			tooltipPos.y += 40;
		}
		wiFontParams fontProps = wiFontParams((int)tooltipPos.x, (int)tooltipPos.y, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_TOP);
		fontProps.color = wiColor(25, 25, 25, 255);
		wiFont tooltipFont = wiFont(tooltip, fontProps);
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(tooltip + "\n" + scriptTip);
		}

		int textWidth = tooltipFont.textWidth();
		if (tooltipPos.x > wiRenderer::GetDevice()->GetScreenWidth() - textWidth)
		{
			tooltipPos.x -= textWidth + 10;
			tooltipFont.params.posX = (int)tooltipPos.x;
		}

		static const float _border = 2;
		float fontWidth = (float)tooltipFont.textWidth() + _border * 2;
		float fontHeight = (float)tooltipFont.textHeight() + _border * 2;
		wiImage::Draw(wiTextureHelper::getWhite(), wiImageParams(tooltipPos.x - _border, tooltipPos.y - _border, fontWidth, fontHeight, wiColor(255, 234, 165).toFloat4()), cmd);
		tooltipFont.SetText(tooltip);
		tooltipFont.Draw(cmd);
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(scriptTip);
			tooltipFont.params.posY += (int)(fontHeight / 2);
			tooltipFont.params.color = wiColor(25, 25, 25, 110);
			tooltipFont.Draw(cmd);
		}
	}
}
const wiHashString& wiWidget::GetName() const
{
	return fastName;
}
void wiWidget::SetName(const std::string& value)
{
	if (value.length() <= 0)
	{
		static unsigned long widgetID = 0;
		stringstream ss("");
		ss << "widget_" << widgetID++;
		fastName = wiHashString(ss.str());
	}
	else
	{
		fastName = wiHashString(value);
	}

}
const string& wiWidget::GetText() const
{
	return text;
}
void wiWidget::SetText(const std::string& value)
{
	text = value;
}
void wiWidget::SetTooltip(const std::string& value)
{
	tooltip = value;
}
void wiWidget::SetScriptTip(const std::string& value)
{
	scriptTip = value;
}
void wiWidget::SetPos(const XMFLOAT2& value)
{
	SetDirty();
	translation_local.x = value.x;
	translation_local.y = value.y;
	UpdateTransform();

	translation = translation_local;
}
void wiWidget::SetSize(const XMFLOAT2& value)
{
	SetDirty();
	scale_local.x = value.x;
	scale_local.y = value.y;
	UpdateTransform();

	scale = scale_local;
}
wiWidget::WIDGETSTATE wiWidget::GetState() const
{
	return state;
}
void wiWidget::SetEnabled(bool val)
{
	enabled = val;
}
bool wiWidget::IsEnabled() const
{
	return enabled && visible;
}
void wiWidget::SetVisible(bool val)
{
	visible = val;
}
bool wiWidget::IsVisible() const
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
void wiWidget::SetColor(wiColor color, WIDGETSTATE state)
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
wiColor wiWidget::GetColor() const
{
	wiColor retVal = colors[GetState()];
	if (!IsEnabled()) {
		retVal = wiColor::lerp(wiColor::Transparent(), retVal, 0.5f);
	}
	return retVal;
}
void wiWidget::LoadShaders()
{
	PipelineStateDesc desc;
	desc.vs = wiRenderer::GetVertexShader(VSTYPE_VERTEXCOLOR);
	desc.ps = wiRenderer::GetPixelShader(PSTYPE_VERTEXCOLOR);
	desc.il = wiRenderer::GetVertexLayout(VLTYPE_VERTEXCOLOR);
	desc.dss = wiRenderer::GetDepthStencilState(DSSTYPE_XRAY);
	desc.bs = wiRenderer::GetBlendState(BSTYPE_TRANSPARENT);
	desc.rs = wiRenderer::GetRasterizerState(RSTYPE_DOUBLESIDED);
	desc.pt = TRIANGLESTRIP;
	wiRenderer::GetDevice()->CreatePipelineState(&desc, &PSO_colored);
}


wiButton::wiButton(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	OnDragStart([](wiEventArgs args) {});
	OnDrag([](wiEventArgs args) {});
	OnDragEnd([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));

	fontParams.h_align = WIFALIGN_CENTER;
	fontParams.v_align = WIFALIGN_CENTER;
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	hitBox.pos.x = translation.x;
	hitBox.pos.y = translation.y;
	hitBox.siz.x = scale.x;
	hitBox.siz.y = scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

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

	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
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
void wiButton::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	ApplyScissor(scissorRect, cmd);

	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, scale.y, color.toFloat4()), cmd);

	wiFontParams fontparams = GetFontParams();
	switch (fontparams.h_align)
	{
	case WIFALIGN_LEFT:
		fontparams.posX = (int)(translation.x + 2);
		break;
	case WIFALIGN_RIGHT:
		fontparams.posX = (int)(translation.x + scale.x - 2);
		break;
	case WIFALIGN_CENTER:
	default:
		fontparams.posX = (int)(translation.x + scale.x * 0.5f);
		break;
	}
	switch (fontparams.v_align)
	{
	case WIFALIGN_TOP:
		fontparams.posY = (int)(translation.y + 2);
		break;
	case WIFALIGN_BOTTOM:
		fontparams.posY = (int)(translation.y + scale.y - 2);
		break;
	case WIFALIGN_CENTER:
	default:
		fontparams.posY = (int)(translation.y + scale.y * 0.5f);
		break;
	}
	wiFont(text, fontparams).Draw(cmd);

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




wiLabel::wiLabel(const std::string& name)
{
	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(100, 20));
}
wiLabel::~wiLabel()
{

}
void wiLabel::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiLabel::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	ApplyScissor(scissorRect, cmd);

	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, scale.y, color.toFloat4()), cmd);

	wiFont(text, wiFontParams((int)translation.x + 2, (int)translation.y + 2, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_TOP, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

}




string wiTextInputField::value_new = "";
wiTextInputField::wiTextInputField(const std::string& name)
{
	SetName(name);
	SetText(fastName.GetString());
	OnInputAccepted([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));
}
wiTextInputField::~wiTextInputField()
{

}
void wiTextInputField::SetValue(const std::string& newValue)
{
	value = newValue;
}
void wiTextInputField::SetValue(int newValue)
{
	stringstream ss("");
	ss << newValue;
	value = ss.str();
}
void wiTextInputField::SetValue(float newValue)
{
	stringstream ss("");
	ss << newValue;
	value = ss.str();
}
const std::string& wiTextInputField::GetValue()
{
	return value;
}
void wiTextInputField::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	hitBox.pos.x = translation.x;
	hitBox.pos.y = translation.y;
	hitBox.siz.x = scale.x;
	hitBox.siz.y = scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));
	bool intersectsPointer = pointerHitbox.intersects(hitBox);

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	bool clicked = false;
	// hover the button
	if (intersectsPointer)
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	if (clicked)
	{
		gui->ActivateWidget(this);

		value_new = value;
	}

	if (state == ACTIVE)
	{
		if (wiInput::Press(wiInput::KEYBOARD_BUTTON_ENTER))
		{
			// accept input...

			value = value_new;
			value_new.clear();

			wiEventArgs args;
			args.sValue = value;
			args.iValue = atoi(value.c_str());
			args.fValue = (float)atof(value.c_str());
			onInputAccepted(args);

			gui->DeactivateWidget(this);
		}
		else if ((wiInput::Press(wiInput::MOUSE_BUTTON_LEFT) && !intersectsPointer) ||
			wiInput::Press(wiInput::KEYBOARD_BUTTON_ESCAPE))
		{
			// cancel input 
			value_new.clear();
			gui->DeactivateWidget(this);
		}

	}
}
void wiTextInputField::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	wiFont(description, wiFontParams((int)(translation.x), (int)(translation.y + scale.y * 0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_RIGHT, WIFALIGN_CENTER, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, scale.y, color.toFloat4()), cmd);

	string activeText = text;
	if (state == ACTIVE)
	{
		activeText = value_new;
	}
	else if (!value.empty())
	{
		activeText = value;
	}
	wiFont(activeText, wiFontParams((int)(translation.x + 2), (int)(translation.y + scale.y*0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_CENTER, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

}
void wiTextInputField::OnInputAccepted(function<void(wiEventArgs args)> func)
{
	onInputAccepted = move(func);
}
void wiTextInputField::AddInput(const char inputChar)
{
	value_new.push_back(inputChar);
}
void wiTextInputField::DeleteFromInput()
{
	if (!value_new.empty())
	{
		value_new.pop_back();
	}
}





wiSlider::wiSlider(float start, float end, float defaultValue, float step, const std::string& name) : start(start), end(end), value(defaultValue), step(std::max(step, 1.0f))
{
	SetName(name);
	SetText(fastName.GetString());
	OnSlide([](wiEventArgs args) {});
	SetSize(XMFLOAT2(200, 40));

	valueInputField = new wiTextInputField(name + "_endInputField");
	valueInputField->SetTooltip("Enter number to modify value even outside slider limits. Enter \"reset\" to reset slider to initial state.");
	valueInputField->SetSize(XMFLOAT2(scale.y * 2, scale.y));
	valueInputField->SetPos(XMFLOAT2(scale.x + 20, 0));
	valueInputField->SetValue(end);
	valueInputField->OnInputAccepted([this, start,end,defaultValue](wiEventArgs args) {
		if (args.sValue.compare("reset") != string::npos)
		{
			this->value = defaultValue;
			this->start = start;
			this->end = end;
			args.fValue = this->value;
			args.iValue = (int)this->value;
		}
		else
		{
			this->value = args.fValue;
			this->start = std::min(this->start, args.fValue);
			this->end = std::max(this->end, args.fValue);
		}
		onSlide(args);
	});
	valueInputField->AttachTo(this);
}
wiSlider::~wiSlider()
{
	SAFE_DELETE(valueInputField);
}
void wiSlider::SetValue(float value)
{
	this->value = value;
}
float wiSlider::GetValue()
{
	return value;
}
void wiSlider::SetRange(float start, float end)
{
	this->start = start;
	this->end = end;
	this->value = wiMath::Clamp(this->value, start, end);
}
void wiSlider::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	scissorRect.bottom = (int32_t)(translation.y + scale.y);
	scissorRect.left = (int32_t)(translation.x);
	scissorRect.right = (int32_t)(translation.x + scale.x + 20 + scale.y * 2); // include the valueInputField
	scissorRect.top = (int32_t)(translation.y);

	for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
	{
		valueInputField->SetColor(this->colors[i], (WIDGETSTATE)i);
	}
	valueInputField->SetTextColor(this->fontParams.color);
	valueInputField->SetTextShadowColor(this->fontParams.shadowColor);
	valueInputField->SetEnabled(IsEnabled());
	valueInputField->Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	bool dragged = false;

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
		if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
		{
			if (state == ACTIVE)
			{
				// continue drag if already grabbed wheter it is intersecting or not
				dragged = true;
			}
		}
		else
		{
			gui->DeactivateWidget(this);
		}
	}

	hitBox.pos.x = translation.x - headWidth * 0.5f;
	hitBox.pos.y = translation.y;
	hitBox.siz.x = scale.x + headWidth;
	hitBox.siz.y = scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));


	if (pointerHitbox.intersects(hitBox))
	{
		// hover the slider
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == FOCUS)
		{
			// activate
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
		args.iValue = (int)value;
		onSlide(args);
		gui->ActivateWidget(this);
	}

	valueInputField->SetValue(value);
}
void wiSlider::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	// text
	wiFont(text, wiFontParams((int)(translation.x), (int)(translation.y + scale.y * 0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_RIGHT, WIFALIGN_CENTER, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	// trail
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, scale.y, colors_base[state].toFloat4()), cmd);
	// head
	float headPosX = wiMath::Lerp(translation.x + headWidth * 0.5f + 2, translation.x + scale.x - headWidth * 0.5f - 2, wiMath::Clamp(wiMath::InverseLerp(start, end, value), 0, 1));
	wiImageParams fx(headPosX, translation.y + 2, headWidth, scale.y - 4, color.toFloat4());
	fx.pivot = XMFLOAT2(0.5f, 0);
	wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

	valueInputField->Render(gui, cmd);
}
void wiSlider::RenderTooltip(const wiGUI* gui, wiGraphics::CommandList cmd) const
{
	wiWidget::RenderTooltip(gui, cmd);
	valueInputField->RenderTooltip(gui, cmd);
}
void wiSlider::SetBaseColor(wiColor color, WIDGETSTATE state)
{
	if (state == WIDGETSTATE_COUNT)
	{
		for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
		{
			colors_base[i] = color;
		}
	}
	else
	{
		colors_base[state] = color;
	}
}
void wiSlider::OnSlide(function<void(wiEventArgs args)> func)
{
	onSlide = move(func);
}





wiCheckBox::wiCheckBox(const std::string& name)
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	SetSize(XMFLOAT2(20, 20));
}
wiCheckBox::~wiCheckBox()
{

}
void wiCheckBox::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

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

	hitBox.pos.x = translation.x;
	hitBox.pos.y = translation.y;
	hitBox.siz.x = scale.x;
	hitBox.siz.y = scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
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
void wiCheckBox::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	wiFont(text, wiFontParams((int)(translation.x), (int)(translation.y + scale.y * 0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_RIGHT, WIFALIGN_CENTER, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	// control
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, scale.y, color.toFloat4()), cmd);

	// check
	if (GetCheck())
	{
		wiImage::Draw(wiTextureHelper::getWhite()
			, wiImageParams(translation.x + scale.x*0.25f, translation.y + scale.y*0.25f, scale.x*0.5f, scale.y*0.5f, wiColor::lerp(color, wiColor::White(), 0.8f).toFloat4())
			, cmd);
	}

}
void wiCheckBox::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}
void wiCheckBox::SetCheck(bool value)
{
	checked = value;
}
bool wiCheckBox::GetCheck() const
{
	return checked;
}





wiComboBox::wiComboBox(const std::string& name)
{
	SetName(name);
	SetText(fastName.GetString());
	OnSelect([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 20));
}
wiComboBox::~wiComboBox()
{

}
float wiComboBox::GetItemOffset(int index) const
{
	index = std::max(firstItemVisible, index) - firstItemVisible;
	return scale.y * (index + 1) + 1;
}
bool wiComboBox::HasScrollbar() const
{
	return maxVisibleItemCount < (int)items.size();
}
void wiComboBox::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

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
	if (state == ACTIVE && combostate == COMBOSTATE_SELECTING)
	{
		hovered = -1;
		gui->DeactivateWidget(this);
	}
	if (state == IDLE && combostate == COMBOSTATE_SELECTING)
	{
		combostate = COMBOSTATE_INACTIVE;
	}

	hitBox.pos.x = translation.x;
	hitBox.pos.y = translation.y;
	hitBox.siz.x = scale.x + scale.y + 1; // + drop-down indicator arrow + little offset
	hitBox.siz.y = scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		// activate
		clicked = true;
	}

	bool click_down = false;
	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
	{
		click_down = true;
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}


	if (clicked && state == FOCUS)
	{
		gui->ActivateWidget(this);
	}

	const float scrollbar_begin = translation.y + scale.y + 1 + scale.y * 0.5f;
	const float scrollbar_end = scrollbar_begin + std::max(0.0f, (float)std::min(maxVisibleItemCount, (int)items.size()) - 1) * scale.y;

	if (state == ACTIVE)
	{
		if (HasScrollbar())
		{
			if (combostate != COMBOSTATE_SELECTING && combostate != COMBOSTATE_INACTIVE)
			{
				if (combostate == COMBOSTATE_SCROLLBAR_GRABBED || pointerHitbox.intersects(Hitbox2D(XMFLOAT2(translation.x + scale.x + 1, translation.y + scale.y + 1), XMFLOAT2(scale.y, (float)std::min(maxVisibleItemCount, (int)items.size()) * scale.y))))
				{
					if (click_down)
					{
						combostate = COMBOSTATE_SCROLLBAR_GRABBED;
						scrollbar_delta = wiMath::Clamp(pointerHitbox.pos.y, scrollbar_begin, scrollbar_end) - scrollbar_begin;
						const float scrollbar_value = wiMath::InverseLerp(scrollbar_begin, scrollbar_end, scrollbar_begin + scrollbar_delta);
						firstItemVisible = int(float(std::max(0, (int)items.size() - maxVisibleItemCount)) * scrollbar_value + 0.5f);
						firstItemVisible = std::max(0, std::min((int)items.size() - maxVisibleItemCount, firstItemVisible));
					}
					else
					{
						combostate = COMBOSTATE_SCROLLBAR_HOVER;
					}
				}
				else if (!click_down)
				{
					combostate = COMBOSTATE_HOVER;
				}
			}
		}

		if (combostate == COMBOSTATE_INACTIVE)
		{
			combostate = COMBOSTATE_HOVER;
		}
		else if (combostate == COMBOSTATE_SELECTING)
		{
			gui->DeactivateWidget(this);
			combostate = COMBOSTATE_INACTIVE;
		}
		else if (combostate == COMBOSTATE_HOVER)
		{
			int scroll = (int)wiInput::GetPointer().z;
			firstItemVisible -= scroll;
			firstItemVisible = std::max(0, std::min((int)items.size() - maxVisibleItemCount, firstItemVisible));
			if (scroll)
			{
				const float scrollbar_value = wiMath::InverseLerp(0, float(std::max(0, (int)items.size() - maxVisibleItemCount)), float(firstItemVisible));
				scrollbar_delta = wiMath::Lerp(scrollbar_begin, scrollbar_end, scrollbar_value) - scrollbar_begin;
			}

			hovered = -1;
			for (int i = firstItemVisible; i < (firstItemVisible + std::min(maxVisibleItemCount, (int)items.size())); ++i)
			{
				Hitbox2D itembox;
				itembox.pos.x = translation.x;
				itembox.pos.y = translation.y + GetItemOffset(i);
				itembox.siz.x = scale.x;
				itembox.siz.y = scale.y;
				if (pointerHitbox.intersects(itembox))
				{
					hovered = i;
					break;
				}
			}

			if (clicked)
			{
				combostate = COMBOSTATE_SELECTING;
				if (hovered >= 0)
				{
					SetSelected(hovered);
				}
			}
		}
	}

}
void wiComboBox::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiColor color = GetColor();
	if (combostate != COMBOSTATE_INACTIVE)
	{
		color = colors[FOCUS];
	}

	wiFont(text, wiFontParams((int)(translation.x), (int)(translation.y + scale.y * 0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_RIGHT, WIFALIGN_CENTER, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
	static GPUBuffer vb_triangle;
	if (!vb_triangle.IsValid())
	{
		Vertex vertices[3];
		vertices[0].col = XMFLOAT4(1, 1, 1, 1);
		vertices[1].col = XMFLOAT4(1, 1, 1, 1);
		vertices[2].col = XMFLOAT4(1, 1, 1, 1);
		wiMath::ConstructTriangleEquilateral(1, vertices[0].pos, vertices[1].pos, vertices[2].pos);

		GPUBufferDesc desc;
		desc.BindFlags = BIND_VERTEX_BUFFER;
		desc.ByteWidth = sizeof(vertices);
		SubresourceData initdata;
		initdata.pSysMem = vertices;
		device->CreateBuffer(&desc, &initdata, &vb_triangle);
	}
	const XMMATRIX Projection = device->GetScreenProjection();

	// control-arrow-background
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x + scale.x + 1, translation.y, scale.y, scale.y, color.toFloat4()), cmd);

	// control-arrow-triangle
	{
		device->BindPipelineState(&PSO_colored, cmd);

		MiscCB cb;
		cb.g_xColor = colors[ACTIVE].toFloat4();
		XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(scale.y * 0.25f, scale.y * 0.25f, 1) *
			XMMatrixRotationZ(XM_PIDIV2) *
			XMMatrixTranslation(translation.x + scale.x + 1 + scale.y * 0.5f, translation.y + scale.y * 0.5f, 0) *
			Projection
		);
		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CBSLOT_RENDERER_MISC, cmd);
		const GPUBuffer* vbs[] = {
			&vb_triangle,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);

		device->Draw(3, 0, cmd);
	}

	ApplyScissor(scissorRect, cmd);

	// control-base
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, scale.y, color.toFloat4()), cmd);

	if (selected >= 0)
	{
		wiFont(items[selected], wiFontParams((int)(translation.x + scale.x*0.5f), (int)(translation.y + scale.y*0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_CENTER, WIFALIGN_CENTER, 0, 0,
			fontParams.color, fontParams.shadowColor)).Draw(cmd);
	}

	// drop-down
	if (state == ACTIVE)
	{
		if (HasScrollbar())
		{
			Rect rect;
			rect.left = int(translation.x + scale.x + 1);
			rect.right = int(translation.x + scale.x + 1 + scale.y);
			rect.top = int(translation.y + scale.y + 1);
			rect.bottom = int(translation.y + scale.y + 1 + scale.y * maxVisibleItemCount);
			ApplyScissor(rect, cmd, false);

			// control-scrollbar-base
			{
				wiColor col = colors[IDLE];
				col.setA(col.getA() / 2);
				wiImage::Draw(wiTextureHelper::getWhite()
					, wiImageParams(translation.x + scale.x + 1, translation.y + scale.y + 1, scale.y, scale.y * maxVisibleItemCount, col.toFloat4()), cmd);
			}

			// control-scrollbar-grab
			{
				wiColor col = colors[IDLE];
				if (combostate == COMBOSTATE_SCROLLBAR_HOVER)
				{
					col = colors[FOCUS];
				}
				else if (combostate == COMBOSTATE_SCROLLBAR_GRABBED)
				{
					col = colors[ACTIVE];
				}
				wiImage::Draw(wiTextureHelper::getWhite()
					, wiImageParams(translation.x + scale.x + 1, translation.y + scale.y + 1 + scrollbar_delta, scale.y, scale.y, col.toFloat4()), cmd);
			}
		}

		Rect rect;
		rect.left = int(translation.x);
		rect.right = rect.left + int(scale.x);
		rect.top = int(translation.y + scale.y + 1);
		rect.bottom = rect.top + int(scale.y * maxVisibleItemCount);
		ApplyScissor(rect, cmd, false);

		// control-list
		for (int i = firstItemVisible; i < (firstItemVisible + std::min(maxVisibleItemCount, (int)items.size())); ++i)
		{
			wiColor col = colors[IDLE];
			if (hovered == i)
			{
				if (combostate == COMBOSTATE_HOVER)
				{
					col = colors[FOCUS];
				}
				else if (combostate == COMBOSTATE_SELECTING)
				{
					col = colors[ACTIVE];
				}
			}
			wiImage::Draw(wiTextureHelper::getWhite()
				, wiImageParams(translation.x, translation.y + GetItemOffset(i), scale.x, scale.y, col.toFloat4()), cmd);
			wiFont(items[i], wiFontParams((int)(translation.x + scale.x*0.5f), (int)(translation.y + scale.y*0.5f + GetItemOffset(i)), WIFONTSIZE_DEFAULT, WIFALIGN_CENTER, WIFALIGN_CENTER, 0, 0,
				fontParams.color, fontParams.shadowColor)).Draw(cmd);
		}
	}
}
void wiComboBox::OnSelect(function<void(wiEventArgs args)> func)
{
	onSelect = move(func);
}
void wiComboBox::AddItem(const std::string& item)
{
	items.push_back(item);

	if (selected < 0)
	{
		selected = 0;
	}
}
void wiComboBox::RemoveItem(int index)
{
	std::vector<string> newItems(0);
	newItems.reserve(items.size());
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i != index)
		{
			newItems.push_back(items[i]);
		}
	}
	items = newItems;

	if (items.empty())
	{
		selected = -1;
	}
	else if (selected > index)
	{
		selected--;
	}
}
void wiComboBox::ClearItems()
{
	items.clear();

	selected = -1;
}
void wiComboBox::SetMaxVisibleItemCount(int value)
{
	maxVisibleItemCount = value;
}
void wiComboBox::SetSelected(int index)
{
	selected = index;

	wiEventArgs args;
	args.iValue = selected;
	args.sValue = GetItemText(selected);
	onSelect(args);
}
string wiComboBox::GetItemText(int index) const
{
	if (index >= 0)
	{
		return items[index];
	}
	return "";
}
int wiComboBox::GetSelected() const
{
	return selected;
}






static const float windowcontrolSize = 20.0f;
wiWindow::wiWindow(wiGUI* gui, const std::string& name, bool window_controls) : gui(gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(640, 480));

	// Add controls
	if (window_controls)
	{
		// Add a grabber onto the title bar
		moveDragger = new wiButton(name + "_move_dragger");
		moveDragger->SetText(name);
		wiFontParams fontparams = moveDragger->GetFontParams();
		fontparams.h_align = WIFALIGN_LEFT;
		moveDragger->SetFontParams(fontparams);
		moveDragger->SetSize(XMFLOAT2(scale.x - windowcontrolSize * 3, windowcontrolSize));
		moveDragger->SetPos(XMFLOAT2(windowcontrolSize, 0));
		moveDragger->OnDrag([this, gui](wiEventArgs args) {
			this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
			this->wiWidget::Update(gui, 0);
			for (auto& x : this->childrenWidgets)
			{
				x->wiWidget::Update(gui, 0);
			}
			});
		AddWidget(moveDragger);

		// Add close button to the top right corner
		closeButton = new wiButton(name + "_close_button");
		closeButton->SetText("x");
		closeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		closeButton->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y));
		closeButton->OnClick([this](wiEventArgs args) {
			this->SetVisible(false);
			});
		closeButton->SetTooltip("Close window");
		AddWidget(closeButton);

		// Add minimize button to the top right corner
		minimizeButton = new wiButton(name + "_minimize_button");
		minimizeButton->SetText("-");
		minimizeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		minimizeButton->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize * 2, translation.y));
		minimizeButton->OnClick([this](wiEventArgs args) {
			this->SetMinimized(!this->IsMinimized());
			});
		minimizeButton->SetTooltip("Minimize window");
		AddWidget(minimizeButton);

		// Add a resizer control to the upperleft corner
		resizeDragger_UpperLeft = new wiButton(name + "_resize_dragger_upper_left");
		resizeDragger_UpperLeft->SetText("");
		resizeDragger_UpperLeft->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		resizeDragger_UpperLeft->SetPos(XMFLOAT2(0, 0));
		resizeDragger_UpperLeft->OnDrag([this, gui](wiEventArgs args) {
			XMFLOAT2 scaleDiff;
			scaleDiff.x = (scale.x - args.deltaPos.x) / scale.x;
			scaleDiff.y = (scale.y - args.deltaPos.y) / scale.y;
			this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
			this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
			this->wiWidget::Update(gui, 0);
			for (auto& x : this->childrenWidgets)
			{
				x->wiWidget::Update(gui, 0);
			}
			});
		AddWidget(resizeDragger_UpperLeft);

		// Add a resizer control to the bottom right corner
		resizeDragger_BottomRight = new wiButton(name + "_resize_dragger_bottom_right");
		resizeDragger_BottomRight->SetText("");
		resizeDragger_BottomRight->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		resizeDragger_BottomRight->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y + scale.y - windowcontrolSize));
		resizeDragger_BottomRight->OnDrag([this, gui](wiEventArgs args) {
			XMFLOAT2 scaleDiff;
			scaleDiff.x = (scale.x + args.deltaPos.x) / scale.x;
			scaleDiff.y = (scale.y + args.deltaPos.y) / scale.y;
			this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
			this->wiWidget::Update(gui, 0);
			for (auto& x : this->childrenWidgets)
			{
				x->wiWidget::Update(gui, 0);
			}
			});
		AddWidget(resizeDragger_BottomRight);
	}


	SetEnabled(true);
	SetVisible(true);
	SetMinimized(false);
}
wiWindow::~wiWindow()
{
	RemoveWidgets(true);
}
void wiWindow::AddWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	widget->SetEnabled(this->IsEnabled());
	widget->SetVisible(this->IsVisible());
	gui->AddWidget(widget);
	widget->AttachTo(this);

	childrenWidgets.push_back(widget);
}
void wiWindow::RemoveWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	gui->RemoveWidget(widget);
	//widget->detach();

	childrenWidgets.remove(widget);
}
void wiWindow::RemoveWidgets(bool alsoDelete)
{
	assert(gui != nullptr && "Ivalid GUI!");

	for (auto& x : childrenWidgets)
	{
		//x->detach();
		gui->RemoveWidget(x);
		if (alsoDelete)
		{
			SAFE_DELETE(x);
		}
	}

	childrenWidgets.clear();
}
void wiWindow::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	for (auto& x : childrenWidgets)
	{
		x->Update(gui, dt);
	}

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiWindow::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	ApplyScissor(scissorRect, cmd);

	// body
	if (!IsMinimized())
	{
		wiImage::Draw(wiTextureHelper::getWhite()
			, wiImageParams(translation.x, translation.y, scale.x, scale.y, color.toFloat4()), cmd);
	}

	for (auto& x : childrenWidgets)
	{
		if (x != gui->GetActiveWidget())
		{
			// the gui will render the active on on top of everything!
			ApplyScissor(scissorRect, cmd);
			x->Render(gui, cmd);
		}
	}

}
void wiWindow::SetVisible(bool value)
{
	wiWidget::SetVisible(value);
	SetMinimized(!value);
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
		if (x == moveDragger)
			continue;
		if (x == minimizeButton)
			continue;
		if (x == closeButton)
			continue;
		if (x == resizeDragger_UpperLeft)
			continue;
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
		if (x == moveDragger)
			continue;
		if (x == minimizeButton)
			continue;
		if (x == closeButton)
			continue;
		if (x == resizeDragger_UpperLeft)
			continue;
		x->SetVisible(!value);
	}
}
bool wiWindow::IsMinimized() const
{
	return minimized;
}




struct rgb {
	float r;       // a fraction between 0 and 1
	float g;       // a fraction between 0 and 1
	float b;       // a fraction between 0 and 1
};
struct hsv {
	float h;       // angle in degrees
	float s;       // a fraction between 0 and 1
	float v;       // a fraction between 0 and 1
};
hsv rgb2hsv(rgb in)
{
	hsv         out;
	float		min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max > in.b ? max : in.b;

	out.v = max;                                // v
	delta = max - min;
	if (delta < 0.00001f)
	{
		out.s = 0;
		out.h = 0; // undefined, maybe nan?
		return out;
	}
	if (max > 0.0f) { // NOTE: if Max is == 0, this divide would cause a crash
		out.s = (delta / max);                  // s
	}
	else {
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		out.s = 0.0f;
		out.h = NAN;                            // its now undefined
		return out;
	}
	if (in.r >= max)                           // > is bogus, just keeps compilor happy
		out.h = (in.g - in.b) / delta;        // between yellow & magenta
	else
		if (in.g >= max)
			out.h = 2.0f + (in.b - in.r) / delta;  // between cyan & yellow
		else
			out.h = 4.0f + (in.r - in.g) / delta;  // between magenta & cyan

	out.h *= 60.0f;                              // degrees

	if (out.h < 0.0f)
		out.h += 360.0f;

	return out;
}
rgb hsv2rgb(hsv in)
{
	float		hh, p, q, t, ff;
	long        i;
	rgb         out;

	if (in.s <= 0.0f) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if (hh >= 360.0f) hh = 0.0f;
	hh /= 60.0f;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0f - in.s);
	q = in.v * (1.0f - (in.s * ff));
	t = in.v * (1.0f - (in.s * (1.0f - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}

wiColorPicker::wiColorPicker(wiGUI* gui, const std::string& name, bool window_controls) :wiWindow(gui, name, window_controls)
{
	SetSize(XMFLOAT2(300, 260));
	SetColor(wiColor::Ghost());

	if (resizeDragger_BottomRight != nullptr)
	{
		RemoveWidget(resizeDragger_BottomRight);
	}
	if (resizeDragger_UpperLeft != nullptr)
	{
		RemoveWidget(resizeDragger_UpperLeft);
	}

	float x = 250;
	float y = 110;
	float step = 20;

	text_R = new wiTextInputField("");
	text_R->SetPos(XMFLOAT2(x, y += step));
	text_R->SetSize(XMFLOAT2(40, 18));
	text_R->SetText("");
	text_R->SetTooltip("Enter value for RED channel (0-255)");
	text_R->SetDescription("R: ");
	text_R->OnInputAccepted([this](wiEventArgs args) {
		wiColor color = GetPickColor();
		color.setR((uint8_t)args.iValue);
		SetPickColor(color);
		FireEvents();
		});
	AddWidget(text_R);

	text_G = new wiTextInputField("");
	text_G->SetPos(XMFLOAT2(x, y += step));
	text_G->SetSize(XMFLOAT2(40, 18));
	text_G->SetText("");
	text_G->SetTooltip("Enter value for GREEN channel (0-255)");
	text_G->SetDescription("G: ");
	text_G->OnInputAccepted([this](wiEventArgs args) {
		wiColor color = GetPickColor();
		color.setG((uint8_t)args.iValue);
		SetPickColor(color);
		FireEvents();
		});
	AddWidget(text_G);

	text_B = new wiTextInputField("");
	text_B->SetPos(XMFLOAT2(x, y += step));
	text_B->SetSize(XMFLOAT2(40, 18));
	text_B->SetText("");
	text_B->SetTooltip("Enter value for BLUE channel (0-255)");
	text_B->SetDescription("B: ");
	text_B->OnInputAccepted([this](wiEventArgs args) {
		wiColor color = GetPickColor();
		color.setB((uint8_t)args.iValue);
		SetPickColor(color);
		FireEvents();
		});
	AddWidget(text_B);


	text_H = new wiTextInputField("");
	text_H->SetPos(XMFLOAT2(x, y += step));
	text_H->SetSize(XMFLOAT2(40, 18));
	text_H->SetText("");
	text_H->SetTooltip("Enter value for HUE channel (0-360)");
	text_H->SetDescription("H: ");
	text_H->OnInputAccepted([this](wiEventArgs args) {
		hue = wiMath::Clamp(args.fValue, 0, 360.0f);
		FireEvents();
		});
	AddWidget(text_H);

	text_S = new wiTextInputField("");
	text_S->SetPos(XMFLOAT2(x, y += step));
	text_S->SetSize(XMFLOAT2(40, 18));
	text_S->SetText("");
	text_S->SetTooltip("Enter value for SATURATION channel (0-100)");
	text_S->SetDescription("S: ");
	text_S->OnInputAccepted([this](wiEventArgs args) {
		saturation = wiMath::Clamp(args.fValue / 100.0f, 0, 1);
		FireEvents();
		});
	AddWidget(text_S);

	text_V = new wiTextInputField("");
	text_V->SetPos(XMFLOAT2(x, y += step));
	text_V->SetSize(XMFLOAT2(40, 18));
	text_V->SetText("");
	text_V->SetTooltip("Enter value for LUMINANCE channel (0-100)");
	text_V->SetDescription("V: ");
	text_V->OnInputAccepted([this](wiEventArgs args) {
		luminance = wiMath::Clamp(args.fValue / 100.0f, 0, 1);
		FireEvents();
		});
	AddWidget(text_V);
}
static const float colorpicker_center = 120;
static const float colorpicker_radius_triangle = 68;
static const float colorpicker_radius = 75;
static const float colorpicker_width = 22;
void wiColorPicker::Update(wiGUI* gui, float dt)
{
	wiWindow::Update(gui, dt);

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

	XMFLOAT2 center = XMFLOAT2(translation.x + colorpicker_center, translation.y + colorpicker_center);
	XMFLOAT2 pointer = gui->GetPointerPos();
	float distance = wiMath::Distance(center, pointer);
	bool hover_hue = (distance > colorpicker_radius) && (distance < colorpicker_radius + colorpicker_width);

	float distTri = 0;
	XMFLOAT4 A, B, C;
	wiMath::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
	XMVECTOR _A = XMLoadFloat4(&A);
	XMVECTOR _B = XMLoadFloat4(&B);
	XMVECTOR _C = XMLoadFloat4(&C);
	XMMATRIX _triTransform = XMMatrixRotationZ(-hue / 360.0f * XM_2PI) * XMMatrixTranslation(center.x, center.y, 0);
	_A = XMVector4Transform(_A, _triTransform);
	_B = XMVector4Transform(_B, _triTransform);
	_C = XMVector4Transform(_C, _triTransform);
	XMVECTOR O = XMVectorSet(pointer.x, pointer.y, 0, 0);
	XMVECTOR D = XMVectorSet(0, 0, 1, 0);
	bool hover_saturation = TriangleTests::Intersects(O, D, _A, _B, _C, distTri);

	if (hover_hue && state == IDLE)
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

	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}

	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
	{
		if (state == ACTIVE)
		{
			// continue drag if already grabbed whether it is intersecting or not
			dragged = true;
		}
	}

	dragged = dragged && !gui->IsWidgetDisabled(this);
	if (huefocus && dragged)
	{
		//hue pick
		const float angle = wiMath::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(colorpicker_radius, 0));
		hue = angle / XM_2PI * 360.0f;
		gui->ActivateWidget(this);
	}
	else if (!huefocus && dragged)
	{
		// saturation pick
		float u, v, w;
		wiMath::GetBarycentric(O, _A, _B, _C, u, v, w, true);
		// u = saturated corner (color)
		// v = desaturated corner (white)
		// w = no luminosity corner (black)

		hsv source;
		source.h = hue;
		source.s = 1;
		source.v = 1;
		rgb result = hsv2rgb(source);

		XMVECTOR color_corner = XMVectorSet(result.r, result.g, result.b, 1);
		XMVECTOR white_corner = XMVectorSet(1, 1, 1, 1);
		XMVECTOR black_corner = XMVectorSet(0, 0, 0, 1);
		XMVECTOR inner_point = u * color_corner + v * white_corner + w * black_corner;

		result.r = XMVectorGetX(inner_point);
		result.g = XMVectorGetY(inner_point);
		result.b = XMVectorGetZ(inner_point);
		source = rgb2hsv(result);

		saturation = source.s;
		luminance = source.v;

		gui->ActivateWidget(this);
	}
	else if (state != IDLE)
	{
		gui->DeactivateWidget(this);
	}

	wiColor color = GetPickColor();
	text_R->SetValue((int)color.getR());
	text_G->SetValue((int)color.getG());
	text_B->SetValue((int)color.getB());
	text_H->SetValue(int(hue));
	text_S->SetValue(int(saturation * 100));
	text_V->SetValue(int(luminance * 100));

	if (dragged)
	{
		FireEvents();
	}
}
void wiColorPicker::Render(const wiGUI* gui, CommandList cmd) const
{
	wiWindow::Render(gui, cmd);


	if (!IsVisible() || IsMinimized())
	{
		return;
	}

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
	static wiGraphics::GPUBuffer vb_saturation;
	static wiGraphics::GPUBuffer vb_hue;
	static wiGraphics::GPUBuffer vb_picker_saturation;
	static wiGraphics::GPUBuffer vb_picker_hue;
	static wiGraphics::GPUBuffer vb_preview;

	static std::vector<Vertex> vertices_saturation;

	static bool buffersComplete = false;
	if (!buffersComplete)
	{
		buffersComplete = true;

		// saturation
		{
			vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,0,0,1) });	// hue
			vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,1,1,1) });	// white
			vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(0,0,0,1) });	// black
			wiMath::ConstructTriangleEquilateral(colorpicker_radius_triangle, vertices_saturation[0].pos, vertices_saturation[1].pos, vertices_saturation[2].pos);

			// create alpha blended edge:
			vertices_saturation.push_back(vertices_saturation[0]); // outer
			vertices_saturation.push_back(vertices_saturation[0]); // inner
			vertices_saturation.push_back(vertices_saturation[1]); // outer
			vertices_saturation.push_back(vertices_saturation[1]); // inner
			vertices_saturation.push_back(vertices_saturation[2]); // outer
			vertices_saturation.push_back(vertices_saturation[2]); // inner
			vertices_saturation.push_back(vertices_saturation[0]); // outer
			vertices_saturation.push_back(vertices_saturation[0]); // inner
			wiMath::ConstructTriangleEquilateral(colorpicker_radius_triangle + 4, vertices_saturation[3].pos, vertices_saturation[5].pos, vertices_saturation[7].pos); // extrude outer
			vertices_saturation[9].pos = vertices_saturation[3].pos; // last outer

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (uint32_t)(vertices_saturation.size() * sizeof(Vertex));
			desc.CPUAccessFlags = CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_DYNAMIC;
			SubresourceData data;
			data.pSysMem = vertices_saturation.data();
			wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_saturation);
		}
		// hue
		{
			const float edge = 2.0f;
			std::vector<Vertex> vertices;
			uint32_t segment_count = 100;
			// inner alpha blended edge
			for (uint32_t i = 0; i <= segment_count; ++i)
			{
				float p = float(i) / segment_count;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				hsv source;
				source.h = p * 360.0f;
				source.s = 1;
				source.v = 1;
				rgb result = hsv2rgb(source);
				XMFLOAT4 color = XMFLOAT4(result.r, result.g, result.b, 1);
				XMFLOAT4 coloralpha = XMFLOAT4(result.r, result.g, result.b, 0);
				vertices.push_back({ XMFLOAT4((colorpicker_radius - edge) * x, (colorpicker_radius - edge) * y, 0, 1), coloralpha });
				vertices.push_back({ XMFLOAT4(colorpicker_radius * x, colorpicker_radius * y, 0, 1), color });
			}
			// middle hue
			for (uint32_t i = 0; i <= segment_count; ++i)
			{
				float p = float(i) / segment_count;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				hsv source;
				source.h = p * 360.0f;
				source.s = 1;
				source.v = 1;
				rgb result = hsv2rgb(source);
				XMFLOAT4 color = XMFLOAT4(result.r, result.g, result.b, 1);
				vertices.push_back({ XMFLOAT4(colorpicker_radius * x, colorpicker_radius * y, 0, 1), color });
				vertices.push_back({ XMFLOAT4((colorpicker_radius + colorpicker_width) * x, (colorpicker_radius + colorpicker_width) * y, 0, 1), color });
			}
			// outer alpha blended edge
			for (uint32_t i = 0; i <= segment_count; ++i)
			{
				float p = float(i) / segment_count;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				hsv source;
				source.h = p * 360.0f;
				source.s = 1;
				source.v = 1;
				rgb result = hsv2rgb(source);
				XMFLOAT4 color = XMFLOAT4(result.r, result.g, result.b, 1);
				XMFLOAT4 coloralpha = XMFLOAT4(result.r, result.g, result.b, 0);
				vertices.push_back({ XMFLOAT4((colorpicker_radius + colorpicker_width) * x, (colorpicker_radius + colorpicker_width) * y, 0, 1), color });
				vertices.push_back({ XMFLOAT4((colorpicker_radius + colorpicker_width + edge) * x, (colorpicker_radius + colorpicker_width + edge) * y, 0, 1), coloralpha });
			}

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (uint32_t)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices.data();
			wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_hue);
		}
		// saturation picker (small circle)
		{
			float _radius = 3;
			float _width = 3;
			std::vector<Vertex> vertices;
			uint32_t segment_count = 100;
			for (uint32_t i = 0; i <= segment_count; ++i)
			{
				float p = float(i) / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				vertices.push_back({ XMFLOAT4(_radius * x, _radius * y, 0, 1), XMFLOAT4(1,1,1,1) });
				vertices.push_back({ XMFLOAT4((_radius + _width) * x, (_radius + _width) * y, 0, 1), XMFLOAT4(1,1,1,1) });
			}

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (uint32_t)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices.data();
			wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_picker_saturation);
		}
		// hue picker (rectangle)
		{
			float boldness = 4.0f;
			float halfheight = 8.0f;
			Vertex vertices[] = {
				// left side:
				{ XMFLOAT4(colorpicker_radius - boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius - boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },

				// bottom side:
				{ XMFLOAT4(colorpicker_radius - boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius - boldness, halfheight - boldness, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, halfheight - boldness, 0, 1),XMFLOAT4(1,1,1,1) },

				// right side:
				{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius + colorpicker_width, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius + colorpicker_width, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },

				// top side:
				{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, -halfheight + boldness, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius - boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(colorpicker_radius - boldness, -halfheight + boldness, 0, 1),XMFLOAT4(1,1,1,1) },
			};

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (uint32_t)sizeof(vertices);
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices;
			wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_picker_hue);
		}
		// preview
		{
			float _width = 20;
			Vertex vertices[] = {
				{ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(-_width, _width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) },
			};

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (uint32_t)sizeof(vertices);
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices;
			wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_preview);
		}

	}

	const wiColor final_color = GetPickColor();
	const float angle = hue / 360.0f * XM_2PI;

	const XMMATRIX Projection = wiRenderer::GetDevice()->GetScreenProjection();

	wiRenderer::GetDevice()->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CBSLOT_RENDERER_MISC, cmd);
	wiRenderer::GetDevice()->BindPipelineState(&PSO_colored, cmd);

	ApplyScissor(scissorRect, cmd);

	MiscCB cb;

	// render saturation triangle
	{
		if (vb_saturation.IsValid() && !vertices_saturation.empty())
		{
			hsv source;
			source.h = hue;
			source.s = 1;
			source.v = 1;
			rgb result = hsv2rgb(source);
			vertices_saturation[0].col = XMFLOAT4(result.r, result.g, result.b, 1);

			vertices_saturation[3].col = vertices_saturation[0].col; vertices_saturation[3].col.w = 0;
			vertices_saturation[4].col = vertices_saturation[0].col;
			vertices_saturation[5].col = vertices_saturation[1].col; vertices_saturation[5].col.w = 0;
			vertices_saturation[6].col = vertices_saturation[1].col;
			vertices_saturation[7].col = vertices_saturation[2].col; vertices_saturation[7].col.w = 0;
			vertices_saturation[8].col = vertices_saturation[2].col;
			vertices_saturation[9].col = vertices_saturation[0].col; vertices_saturation[9].col.w = 0;
			vertices_saturation[10].col = vertices_saturation[0].col;

			wiRenderer::GetDevice()->UpdateBuffer(&vb_saturation, vertices_saturation.data(), cmd, vb_saturation.GetDesc().ByteWidth);
		}

		XMStoreFloat4x4(&cb.g_xTransform, 
			XMMatrixRotationZ(-angle) *
			XMMatrixTranslation(translation.x + colorpicker_center, translation.y + colorpicker_center, 0) *
			Projection
		);
		cb.g_xColor = IsEnabled() ? float4(1, 1, 1, 1) : float4(0.5f, 0.5f, 0.5f, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_saturation,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		wiRenderer::GetDevice()->Draw(vb_saturation.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render hue circle
	{
		XMStoreFloat4x4(&cb.g_xTransform, 
			XMMatrixTranslation(translation.x + colorpicker_center, translation.y + colorpicker_center, 0) *
			Projection
		);
		cb.g_xColor = IsEnabled() ? float4(1, 1, 1, 1) : float4(0.5f, 0.5f, 0.5f, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_hue,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		wiRenderer::GetDevice()->Draw(vb_hue.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render hue picker
	if(IsEnabled())
	{
		XMStoreFloat4x4(&cb.g_xTransform,
			XMMatrixRotationZ(-hue / 360.0f * XM_2PI)*
			XMMatrixTranslation(translation.x + colorpicker_center, translation.y + colorpicker_center, 0) *
			Projection
		);

		hsv source;
		source.h = hue;
		source.s = 1;
		source.v = 1;
		rgb result = hsv2rgb(source);
		cb.g_xColor = float4(1 - result.r, 1 - result.g, 1 - result.b, 1);

		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_picker_hue,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		wiRenderer::GetDevice()->Draw(vb_picker_hue.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render saturation picker
	if (IsEnabled())
	{
		XMFLOAT2 center = XMFLOAT2(translation.x + colorpicker_center, translation.y + colorpicker_center);
		XMFLOAT4 A, B, C;
		wiMath::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
		XMVECTOR _A = XMLoadFloat4(&A);
		XMVECTOR _B = XMLoadFloat4(&B);
		XMVECTOR _C = XMLoadFloat4(&C);
		XMMATRIX _triTransform = XMMatrixRotationZ(-hue / 360.0f * XM_2PI) * XMMatrixTranslation(center.x, center.y, 0);
		_A = XMVector4Transform(_A, _triTransform);
		_B = XMVector4Transform(_B, _triTransform);
		_C = XMVector4Transform(_C, _triTransform);

		hsv source;
		source.h = hue;
		source.s = 1;
		source.v = 1;
		rgb result = hsv2rgb(source);

		XMVECTOR color_corner = XMVectorSet(result.r, result.g, result.b, 1);
		XMVECTOR white_corner = XMVectorSet(1, 1, 1, 1);
		XMVECTOR black_corner = XMVectorSet(0, 0, 0, 1);

		source.h = hue;
		source.s = saturation;
		source.v = luminance;
		result = hsv2rgb(source);
		XMVECTOR inner_point = XMVectorSet(result.r, result.g, result.b, 1);

		float u, v, w;
		wiMath::GetBarycentric(inner_point, color_corner, white_corner, black_corner, u, v, w, true);

		XMVECTOR picker_center = u * _A + v * _B + w * _C;

		XMStoreFloat4x4(&cb.g_xTransform, 
			XMMatrixTranslationFromVector(picker_center) *
			Projection
		);
		cb.g_xColor = float4(1 - final_color.toFloat3().x, 1 - final_color.toFloat3().y, 1 - final_color.toFloat3().z, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_picker_saturation,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		wiRenderer::GetDevice()->Draw(vb_picker_saturation.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render preview
	{
		XMStoreFloat4x4(&cb.g_xTransform, 
			XMMatrixTranslation(translation.x + 260, translation.y + 40, 0) *
			Projection
		);
		cb.g_xColor = final_color.toFloat4();
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_preview,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		wiRenderer::GetDevice()->Draw(vb_preview.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}
}
wiColor wiColorPicker::GetPickColor() const
{
	hsv source;
	source.h = hue;
	source.s = saturation;
	source.v = luminance;
	rgb result = hsv2rgb(source);
	return wiColor::fromFloat3(XMFLOAT3(result.r, result.g, result.b));
}
void wiColorPicker::SetPickColor(wiColor value)
{
	rgb source;
	source.r = value.toFloat3().x;
	source.g = value.toFloat3().y;
	source.b = value.toFloat3().z;
	hsv result = rgb2hsv(source);
	if (
		(value.getR() < 255 || value.getG() < 255 || value.getB() < 255) &&
		(value.getR() > 0 || value.getG() > 0 || value.getB() > 0)
		)
	{
		hue = result.h; // only change the hue when not pure black or white because those don't retain the saturation value
	}
	saturation = result.s;
	luminance = result.v;
}
void wiColorPicker::FireEvents()
{
	wiEventArgs args;
	args.color = GetPickColor();
	onColorChanged(args);
}
void wiColorPicker::OnColorChanged(function<void(wiEventArgs args)> func)
{
	onColorChanged = move(func);
}






static const float item_height = 20;
static const float tree_scrollbar_width = 12;
wiTreeList::wiTreeList(const std::string& name)
{
	SetName(name);
	SetText(fastName.GetString());
	OnSelect([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 20));
}
wiTreeList::~wiTreeList()
{

}
float wiTreeList::GetItemOffset(int index) const
{
	return 2 + list_offset + index * item_height;
}
Hitbox2D wiTreeList::GetHitbox_ListArea() const
{
	return Hitbox2D(XMFLOAT2(translation.x, translation.y + item_height + 1), XMFLOAT2(scale.x - tree_scrollbar_width - 1, scale.y - item_height - 1));
}
Hitbox2D wiTreeList::GetHitbox_Item(int visible_count, int level) const
{
	XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height, translation.y + GetItemOffset(visible_count) + item_height * 0.5f);
	Hitbox2D hitbox;
	hitbox.pos = XMFLOAT2(pos.x + item_height * 0.5f + 2, pos.y - item_height * 0.5f);
	hitbox.siz = XMFLOAT2(scale.x - 2 - item_height * 0.5f - 2 - level * item_height - tree_scrollbar_width - 2, item_height);
	return hitbox;
}
Hitbox2D wiTreeList::GetHitbox_ItemOpener(int visible_count, int level) const
{
	XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height, translation.y + GetItemOffset(visible_count) + item_height * 0.5f);
	return Hitbox2D(XMFLOAT2(pos.x, pos.y - item_height * 0.25f), XMFLOAT2(item_height * 0.5f, item_height * 0.5f));
}
bool wiTreeList::HasScrollbar() const
{
	return scale.y < (int)items.size() * item_height;
}
void wiTreeList::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

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

	Hitbox2D hitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));
	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	if (state == IDLE && hitbox.intersects(pointerHitbox))
	{
		state = FOCUS;
	}

	bool clicked = false;
	if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
	{
		clicked = true;
	}

	bool click_down = false;
	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
	{
		click_down = true;
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	// compute control-list height
	list_height = 0;
	{
		int visible_count = 0;
		int parent_level = 0;
		bool parent_open = true;
		for (const Item& item : items)
		{
			if (!parent_open && item.level > parent_level)
			{
				continue;
			}
			visible_count++;
			parent_open = item.open;
			parent_level = item.level;
			list_height += item_height;
		}
	}

	const float scrollbar_begin = translation.y + item_height;
	const float scrollbar_end = scrollbar_begin + scale.y - item_height;
	const float scrollbar_size = scrollbar_end - scrollbar_begin;
	const float scrollbar_granularity = std::min(1.0f, scrollbar_size / list_height);
	scrollbar_height = std::max(tree_scrollbar_width, scrollbar_size * scrollbar_granularity);

	if (!click_down)
	{
		scrollbar_state = SCROLLBAR_INACTIVE;
	}

	Hitbox2D scroll_box;
	scroll_box.pos = XMFLOAT2(translation.x + scale.x - tree_scrollbar_width, translation.y + item_height + 1);
	scroll_box.siz = XMFLOAT2(tree_scrollbar_width, scale.y - item_height - 1);
	if (scroll_box.intersects(pointerHitbox))
	{
		if (clicked)
		{
			scrollbar_state = SCROLLBAR_GRABBED;
		}
		else if(!click_down)
		{
			scrollbar_state = SCROLLBAR_HOVER;
			state = FOCUS;
		}
	}

	if (scrollbar_state == SCROLLBAR_GRABBED)
	{
		gui->ActivateWidget(this);
		scrollbar_delta = pointerHitbox.pos.y - scrollbar_height * 0.5f - scrollbar_begin;
	}

	if (state == FOCUS)
	{
		scrollbar_delta -= wiInput::GetPointer().z;
	}
	scrollbar_delta = wiMath::Clamp(scrollbar_delta, 0, scrollbar_size - scrollbar_height);
	scrollbar_value = wiMath::InverseLerp(scrollbar_begin, scrollbar_end, scrollbar_begin + scrollbar_delta);

	list_offset = -scrollbar_value * list_height;

	Hitbox2D itemlist_box = GetHitbox_ListArea();

	// control-list
	item_highlight = -1;
	opener_highlight = -1;
	if (scrollbar_state == SCROLLBAR_INACTIVE)
	{
		int i = -1;
		int visible_count = 0;
		int parent_level = 0;
		bool parent_open = true;
		for (Item& item : items)
		{
			i++;
			if (!parent_open && item.level > parent_level)
			{
				continue;
			}
			visible_count++;
			parent_open = item.open;
			parent_level = item.level;

			Hitbox2D open_box = GetHitbox_ItemOpener(visible_count, item.level);
			if (!open_box.intersects(itemlist_box))
			{
				continue;
			}

			if (open_box.intersects(pointerHitbox))
			{
				// Opened flag box:
				opener_highlight = i;
				if (clicked)
				{
					item.open = !item.open;
					gui->ActivateWidget(this);
				}
			}
			else
			{
				// Item name box:
				Hitbox2D name_box = GetHitbox_Item(visible_count, item.level);
				if (name_box.intersects(pointerHitbox))
				{
					item_highlight = i;
					if (clicked)
					{
						if (!wiInput::Down(wiInput::KEYBOARD_BUTTON_LCONTROL) && !wiInput::Down(wiInput::KEYBOARD_BUTTON_RCONTROL)
							&& !wiInput::Down(wiInput::KEYBOARD_BUTTON_LSHIFT) && !wiInput::Down(wiInput::KEYBOARD_BUTTON_RSHIFT))
						{
							ClearSelection();
						}
						Select(i);
						gui->ActivateWidget(this);
					}
				}
			}
		}
	}
}
void wiTreeList::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}
	GraphicsDevice* device = wiRenderer::GetDevice();

	ApplyScissor(scissorRect, cmd);

	// control-base
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x, translation.y, scale.x, item_height, colors[state == IDLE ? IDLE : FOCUS].toFloat4()), cmd);

	wiFont(text, wiFontParams((int)(translation.x) + 1, (int)(translation.y) + 1, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_TOP, 0, 0,
		fontParams.color, fontParams.shadowColor)).Draw(cmd);

	// scrollbar background
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x + scale.x - tree_scrollbar_width, translation.y + item_height + 1, tree_scrollbar_width, scale.y - item_height - 1, colors[IDLE].toFloat4()), cmd);

	// scrollbar
	wiColor scrollbar_color = colors[IDLE];
	if (scrollbar_state == SCROLLBAR_HOVER)
	{
		scrollbar_color = colors[FOCUS];
	}
	else if (scrollbar_state == SCROLLBAR_GRABBED)
	{
		scrollbar_color = colors[ACTIVE];
	}
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x + scale.x - tree_scrollbar_width, translation.y + item_height + 1 + scrollbar_delta, tree_scrollbar_width, scrollbar_height, scrollbar_color.toFloat4()), cmd);

	// list background
	Hitbox2D itemlist_box = GetHitbox_ListArea();
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(itemlist_box.pos.x, itemlist_box.pos.y, itemlist_box.siz.x, itemlist_box.siz.y, colors[IDLE].toFloat4()), cmd);

	Rect rect_without_scrollbar;
	rect_without_scrollbar.left = (int)itemlist_box.pos.x;
	rect_without_scrollbar.right = (int)(itemlist_box.pos.x + itemlist_box.siz.x);
	rect_without_scrollbar.top = (int)itemlist_box.pos.y;
	rect_without_scrollbar.bottom = (int)(itemlist_box.pos.y + itemlist_box.siz.y);
	ApplyScissor(rect_without_scrollbar, cmd);

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
	static GPUBuffer vb_triangle;
	if (!vb_triangle.IsValid())
	{
		Vertex vertices[3];
		vertices[0].col = XMFLOAT4(1, 1, 1, 1);
		vertices[1].col = XMFLOAT4(1, 1, 1, 1);
		vertices[2].col = XMFLOAT4(1, 1, 1, 1);
		wiMath::ConstructTriangleEquilateral(1, vertices[0].pos, vertices[1].pos, vertices[2].pos);

		GPUBufferDesc desc;
		desc.BindFlags = BIND_VERTEX_BUFFER;
		desc.ByteWidth = sizeof(vertices);
		SubresourceData initdata;
		initdata.pSysMem = vertices;
		device->CreateBuffer(&desc, &initdata, &vb_triangle);
	}
	const XMMATRIX Projection = device->GetScreenProjection();

	// control-list
	int i = -1;
	int visible_count = 0;
	int parent_level = 0;
	bool parent_open = true;
	for (const Item& item : items)
	{
		i++;
		if (!parent_open && item.level > parent_level)
		{
			continue;
		}
		visible_count++;
		parent_open = item.open;
		parent_level = item.level;

		Hitbox2D open_box = GetHitbox_ItemOpener(visible_count, item.level);
		if (!open_box.intersects(itemlist_box))
		{
			continue;
		}

		Hitbox2D name_box = GetHitbox_Item(visible_count, item.level);

		// selected box:
		if (item.selected || item_highlight == i)
		{
			wiImage::Draw(wiTextureHelper::getWhite()
				, wiImageParams(name_box.pos.x, name_box.pos.y, name_box.siz.x, name_box.siz.y,
					colors[item.selected ? FOCUS : IDLE].toFloat4()), cmd);
		}
		
		// opened flag triangle:
		{
			device->BindPipelineState(&PSO_colored, cmd);

			MiscCB cb;
			cb.g_xColor = colors[opener_highlight == i ? ACTIVE : FOCUS].toFloat4();
			XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(item_height * 0.3f, item_height * 0.3f, 1) * 
				XMMatrixRotationZ(item.open ? XM_PIDIV2 : 0) * 
				XMMatrixTranslation(open_box.pos.x + open_box.siz.x * 0.5f, open_box.pos.y + open_box.siz.y * 0.25f, 0) *
				Projection
			);
			device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
			device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_triangle,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);

			device->Draw(3, 0, cmd);
		}
		
		// Item name text:
		wiFont(item.name, wiFontParams((int)name_box.pos.x + 1, (int)(name_box.pos.y + name_box.siz.y * 0.5f), WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_CENTER, 0, 0,
			fontParams.color, fontParams.shadowColor)).Draw(cmd);
	}
}
void wiTreeList::OnSelect(function<void(wiEventArgs args)> func)
{
	onSelect = move(func);
}
void wiTreeList::AddItem(const Item& item)
{
	items.push_back(item);
}
void wiTreeList::ClearItems()
{
	items.clear();
}
void wiTreeList::ClearSelection()
{
	for (Item& item : items)
	{
		item.selected = false;
	}

	wiEventArgs args;
	args.iValue = -1;
	onSelect(args);
}
void wiTreeList::Select(int index)
{
	items[index].selected = true;

	wiEventArgs args;
	args.iValue = index;
	args.sValue = items[index].name;
	args.userdata = items[index].userdata;
	onSelect(args);
}
const wiTreeList::Item& wiTreeList::GetItem(int index) const
{
	return items[index];
}
