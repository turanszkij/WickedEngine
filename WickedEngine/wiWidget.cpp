#include "wiWidget.h"
#include "wiGUI.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiMath.h"
#include "wiHelper.h"
#include "wiInput.h"
#include "wiRenderer.h"
#include "ShaderInterop_Renderer.h"
#include "wiEvent.h"

#include <sstream>

using namespace std;
using namespace wiGraphics;
using namespace wiScene;


static wiGraphics::PipelineState PSO_colored;

wiWidget::wiWidget()
{
	sprites[IDLE].params.color = wiColor::Booger();
	sprites[FOCUS].params.color = wiColor::Gray();
	sprites[ACTIVE].params.color = wiColor::White();
	sprites[DEACTIVATING].params.color = wiColor::Gray();
	font.params.shadowColor = wiColor::Black();

	for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites[i].params.blendFlag = BLENDMODE_OPAQUE;
		sprites[i].params.enableBackgroundBlur();
	}
}

void wiWidget::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}


	hitBox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));

	if (GetState() != WIDGETSTATE::ACTIVE && !tooltip.empty() && gui->GetPointerHitbox().intersects(hitBox))
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
		this->UpdateTransform_Parented(*parent);
	}

	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
	XMStoreFloat3(&translation, T);
	XMStoreFloat3(&scale, S);

	scissorRect.bottom = (int32_t)(translation.y + scale.y);
	scissorRect.left = (int32_t)(translation.x);
	scissorRect.right = (int32_t)(translation.x + scale.x);
	scissorRect.top = (int32_t)(translation.y);

	// default sprite and font placement:
	for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites[i].params.pos.x = translation.x;
		sprites[i].params.pos.y = translation.y;
		sprites[i].params.siz.x = scale.x;
		sprites[i].params.siz.y = scale.y;
		sprites[i].params.fade = IsEnabled() ? 0.0f : 0.5f;
	}
	font.params.posX = translation.x;
	font.params.posY = translation.y;
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
		float screenwidth = wiRenderer::GetDevice()->GetScreenWidth();
		float screenheight = wiRenderer::GetDevice()->GetScreenHeight();

		wiFontParams fontProps = wiFontParams(0, 0, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_TOP);
		fontProps.color = wiColor(25, 25, 25, 255);
		wiSpriteFont tooltipFont = wiSpriteFont(tooltip, fontProps);
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(tooltip + "\n" + scriptTip);
		}

		static const float _border = 2;
		float textWidth = tooltipFont.textWidth() + _border * 2;
		float textHeight = tooltipFont.textHeight() + _border * 2;

		tooltipFont.params.posX = gui->pointerpos.x;
		tooltipFont.params.posY = gui->pointerpos.y;

		if (tooltipFont.params.posX + textWidth > screenwidth)
		{
			tooltipFont.params.posX -= tooltipFont.params.posX + textWidth - screenwidth;
		}
		if (tooltipFont.params.posX < 0)
		{
			tooltipFont.params.posX = 0;
		}

		if (tooltipFont.params.posY > screenheight * 0.8f)
		{
			tooltipFont.params.posY -= 30;
		}
		else
		{
			tooltipFont.params.posY += 40;
		}

		wiImage::Draw(wiTextureHelper::getWhite(), 
			wiImageParams(tooltipFont.params.posX - _border, tooltipFont.params.posY - _border, 
				textWidth, textHeight, wiColor(255, 234, 165)), cmd);
		tooltipFont.SetText(tooltip);
		tooltipFont.Draw(cmd);
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(scriptTip);
			tooltipFont.params.posY += (int)(textHeight / 2);
			tooltipFont.params.color = wiColor(25, 25, 25, 110);
			tooltipFont.Draw(cmd);
		}
	}
}
const std::string& wiWidget::GetName() const
{
	return name;
}
void wiWidget::SetName(const std::string& value)
{
	if (value.length() <= 0)
	{
		static unsigned long widgetID = 0;
		stringstream ss("");
		ss << "widget_" << widgetID++;
		name = ss.str();
	}
	else
	{
		name = value;
	}

}
const string wiWidget::GetText() const
{
	return font.GetTextA();
}
void wiWidget::SetText(const std::string& value)
{
	font.SetText(value);
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
	if (enabled != val)
	{
		priority_change |= val;
		enabled = val;
	}
}
bool wiWidget::IsEnabled() const
{
	return enabled && visible;
}
void wiWidget::SetVisible(bool val)
{
	if (visible != val)
	{
		priority_change |= val;
		visible = val;
	}
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
			sprites[i].params.color = color;
		}
	}
	else
	{
		sprites[state].params.color = color;
	}
}
wiColor wiWidget::GetColor() const
{
	return wiColor::fromFloat4(sprites[GetState()].params.color);
}

namespace wiWidget_Internal
{
	void LoadShaders()
	{
		PipelineStateDesc desc;
		desc.vs = wiRenderer::GetVertexShader(VSTYPE_VERTEXCOLOR);
		desc.ps = wiRenderer::GetPixelShader(PSTYPE_VERTEXCOLOR);
		desc.il = wiRenderer::GetInputLayout(ILTYPE_VERTEXCOLOR);
		desc.dss = wiRenderer::GetDepthStencilState(DSSTYPE_XRAY);
		desc.bs = wiRenderer::GetBlendState(BSTYPE_TRANSPARENT);
		desc.rs = wiRenderer::GetRasterizerState(RSTYPE_DOUBLESIDED);
		desc.pt = TRIANGLESTRIP;
		wiRenderer::GetDevice()->CreatePipelineState(&desc, &PSO_colored);
	}
}

void wiWidget::Initialize()
{
	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { wiWidget_Internal::LoadShaders(); });
	wiWidget_Internal::LoadShaders();
}


wiButton::wiButton(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(name);
	OnClick([](wiEventArgs args) {});
	OnDragStart([](wiEventArgs args) {});
	OnDrag([](wiEventArgs args) {});
	OnDragEnd([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));

	font.params.h_align = WIFALIGN_CENTER;
	font.params.v_align = WIFALIGN_CENTER;
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{
		hitBox.pos.x = translation.x;
		hitBox.pos.y = translation.y;
		hitBox.siz.x = scale.x;
		hitBox.siz.y = scale.y;

		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();

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

	switch (font.params.h_align)
	{
	case WIFALIGN_LEFT:
		font.params.posX = translation.x + 2;
		break;
	case WIFALIGN_RIGHT:
		font.params.posX = translation.x + scale.x - 2;
		break;
	case WIFALIGN_CENTER:
	default:
		font.params.posX = translation.x + scale.x * 0.5f;
		break;
	}
	switch (font.params.v_align)
	{
	case WIFALIGN_TOP:
		font.params.posY = translation.y + 2;
		break;
	case WIFALIGN_BOTTOM:
		font.params.posY = translation.y + scale.y - 2;
		break;
	case WIFALIGN_CENTER:
	default:
		font.params.posY = translation.y + scale.y * 0.5f;
		break;
	}
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

	sprites[state].Draw(cmd);
	font.Draw(cmd);
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
	SetText(name);
	SetSize(XMFLOAT2(100, 20));
}
wiLabel::~wiLabel()
{

}
void wiLabel::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{
		// ...
	}

	font.params.h_wrap = scale.x;

	switch (font.params.h_align)
	{
	case WIFALIGN_LEFT:
		font.params.posX = translation.x + 2;
		break;
	case WIFALIGN_RIGHT:
		font.params.posX = translation.x + scale.x - 2;
		break;
	case WIFALIGN_CENTER:
	default:
		font.params.posX = translation.x + scale.x * 0.5f;
		break;
	}
	switch (font.params.v_align)
	{
	case WIFALIGN_TOP:
		font.params.posY = translation.y + 2;
		break;
	case WIFALIGN_BOTTOM:
		font.params.posY = translation.y + scale.y - 2;
		break;
	case WIFALIGN_CENTER:
	default:
		font.params.posY = translation.y + scale.y * 0.5f;
		break;
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

	sprites[state].Draw(cmd);
	font.Draw(cmd);
}




wiSpriteFont wiTextInputField::font_input;
wiTextInputField::wiTextInputField(const std::string& name)
{
	SetName(name);
	SetText(name);
	OnInputAccepted([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));

	font.params.v_align = WIFALIGN_CENTER;

	font_description.params = font.params;
	font_description.params.h_align = WIFALIGN_RIGHT;
}
wiTextInputField::~wiTextInputField()
{

}
void wiTextInputField::SetValue(const std::string& newValue)
{
	font.SetText(newValue);
}
void wiTextInputField::SetValue(int newValue)
{
	stringstream ss("");
	ss << newValue;
	font.SetText(ss.str());
}
void wiTextInputField::SetValue(float newValue)
{
	stringstream ss("");
	ss << newValue;
	font.SetText(ss.str());
}
const std::string wiTextInputField::GetValue()
{
	return font.GetTextA();
}
void wiTextInputField::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{
		hitBox.pos.x = translation.x;
		hitBox.pos.y = translation.y;
		hitBox.siz.x = scale.x;
		hitBox.siz.y = scale.y;

		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();
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

			font_input.SetText(font.GetText());
		}

		if (state == ACTIVE)
		{
			if (wiInput::Press(wiInput::KEYBOARD_BUTTON_ENTER))
			{
				// accept input...

				font.SetText(font_input.GetText());
				font_input.text.clear();

				wiEventArgs args;
				args.sValue = font.GetTextA();
				args.iValue = atoi(args.sValue.c_str());
				args.fValue = (float)atof(args.sValue.c_str());
				onInputAccepted(args);

				gui->DeactivateWidget(this);
			}
			else if ((wiInput::Press(wiInput::MOUSE_BUTTON_LEFT) && !intersectsPointer) ||
				wiInput::Press(wiInput::KEYBOARD_BUTTON_ESCAPE))
			{
				// cancel input 
				font_input.text.clear();
				gui->DeactivateWidget(this);
			}

		}
	}

	font.params.posX = translation.x + 2;
	font.params.posY = translation.y + scale.y * 0.5f;
	font_description.params.posX = translation.x - 2;
	font_description.params.posY = translation.y + scale.y * 0.5f;

	if (state == ACTIVE)
	{
		font_input.params = font.params;
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

	font_description.Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	sprites[state].Draw(cmd);

	if (state == ACTIVE)
	{
		font_input.Draw(cmd);
	}
	else
	{
		font.Draw(cmd);
	}

}
void wiTextInputField::OnInputAccepted(function<void(wiEventArgs args)> func)
{
	onInputAccepted = move(func);
}
void wiTextInputField::AddInput(const char inputChar)
{
	string value_new = font_input.GetTextA();
	value_new.push_back(inputChar);
	font_input.SetText(value_new);
}
void wiTextInputField::DeleteFromInput()
{
	string value_new = font_input.GetTextA();
	if (!value_new.empty())
	{
		value_new.pop_back();
	}
	font_input.SetText(value_new);
}





wiSlider::wiSlider(float start, float end, float defaultValue, float step, const std::string& name) : start(start), end(end), value(defaultValue), step(std::max(step, 1.0f))
{
	SetName(name);
	SetText(name);
	OnSlide([](wiEventArgs args) {});
	SetSize(XMFLOAT2(200, 40));

	valueInputField = new wiTextInputField(name + "_endInputField");
	valueInputField->SetTooltip("Enter number to modify value even outside slider limits. Enter \"reset\" to reset slider to initial state.");
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

	for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites_knob[i].params = sprites[i].params;
	}

	sprites[IDLE].params.color = wiColor(60, 60, 60, 200);
	sprites[FOCUS].params.color = wiColor(50, 50, 50, 200);
	sprites[ACTIVE].params.color = wiColor(50, 50, 50, 200);
	sprites[DEACTIVATING].params.color = wiColor(60, 60, 60, 200);

	font.params.h_align = WIFALIGN_RIGHT;
	font.params.v_align = WIFALIGN_CENTER;
}
wiSlider::~wiSlider()
{
	delete valueInputField;
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
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	valueInputField->Detach();
	valueInputField->SetSize(XMFLOAT2(40.0f, scale.y));
	valueInputField->SetPos(XMFLOAT2(translation.x + scale.x + 2, translation.y));
	valueInputField->AttachTo(this);

	scissorRect.bottom = (int32_t)(translation.y + scale.y);
	scissorRect.left = (int32_t)(translation.x);
	scissorRect.right = (int32_t)(translation.x + scale.x + 20 + scale.y * 2); // include the valueInputField
	scissorRect.top = (int32_t)(translation.y);

	for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites_knob[i].params.siz.x = 16.0f;
		valueInputField->SetColor(wiColor::fromFloat4(this->sprites_knob[i].params.color), (WIDGETSTATE)i);
	}
	valueInputField->font.params.color = this->font.params.color;
	valueInputField->font.params.shadowColor = this->font.params.shadowColor;
	valueInputField->SetEnabled(IsEnabled());
	valueInputField->Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{

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

		const float knobWidth = sprites_knob[state].params.siz.x;
		hitBox.pos.x = translation.x - knobWidth * 0.5f;
		hitBox.pos.y = translation.y;
		hitBox.siz.x = scale.x + knobWidth;
		hitBox.siz.y = scale.y;

		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();


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

	font.params.posY = translation.y + scale.y * 0.5f;

	const float knobWidth = sprites_knob[state].params.siz.x;
	const float knobWidthHalf = knobWidth * 0.5f;
	sprites_knob[state].params.pos.x = wiMath::Lerp(translation.x + knobWidthHalf + 2, translation.x + scale.x - knobWidthHalf - 2, wiMath::Clamp(wiMath::InverseLerp(start, end, value), 0, 1));
	sprites_knob[state].params.pos.y = translation.y + 2;
	sprites_knob[state].params.siz.y = scale.y - 4;
	sprites_knob[state].params.pivot = XMFLOAT2(0.5f, 0);
	sprites_knob[state].params.fade = IsEnabled() ? 0.0f : 0.5f;
}
void wiSlider::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	font.Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	// base
	sprites[state].Draw(cmd);

	// knob
	sprites_knob[state].Draw(cmd);

	valueInputField->Render(gui, cmd);
}
void wiSlider::RenderTooltip(const wiGUI* gui, wiGraphics::CommandList cmd) const
{
	wiWidget::RenderTooltip(gui, cmd);
	valueInputField->RenderTooltip(gui, cmd);
}
void wiSlider::OnSlide(function<void(wiEventArgs args)> func)
{
	onSlide = move(func);
}





wiCheckBox::wiCheckBox(const std::string& name)
{
	SetName(name);
	SetText(name);
	OnClick([](wiEventArgs args) {});
	SetSize(XMFLOAT2(20, 20));

	font.params.h_align = WIFALIGN_RIGHT;
	font.params.v_align = WIFALIGN_CENTER;

	for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites_check[i].params = sprites[i].params;
		sprites_check[i].params.color = wiMath::Lerp(sprites[i].params.color, wiColor::White().toFloat4(), 0.8f);
	}
}
wiCheckBox::~wiCheckBox()
{

}
void wiCheckBox::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{

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

		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();

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

	font.params.posY = translation.y + scale.y * 0.5f;

	sprites_check[state].params.pos.x = translation.x + scale.x * 0.25f;
	sprites_check[state].params.pos.y = translation.y + scale.y * 0.25f;
	sprites_check[state].params.siz = XMFLOAT2(scale.x * 0.5f, scale.y * 0.5f);
}
void wiCheckBox::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	font.Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	// control
	sprites[state].Draw(cmd);

	// check
	if (GetCheck())
	{
		sprites_check[state].Draw(cmd);
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
	SetText(name);
	OnSelect([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 20));

	font.params.h_align = WIFALIGN_RIGHT;
	font.params.v_align = WIFALIGN_CENTER;
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
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{

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

		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();

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

	font.params.posY = translation.y + sprites[state].params.siz.y * 0.5f;
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
		color = wiColor::fromFloat4(sprites[FOCUS].params.color);
	}

	font.Draw(cmd);

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
	wiImageParams fx = sprites[state].params;
	fx.pos = XMFLOAT3(translation.x + scale.x + 1, translation.y, 0);
	fx.siz = XMFLOAT2(scale.y, scale.y);
	wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

	// control-arrow-triangle
	{
		device->BindPipelineState(&PSO_colored, cmd);

		MiscCB cb;
		cb.g_xColor = sprites[ACTIVE].params.color;
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
	sprites[state].Draw(cmd);

	if (selected >= 0)
	{
		wiFont::Draw(items[selected], wiFontParams(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f, WIFONTSIZE_DEFAULT, WIFALIGN_CENTER, WIFALIGN_CENTER,
			font.params.color, font.params.shadowColor), cmd);
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
				fx = sprites[state].params;
				fx.pos = XMFLOAT3(translation.x + scale.x + 1, translation.y + scale.y + 1, 0);
				fx.siz = XMFLOAT2(scale.y, scale.y * maxVisibleItemCount);
				fx.color = wiMath::Lerp(wiColor::Transparent(), sprites[IDLE].params.color, 0.5f);
				wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
			}

			// control-scrollbar-grab
			{
				wiColor col = wiColor::fromFloat4(sprites[IDLE].params.color);
				if (combostate == COMBOSTATE_SCROLLBAR_HOVER)
				{
					col = wiColor::fromFloat4(sprites[FOCUS].params.color);
				}
				else if (combostate == COMBOSTATE_SCROLLBAR_GRABBED)
				{
					col = wiColor::fromFloat4(sprites[ACTIVE].params.color);
				}
				wiImage::Draw(wiTextureHelper::getWhite()
					, wiImageParams(translation.x + scale.x + 1, translation.y + scale.y + 1 + scrollbar_delta, scale.y, scale.y, col), cmd);
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
			fx = sprites[state].params;
			fx.pos = XMFLOAT3(translation.x, translation.y + GetItemOffset(i), 0);
			fx.siz = XMFLOAT2(scale.x, scale.y);
			fx.color = wiMath::Lerp(wiColor::Transparent(), sprites[IDLE].params.color, 0.5f);
			if (hovered == i)
			{
				if (combostate == COMBOSTATE_HOVER)
				{
					fx.color = sprites[FOCUS].params.color;
				}
				else if (combostate == COMBOSTATE_SELECTING)
				{
					fx.color = sprites[ACTIVE].params.color;
				}
			}
			wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);
			wiFont::Draw(items[i], wiFontParams(translation.x + scale.x*0.5f, translation.y + scale.y*0.5f + GetItemOffset(i), WIFONTSIZE_DEFAULT, WIFALIGN_CENTER, WIFALIGN_CENTER,
				font.params.color, font.params.shadowColor), cmd);
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

	SetColor(wiColor::Ghost());

	SetName(name);
	SetText(name);
	SetSize(XMFLOAT2(640, 480)); 
	
	for (int i = IDLE + 1; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites[i].params.color = sprites[IDLE].params.color;
	}

	// Add controls
	if (window_controls)
	{
		// Add a resizer control to the upperleft corner
		resizeDragger_UpperLeft = new wiButton(name + "_resize_dragger_upper_left");
		resizeDragger_UpperLeft->SetText("");
		resizeDragger_UpperLeft->OnDrag([this, gui](wiEventArgs args) {
			auto saved_parent = this->parent;
			this->Detach();
			XMFLOAT2 scaleDiff;
			scaleDiff.x = (scale.x - args.deltaPos.x) / scale.x;
			scaleDiff.y = (scale.y - args.deltaPos.y) / scale.y;
			this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
			this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
			this->scale_local = wiMath::Max(this->scale_local, XMFLOAT3(windowcontrolSize * 3, windowcontrolSize * 2, 1)); // don't allow resize to negative or too small
			// Don't allow control outside of screen:
			this->translation_local.x = wiMath::Clamp(this->translation_local.x, 0, wiRenderer::GetDevice()->GetScreenWidth() - this->scale_local.x);
			this->translation_local.y = wiMath::Clamp(this->translation_local.y, 0, wiRenderer::GetDevice()->GetScreenHeight() - windowcontrolSize);
			this->wiWidget::Update(gui, 0);
			for (auto& x : this->childrenWidgets)
			{
				x->wiWidget::Update(gui, 0);
			}
			this->AttachTo(saved_parent);
			});
		AddWidget(resizeDragger_UpperLeft);

		// Add a resizer control to the bottom right corner
		resizeDragger_BottomRight = new wiButton(name + "_resize_dragger_bottom_right");
		resizeDragger_BottomRight->SetText("");
		resizeDragger_BottomRight->OnDrag([this, gui](wiEventArgs args) {
			auto saved_parent = this->parent;
			this->Detach();
			XMFLOAT2 scaleDiff;
			scaleDiff.x = (scale.x + args.deltaPos.x) / scale.x;
			scaleDiff.y = (scale.y + args.deltaPos.y) / scale.y;
			this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
			this->scale_local = wiMath::Max(this->scale_local, XMFLOAT3(windowcontrolSize * 3, windowcontrolSize * 2, 1)); // don't allow resize to negative or too small
			// Don't allow control outside of screen:
			this->translation_local.x = wiMath::Clamp(this->translation_local.x, 0, wiRenderer::GetDevice()->GetScreenWidth() - this->scale_local.x);
			this->translation_local.y = wiMath::Clamp(this->translation_local.y, 0, wiRenderer::GetDevice()->GetScreenHeight() - windowcontrolSize);
			this->wiWidget::Update(gui, 0);
			for (auto& x : this->childrenWidgets)
			{
				x->wiWidget::Update(gui, 0);
			}
			this->AttachTo(saved_parent);
			});
		AddWidget(resizeDragger_BottomRight);

		// Add a grabber onto the title bar
		moveDragger = new wiButton(name + "_move_dragger");
		moveDragger->SetText(name);
		moveDragger->font.params.h_align = WIFALIGN_LEFT;
		moveDragger->OnDrag([this, gui](wiEventArgs args) {
			auto saved_parent = this->parent;
			this->Detach();
			this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
			// Don't allow control outside of screen:
			this->translation_local.x = wiMath::Clamp(this->translation_local.x, 0, wiRenderer::GetDevice()->GetScreenWidth() - this->scale_local.x);
			this->translation_local.y = wiMath::Clamp(this->translation_local.y, 0, wiRenderer::GetDevice()->GetScreenHeight() - windowcontrolSize);
			this->wiWidget::Update(gui, 0);
			for (auto& x : this->childrenWidgets)
			{
				x->wiWidget::Update(gui, 0);
			}
			this->AttachTo(saved_parent);
			});
		AddWidget(moveDragger);

		// Add close button to the top right corner
		closeButton = new wiButton(name + "_close_button");
		closeButton->SetText("x");
		closeButton->OnClick([this, gui](wiEventArgs args) {
			this->SetVisible(false);
			});
		closeButton->SetTooltip("Close window");
		AddWidget(closeButton);

		// Add minimize button to the top right corner
		minimizeButton = new wiButton(name + "_minimize_button");
		minimizeButton->SetText("-");
		minimizeButton->OnClick([this, gui](wiEventArgs args) {
			this->SetMinimized(!this->IsMinimized());
			});
		minimizeButton->SetTooltip("Minimize window");
		AddWidget(minimizeButton);
	}
	else
	{
		// Simple title bar
		label = new wiLabel(name);
		label->SetText(name);
		label->font.params.h_align = WIFALIGN_LEFT;
		AddWidget(label);
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
			delete x;
		}
	}

	childrenWidgets.clear();
}
void wiWindow::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	if (moveDragger != nullptr)
	{
		moveDragger->Detach();
		moveDragger->SetSize(XMFLOAT2(scale.x - windowcontrolSize * 3, windowcontrolSize));
		moveDragger->SetPos(XMFLOAT2(translation.x + windowcontrolSize, translation.y));
		moveDragger->AttachTo(this);
	}
	if (closeButton != nullptr)
	{
		closeButton->Detach();
		closeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		closeButton->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y));
		closeButton->AttachTo(this);
	}
	if (minimizeButton != nullptr)
	{
		minimizeButton->Detach();
		minimizeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		minimizeButton->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize * 2, translation.y));
		minimizeButton->AttachTo(this);
	}
	if (resizeDragger_UpperLeft != nullptr)
	{
		resizeDragger_UpperLeft->Detach();
		resizeDragger_UpperLeft->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		resizeDragger_UpperLeft->SetPos(XMFLOAT2(translation.x, translation.y));
		resizeDragger_UpperLeft->AttachTo(this);
	}
	if (resizeDragger_BottomRight != nullptr)
	{
		resizeDragger_BottomRight->Detach();
		resizeDragger_BottomRight->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
		resizeDragger_BottomRight->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y + scale.y - windowcontrolSize));
		resizeDragger_BottomRight->AttachTo(this);
	}
	if (label != nullptr)
	{
		label->Detach();
		label->SetSize(XMFLOAT2(scale.x, windowcontrolSize));
		label->SetPos(XMFLOAT2(translation.x, translation.y));
		label->AttachTo(this);
	}

	for (auto& x : childrenWidgets)
	{
		x->Update(gui, dt);

		if (x->GetState() == ACTIVE)
		{
			gui->ActivateWidget(this);
		}
	}

	if (IsEnabled() && !gui->IsWidgetDisabled(this) && !IsMinimized())
	{
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

		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();

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
			gui->ActivateWidget(this);
		}
	}
	else
	{
		state = IDLE;
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

	// body
	if (!IsMinimized())
	{
		wiImageParams fx(translation.x-2, translation.y-2, scale.x+4, scale.y+4, wiColor(0, 0, 0, 100));
		wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd); // simple shadow under the window
		sprites[state].Draw(cmd);
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
		if (x == resizeDragger_BottomRight)
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

static const float cp_width = 300;
static const float cp_height = 260;
wiColorPicker::wiColorPicker(wiGUI* gui, const std::string& name, bool window_controls) :wiWindow(gui, name, window_controls)
{
	SetSize(XMFLOAT2(cp_width, cp_height));
	SetColor(wiColor(100, 100, 100, 100));

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

	alphaSlider = new wiSlider(0, 255, 255, 255, "");
	alphaSlider->SetPos(XMFLOAT2(20, 230));
	alphaSlider->SetSize(XMFLOAT2(150, 18));
	alphaSlider->SetText("A: ");
	alphaSlider->SetTooltip("Value for ALPHA - TRANSPARENCY channel (0-255)");
	alphaSlider->OnSlide([this](wiEventArgs args) {
		FireEvents();
		});
	AddWidget(alphaSlider);
}
static const float colorpicker_radius_triangle = 68;
static const float colorpicker_radius = 75;
static const float colorpicker_width = 22;
void wiColorPicker::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWindow::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{

		if (state == DEACTIVATING)
		{
			state = IDLE;
		}

		float sca = std::min(scale.x / cp_width, scale.y / cp_height);

		XMMATRIX W =
			XMMatrixScaling(sca, sca, 1) *
			XMMatrixTranslation(translation.x + scale.x * 0.4f, translation.y + scale.y * 0.5f, 0)
			;

		XMFLOAT2 center = XMFLOAT2(translation.x + scale.x * 0.4f, translation.y + scale.y * 0.5f);
		XMFLOAT2 pointer = gui->GetPointerHitbox().pos;
		float distance = wiMath::Distance(center, pointer);
		bool hover_hue = (distance > colorpicker_radius * sca) && (distance < (colorpicker_radius + colorpicker_width) * sca);

		float distTri = 0;
		XMFLOAT4 A, B, C;
		wiMath::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
		XMVECTOR _A = XMLoadFloat4(&A);
		XMVECTOR _B = XMLoadFloat4(&B);
		XMVECTOR _C = XMLoadFloat4(&C);
		XMMATRIX _triTransform = XMMatrixRotationZ(-hue / 360.0f * XM_2PI) * W;
		_A = XMVector4Transform(_A, _triTransform);
		_B = XMVector4Transform(_B, _triTransform);
		_C = XMVector4Transform(_C, _triTransform);
		XMVECTOR O = XMVectorSet(pointer.x, pointer.y, 0, 0);
		XMVECTOR D = XMVectorSet(0, 0, 1, 0);
		bool hover_saturation = TriangleTests::Intersects(O, D, _A, _B, _C, distTri);

		bool dragged = false;

		if (wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
		{
			if (hover_hue)
			{
				colorpickerstate = CPS_HUE;
				dragged = true;
			}
			else if (hover_saturation)
			{
				colorpickerstate = CPS_SATURATION;
				dragged = true;
			}
		}

		if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
		{
			if (colorpickerstate > CPS_IDLE)
			{
				// continue drag if already grabbed whether it is intersecting or not
				dragged = true;
			}
		}
		else
		{
			colorpickerstate = CPS_IDLE;
		}

		dragged = dragged && !gui->IsWidgetDisabled(this);
		if (colorpickerstate == CPS_HUE && dragged)
		{
			//hue pick
			const float angle = wiMath::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(colorpicker_radius, 0));
			hue = angle / XM_2PI * 360.0f;
			gui->ActivateWidget(this);
		}
		else if (colorpickerstate == CPS_SATURATION && dragged)
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
}
void wiColorPicker::Render(const wiGUI* gui, CommandList cmd) const
{
	wiWindow::Render(gui, cmd);

	if (!IsVisible() || IsMinimized())
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
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
			device->CreateBuffer(&desc, &data, &vb_hue);
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
			device->CreateBuffer(&desc, &data, &vb_picker_saturation);
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
			device->CreateBuffer(&desc, &data, &vb_picker_hue);
		}
		// preview
		{
			float _width = 20;
			Vertex vertices[] = {
				{ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(-_width, _width, 0, 1),XMFLOAT4(1,1,1,1) },
				{ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) },
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
			device->CreateBuffer(&desc, &data, &vb_preview);
		}

	}

	const wiColor final_color = GetPickColor();
	const float angle = hue / 360.0f * XM_2PI;

	const XMMATRIX Projection = device->GetScreenProjection();

	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CBSLOT_RENDERER_MISC, cmd);
	device->BindPipelineState(&PSO_colored, cmd);

	ApplyScissor(scissorRect, cmd);

	float sca = std::min(scale.x / cp_width, scale.y / cp_height);

	XMMATRIX W =
		XMMatrixScaling(sca, sca, 1) *
		XMMatrixTranslation(translation.x + scale.x * 0.4f, translation.y + scale.y * 0.5f, 0)
		;

	MiscCB cb;

	// render saturation triangle
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
		
		size_t alloc_size = sizeof(Vertex) * vertices_saturation.size();
		GraphicsDevice::GPUAllocation vb_saturation = device->AllocateGPU(alloc_size, cmd);
		memcpy(vb_saturation.data, vertices_saturation.data(), alloc_size);

		XMStoreFloat4x4(&cb.g_xTransform, 
			XMMatrixRotationZ(-angle) *
			W *
			Projection
		);
		cb.g_xColor = IsEnabled() ? float4(1, 1, 1, 1) : float4(0.5f, 0.5f, 0.5f, 1);
		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			vb_saturation.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint32_t offsets[] = {
			vb_saturation.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
		device->Draw((uint32_t)vertices_saturation.size(), 0, cmd);
	}

	// render hue circle
	{
		XMStoreFloat4x4(&cb.g_xTransform,
			W *
			Projection
		);
		cb.g_xColor = IsEnabled() ? float4(1, 1, 1, 1) : float4(0.5f, 0.5f, 0.5f, 1);
		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_hue,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->Draw(vb_hue.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render hue picker
	if(IsEnabled())
	{
		XMStoreFloat4x4(&cb.g_xTransform,
			XMMatrixRotationZ(-hue / 360.0f * XM_2PI)*
			W *
			Projection
		);

		hsv source;
		source.h = hue;
		source.s = 1;
		source.v = 1;
		rgb result = hsv2rgb(source);
		cb.g_xColor = float4(1 - result.r, 1 - result.g, 1 - result.b, 1);

		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_picker_hue,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->Draw(vb_picker_hue.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render saturation picker
	if (IsEnabled())
	{
		XMFLOAT4 A, B, C;
		wiMath::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
		XMVECTOR _A = XMLoadFloat4(&A);
		XMVECTOR _B = XMLoadFloat4(&B);
		XMVECTOR _C = XMLoadFloat4(&C);
		XMMATRIX _triTransform = XMMatrixRotationZ(-hue / 360.0f * XM_2PI);
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
			W *
			Projection
		);
		cb.g_xColor = float4(1 - final_color.toFloat3().x, 1 - final_color.toFloat3().y, 1 - final_color.toFloat3().z, 1);
		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_picker_saturation,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->Draw(vb_picker_saturation.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}

	// render preview
	{
		XMStoreFloat4x4(&cb.g_xTransform, 
			XMMatrixScaling(sca, sca, 1) *
			XMMatrixTranslation(translation.x + scale.x - 40 * sca, translation.y + 40, 0) *
			Projection
		);
		cb.g_xColor = final_color.toFloat4();
		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &cb, cmd);
		const GPUBuffer* vbs[] = {
			&vb_preview,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->Draw(vb_preview.GetDesc().ByteWidth / sizeof(Vertex), 0, cmd);
	}
}
wiColor wiColorPicker::GetPickColor() const
{
	hsv source;
	source.h = hue;
	source.s = saturation;
	source.v = luminance;
	rgb result = hsv2rgb(source);
	return wiColor::fromFloat4(XMFLOAT4(result.r, result.g, result.b, alphaSlider->GetValue() / 255.0f));
}
void wiColorPicker::SetPickColor(wiColor value)
{
	if (colorpickerstate != CPS_IDLE)
	{
		// Only allow setting the pick color when picking is not active, the RGB and HSV precision combat each other
		return;
	}

	rgb source;
	source.r = value.toFloat3().x;
	source.g = value.toFloat3().y;
	source.b = value.toFloat3().z;
	hsv result = rgb2hsv(source);
	if (value.getR() != value.getG() || value.getG() != value.getB() || value.getR() != value.getB())
	{
		hue = result.h; // only change the hue when not pure greyscale because those don't retain the saturation value
	}
	saturation = result.s;
	luminance = result.v;
	alphaSlider->SetValue((float)value.getA());
}
void wiColorPicker::FireEvents()
{
	if (onColorChanged == nullptr)
		return;
	wiEventArgs args;
	args.color = GetPickColor();
	onColorChanged(args);
}
void wiColorPicker::OnColorChanged(function<void(wiEventArgs args)> func)
{
	onColorChanged = move(func);
}






inline float item_height() { return 20.0f; }
inline float tree_scrollbar_width() { return 12.0f; }
wiTreeList::wiTreeList(const std::string& name)
{
	SetName(name);
	SetText(name);
	OnSelect([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 20));

	for (int i = FOCUS + 1; i < WIDGETSTATE_COUNT; ++i)
	{
		sprites[i].params.color = sprites[FOCUS].params.color;
	}
	font.params.v_align = WIFALIGN_CENTER;
}
wiTreeList::~wiTreeList()
{

}
float wiTreeList::GetItemOffset(int index) const
{
	return 2 + list_offset + index * item_height();
}
Hitbox2D wiTreeList::GetHitbox_ListArea() const
{
	return Hitbox2D(XMFLOAT2(translation.x, translation.y + item_height() + 1), XMFLOAT2(scale.x - tree_scrollbar_width() - 1, scale.y - item_height() - 1));
}
Hitbox2D wiTreeList::GetHitbox_Item(int visible_count, int level) const
{
	XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height(), translation.y + GetItemOffset(visible_count) + item_height() * 0.5f);
	Hitbox2D hitbox;
	hitbox.pos = XMFLOAT2(pos.x + item_height() * 0.5f + 2, pos.y - item_height() * 0.5f);
	hitbox.siz = XMFLOAT2(scale.x - 2 - item_height() * 0.5f - 2 - level * item_height() - tree_scrollbar_width() - 2, item_height());
	return hitbox;
}
Hitbox2D wiTreeList::GetHitbox_ItemOpener(int visible_count, int level) const
{
	XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height(), translation.y + GetItemOffset(visible_count) + item_height() * 0.5f);
	return Hitbox2D(XMFLOAT2(pos.x, pos.y - item_height() * 0.25f), XMFLOAT2(item_height() * 0.5f, item_height() * 0.5f));
}
bool wiTreeList::HasScrollbar() const
{
	return scale.y < (int)items.size() * item_height();
}
void wiTreeList::Update(wiGUI* gui, float dt)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiWidget::Update(gui, dt);

	if (IsEnabled() && !gui->IsWidgetDisabled(this))
	{
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
		const Hitbox2D& pointerHitbox = gui->GetPointerHitbox();

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
			if (state == FOCUS || state == DEACTIVATING)
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
				list_height += item_height();
			}
		}

		const float scrollbar_begin = translation.y + item_height();
		const float scrollbar_end = scrollbar_begin + scale.y - item_height();
		const float scrollbar_size = scrollbar_end - scrollbar_begin;
		const float scrollbar_granularity = std::min(1.0f, scrollbar_size / list_height);
		scrollbar_height = std::max(tree_scrollbar_width(), scrollbar_size * scrollbar_granularity);

		if (!click_down)
		{
			scrollbar_state = SCROLLBAR_INACTIVE;
		}

		Hitbox2D scroll_box;
		scroll_box.pos = XMFLOAT2(translation.x + scale.x - tree_scrollbar_width(), translation.y + item_height() + 1);
		scroll_box.siz = XMFLOAT2(tree_scrollbar_width(), scale.y - item_height() - 1);
		if (scroll_box.intersects(pointerHitbox))
		{
			if (clicked)
			{
				scrollbar_state = SCROLLBAR_GRABBED;
			}
			else if (!click_down)
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
			scrollbar_delta -= wiInput::GetPointer().z * 10;
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

	sprites[state].params.siz.y = item_height();
	font.params.posX = translation.x + 2;
	font.params.posY = translation.y + sprites[state].params.siz.y * 0.5f;
}
void wiTreeList::Render(const wiGUI* gui, CommandList cmd) const
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}
	GraphicsDevice* device = wiRenderer::GetDevice();

	// control-base
	sprites[state].Draw(cmd);

	ApplyScissor(scissorRect, cmd);

	font.Draw(cmd);

	// scrollbar background
	wiImageParams fx = sprites[state].params;
	fx.pos = XMFLOAT3(translation.x + scale.x - tree_scrollbar_width(), translation.y + item_height() + 1, 0);
	fx.siz = XMFLOAT2(tree_scrollbar_width(), scale.y - item_height() - 1);
	fx.color = sprites[IDLE].params.color;
	wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

	// scrollbar
	wiColor scrollbar_color = wiColor::fromFloat4(sprites[IDLE].params.color);
	if (scrollbar_state == SCROLLBAR_HOVER)
	{
		scrollbar_color = wiColor::fromFloat4(sprites[FOCUS].params.color);
	}
	else if (scrollbar_state == SCROLLBAR_GRABBED)
	{
		scrollbar_color = wiColor::White();
	}
	wiImage::Draw(wiTextureHelper::getWhite()
		, wiImageParams(translation.x + scale.x - tree_scrollbar_width(), translation.y + item_height() + 1 + scrollbar_delta, tree_scrollbar_width(), scrollbar_height, scrollbar_color), cmd);

	// list background
	Hitbox2D itemlist_box = GetHitbox_ListArea();
	fx.pos = XMFLOAT3(itemlist_box.pos.x, itemlist_box.pos.y, 0);
	fx.siz = XMFLOAT2(itemlist_box.siz.x, itemlist_box.siz.y);
	wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

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
					sprites[item.selected ? FOCUS : IDLE].params.color), cmd);
		}
		
		// opened flag triangle:
		{
			device->BindPipelineState(&PSO_colored, cmd);

			MiscCB cb;
			cb.g_xColor = opener_highlight == i ? wiColor::White().toFloat4() : sprites[FOCUS].params.color;
			XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(item_height() * 0.3f, item_height() * 0.3f, 1) * 
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
		wiFont::Draw(item.name, wiFontParams(name_box.pos.x + 1, name_box.pos.y + name_box.siz.y * 0.5f, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_CENTER,
			font.params.color, font.params.shadowColor), cmd);
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
