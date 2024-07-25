#include "wiBacklog.h"
#include "wiMath.h"
#include "wiResourceManager.h"
#include "wiTextureHelper.h"
#include "wiSpinLock.h"
#include "wiFont.h"
#include "wiSpriteFont.h"
#include "wiImage.h"
#include "wiLua.h"
#include "wiInput.h"
#include "wiPlatform.h"
#include "wiHelper.h"
#include "wiGUI.h"

#include <mutex>
#include <deque>
#include <limits>
#include <thread>
#include <iostream>

using namespace wi::graphics;

namespace wi::backlog
{
	bool enabled = false;
	bool was_ever_enabled = enabled;
	struct LogEntry
	{
		std::string text;
		LogLevel level = LogLevel::Default;
	};
	std::deque<LogEntry> entries;
	std::deque<LogEntry> history;
	const float speed = 4000.0f;
	const size_t deletefromline = 500;
	float pos = 5;
	float scroll = 0;
	int historyPos = 0;
	wi::font::Params font_params;
	wi::SpinLock logLock;
	Texture backgroundTex;
	bool refitscroll = false;
	wi::gui::TextInputField inputField;
	wi::gui::Button toggleButton;
	wi::gui::GUI GUI;

	bool locked = false;
	bool blockLuaExec = false;
	LogLevel logLevel = LogLevel::Default;
	LogLevel unseen = LogLevel::None;

	std::string getTextWithoutLock()
	{
		std::string retval;
		for (auto& x : entries)
		{
			retval += x.text;
		}
		return retval;
	}
	void write_logfile()
	{
		std::string filename = wi::helper::GetTempDirectoryPath() + "wiBacklog.txt";
		std::string text = getText(); // will lock mutex
		wi::helper::FileWrite(filename, (const uint8_t*)text.c_str(), text.length());
	}

	// The logwriter object will automatically write out the backlog to the temp folder when it's destroyed
	//	Should happen on application exit
	struct LogWriter
	{
		~LogWriter()
		{
			write_logfile();
		}
	} logwriter;

	void Toggle()
	{
		enabled = !enabled;
		was_ever_enabled = true;
	}
	void Scroll(float dir)
	{
		scroll += dir;
	}
	void Update(const wi::Canvas& canvas, float dt)
	{
		if (!locked)
		{
			if (wi::input::Press(wi::input::KEYBOARD_BUTTON_HOME) && !GUI.IsTyping())
			{
				Toggle();
			}

			if (isActive())
			{
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_UP))
				{
					historyPrev();
				}
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_DOWN))
				{
					historyNext();
				}
				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_PAGEUP))
				{
					Scroll(1000.0f * dt);
				}
				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_PAGEDOWN))
				{
					Scroll(-1000.0f * dt);
				}

				Scroll(wi::input::GetPointer().z * 20);

				static bool created = false;
				if (!created)
				{
					created = true;
					inputField.Create("");
					inputField.SetCancelInputEnabled(false);
					inputField.OnInputAccepted([](wi::gui::EventArgs args) {
						historyPos = 0;
						post(args.sValue);
						LogEntry entry;
						entry.text = args.sValue;
						entry.level = LogLevel::Default;
						history.push_back(entry);
						if (history.size() > deletefromline)
						{
							history.pop_front();
						}
						if (!blockLuaExec)
						{
							wi::lua::RunText(args.sValue);
						}
						else
						{
							post("Lua execution is disabled", LogLevel::Error);
						}
						inputField.SetText("");
					});
					wi::Color theme_color_idle = wi::Color(30, 40, 60, 200);
					wi::Color theme_color_focus = wi::Color(70, 150, 170, 220);
					wi::Color theme_color_active = wi::Color::White();
					wi::Color theme_color_deactivating = wi::Color::lerp(theme_color_focus, wi::Color::White(), 0.5f);
					inputField.SetColor(theme_color_idle); // all states the same, it's gonna be always active anyway
					inputField.font.params.color = wi::Color(160, 240, 250, 255);
					inputField.font.params.shadowColor = wi::Color::Transparent();

					toggleButton.Create("V");
					toggleButton.OnClick([](wi::gui::EventArgs args) {
						Toggle();
						});
					toggleButton.SetColor(theme_color_idle, wi::gui::IDLE);
					toggleButton.SetColor(theme_color_focus, wi::gui::FOCUS);
					toggleButton.SetColor(theme_color_active, wi::gui::ACTIVE);
					toggleButton.SetColor(theme_color_deactivating, wi::gui::DEACTIVATING);
					toggleButton.SetShadowRadius(5);
					toggleButton.SetShadowColor(wi::Color(80, 140, 180, 100));
					toggleButton.font.params.color = wi::Color(160, 240, 250, 255);
					toggleButton.font.params.rotation = XM_PI;
					toggleButton.font.params.size = 24;
					toggleButton.font.params.scaling = 3;
					toggleButton.font.params.shadowColor = wi::Color::Transparent();
					for (int i = 0; i < arraysize(toggleButton.sprites); ++i)
					{
						toggleButton.sprites[i].params.enableCornerRounding();
						toggleButton.sprites[i].params.corners_rounding[2].radius = 50;
					}
				}
				if (inputField.GetState() != wi::gui::ACTIVE)
				{
					inputField.SetAsActive();
				}

			}
			else
			{
				inputField.Deactivate();
			}
		}

		if (enabled)
		{
			pos += speed * dt;
		}
		else
		{
			pos -= speed * dt;
		}
		pos = wi::math::Clamp(pos, -canvas.GetLogicalHeight(), 0);

		inputField.SetSize(XMFLOAT2(canvas.GetLogicalWidth() - 40, 20));
		inputField.SetPos(XMFLOAT2(20, canvas.GetLogicalHeight() - 40 + pos));
		inputField.Update(canvas, dt);

		toggleButton.SetSize(XMFLOAT2(100, 100));
		toggleButton.SetPos(XMFLOAT2(canvas.GetLogicalWidth() - toggleButton.GetSize().x - 20, 20 + pos));
		toggleButton.Update(canvas, dt);
	}
	void Draw(
		const wi::Canvas& canvas,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		if (!was_ever_enabled)
			return;
		if (pos <= -canvas.GetLogicalHeight())
			return;

		GraphicsDevice* device = GetDevice();
		device->EventBegin("Backlog", cmd);

		if (!backgroundTex.IsValid())
		{
			const uint8_t colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
			wi::texturehelper::CreateTexture(backgroundTex, colorData, 1, 2);
			device->SetName(&backgroundTex, "wi::backlog::backgroundTex");
		}

		wi::image::Params fx = wi::image::Params((float)canvas.GetLogicalWidth(), (float)canvas.GetLogicalHeight());
		fx.pos = XMFLOAT3(0, pos, 0);
		fx.opacity = wi::math::Lerp(1, 0, -pos / canvas.GetLogicalHeight());
		if (colorspace != ColorSpace::SRGB)
		{
			fx.enableLinearOutputMapping(9);
		}
		wi::image::Draw(&backgroundTex, fx, cmd);

		wi::image::Params inputbg;
		inputbg.color = wi::Color(80, 140, 180, 200);
		inputbg.pos = inputField.translation;
		inputbg.pos.x -= 8;
		inputbg.pos.y -= 8;
		inputbg.siz = inputField.GetSize();
		inputbg.siz.x += 16;
		inputbg.siz.y += 16;
		inputbg.enableCornerRounding();
		inputbg.corners_rounding[0].radius = 10;
		inputbg.corners_rounding[1].radius = 10;
		inputbg.corners_rounding[2].radius = 10;
		inputbg.corners_rounding[3].radius = 10;
		if (colorspace != ColorSpace::SRGB)
		{
			inputbg.enableLinearOutputMapping(9);
		}
		wi::image::Draw(nullptr, inputbg, cmd);

		if (colorspace != ColorSpace::SRGB)
		{
			inputField.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
			inputField.font.params.enableLinearOutputMapping(9);
			toggleButton.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
			toggleButton.font.params.enableLinearOutputMapping(9);
		}
		inputField.Render(canvas, cmd);

		Rect rect;
		rect.left = 0;
		rect.right = (int32_t)canvas.GetPhysicalWidth();
		rect.top = 0;
		rect.bottom = (int32_t)canvas.GetPhysicalHeight();
		device->BindScissorRects(1, &rect, cmd);

		toggleButton.Render(canvas, cmd);

		rect.bottom = int32_t(canvas.LogicalToPhysical(inputField.GetPos().y - 15));
		device->BindScissorRects(1, &rect, cmd);

		DrawOutputText(canvas, cmd, colorspace);

		rect.left = 0;
		rect.right = std::numeric_limits<int>::max();
		rect.top = 0;
		rect.bottom = std::numeric_limits<int>::max();
		device->BindScissorRects(1, &rect, cmd);
		device->EventEnd(cmd);
	}

	void DrawOutputText(
		const wi::Canvas& canvas,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		std::scoped_lock lock(logLock);
		wi::font::SetCanvas(canvas); // always set here as it can be called from outside...
		wi::font::Params params = font_params;
		params.cursor = {};
		if (refitscroll)
		{
			float textheight = wi::font::TextHeight(getTextWithoutLock(), params);
			float limit = canvas.GetLogicalHeight() - 50;
			if (scroll + textheight > limit)
			{
				scroll = limit - textheight;
			}
			refitscroll = false;
		}
		params.posX = 5;
		params.posY = pos + scroll;
		params.h_wrap = canvas.GetLogicalWidth() - params.posX;
		if (colorspace != ColorSpace::SRGB)
		{
			params.enableLinearOutputMapping(9);
		}
		for (auto& x : entries)
		{
			switch (x.level)
			{
			case LogLevel::Warning:
				params.color = wi::Color::Warning();
				break;
			case LogLevel::Error:
				params.color = wi::Color::Error();
				break;
			default:
				params.color = font_params.color;
				break;
			}
			params.cursor = wi::font::Draw(x.text, params, cmd);
		}
		unseen = LogLevel::None;
	}

	std::string getText()
	{
		std::scoped_lock lock(logLock);
		return getTextWithoutLock();
	}
	void clear()
	{
		std::scoped_lock lock(logLock);
		entries.clear();
		scroll = 0;
	}
	void post(const char* input, LogLevel level)
	{
		if (logLevel > level)
		{
			return;
		}

		// This is explicitly scoped for scoped_lock!
		{
			std::scoped_lock lock(logLock);

			std::string str;
			switch (level)
			{
			default:
			case LogLevel::Default:
				str = "";
				break;
			case LogLevel::Warning:
				str = "[Warning] ";
				break;
			case LogLevel::Error:
				str = "[Error] ";
				break;
			}
			str += input;
			str += '\n';
			LogEntry entry;
			entry.text = str;
			entry.level = level;
			entries.push_back(entry);
			if (entries.size() > deletefromline)
			{
				entries.pop_front();
			}
			refitscroll = true;

			switch (level)
			{
			default:
			case LogLevel::Default:
				wi::helper::DebugOut(str, wi::helper::DebugLevel::Normal);
				break;
			case LogLevel::Warning:
				wi::helper::DebugOut(str, wi::helper::DebugLevel::Warning);
				break;
			case LogLevel::Error:
				wi::helper::DebugOut(str, wi::helper::DebugLevel::Error);
				break;
			}

			unseen = std::max(unseen, level);

			// lock released on block end
		}

		if (level >= LogLevel::Error)
		{
			write_logfile(); // will lock mutex
		}
	}
	void post(const std::string& input, LogLevel level)
	{
		post(input.c_str(), level);
	}

	void historyPrev()
	{
		std::scoped_lock lock(logLock);
		if (!history.empty())
		{
			inputField.SetText(history[history.size() - 1 - historyPos].text);
			inputField.SetAsActive();
			if ((size_t)historyPos < history.size() - 1)
			{
				historyPos++;
			}
		}
	}
	void historyNext()
	{
		std::scoped_lock lock(logLock);
		if (!history.empty())
		{
			if (historyPos > 0)
			{
				historyPos--;
			}
			inputField.SetText(history[history.size() - 1 - historyPos].text);
			inputField.SetAsActive();
		}
	}

	void setBackground(Texture* texture)
	{
		backgroundTex = *texture;
	}
	void setFontSize(int value)
	{
		font_params.size = value;
	}
	void setFontRowspacing(float value)
	{
		font_params.spacingY = value;
	}
	void setFontColor(wi::Color color)
	{
		font_params.color = color;
	}

	bool isActive() { return enabled; }

	void Lock()
	{
		locked = true;
		enabled = false;
	}
	void Unlock()
	{
		locked = false;
	}

	void BlockLuaExecution()
	{
		blockLuaExec = true;
	}
	void UnblockLuaExecution()
	{
		blockLuaExec = false;
	}

	void SetLogLevel(LogLevel newLevel)
	{
		logLevel = newLevel;
	}

	LogLevel GetUnseenLogLevelMax()
	{
		return unseen;
	}
}
