#include "wiGUI.h"
#include "wiInput.h"
#include "wiPrimitive.h"
#include "wiProfiler.h"
#include "wiRenderer.h"
#include "wiTimer.h"
#include "wiEventHandler.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiBacklog.h"

#include <sstream>

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::gui
{
	struct InternalState
	{
		wi::graphics::PipelineState PSO_colored;

		InternalState()
		{
			wi::Timer timer;

			static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [this](uint64_t userdata) { LoadShaders(); });
			LoadShaders();

			wi::backlog::post("wi::gui Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}

		void LoadShaders()
		{
			PipelineStateDesc desc;
			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
			desc.pt = PrimitiveTopology::TRIANGLESTRIP;
			wi::graphics::GetDevice()->CreatePipelineState(&desc, &PSO_colored);
		}
	};
	inline InternalState& gui_internal()
	{
		static InternalState internal_state;
		return internal_state;
	}


	void GUI::Update(const wi::Canvas& canvas, float dt)
	{
		if (!visible)
		{
			return;
		}

		auto range = wi::profiler::BeginRangeCPU("GUI Update");

		XMFLOAT4 pointer = wi::input::GetPointer();
		Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));

		uint32_t priority = 0;

		focus = false;
		for (auto& widget : widgets)
		{
			widget->force_disable = focus;
			widget->Update(canvas, dt);
			widget->force_disable = false;

			if (widget->priority_change)
			{
				widget->priority_change = false;
				widget->priority = priority++;
			}
			else
			{
				widget->priority = ~0u;
			}

			if (widget->IsVisible() && widget->hitBox.intersects(pointerHitbox))
			{
				focus = true;
			}
			if (widget->GetState() > IDLE)
			{
				focus = true;
			}
		}

		//Sort only if there are priority changes
		if (priority > 0)
		{
			//Use std::stable_sort instead of std::sort to preserve UI element order with equal priorities
			std::stable_sort(widgets.begin(), widgets.end(), [](const Widget* a, const Widget* b) {
				return a->priority < b->priority;
				});
		}

		wi::profiler::EndRange(range);
	}
	void GUI::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!visible)
		{
			return;
		}

		auto range_cpu = wi::profiler::BeginRangeCPU("GUI Render");
		auto range_gpu = wi::profiler::BeginRangeGPU("GUI Render", cmd);

		Rect scissorRect;
		scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
		scissorRect.left = (int32_t)(0);
		scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
		scissorRect.top = (int32_t)(0);

		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("GUI", cmd);
		// Rendering is back to front:
		for (size_t i = 0; i < widgets.size(); ++i)
		{
			const Widget* widget = widgets[widgets.size() - i - 1];
			device->BindScissorRects(1, &scissorRect, cmd);
			widget->Render(canvas, cmd);
		}

		device->BindScissorRects(1, &scissorRect, cmd);
		for (auto& x : widgets)
		{
			x->RenderTooltip(canvas, cmd);
		}

		device->EventEnd(cmd);

		wi::profiler::EndRange(range_cpu);
		wi::profiler::EndRange(range_gpu);
	}
	void GUI::AddWidget(Widget* widget)
	{
		if (widget != nullptr)
		{
			assert(std::find(widgets.begin(), widgets.end(), widget) == widgets.end()); // don't attach one widget twice!
			widgets.push_back(widget);
		}
	}
	void GUI::RemoveWidget(Widget* widget)
	{
		for (auto& x : widgets)
		{
			if (x == widget)
			{
				x = widgets.back();
				widgets.pop_back();
				break;
			}
		}
	}
	Widget* GUI::GetWidget(const std::string& name)
	{
		for (auto& x : widgets)
		{
			if (x->GetName() == name)
			{
				return x;
			}
		}
		return nullptr;
	}
	bool GUI::HasFocus()
	{
		if (!visible)
		{
			return false;
		}

		return focus;
	}



	Widget::Widget()
	{
		sprites[IDLE].params.color = wi::Color::Booger();
		sprites[FOCUS].params.color = wi::Color::Gray();
		sprites[ACTIVE].params.color = wi::Color::White();
		sprites[DEACTIVATING].params.color = wi::Color::Gray();
		font.params.shadowColor = wi::Color::Black();

		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites[i].params.blendFlag = wi::enums::BLENDMODE_OPAQUE;
			sprites[i].params.enableBackground();
		}
	}
	void Widget::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		if (force_disable)
		{
			state = IDLE;
		}

		hitBox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));

		if (!force_disable && GetState() != WIDGETSTATE::ACTIVE && !tooltip.empty() && GetPointerHitbox().intersects(hitBox))
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
			sprites[i].params.fade = enabled ? 0.0f : 0.5f;
		}
		font.params.posX = translation.x;
		font.params.posY = translation.y;
	}
	void Widget::RenderTooltip(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		if (tooltipTimer > 25)
		{
			float screenwidth = canvas.GetLogicalWidth();
			float screenheight = canvas.GetLogicalHeight();

			wi::font::Params fontProps = wi::font::Params(0, 0, wi::font::WIFONTSIZE_DEFAULT, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_TOP);
			fontProps.color = wi::Color(25, 25, 25, 255);
			wi::SpriteFont tooltipFont = wi::SpriteFont(tooltip, fontProps);
			if (!scriptTip.empty())
			{
				tooltipFont.SetText(tooltip + "\n" + scriptTip);
			}

			static const float _border = 2;
			float textWidth = tooltipFont.TextWidth() + _border * 2;
			float textHeight = tooltipFont.TextHeight() + _border * 2;

			XMFLOAT2 pointer = GetPointerHitbox().pos;
			tooltipFont.params.posX = pointer.x;
			tooltipFont.params.posY = pointer.y;

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

			wi::image::Draw(wi::texturehelper::getWhite(),
				wi::image::Params(tooltipFont.params.posX - _border, tooltipFont.params.posY - _border,
					textWidth, textHeight, wi::Color(255, 234, 165)), cmd);
			tooltipFont.SetText(tooltip);
			tooltipFont.Draw(cmd);
			if (!scriptTip.empty())
			{
				tooltipFont.SetText(scriptTip);
				tooltipFont.params.posY += (int)(textHeight / 2);
				tooltipFont.params.color = wi::Color(25, 25, 25, 110);
				tooltipFont.Draw(cmd);
			}
		}
	}
	const std::string& Widget::GetName() const
	{
		return name;
	}
	void Widget::SetName(const std::string& value)
	{
		if (value.length() <= 0)
		{
			static std::atomic<uint32_t> widgetID{ 0 };
			name = "widget_" + std::to_string(widgetID.fetch_add(1));
		}
		else
		{
			name = value;
		}
	}
	const std::string Widget::GetText() const
	{
		return font.GetTextA();
	}
	void Widget::SetText(const std::string& value)
	{
		font.SetText(value);
	}
	void Widget::SetText(std::string&& value)
	{
		font.SetText(std::move(value));
	}
	void Widget::SetTooltip(const std::string& value)
	{
		tooltip = value;
	}
	void Widget::SetTooltip(std::string&& value)
	{
		tooltip = std::move(value);
	}
	void Widget::SetScriptTip(const std::string& value)
	{
		scriptTip = value;
	}
	void Widget::SetScriptTip(std::string&& value)
	{
		scriptTip = std::move(value);
	}
	void Widget::SetPos(const XMFLOAT2& value)
	{
		SetDirty();
		translation_local.x = value.x;
		translation_local.y = value.y;
		UpdateTransform();

		translation = translation_local;
	}
	void Widget::SetSize(const XMFLOAT2& value)
	{
		SetDirty();
		scale_local.x = value.x;
		scale_local.y = value.y;
		UpdateTransform();

		scale = scale_local;
	}
	WIDGETSTATE Widget::GetState() const
	{
		return state;
	}
	void Widget::SetEnabled(bool val)
	{
		if (enabled != val)
		{
			priority_change = val;
			enabled = val;
		}
	}
	bool Widget::IsEnabled() const
	{
		return enabled && visible && !force_disable;
	}
	void Widget::SetVisible(bool val)
	{
		if (visible != val)
		{
			priority_change |= val;
			visible = val;
		}
	}
	bool Widget::IsVisible() const
	{
		return visible;
	}
	void Widget::Activate()
	{
		priority_change = true;
		state = ACTIVE;
		tooltipTimer = 0;
	}
	void Widget::Deactivate()
	{
		state = DEACTIVATING;
		tooltipTimer = 0;
	}
	void Widget::SetColor(wi::Color color, WIDGETSTATE state)
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
	wi::Color Widget::GetColor() const
	{
		return wi::Color::fromFloat4(sprites[GetState()].params.color);
	}
	void Widget::SetImage(wi::Resource textureResource, WIDGETSTATE state)
	{
		if (state == WIDGETSTATE_COUNT)
		{
			for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
			{
				sprites[i].textureResource = textureResource;
			}
		}
		else
		{
			sprites[state].textureResource = textureResource;
		}
	}

	void Widget::AttachTo(Widget* parent)
	{
		this->parent = parent;

		if (parent != nullptr)
		{
			parent->UpdateTransform();
			XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&parent->world));
			MatrixTransform(B);
		}

		UpdateTransform();

		if (parent != nullptr)
		{
			UpdateTransform_Parented(*parent);
		}
	}
	void Widget::Detach()
	{
		if (parent != nullptr)
		{
			parent = nullptr;
			ApplyTransform();
		}
	}
	void Widget::ApplyScissor(const wi::Canvas& canvas, const Rect rect, CommandList cmd, bool constrain_to_parent) const
	{
		Rect scissor = rect;

		if (constrain_to_parent && parent != nullptr)
		{
			Widget* recurse_parent = parent;
			while (recurse_parent != nullptr)
			{
				scissor.bottom = std::min(scissor.bottom, recurse_parent->scissorRect.bottom);
				scissor.top = std::max(scissor.top, recurse_parent->scissorRect.top);
				scissor.left = std::max(scissor.left, recurse_parent->scissorRect.left);
				scissor.right = std::min(scissor.right, recurse_parent->scissorRect.right);

				recurse_parent = recurse_parent->parent;
			}
		}

		if (scissor.left > scissor.right)
		{
			scissor.left = scissor.right;
		}
		if (scissor.top > scissor.bottom)
		{
			scissor.top = scissor.bottom;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		float scale = canvas.GetDPIScaling();
		scissor.bottom = int32_t((float)scissor.bottom * scale);
		scissor.top = int32_t((float)scissor.top * scale);
		scissor.left = int32_t((float)scissor.left * scale);
		scissor.right = int32_t((float)scissor.right * scale);
		device->BindScissorRects(1, &scissor, cmd);
	}
	Hitbox2D Widget::GetPointerHitbox() const
	{
		XMFLOAT4 pointer = wi::input::GetPointer();
		return Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));
	}




	void Button::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnClick([](EventArgs args) {});
		OnDragStart([](EventArgs args) {});
		OnDrag([](EventArgs args) {});
		OnDragEnd([](EventArgs args) {});
		SetSize(XMFLOAT2(100, 30));

		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_CENTER;
	}
	void Button::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		if (IsEnabled())
		{
			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x;
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();

			if (state == FOCUS)
			{
				state = IDLE;
			}
			if (state == DEACTIVATING)
			{
				EventArgs args;
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
				Deactivate();
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

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == FOCUS)
				{
					// activate
					clicked = true;
				}
			}

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();

					EventArgs args;
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
				EventArgs args;
				args.clickPos = pointerHitbox.pos;
				dragStart = args.clickPos;
				args.startPos = dragStart;
				onDragStart(args);
				Activate();
			}

			prevPos.x = pointerHitbox.pos.x;
			prevPos.y = pointerHitbox.pos.y;
		}

		switch (font.params.h_align)
		{
		case wi::font::WIFALIGN_LEFT:
			font.params.posX = translation.x + 2;
			break;
		case wi::font::WIFALIGN_RIGHT:
			font.params.posX = translation.x + scale.x - 2;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posX = translation.x + scale.x * 0.5f;
			break;
		}
		switch (font.params.v_align)
		{
		case wi::font::WIFALIGN_TOP:
			font.params.posY = translation.y + 2;
			break;
		case wi::font::WIFALIGN_BOTTOM:
			font.params.posY = translation.y + scale.y - 2;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posY = translation.y + scale.y * 0.5f;
			break;
		}
	}
	void Button::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		wi::Color color = GetColor();

		ApplyScissor(canvas, scissorRect, cmd);

		sprites[state].Draw(cmd);
		font.Draw(cmd);
	}
	void Button::OnClick(std::function<void(EventArgs args)> func)
	{
		onClick = move(func);
	}
	void Button::OnDragStart(std::function<void(EventArgs args)> func)
	{
		onDragStart = move(func);
	}
	void Button::OnDrag(std::function<void(EventArgs args)> func)
	{
		onDrag = move(func);
	}
	void Button::OnDragEnd(std::function<void(EventArgs args)> func)
	{
		onDragEnd = move(func);
	}




	void Label::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		SetSize(XMFLOAT2(100, 20));
	}
	void Label::Update(const wi::Canvas& canvas, float dt)
	{
		Widget::Update(canvas, dt);

		font.params.h_wrap = scale.x;

		switch (font.params.h_align)
		{
		case wi::font::WIFALIGN_LEFT:
			font.params.posX = translation.x + 2;
			break;
		case wi::font::WIFALIGN_RIGHT:
			font.params.posX = translation.x + scale.x - 2;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posX = translation.x + scale.x * 0.5f;
			break;
		}
		switch (font.params.v_align)
		{
		case wi::font::WIFALIGN_TOP:
			font.params.posY = translation.y + 2;
			break;
		case wi::font::WIFALIGN_BOTTOM:
			font.params.posY = translation.y + scale.y - 2;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posY = translation.y + scale.y * 0.5f;
			break;
		}
	}
	void Label::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		wi::Color color = GetColor();

		ApplyScissor(canvas, scissorRect, cmd);

		sprites[state].Draw(cmd);
		font.Draw(cmd);
	}




	wi::SpriteFont TextInputField::font_input;
	void TextInputField::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnInputAccepted([](EventArgs args) {});
		SetSize(XMFLOAT2(100, 30));

		font.params.v_align = wi::font::WIFALIGN_CENTER;

		font_description.params = font.params;
		font_description.params.h_align = wi::font::WIFALIGN_RIGHT;
	}
	void TextInputField::SetValue(const std::string& newValue)
	{
		font.SetText(newValue);
	}
	void TextInputField::SetValue(int newValue)
	{
		std::stringstream ss("");
		ss << newValue;
		font.SetText(ss.str());
	}
	void TextInputField::SetValue(float newValue)
	{
		std::stringstream ss("");
		ss << newValue;
		font.SetText(ss.str());
	}
	const std::string TextInputField::GetValue()
	{
		return font.GetTextA();
	}
	void TextInputField::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		if (IsEnabled())
		{
			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x;
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();
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

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == FOCUS)
				{
					// activate
					clicked = true;
				}
			}

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
				}
			}

			if (clicked)
			{
				Activate();

				font_input.SetText(font.GetText());
			}

			if (state == ACTIVE)
			{
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_ENTER))
				{
					// accept input...

					font.SetText(font_input.GetText());
					font_input.text.clear();

					EventArgs args;
					args.sValue = font.GetTextA();
					args.iValue = atoi(args.sValue.c_str());
					args.fValue = (float)atof(args.sValue.c_str());
					onInputAccepted(args);

					Deactivate();
				}
				else if ((wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) && !intersectsPointer) ||
					wi::input::Press(wi::input::KEYBOARD_BUTTON_ESCAPE))
				{
					// cancel input 
					font_input.text.clear();
					Deactivate();
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
	void TextInputField::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		wi::Color color = GetColor();

		font_description.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

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
	void TextInputField::OnInputAccepted(std::function<void(EventArgs args)> func)
	{
		onInputAccepted = move(func);
	}
	void TextInputField::AddInput(const char inputChar)
	{
		std::string value_new = font_input.GetTextA();
		value_new.push_back(inputChar);
		font_input.SetText(value_new);
	}
	void TextInputField::DeleteFromInput()
	{
		std::string value_new = font_input.GetTextA();
		if (!value_new.empty())
		{
			value_new.pop_back();
		}
		font_input.SetText(value_new);
	}





	void Slider::Create(float start, float end, float defaultValue, float step, const std::string& name)
	{
		this->start = start;
		this->end = end;
		this->value = defaultValue;
		this->step = std::max(step, 1.0f);

		SetName(name);
		SetText(name);
		OnSlide([](EventArgs args) {});
		SetSize(XMFLOAT2(200, 40));

		valueInputField.Create(name + "_endInputField");
		valueInputField.SetTooltip("Enter number to modify value even outside slider limits. Enter \"reset\" to reset slider to initial state.");
		valueInputField.SetValue(end);
		valueInputField.OnInputAccepted([this, start, end, defaultValue](EventArgs args) {
			if (args.sValue.compare("reset") != std::string::npos)
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

		sprites[IDLE].params.color = wi::Color(60, 60, 60, 200);
		sprites[FOCUS].params.color = wi::Color(50, 50, 50, 200);
		sprites[ACTIVE].params.color = wi::Color(50, 50, 50, 200);
		sprites[DEACTIVATING].params.color = wi::Color(60, 60, 60, 200);

		font.params.h_align = wi::font::WIFALIGN_RIGHT;
		font.params.v_align = wi::font::WIFALIGN_CENTER;
	}
	void Slider::SetValue(float value)
	{
		this->value = value;
	}
	float Slider::GetValue() const
	{
		return value;
	}
	void Slider::SetRange(float start, float end)
	{
		this->start = start;
		this->end = end;
		this->value = wi::math::Clamp(this->value, start, end);
	}
	void Slider::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		valueInputField.Detach();
		valueInputField.SetSize(XMFLOAT2(std::max(scale.y * 2, wi::font::TextWidth(valueInputField.GetText(), valueInputField.font.params) + 4), scale.y));
		valueInputField.SetPos(XMFLOAT2(translation.x + scale.x + 2, translation.y));
		valueInputField.AttachTo(this);

		scissorRect.bottom = (int32_t)(translation.y + scale.y);
		scissorRect.left = (int32_t)(translation.x);
		scissorRect.right = (int32_t)(translation.x + scale.x + 20 + scale.y * 2); // include the valueInputField
		scissorRect.top = (int32_t)(translation.y);

		for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites_knob[i].params.siz.x = 16.0f;
			valueInputField.SetColor(wi::Color::fromFloat4(this->sprites_knob[i].params.color), (WIDGETSTATE)i);
		}
		valueInputField.font.params.color = this->font.params.color;
		valueInputField.font.params.shadowColor = this->font.params.shadowColor;
		valueInputField.SetEnabled(enabled);
		valueInputField.force_disable = force_disable;
		valueInputField.Update(canvas, dt);

		if (IsEnabled())
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
				if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
				{
					if (state == ACTIVE)
					{
						// continue drag if already grabbed wheter it is intersecting or not
						dragged = true;
					}
				}
				else
				{
					Deactivate();
				}
			}

			const float knobWidth = sprites_knob[state].params.siz.x;
			hitBox.pos.x = translation.x - knobWidth * 0.5f;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x + knobWidth;
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();


			if (pointerHitbox.intersects(hitBox))
			{
				// hover the slider
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == FOCUS)
				{
					// activate
					dragged = true;
				}
			}


			if (dragged)
			{
				EventArgs args;
				args.clickPos = pointerHitbox.pos;
				value = wi::math::InverseLerp(translation.x, translation.x + scale.x, args.clickPos.x);
				value = wi::math::Clamp(value, 0, 1);
				value *= step;
				value = std::floor(value);
				value /= step;
				value = wi::math::Lerp(start, end, value);
				args.fValue = value;
				args.iValue = (int)value;
				onSlide(args);
				Activate();
			}

			valueInputField.SetValue(value);
		}

		font.params.posY = translation.y + scale.y * 0.5f;

		const float knobWidth = sprites_knob[state].params.siz.x;
		const float knobWidthHalf = knobWidth * 0.5f;
		sprites_knob[state].params.pos.x = wi::math::Lerp(translation.x + knobWidthHalf + 2, translation.x + scale.x - knobWidthHalf - 2, wi::math::Clamp(wi::math::InverseLerp(start, end, value), 0, 1));
		sprites_knob[state].params.pos.y = translation.y + 2;
		sprites_knob[state].params.siz.y = scale.y - 4;
		sprites_knob[state].params.pivot = XMFLOAT2(0.5f, 0);
		sprites_knob[state].params.fade = sprites[state].params.fade;
	}
	void Slider::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		font.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		// base
		sprites[state].Draw(cmd);

		// knob
		sprites_knob[state].Draw(cmd);

		valueInputField.Render(canvas, cmd);
	}
	void Slider::RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
	{
		Widget::RenderTooltip(canvas, cmd);
		valueInputField.RenderTooltip(canvas, cmd);
	}
	void Slider::OnSlide(std::function<void(EventArgs args)> func)
	{
		onSlide = move(func);
	}





	void CheckBox::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnClick([](EventArgs args) {});
		SetSize(XMFLOAT2(20, 20));

		font.params.h_align = wi::font::WIFALIGN_RIGHT;
		font.params.v_align = wi::font::WIFALIGN_CENTER;

		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites_check[i].params = sprites[i].params;
			sprites_check[i].params.color = wi::math::Lerp(sprites[i].params.color, wi::Color::White().toFloat4(), 0.8f);
		}
	}
	void CheckBox::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		if (IsEnabled())
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
				Deactivate();
			}

			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x;
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();

			// hover the button
			if (pointerHitbox.intersects(hitBox))
			{
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == FOCUS)
				{
					Activate();
				}
			}

			if (state == DEACTIVATING)
			{
				if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
				{
					// Keep pressed until mouse is released
					Activate();
				}
				else
				{
					// Deactivation event
					SetCheck(!GetCheck());
					EventArgs args;
					args.clickPos = pointerHitbox.pos;
					args.bValue = GetCheck();
					onClick(args);
				}
			}
		}

		font.params.posY = translation.y + scale.y * 0.5f;

		sprites_check[state].params.pos.x = translation.x + scale.x * 0.25f;
		sprites_check[state].params.pos.y = translation.y + scale.y * 0.25f;
		sprites_check[state].params.siz = XMFLOAT2(scale.x * 0.5f, scale.y * 0.5f);
	}
	void CheckBox::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		font.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		// control
		sprites[state].Draw(cmd);

		// check
		if (GetCheck())
		{
			sprites_check[state].Draw(cmd);
		}

	}
	void CheckBox::OnClick(std::function<void(EventArgs args)> func)
	{
		onClick = move(func);
	}
	void CheckBox::SetCheck(bool value)
	{
		checked = value;
	}
	bool CheckBox::GetCheck() const
	{
		return checked;
	}





	void ComboBox::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnSelect([](EventArgs args) {});
		SetSize(XMFLOAT2(100, 20));

		font.params.h_align = wi::font::WIFALIGN_RIGHT;
		font.params.v_align = wi::font::WIFALIGN_CENTER;
	}
	float ComboBox::GetItemOffset(int index) const
	{
		index = std::max(firstItemVisible, index) - firstItemVisible;
		return scale.y * (index + 1) + 1;
	}
	bool ComboBox::HasScrollbar() const
	{
		return maxVisibleItemCount < (int)items.size();
	}
	void ComboBox::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		if (IsEnabled())
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
				Deactivate();
			}
			if (state == IDLE)
			{
				combostate = COMBOSTATE_INACTIVE;
			}

			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x + scale.y + 1; // + drop-down indicator arrow + little offset
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();

			bool clicked = false;
			// hover the button
			if (pointerHitbox.intersects(hitBox))
			{
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				// activate
				clicked = true;
			}

			bool click_down = false;
			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				click_down = true;
				if (state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
				}
			}


			if (clicked && state == FOCUS)
			{
				Activate();
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
								scrollbar_delta = wi::math::Clamp(pointerHitbox.pos.y, scrollbar_begin, scrollbar_end) - scrollbar_begin;
								const float scrollbar_value = wi::math::InverseLerp(scrollbar_begin, scrollbar_end, scrollbar_begin + scrollbar_delta);
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
					Deactivate();
					combostate = COMBOSTATE_INACTIVE;
				}
				else if (combostate == COMBOSTATE_HOVER)
				{
					int scroll = (int)wi::input::GetPointer().z;
					firstItemVisible -= scroll;
					firstItemVisible = std::max(0, std::min((int)items.size() - maxVisibleItemCount, firstItemVisible));
					if (scroll)
					{
						const float scrollbar_value = wi::math::InverseLerp(0, float(std::max(0, (int)items.size() - maxVisibleItemCount)), float(firstItemVisible));
						scrollbar_delta = wi::math::Lerp(scrollbar_begin, scrollbar_end, scrollbar_value) - scrollbar_begin;
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

		selected = std::min((int)items.size(), selected);
	}
	void ComboBox::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}
		GraphicsDevice* device = wi::graphics::GetDevice();

		wi::Color color = GetColor();
		if (combostate != COMBOSTATE_INACTIVE)
		{
			color = wi::Color::fromFloat4(sprites[FOCUS].params.color);
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
			wi::math::ConstructTriangleEquilateral(1, vertices[0].pos, vertices[1].pos, vertices[2].pos);

			GPUBufferDesc desc;
			desc.bind_flags = BindFlag::VERTEX_BUFFER;
			desc.size = sizeof(vertices);
			device->CreateBuffer(&desc, &vertices, &vb_triangle);
		}
		const XMMATRIX Projection = canvas.GetProjection();

		// control-arrow-background
		wi::image::Params fx = sprites[state].params;
		fx.pos = XMFLOAT3(translation.x + scale.x + 1, translation.y, 0);
		fx.siz = XMFLOAT2(scale.y, scale.y);
		wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

		// control-arrow-triangle
		{
			device->BindPipelineState(&gui_internal().PSO_colored, cmd);

			MiscCB cb;
			cb.g_xColor = sprites[ACTIVE].params.color;
			XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(scale.y * 0.25f, scale.y * 0.25f, 1) *
				XMMatrixRotationZ(XM_PIDIV2) *
				XMMatrixTranslation(translation.x + scale.x + 1 + scale.y * 0.5f, translation.y + scale.y * 0.5f, 0) *
				Projection
			);
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_triangle,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);

			device->Draw(3, 0, cmd);
		}

		ApplyScissor(canvas, scissorRect, cmd);

		// control-base
		sprites[state].Draw(cmd);

		if (selected >= 0)
		{
			wi::font::Draw(items[selected].name, wi::font::Params(translation.x + scale.x * 0.5f, translation.y + scale.y * 0.5f, wi::font::WIFONTSIZE_DEFAULT, wi::font::WIFALIGN_CENTER, wi::font::WIFALIGN_CENTER,
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
				ApplyScissor(canvas, rect, cmd, false);

				// control-scrollbar-base
				{
					fx = sprites[state].params;
					fx.pos = XMFLOAT3(translation.x + scale.x + 1, translation.y + scale.y + 1, 0);
					fx.siz = XMFLOAT2(scale.y, scale.y * maxVisibleItemCount);
					fx.color = wi::math::Lerp(wi::Color::Transparent(), sprites[IDLE].params.color, 0.5f);
					wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);
				}

				// control-scrollbar-grab
				{
					wi::Color col = wi::Color::fromFloat4(sprites[IDLE].params.color);
					if (combostate == COMBOSTATE_SCROLLBAR_HOVER)
					{
						col = wi::Color::fromFloat4(sprites[FOCUS].params.color);
					}
					else if (combostate == COMBOSTATE_SCROLLBAR_GRABBED)
					{
						col = wi::Color::fromFloat4(sprites[ACTIVE].params.color);
					}
					wi::image::Draw(wi::texturehelper::getWhite()
						, wi::image::Params(translation.x + scale.x + 1, translation.y + scale.y + 1 + scrollbar_delta, scale.y, scale.y, col), cmd);
				}
			}

			Rect rect;
			rect.left = int(translation.x);
			rect.right = rect.left + int(scale.x);
			rect.top = int(translation.y + scale.y + 1);
			rect.bottom = rect.top + int(scale.y * maxVisibleItemCount);
			ApplyScissor(canvas, rect, cmd, false);

			// control-list
			for (int i = firstItemVisible; i < (firstItemVisible + std::min(maxVisibleItemCount, (int)items.size())); ++i)
			{
				fx = sprites[state].params;
				fx.pos = XMFLOAT3(translation.x, translation.y + GetItemOffset(i), 0);
				fx.siz = XMFLOAT2(scale.x, scale.y);
				fx.color = wi::math::Lerp(wi::Color::Transparent(), sprites[IDLE].params.color, 0.5f);
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
				wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);
				wi::font::Draw(items[i].name, wi::font::Params(translation.x + scale.x * 0.5f, translation.y + scale.y * 0.5f + GetItemOffset(i), wi::font::WIFONTSIZE_DEFAULT, wi::font::WIFALIGN_CENTER, wi::font::WIFALIGN_CENTER,
					font.params.color, font.params.shadowColor), cmd);
			}
		}
	}
	void ComboBox::OnSelect(std::function<void(EventArgs args)> func)
	{
		onSelect = move(func);
	}
	void ComboBox::AddItem(const std::string& name, uint64_t userdata)
	{
		items.emplace_back();
		items.back().name = name;
		items.back().userdata = userdata;

		if (selected < 0)
		{
			selected = 0;
		}
	}
	void ComboBox::RemoveItem(int index)
	{
		wi::vector<Item> newItems(0);
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
	void ComboBox::ClearItems()
	{
		items.clear();

		selected = -1;
	}
	void ComboBox::SetMaxVisibleItemCount(int value)
	{
		maxVisibleItemCount = value;
	}
	void ComboBox::SetSelected(int index)
	{
		selected = index;

		if (onSelect != nullptr)
		{
			EventArgs args;
			args.iValue = selected;
			args.sValue = GetItemText(selected);
			args.userdata = GetItemUserData(selected);
			onSelect(args);
		}
	}
	void ComboBox::SetSelectedByUserdata(uint64_t userdata)
	{
		for (int i = 0; i < GetItemCount(); ++i)
		{
			if (userdata == GetItemUserData(i))
			{
				SetSelected(i);
				return;
			}
		}
	}
	std::string ComboBox::GetItemText(int index) const
	{
		if (index >= 0)
		{
			return items[index].name;
		}
		return "";
	}
	uint64_t ComboBox::GetItemUserData(int index) const
	{
		if (index >= 0)
		{
			return items[index].userdata;
		}
		return 0;
	}
	int ComboBox::GetSelected() const
	{
		return selected;
	}






	static const float windowcontrolSize = 20.0f;
	void Window::Create(const std::string& name, bool window_controls)
	{
		SetColor(wi::Color::Ghost());

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
			resizeDragger_UpperLeft.Create(name + "_resize_dragger_upper_left");
			resizeDragger_UpperLeft.SetText("");
			resizeDragger_UpperLeft.OnDrag([this](EventArgs args) {
				auto saved_parent = this->parent;
				this->Detach();
				XMFLOAT2 scaleDiff;
				scaleDiff.x = (scale.x - args.deltaPos.x) / scale.x;
				scaleDiff.y = (scale.y - args.deltaPos.y) / scale.y;
				this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
				this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
				this->scale_local = wi::math::Max(this->scale_local, XMFLOAT3(windowcontrolSize * 3, windowcontrolSize * 2, 1)); // don't allow resize to negative or too small
				this->AttachTo(saved_parent);
				});
			AddWidget(&resizeDragger_UpperLeft);

			// Add a resizer control to the bottom right corner
			resizeDragger_BottomRight.Create(name + "_resize_dragger_bottom_right");
			resizeDragger_BottomRight.SetText("");
			resizeDragger_BottomRight.OnDrag([this](EventArgs args) {
				auto saved_parent = this->parent;
				this->Detach();
				XMFLOAT2 scaleDiff;
				scaleDiff.x = (scale.x + args.deltaPos.x) / scale.x;
				scaleDiff.y = (scale.y + args.deltaPos.y) / scale.y;
				this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
				this->scale_local = wi::math::Max(this->scale_local, XMFLOAT3(windowcontrolSize * 3, windowcontrolSize * 2, 1)); // don't allow resize to negative or too small
				this->AttachTo(saved_parent);
				});
			AddWidget(&resizeDragger_BottomRight);

			// Add a grabber onto the title bar
			moveDragger.Create(name + "_move_dragger");
			moveDragger.SetText(name);
			moveDragger.font.params.h_align = wi::font::WIFALIGN_LEFT;
			moveDragger.OnDrag([this](EventArgs args) {
				auto saved_parent = this->parent;
				this->Detach();
				this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
				this->AttachTo(saved_parent);
				});
			AddWidget(&moveDragger);

			// Add close button to the top right corner
			closeButton.Create(name + "_close_button");
			closeButton.SetText("x");
			closeButton.OnClick([this](EventArgs args) {
				this->SetVisible(false);
				});
			closeButton.SetTooltip("Close window");
			AddWidget(&closeButton);

			// Add minimize button to the top right corner
			minimizeButton.Create(name + "_minimize_button");
			minimizeButton.SetText("-");
			minimizeButton.OnClick([this](EventArgs args) {
				this->SetMinimized(!this->IsMinimized());
				});
			minimizeButton.SetTooltip("Minimize window");
			AddWidget(&minimizeButton);
		}
		else
		{
			// Simple title bar
			label.Create(name);
			label.SetText(name);
			label.font.params.h_align = wi::font::WIFALIGN_LEFT;
			AddWidget(&label);
		}


		SetEnabled(true);
		SetVisible(true);
		SetMinimized(false);
	}
	void Window::AddWidget(Widget* widget)
	{
		widget->SetEnabled(this->IsEnabled());
		widget->SetVisible(this->IsVisible());
		widget->AttachTo(this);

		widgets.push_back(widget);
	}
	void Window::RemoveWidget(Widget* widget)
	{
		for (auto& x : widgets)
		{
			if (x == widget)
			{
				x = widgets.back();
				widgets.pop_back();
				break;
			}
		}
	}
	void Window::RemoveWidgets()
	{
		widgets.clear();
	}
	void Window::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		moveDragger.force_disable = force_disable;
		resizeDragger_UpperLeft.force_disable = force_disable;
		resizeDragger_BottomRight.force_disable = force_disable;

		moveDragger.Update(canvas, dt);
		resizeDragger_UpperLeft.Update(canvas, dt);
		resizeDragger_BottomRight.Update(canvas, dt);

		// Don't allow moving outside of screen:
		if (parent == nullptr)
		{
			translation_local.x = wi::math::Clamp(translation_local.x, 0, canvas.GetLogicalWidth() - scale_local.x);
			translation_local.y = wi::math::Clamp(translation_local.y, 0, canvas.GetLogicalHeight() - windowcontrolSize);
			SetDirty();
		}

		Widget::Update(canvas, dt);

		if (moveDragger.parent != nullptr)
		{
			moveDragger.Detach();
			moveDragger.SetSize(XMFLOAT2(scale.x - windowcontrolSize * 3, windowcontrolSize));
			moveDragger.SetPos(XMFLOAT2(translation.x + windowcontrolSize, translation.y));
			moveDragger.AttachTo(this);
		}
		if (closeButton.parent != nullptr)
		{
			closeButton.Detach();
			closeButton.SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
			closeButton.SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y));
			closeButton.AttachTo(this);
		}
		if (minimizeButton.parent != nullptr)
		{
			minimizeButton.Detach();
			minimizeButton.SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
			minimizeButton.SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize * 2, translation.y));
			minimizeButton.AttachTo(this);
		}
		if (resizeDragger_UpperLeft.parent != nullptr)
		{
			resizeDragger_UpperLeft.Detach();
			resizeDragger_UpperLeft.SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
			resizeDragger_UpperLeft.SetPos(XMFLOAT2(translation.x, translation.y));
			resizeDragger_UpperLeft.AttachTo(this);
		}
		if (resizeDragger_BottomRight.parent != nullptr)
		{
			resizeDragger_BottomRight.Detach();
			resizeDragger_BottomRight.SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
			resizeDragger_BottomRight.SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y + scale.y - windowcontrolSize));
			resizeDragger_BottomRight.AttachTo(this);
		}
		if (label.parent != nullptr)
		{
			label.Detach();
			label.SetSize(XMFLOAT2(scale.x, windowcontrolSize));
			label.SetPos(XMFLOAT2(translation.x, translation.y));
			label.AttachTo(this);
		}

		Hitbox2D pointerHitbox = GetPointerHitbox();

		uint32_t priority = 0;

		bool focus = false;
		for (auto& widget : widgets)
		{
			widget->force_disable = force_disable || focus;
			widget->Update(canvas, dt);
			widget->force_disable = false;

			if (widget->priority_change)
			{
				widget->priority_change = false;
				widget->priority = priority++;
			}
			else
			{
				widget->priority = ~0u;
			}

			if (widget->IsVisible() && widget->hitBox.intersects(pointerHitbox))
			{
				focus = true;
			}
			if (widget->GetState() > IDLE)
			{
				focus = true;
			}
		}

		if (priority > 0) //Sort only if there are priority changes
			//Use std::stable_sort instead of std::sort to preserve UI element order with equal priorities
			std::stable_sort(widgets.begin(), widgets.end(), [](const Widget* a, const Widget* b) {
			return a->priority < b->priority;
				});

		if (IsMinimized())
		{
			hitBox.siz.y = windowcontrolSize;
		}

		if (IsEnabled() && !IsMinimized())
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
				Deactivate();
			}


			bool clicked = false;
			if (pointerHitbox.intersects(hitBox))
			{
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == FOCUS)
				{
					// activate
					clicked = true;
				}
			}

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
				}
			}

			if (clicked)
			{
				Activate();
			}
		}
		else
		{
			state = IDLE;
		}
	}
	void Window::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		wi::Color color = GetColor();

		// body
		wi::image::Params fx(translation.x - 2, translation.y - 2, scale.x + 4, scale.y + 4, wi::Color(0, 0, 0, 100));
		if (IsMinimized())
		{
			fx.siz.y = windowcontrolSize + 4;
			wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd); // shadow
		}
		else
		{
			wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd); // shadow
			sprites[state].Draw(cmd);
		}

		for (size_t i = 0; i < widgets.size(); ++i)
		{
			const Widget* widget = widgets[widgets.size() - i - 1];
			ApplyScissor(canvas, scissorRect, cmd);
			widget->Render(canvas, cmd);
		}

	}
	void Window::RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
	{
		Widget::RenderTooltip(canvas, cmd);
		for (auto& x : widgets)
		{
			x->RenderTooltip(canvas, cmd);
		}
	}
	void Window::SetVisible(bool value)
	{
		Widget::SetVisible(value);
		SetMinimized(!value);
		for (auto& x : widgets)
		{
			x->SetVisible(value);
		}
	}
	void Window::SetEnabled(bool value)
	{
		Widget::SetEnabled(value);
		for (auto& x : widgets)
		{
			if (x == &moveDragger)
				continue;
			if (x == &minimizeButton)
				continue;
			if (x == &closeButton)
				continue;
			if (x == &resizeDragger_UpperLeft)
				continue;
			if (x == &resizeDragger_BottomRight)
				continue;
			x->SetEnabled(value);
		}
	}
	void Window::SetMinimized(bool value)
	{
		minimized = value;

		if (resizeDragger_BottomRight.parent != nullptr)
		{
			resizeDragger_BottomRight.SetVisible(!value);
		}
		for (auto& x : widgets)
		{
			if (x == &moveDragger)
				continue;
			if (x == &minimizeButton)
				continue;
			if (x == &closeButton)
				continue;
			if (x == &resizeDragger_UpperLeft)
				continue;
			x->SetVisible(!value);
		}
	}
	bool Window::IsMinimized() const
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
	void ColorPicker::Create(const std::string& name, bool window_controls)
	{
		Window::Create(name, window_controls);

		SetSize(XMFLOAT2(cp_width, cp_height));
		SetColor(wi::Color(100, 100, 100, 100));

		float x = 250;
		float y = 110;
		float step = 20;

		text_R.Create("R");
		text_R.SetPos(XMFLOAT2(x, y += step));
		text_R.SetSize(XMFLOAT2(40, 18));
		text_R.SetText("");
		text_R.SetTooltip("Enter value for RED channel (0-255)");
		text_R.SetDescription("R: ");
		text_R.OnInputAccepted([this](EventArgs args) {
			wi::Color color = GetPickColor();
			color.setR((uint8_t)args.iValue);
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_R);

		text_G.Create("G");
		text_G.SetPos(XMFLOAT2(x, y += step));
		text_G.SetSize(XMFLOAT2(40, 18));
		text_G.SetText("");
		text_G.SetTooltip("Enter value for GREEN channel (0-255)");
		text_G.SetDescription("G: ");
		text_G.OnInputAccepted([this](EventArgs args) {
			wi::Color color = GetPickColor();
			color.setG((uint8_t)args.iValue);
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_G);

		text_B.Create("B");
		text_B.SetPos(XMFLOAT2(x, y += step));
		text_B.SetSize(XMFLOAT2(40, 18));
		text_B.SetText("");
		text_B.SetTooltip("Enter value for BLUE channel (0-255)");
		text_B.SetDescription("B: ");
		text_B.OnInputAccepted([this](EventArgs args) {
			wi::Color color = GetPickColor();
			color.setB((uint8_t)args.iValue);
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_B);


		text_H.Create("H");
		text_H.SetPos(XMFLOAT2(x, y += step));
		text_H.SetSize(XMFLOAT2(40, 18));
		text_H.SetText("");
		text_H.SetTooltip("Enter value for HUE channel (0-360)");
		text_H.SetDescription("H: ");
		text_H.OnInputAccepted([this](EventArgs args) {
			hue = wi::math::Clamp(args.fValue, 0, 360.0f);
			FireEvents();
			});
		AddWidget(&text_H);

		text_S.Create("S");
		text_S.SetPos(XMFLOAT2(x, y += step));
		text_S.SetSize(XMFLOAT2(40, 18));
		text_S.SetText("");
		text_S.SetTooltip("Enter value for SATURATION channel (0-100)");
		text_S.SetDescription("S: ");
		text_S.OnInputAccepted([this](EventArgs args) {
			saturation = wi::math::Clamp(args.fValue / 100.0f, 0, 1);
			FireEvents();
			});
		AddWidget(&text_S);

		text_V.Create("V");
		text_V.SetPos(XMFLOAT2(x, y += step));
		text_V.SetSize(XMFLOAT2(40, 18));
		text_V.SetText("");
		text_V.SetTooltip("Enter value for LUMINANCE channel (0-100)");
		text_V.SetDescription("V: ");
		text_V.OnInputAccepted([this](EventArgs args) {
			luminance = wi::math::Clamp(args.fValue / 100.0f, 0, 1);
			FireEvents();
			});
		AddWidget(&text_V);

		alphaSlider.Create(0, 255, 255, 255, "");
		alphaSlider.SetPos(XMFLOAT2(20, 230));
		alphaSlider.SetSize(XMFLOAT2(150, 18));
		alphaSlider.SetText("A: ");
		alphaSlider.SetTooltip("Value for ALPHA - TRANSPARENCY channel (0-255)");
		alphaSlider.OnSlide([this](EventArgs args) {
			FireEvents();
			});
		AddWidget(&alphaSlider);
	}
	static const float colorpicker_radius_triangle = 68;
	static const float colorpicker_radius = 75;
	static const float colorpicker_width = 22;
	void ColorPicker::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Window::Update(canvas, dt);

		if (IsEnabled())
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
			XMFLOAT2 pointer = GetPointerHitbox().pos;
			float distance = wi::math::Distance(center, pointer);
			bool hover_hue = (distance > colorpicker_radius * sca) && (distance < (colorpicker_radius + colorpicker_width)* sca);

			float distTri = 0;
			XMFLOAT4 A, B, C;
			wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
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

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
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

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
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

			dragged = dragged;
			if (colorpickerstate == CPS_HUE && dragged)
			{
				//hue pick
				const float angle = wi::math::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(colorpicker_radius, 0));
				hue = angle / XM_2PI * 360.0f;
				Activate();
			}
			else if (colorpickerstate == CPS_SATURATION && dragged)
			{
				// saturation pick
				float u, v, w;
				wi::math::GetBarycentric(O, _A, _B, _C, u, v, w, true);
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

				Activate();
			}
			else if (state != IDLE)
			{
				Deactivate();
			}

			wi::Color color = GetPickColor();
			text_R.SetValue((int)color.getR());
			text_G.SetValue((int)color.getG());
			text_B.SetValue((int)color.getB());
			text_H.SetValue(int(hue));
			text_S.SetValue(int(saturation * 100));
			text_V.SetValue(int(luminance * 100));

			if (dragged)
			{
				FireEvents();
			}
		}
	}
	void ColorPicker::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Window::Render(canvas, cmd);

		if (!IsVisible() || IsMinimized())
		{
			return;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();

		struct Vertex
		{
			XMFLOAT4 pos;
			XMFLOAT4 col;
		};
		static wi::graphics::GPUBuffer vb_hue;
		static wi::graphics::GPUBuffer vb_picker_saturation;
		static wi::graphics::GPUBuffer vb_picker_hue;
		static wi::graphics::GPUBuffer vb_preview;

		static wi::vector<Vertex> vertices_saturation;

		static bool buffersComplete = false;
		if (!buffersComplete)
		{
			buffersComplete = true;

			// saturation
			{
				vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,0,0,1) });	// hue
				vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,1,1,1) });	// white
				vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(0,0,0,1) });	// black
				wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle, vertices_saturation[0].pos, vertices_saturation[1].pos, vertices_saturation[2].pos);

				// create alpha blended edge:
				vertices_saturation.push_back(vertices_saturation[0]); // outer
				vertices_saturation.push_back(vertices_saturation[0]); // inner
				vertices_saturation.push_back(vertices_saturation[1]); // outer
				vertices_saturation.push_back(vertices_saturation[1]); // inner
				vertices_saturation.push_back(vertices_saturation[2]); // outer
				vertices_saturation.push_back(vertices_saturation[2]); // inner
				vertices_saturation.push_back(vertices_saturation[0]); // outer
				vertices_saturation.push_back(vertices_saturation[0]); // inner
				wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle + 4, vertices_saturation[3].pos, vertices_saturation[5].pos, vertices_saturation[7].pos); // extrude outer
				vertices_saturation[9].pos = vertices_saturation[3].pos; // last outer
			}
			// hue
			{
				const float edge = 2.0f;
				wi::vector<Vertex> vertices;
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
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = vertices.size() * sizeof(Vertex);
				desc.stride = 0;
				device->CreateBuffer(&desc, vertices.data(), &vb_hue);
			}
			// saturation picker (small circle)
			{
				float _radius = 3;
				float _width = 3;
				wi::vector<Vertex> vertices;
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
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = vertices.size() * sizeof(Vertex);
				desc.stride = 0;
				device->CreateBuffer(&desc, vertices.data(), &vb_picker_saturation);
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
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = sizeof(vertices);
				desc.stride = 0;
				device->CreateBuffer(&desc, vertices, &vb_picker_hue);
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
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = sizeof(vertices);
				device->CreateBuffer(&desc, vertices, &vb_preview);
			}

		}

		const wi::Color final_color = GetPickColor();
		const float angle = hue / 360.0f * XM_2PI;

		const XMMATRIX Projection = canvas.GetProjection();

		device->BindPipelineState(&gui_internal().PSO_colored, cmd);

		ApplyScissor(canvas, scissorRect, cmd);

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
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_saturation.buffer,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
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
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_hue,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_hue.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}

		// render hue picker
		if (IsEnabled())
		{
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixRotationZ(-hue / 360.0f * XM_2PI) *
				W *
				Projection
			);

			hsv source;
			source.h = hue;
			source.s = 1;
			source.v = 1;
			rgb result = hsv2rgb(source);
			cb.g_xColor = float4(1 - result.r, 1 - result.g, 1 - result.b, 1);

			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_picker_hue,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_picker_hue.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}

		// render saturation picker
		if (IsEnabled())
		{
			XMFLOAT4 A, B, C;
			wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
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
			wi::math::GetBarycentric(inner_point, color_corner, white_corner, black_corner, u, v, w, true);

			XMVECTOR picker_center = u * _A + v * _B + w * _C;

			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixTranslationFromVector(picker_center) *
				W *
				Projection
			);
			cb.g_xColor = float4(1 - final_color.toFloat3().x, 1 - final_color.toFloat3().y, 1 - final_color.toFloat3().z, 1);
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_picker_saturation,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_picker_saturation.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}

		// render preview
		{
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixScaling(sca, sca, 1) *
				XMMatrixTranslation(translation.x + scale.x - 40 * sca, translation.y + 40, 0) *
				Projection
			);
			cb.g_xColor = final_color.toFloat4();
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_preview,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_preview.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}
	}
	wi::Color ColorPicker::GetPickColor() const
	{
		hsv source;
		source.h = hue;
		source.s = saturation;
		source.v = luminance;
		rgb result = hsv2rgb(source);
		return wi::Color::fromFloat4(XMFLOAT4(result.r, result.g, result.b, alphaSlider.GetValue() / 255.0f));
	}
	void ColorPicker::SetPickColor(wi::Color value)
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
		alphaSlider.SetValue((float)value.getA());
	}
	void ColorPicker::FireEvents()
	{
		if (onColorChanged == nullptr)
			return;
		EventArgs args;
		args.color = GetPickColor();
		onColorChanged(args);
	}
	void ColorPicker::OnColorChanged(std::function<void(EventArgs args)> func)
	{
		onColorChanged = move(func);
	}






	inline float item_height() { return 20.0f; }
	inline float tree_scrollbar_width() { return 12.0f; }
	void TreeList::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnSelect([](EventArgs args) {});
		SetSize(XMFLOAT2(100, 20));

		for (int i = FOCUS + 1; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites[i].params.color = sprites[FOCUS].params.color;
		}
		font.params.v_align = wi::font::WIFALIGN_CENTER;
	}
	float TreeList::GetItemOffset(int index) const
	{
		return 2 + list_offset + index * item_height();
	}
	Hitbox2D TreeList::GetHitbox_ListArea() const
	{
		return Hitbox2D(XMFLOAT2(translation.x, translation.y + item_height() + 1), XMFLOAT2(scale.x - tree_scrollbar_width() - 1, scale.y - item_height() - 1));
	}
	Hitbox2D TreeList::GetHitbox_Item(int visible_count, int level) const
	{
		XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height(), translation.y + GetItemOffset(visible_count) + item_height() * 0.5f);
		Hitbox2D hitbox;
		hitbox.pos = XMFLOAT2(pos.x + item_height() * 0.5f + 2, pos.y - item_height() * 0.5f);
		hitbox.siz = XMFLOAT2(scale.x - 2 - item_height() * 0.5f - 2 - level * item_height() - tree_scrollbar_width() - 2, item_height());
		return hitbox;
	}
	Hitbox2D TreeList::GetHitbox_ItemOpener(int visible_count, int level) const
	{
		XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height(), translation.y + GetItemOffset(visible_count) + item_height() * 0.5f);
		return Hitbox2D(XMFLOAT2(pos.x, pos.y - item_height() * 0.25f), XMFLOAT2(item_height() * 0.5f, item_height() * 0.5f));
	}
	bool TreeList::HasScrollbar() const
	{
		return scale.y < (int)items.size()* item_height();
	}
	void TreeList::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		if (IsEnabled())
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
				Deactivate();
			}

			Hitbox2D hitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));
			Hitbox2D pointerHitbox = GetPointerHitbox();

			if (state == IDLE && hitbox.intersects(pointerHitbox))
			{
				state = FOCUS;
			}

			bool clicked = false;
			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				clicked = true;
			}

			bool click_down = false;
			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				click_down = true;
				if (state == FOCUS || state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
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
				Activate();
				scrollbar_delta = pointerHitbox.pos.y - scrollbar_height * 0.5f - scrollbar_begin;
			}

			if (state == FOCUS)
			{
				scrollbar_delta -= wi::input::GetPointer().z * 10;
			}
			scrollbar_delta = wi::math::Clamp(scrollbar_delta, 0, scrollbar_size - scrollbar_height);
			scrollbar_value = wi::math::InverseLerp(scrollbar_begin, scrollbar_end, scrollbar_begin + scrollbar_delta);

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
							Activate();
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
								if (!wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) && !wi::input::Down(wi::input::KEYBOARD_BUTTON_RCONTROL)
									&& !wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) && !wi::input::Down(wi::input::KEYBOARD_BUTTON_RSHIFT))
								{
									ClearSelection();
								}
								Select(i);
								Activate();
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
	void TreeList::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}
		GraphicsDevice* device = wi::graphics::GetDevice();

		// control-base
		sprites[state].Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		font.Draw(cmd);

		// scrollbar background
		wi::image::Params fx = sprites[state].params;
		fx.pos = XMFLOAT3(translation.x + scale.x - tree_scrollbar_width(), translation.y + item_height() + 1, 0);
		fx.siz = XMFLOAT2(tree_scrollbar_width(), scale.y - item_height() - 1);
		fx.color = sprites[IDLE].params.color;
		wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

		// scrollbar
		wi::Color scrollbar_color = wi::Color::fromFloat4(sprites[IDLE].params.color);
		if (scrollbar_state == SCROLLBAR_HOVER)
		{
			scrollbar_color = wi::Color::fromFloat4(sprites[FOCUS].params.color);
		}
		else if (scrollbar_state == SCROLLBAR_GRABBED)
		{
			scrollbar_color = wi::Color::White();
		}
		wi::image::Draw(wi::texturehelper::getWhite()
			, wi::image::Params(translation.x + scale.x - tree_scrollbar_width(), translation.y + item_height() + 1 + scrollbar_delta, tree_scrollbar_width(), scrollbar_height, scrollbar_color), cmd);

		// list background
		Hitbox2D itemlist_box = GetHitbox_ListArea();
		fx.pos = XMFLOAT3(itemlist_box.pos.x, itemlist_box.pos.y, 0);
		fx.siz = XMFLOAT2(itemlist_box.siz.x, itemlist_box.siz.y);
		wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

		Rect rect_without_scrollbar;
		rect_without_scrollbar.left = (int)itemlist_box.pos.x;
		rect_without_scrollbar.right = (int)(itemlist_box.pos.x + itemlist_box.siz.x);
		rect_without_scrollbar.top = (int)itemlist_box.pos.y;
		rect_without_scrollbar.bottom = (int)(itemlist_box.pos.y + itemlist_box.siz.y);
		ApplyScissor(canvas, rect_without_scrollbar, cmd);

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
			wi::math::ConstructTriangleEquilateral(1, vertices[0].pos, vertices[1].pos, vertices[2].pos);

			GPUBufferDesc desc;
			desc.bind_flags = BindFlag::VERTEX_BUFFER;
			desc.size = sizeof(vertices);
			device->CreateBuffer(&desc, vertices, &vb_triangle);
		}
		const XMMATRIX Projection = canvas.GetProjection();

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
				wi::image::Draw(wi::texturehelper::getWhite()
					, wi::image::Params(name_box.pos.x, name_box.pos.y, name_box.siz.x, name_box.siz.y,
						sprites[item.selected ? FOCUS : IDLE].params.color), cmd);
			}

			// opened flag triangle:
			{
				device->BindPipelineState(&gui_internal().PSO_colored, cmd);

				MiscCB cb;
				cb.g_xColor = opener_highlight == i ? wi::Color::White().toFloat4() : sprites[FOCUS].params.color;
				XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(item_height() * 0.3f, item_height() * 0.3f, 1) *
					XMMatrixRotationZ(item.open ? XM_PIDIV2 : 0) *
					XMMatrixTranslation(open_box.pos.x + open_box.siz.x * 0.5f, open_box.pos.y + open_box.siz.y * 0.25f, 0) *
					Projection
				);
				device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
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
			wi::font::Draw(item.name, wi::font::Params(name_box.pos.x + 1, name_box.pos.y + name_box.siz.y * 0.5f, wi::font::WIFONTSIZE_DEFAULT, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_CENTER,
				font.params.color, font.params.shadowColor), cmd);
		}
	}
	void TreeList::OnSelect(std::function<void(EventArgs args)> func)
	{
		onSelect = move(func);
	}
	void TreeList::AddItem(const Item& item)
	{
		items.push_back(item);
	}
	void TreeList::ClearItems()
	{
		items.clear();
	}
	void TreeList::ClearSelection()
	{
		for (Item& item : items)
		{
			item.selected = false;
		}

		EventArgs args;
		args.iValue = -1;
		onSelect(args);
	}
	void TreeList::Select(int index)
	{
		items[index].selected = true;

		EventArgs args;
		args.iValue = index;
		args.sValue = items[index].name;
		args.userdata = items[index].userdata;
		onSelect(args);
	}
	const TreeList::Item& TreeList::GetItem(int index) const
	{
		return items[index];
	}


}
