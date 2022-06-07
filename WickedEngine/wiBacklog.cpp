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

#include <mutex>
#include <deque>
#include <limits>
#include <thread>
#include <iostream>

using namespace wi::graphics;

namespace wi::backlog
{
	bool enabled = false;
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
	std::string inputArea;
	int historyPos = 0;
	wi::font::Params font_params;
	wi::SpinLock logLock;
	Texture backgroundTex;
	bool refitscroll = false;

	bool locked = false;
	bool blockLuaExec = false;
	LogLevel logLevel = LogLevel::Default;

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
	}
	void Scroll(float dir)
	{
		scroll += dir;
	}
	void Update(const wi::Canvas& canvas, float dt)
	{
		if (!locked)
		{
			if (wi::input::Press(wi::input::KEYBOARD_BUTTON_HOME))
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
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_ENTER))
				{
					acceptInput();
				}
				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_PAGEUP))
				{
					Scroll(1000.0f * dt);
				}
				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_PAGEDOWN))
				{
					Scroll(-1000.0f * dt);
				}
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
	}
	void Draw(const wi::Canvas& canvas, CommandList cmd)
	{
		if (pos > -canvas.GetLogicalHeight())
		{
			if (!backgroundTex.IsValid())
			{
				const uint8_t colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
				wi::texturehelper::CreateTexture(backgroundTex, colorData, 1, 2);
			}

			wi::image::Params fx = wi::image::Params((float)canvas.GetLogicalWidth(), (float)canvas.GetLogicalHeight());
			fx.pos = XMFLOAT3(0, pos, 0);
			fx.opacity = wi::math::Lerp(1, 0, -pos / canvas.GetLogicalHeight());
			wi::image::Draw(&backgroundTex, fx, cmd);

			fx.pos = XMFLOAT3(5, canvas.GetLogicalHeight() - 30, 0);
			fx.siz = XMFLOAT2(canvas.GetLogicalWidth() - 10, 25);
			fx.color = XMFLOAT4(1, 1, 1, 0.2f);
			wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

			wi::font::Params params = wi::font::Params(10, canvas.GetLogicalHeight() - 10, wi::font::WIFONTSIZE_DEFAULT, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_BOTTOM);
			params.h_wrap = canvas.GetLogicalWidth() - params.posX;
			params.v_align = wi::font::WIFALIGN_BOTTOM;
			params.shadowColor = 0x10000000u;
			params.shadow_offset_x = 2;
			params.shadow_offset_y = 2;
			params.shadow_softness = 1;
			wi::font::Draw(inputArea, params, cmd);

			Rect rect;
			rect.left = 0;
			rect.right = (int32_t)canvas.GetPhysicalWidth();
			rect.top = 0;
			rect.bottom = int32_t(canvas.LogicalToPhysical(canvas.GetLogicalHeight() - 35));
			wi::graphics::GetDevice()->BindScissorRects(1, &rect, cmd);

			DrawOutputText(canvas, cmd);

			rect.left = -std::numeric_limits<int>::max();
			rect.right = std::numeric_limits<int>::max();
			rect.top = -std::numeric_limits<int>::max();
			rect.bottom = std::numeric_limits<int>::max();
			wi::graphics::GetDevice()->BindScissorRects(1, &rect, cmd);
		}
	}

	void DrawOutputText(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
	{
		std::scoped_lock lock(logLock);
		wi::font::SetCanvas(canvas); // always set here as it can be called from outside...
		font_params.cursor = {};
		if (refitscroll)
		{
			float textheight = wi::font::TextHeight(getTextWithoutLock(), font_params);
			float limit = canvas.GetLogicalHeight() - 35;
			if (scroll + textheight > limit)
			{
				scroll = limit - textheight;
			}
			refitscroll = false;
		}
		font_params.posX = 5;
		font_params.posY = pos + scroll;
		font_params.h_wrap = canvas.GetLogicalWidth() - font_params.posX;
		for (auto& x : entries)
		{
			switch (x.level)
			{
			case LogLevel::Warning:
				font_params.color = wi::Color::Warning();
				break;
			case LogLevel::Error:
				font_params.color = wi::Color::Error();
				break;
			default:
				font_params.color = wi::Color::White();
				break;
			}
			font_params.cursor = wi::font::Draw(x.text, font_params, cmd);
		}
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
	void post(const std::string& input, LogLevel level)
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

#ifdef _WIN32
			OutputDebugStringA(str.c_str());
#endif // _WIN32

			switch (level)
			{
			default:
			case LogLevel::Default:
				std::cout << str;
				break;
			case LogLevel::Warning:
				std::clog << str;
				break;
			case LogLevel::Error:
				std::cerr << str;
				break;
			}

			// lock released on block end
		}

		if (level >= LogLevel::Error)
		{
			write_logfile(); // will lock mutex
		}
	}
	void input(const char input)
	{
		std::scoped_lock lock(logLock);
		inputArea += input;
	}
	void acceptInput()
	{
		historyPos = 0;
		post(inputArea.c_str());
		LogEntry entry;
		entry.text = inputArea;
		entry.level = LogLevel::Default;
		history.push_back(entry);
		if (history.size() > deletefromline)
		{
			history.pop_front();
		}
		if (!blockLuaExec)
		{
			wi::lua::RunText(inputArea);
		}
		else
		{
			post("Lua execution is disabled", LogLevel::Error);
		}
		inputArea.clear();
	}
	void deletefromInput()
	{
		std::scoped_lock lock(logLock);
		if (!inputArea.empty())
		{
			inputArea.pop_back();
		}
	}

	void historyPrev()
	{
		std::scoped_lock lock(logLock);
		if (!history.empty())
		{
			inputArea = history[history.size() - 1 - historyPos].text;
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
			inputArea = history[history.size() - 1 - historyPos].text;
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
}
