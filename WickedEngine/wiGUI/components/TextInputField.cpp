#include "wiGUI/components/TextInputField.h"
#include "wiGUI/GUIInternal.h"

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
#include "wiHelper.h"

#include <sstream>
#include <iomanip> // setprecision
#include <utility>

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::gui
{
	wi::SpriteFont TextInputField::font_input;
	int caret_pos = 0;
	int caret_begin = 0;
	int caret_delay = 0;
	bool input_updated = false;
	bool doubleclick_select = false;
	wi::Timer caret_timer;
	wi::Timer key_repeat_timer;
	wi::input::BUTTON key_repeat_button = {};
	bool key_repeat_active = false;
	static constexpr float KEY_REPEAT_DELAY = 0.5f;
	static constexpr float KEY_REPEAT_RATE = 0.035f;
	bool KeyRepeat(wi::input::BUTTON button)
	{
		if (wi::input::Press(button))
		{
			key_repeat_button = button;
			key_repeat_timer.record();
			key_repeat_active = false;
			return true;
		}
		if (wi::input::Down(button) && key_repeat_button == button)
		{
			const float elapsed = (float)key_repeat_timer.elapsed_seconds();
			if (!key_repeat_active && elapsed > KEY_REPEAT_DELAY)
			{
				key_repeat_active = true;
				key_repeat_timer.record();
				return true;
			}
			if (key_repeat_active && elapsed > KEY_REPEAT_RATE)
			{
				key_repeat_timer.record();
				return true;
			}
		}
		return false;
	}

	// Simple recursive-descent arithmetic expression evaluator.
	// Supports: +, -, *, /, parentheses, unary +/-, floating-point literals.
	// Returns true and sets 'result' if 'str' is a fully-consumed valid expression.
	// Returns false for non-numeric input (e.g. plain text strings) so callers fall back.
	namespace {
		struct ArithExprParser
		{
			const char* pos;
			bool valid = true;

			void skipWS() { while (*pos == ' ' || *pos == '\t') pos++; }

			double parsePrimary()
			{
				skipWS();
				if (*pos == '(')
				{
					pos++;
					double v = parseExpr();
					skipWS();
					if (*pos == ')') pos++;
					return v;
				}
				if (*pos == '-') { pos++; return -parsePrimary(); }
				if (*pos == '+') { pos++; return  parsePrimary(); }
				char* end;
				double v = strtod(pos, &end);
				if (end == pos) { valid = false; return 0; }
				pos = end;
				return v;
			}

			double parseTerm()
			{
				double left = parsePrimary();
				while (valid)
				{
					skipWS();
					if (*pos == '*') { pos++; left *= parsePrimary(); }
					else if (*pos == '/') { pos++; double r = parsePrimary(); if (r == 0.0) { valid = false; return 0; } left /= r; }
					else break;
				}
				return left;
			}

			double parseExpr()
			{
				double left = parseTerm();
				while (valid)
				{
					skipWS();
					if (*pos == '+') { pos++; left += parseTerm(); }
					else if (*pos == '-') { pos++; left -= parseTerm(); }
					else break;
				}
				return left;
			}
		};

		bool EvaluateArithmetic(const std::string& str, double& result)
		{
			if (str.empty()) return false;
			ArithExprParser p;
			p.pos = str.c_str();
			result = p.parseExpr();
			p.skipWS();
			return p.valid && (*p.pos == '\0') && std::isfinite(result);
		}
	} // anonymous namespace

	void TextInputField::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnInputAccepted([](const EventArgs& args) {});
		SetSize(XMFLOAT2(100, 20));

		font.params.v_align = wi::font::WIFALIGN_CENTER;

		font_description.params = font.params;
		font_description.params.h_align = wi::font::WIFALIGN_RIGHT;

		SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip); // disable localization of text because that can come from user input and musn't be overwritten!
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
		if (newValue == FLT_MAX)
		{
			font.SetText(L"FLT_MAX");
		}
		else if (newValue == -FLT_MAX)
		{
			font.SetText(L"-FLT_MAX");
		}
		else
		{
			std::stringstream ss("");
			if (float_precision >= 0)
			{
				ss << std::fixed << std::setprecision(float_precision);
			}
			ss << newValue;
			font.SetText(ss.str());
		}
	}
	const std::string TextInputField::GetValue()
	{
		return font.GetTextA();
	}
	const std::string TextInputField::GetCurrentInputValue()
	{
		if (state == ACTIVE)
		{
			return font_input.GetTextA();
		}
		return GetValue();
	}
	void TextInputField::Update(const wi::Canvas& canvas, float dt)
	{
		const bool was_active = (state == ACTIVE);

		if (!IsVisible())
		{
			// If this text input was active but became invisible, clear typing state
			if (was_active)
			{
				font_input.text.clear();
				typing_active = false;
				state = IDLE;
			}
			return;
		}

		Widget::Update(canvas, dt);

		// If this text input was active but state was externally reset,
		//	properly clear the typing state so IsTyping() no longer blocks input
		if (was_active && state != ACTIVE)
		{
			font_input.text.clear();
			typing_active = false;
		}

		if (IsEnabled() && dt > 0)
		{
			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x;
			hitBox.siz.y = scale.y;

			const Hitbox2D pointerHitbox = GetPointerHitbox();
			const bool intersectsPointer = pointerHitbox.intersects(hitBox);

			if (state == FOCUS)
			{
				state = IDLE;
			}
			if (state == DEACTIVATING)
			{
				state = IDLE;
			}

			// hover the button
			if (intersectsPointer)
			{
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			bool leftButtonClicked = false;
			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				leftButtonClicked = true;
			}
			bool rightButtonClicked = false;
			if (wi::input::Press(wi::input::MOUSE_BUTTON_RIGHT))
			{
				rightButtonClicked = true;
			}

			if (state == ACTIVE)
			{
				if (!wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
				{
					doubleclick_select = false;
				}
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_ENTER))
				{
					// accept input, evaluating arithmetic expressions (e.g. "10 * 2" -> "20")
					std::string inputStr = font_input.GetTextA();
					double arithmeticResult = 0;
					if (EvaluateArithmetic(inputStr, arithmeticResult))
					{
						std::stringstream ssResult;
						if (float_precision >= 0)
						{
							ssResult << std::fixed << std::setprecision(float_precision);
						}
						ssResult << arithmeticResult;
						font.SetText(ssResult.str());
					}
					else
					{
						font.SetText(font_input.GetText());
					}
					font_input.text.clear();

					if (onInputAccepted)
					{
						EventArgs args;
						args.sValue = font.GetTextA();
						args.iValue = atoi(args.sValue.c_str());
						args.fValue = (float)atof(args.sValue.c_str());
						onInputAccepted(args);
					}
					Deactivate();
				}
				//else if (wi::input::Press(wi::input::KEYBOARD_BUTTON_BACKSPACE))
				//{
				//	// delete input...
				//	DeleteFromInput(-1);
				//}
				else if (KeyRepeat(wi::input::KEYBOARD_BUTTON_DELETE))
				{
					// delete input...
					DeleteFromInput(1);
				}
				else if (KeyRepeat(wi::input::KEYBOARD_BUTTON_LEFT) && caret_pos > 0)
				{
					// caret repositioning left:
					caret_pos--;
					if (!wi::input::Down(wi::input::BUTTON::KEYBOARD_BUTTON_LSHIFT) && !wi::input::Down(wi::input::BUTTON::KEYBOARD_BUTTON_RSHIFT))
					{
						caret_begin = caret_pos;
					}
					caret_timer.record();
				}
				else if (KeyRepeat(wi::input::KEYBOARD_BUTTON_RIGHT))
				{
					// caret repositioning right:
					if (caret_pos < font_input.GetText().size())
					{
						caret_pos++;
					}
					if (!wi::input::Down(wi::input::BUTTON::KEYBOARD_BUTTON_LSHIFT) && !wi::input::Down(wi::input::BUTTON::KEYBOARD_BUTTON_RSHIFT))
					{
						caret_begin = caret_pos;
					}
					caret_timer.record();
				}
				else if ((leftButtonClicked && !intersectsPointer) || (rightButtonClicked && !intersectsPointer) || wi::input::Press(wi::input::KEYBOARD_BUTTON_ESCAPE))
				{
					// cancel input
					font_input.text.clear();
					typing_active = false;
					state = IDLE;
				}
				else if (leftButtonClicked && intersectsPointer && wi::input::IsDoubleClicked())
				{
					caret_begin = 0;
					caret_pos = (int)font_input.GetText().size();
					caret_timer.record();
					doubleclick_select = true;
				}
				else if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) && !doubleclick_select)
				{
					// caret repositioning by mouse click:
					caret_timer.record();
					caret_pos = (int)font_input.GetText().size();
					const std::wstring& str = font_input.GetText();
					float pos = font_input.params.position.x;
					for (size_t i = 0; i < str.size(); ++i)
					{
						XMFLOAT2 size = wi::font::TextSize(str.c_str() + i, 1, font_input.params);
						pos += size.x;
						if (pos > pointerHitbox.pos.x)
						{
							caret_pos = int(i);
							break;
						}
					}
					if (leftButtonClicked && intersectsPointer)
					{
						caret_begin = caret_pos;
					}
					if (caret_delay < 1) // fix for bug: first click creates incorrect highlight state
					{
						caret_delay++;
						caret_begin = caret_pos;
					}
				}

				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) && wi::input::Down((wi::input::BUTTON)'A'))
				{
					caret_begin = 0;
					caret_pos = (int)font_input.GetText().size();
				}

			}

			if (leftButtonClicked && state == FOCUS)
			{
				// activate
				const bool select_all = intersectsPointer && wi::input::IsDoubleClicked();
				SetAsActive(select_all);
				doubleclick_select = select_all;
			}
		}

		left_text_width = 0;
		right_text_width = 0;

		font.params.posX = translation.x + 2;
		font.params.posY = translation.y + scale.y * 0.5f;
		font_description.params.posX = translation.x;
		font_description.params.posY = translation.y + scale.y * 0.5f;
		switch (font_description.params.h_align)
		{
		case wi::font::WIFALIGN_LEFT:
			font_description.params.posX = translation.x + scale.x + shadow;
			right_text_width = font_description.TextWidth();
			break;
		case wi::font::WIFALIGN_RIGHT:
			font_description.params.posX = translation.x - shadow;
			left_text_width = font_description.TextWidth();
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font_description.params.posX = translation.x + scale.x * 0.5f;
			break;
		}
		switch (font_description.params.v_align)
		{
		case wi::font::WIFALIGN_TOP:
			font_description.params.posY = translation.y + scale.y + shadow;
			break;
		case wi::font::WIFALIGN_BOTTOM:
			font_description.params.posY = translation.y - shadow;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font_description.params.posY = translation.y + scale.y * 0.5f;
			break;
		}

		if (state == ACTIVE)
		{
			font_input.params = font.params;

			if (!cancel_input_enabled)
			{
				SetValue(font_input.GetTextA());
			}

			if (input_updated && onInput)
			{
				wi::gui::EventArgs args;
				args.sValue = GetCurrentInputValue();
				onInput(args);
			}
			input_updated = false;
		}

	}
	void TextInputField::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Widget::Render(canvas, cmd);
		if (!IsVisible())
		{
			return;
		}

		// shadow:
		if (shadow > 0)
		{
			wi::image::Params fx = sprites[state].params;
			fx.gradient = wi::image::Params::Gradient::None;
			fx.pos.x -= shadow;
			fx.pos.y -= shadow;
			fx.siz.x += shadow * 2;
			fx.siz.y += shadow * 2;
			fx.color = shadow_color;
			if (fx.isCornerRoundingEnabled())
			{
				for (auto& corner_rounding : fx.corners_rounding)
				{
					if (corner_rounding.radius > 0)
					{
						corner_rounding.radius += shadow;
					}
				}
			}
			if (shadow_highlight)
			{
				fx.enableHighlight();
				fx.highlight_pos = GetPointerHighlightPos(canvas);
				fx.highlight_color = shadow_highlight_color;
				fx.highlight_spread = shadow_highlight_spread;
			}
			else
			{
				fx.disableHighlight();
			}
			wi::image::Draw(nullptr, fx, cmd);
		}

		font_description.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		sprites[state].Draw(cmd);

		if (state == ACTIVE)
		{
			font_input.Draw(cmd);

			// caret:
			if(std::fmod(caret_timer.elapsed_seconds(), 1) < 0.5f)
			{
				wi::font::Params params = font_input.params;
				XMFLOAT2 size = wi::font::TextSize(font_input.GetText().c_str(), caret_pos, font_input.params);
				params.posX += size.x;
				params.color = wi::Color::lerp(params.color, wi::Color::Transparent(), 0.1f);
				params.size += 4;
				params.posY -= 2;
				params.h_align = wi::font::WIFALIGN_CENTER;
				wi::font::Draw(L"|", params, cmd);
			}

			// selection:
			if(caret_pos != caret_begin)
			{
				int start = std::min(caret_begin, caret_pos);
				int end = std::max(caret_begin, caret_pos);
				const std::wstring& str = font_input.GetText();
				float pos_start = font_input.params.position.x + wi::font::TextSize(str.c_str(), start, font_input.params).x;
				float pos_end = font_input.params.position.x + wi::font::TextSize(str.c_str(), end, font_input.params).x;
				float width = pos_end - pos_start;
				wi::image::Params params;
				params.pos.x = pos_start;
				params.pos.y = translation.y + 1;
				params.siz.x = width;
				params.siz.y = scale.y - 2;
				params.blendFlag = wi::enums::BLENDMODE_ALPHA;
				params.color = wi::Color::lerp(font_input.params.color, wi::Color::Transparent(), 0.5f);
				wi::image::Draw(nullptr, params, cmd);
			}

			//Rect scissorRect;
			//scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
			//scissorRect.left = (int32_t)(0);
			//scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
			//scissorRect.top = (int32_t)(0);
			//GraphicsDevice* device = wi::graphics::GetDevice();
			//device->BindScissorRects(1, &scissorRect, cmd);
			//wi::font::Draw("caret_begin = " + std::to_string(caret_begin) + "\ncaret_pos = " + std::to_string(caret_pos), wi::font::Params(0, 20), cmd);
		}
		else
		{
			font.Draw(cmd);
		}
	}
	void TextInputField::OnInputAccepted(std::function<void(const EventArgs& args)> func)
	{
		onInputAccepted = std::move(func);
	}
	void TextInputField::OnInput(std::function<void(const EventArgs& args)> func)
	{
		onInput = std::move(func);
	}
	void TextInputField::AddInput(const wchar_t inputChar)
	{
		input_updated = true;
		switch (inputChar)
		{
		case '\b':	// BACKSPACE
		case '\n':	// ENTER
		case '\r':	// ENTER
		case 127:	// DEL
			return;
		default:
			break;
		}
		std::wstring value_new = font_input.GetText();
		if (value_new.size() >= caret_pos)
		{
			int caret_pos_prev = caret_pos;
			if (caret_begin != caret_pos)
			{
				int offset = std::min(caret_pos, caret_begin);
				value_new.erase(offset, std::abs(caret_pos - caret_begin));
				caret_pos = offset;
			}
			int num = 0;
#ifdef __APPLE__
			if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCOMMAND) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RCOMMAND))
#else
			if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RCONTROL))
#endif // __APPLE__
			{
				if (wi::input::Down((wi::input::BUTTON)'V'))
				{
					// Paste:
					std::wstring clipboard = wi::helper::GetClipboardText();
					value_new.insert(value_new.begin() + caret_pos, clipboard.begin(), clipboard.end());
					num = (int)clipboard.length();
				}
				else if (wi::input::Down((wi::input::BUTTON)'C'))
				{
					// Copy:
					caret_pos = caret_pos_prev;
					const std::wstring& text = font_input.GetText();
					int start = std::min(caret_begin, caret_pos);
					int end = std::max(caret_begin, caret_pos);
					std::wstring clipboard = std::wstring(text.c_str() + start, text.c_str() + end);
					wi::helper::SetClipboardText(clipboard);
					return;
				}
				else if (wi::input::Down((wi::input::BUTTON)'X'))
				{
					// Cut:
					caret_pos = caret_pos_prev;
					const std::wstring& text = font_input.GetText();
					int start = std::min(caret_begin, caret_pos);
					int end = std::max(caret_begin, caret_pos);
					std::wstring clipboard = std::wstring(text.c_str() + start, text.c_str() + end);
					wi::helper::SetClipboardText(clipboard);
				}
				else
					return;
			}
			else
			{
				value_new.insert(value_new.begin() + caret_pos, inputChar);
				num = 1;
			}
			font_input.SetText(value_new);
			caret_pos = std::min((int)font_input.GetText().size(), caret_pos + num);
		}
		caret_begin = caret_pos;
	}
	void TextInputField::AddInput(const char inputChar)
	{
		AddInput((wchar_t)inputChar);
	}
	void TextInputField::DeleteFromInput(int direction)
	{
		input_updated = true;
		std::wstring value_new = font_input.GetText();
		if (caret_begin != caret_pos)
		{
			int offset = std::min(caret_pos, caret_begin);
			value_new.erase(offset, std::abs(caret_pos - caret_begin));
			caret_pos = offset;
		}
		else
		{
			if (direction < 0)
			{
				if (caret_pos > 0 && value_new.size() > caret_pos - 1)
				{
					value_new.erase(value_new.begin() + caret_pos - 1);
					caret_pos = std::max(0, caret_pos - 1);
				}
			}
			else
			{
				if (value_new.size() > caret_pos)
				{
					value_new.erase(value_new.begin() + caret_pos);
					caret_pos = std::min((int)value_new.size(), caret_pos);
				}
			}
		}
		caret_begin = caret_pos;
		font_input.SetText(value_new);
	}
	void TextInputField::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);

		if (id > WIDGET_ID_TEXTINPUTFIELD_BEGIN && id < WIDGET_ID_TEXTINPUTFIELD_END)
		{
			sprites[id - WIDGET_ID_TEXTINPUTFIELD_IDLE].params.color = color;
		}
	}
	void TextInputField::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		theme.font.Apply(font_description.params);
	}
	void TextInputField::SetAsActive(bool selectall)
	{
		Activate();
		font_input.SetText(font.GetText());
		caret_pos = (int)font_input.GetText().size();
		caret_begin = selectall ? 0 : caret_pos;
		caret_delay = 0;
	}
	void TextInputField::Activate()
	{
		if (state != ACTIVE)
			typing_active = true;
		Widget::Activate();
	}
	void TextInputField::Deactivate()
	{
		if(state == ACTIVE)
			typing_active = false;
		Widget::Deactivate();
	}
	void TextInputField::SetEnabled(bool val)
	{
		if (!val && state == ACTIVE)
		{
			font_input.text.clear();
			typing_active = false;
		}
		Widget::SetEnabled(val);
	}
	void TextInputField::SetVisible(bool val)
	{
		if (!val && state == ACTIVE)
		{
			font_input.text.clear();
			typing_active = false;
		}
		Widget::SetVisible(val);
	}
}
