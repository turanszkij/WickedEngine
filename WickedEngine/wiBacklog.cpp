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
	struct InternalState
	{
		bool enabled = false;
		std::deque<std::string> stream;
		std::deque<std::string> history;
		const float speed = 4000.0f;
		const size_t deletefromline = 500;
		float pos = std::numeric_limits<float>::lowest();
		float scroll = 0;
		std::string inputArea;
		int historyPos = 0;
		wi::SpriteFont font;
		wi::SpinLock logLock;
		Texture backgroundTex;
		bool refitscroll = false;

		bool locked = false;
		bool blockLuaExec = false;
		LogLevel logLevel = LogLevel::Default;

		// The logwriter object will automatically write out the backlog to the temp folder when it's destroyed
		//	Should happen on application exit
		struct LogWriter
		{
			void write_logfile()
			{
				std::string filename = wi::helper::GetTempDirectoryPath() + "wiBacklog.txt";
				std::string text = getText(); // will lock mutex
				wi::helper::FileWrite(filename, (const uint8_t*)text.c_str(), text.length());
			}

			~LogWriter()
			{
				write_logfile();
			}
		} logwriter;
	};
	inline InternalState& backlog_internal()
	{
		static InternalState internal_state;
		return internal_state;
	}

	void Toggle() 
	{
		backlog_internal().enabled = !backlog_internal().enabled;
	}
	void Scroll(float dir) 
	{
		backlog_internal().scroll += dir;
	}
	void Update(const wi::Canvas& canvas, float dt)
	{
		if (!backlog_internal().locked)
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

		if (backlog_internal().enabled)
		{
			backlog_internal().pos += backlog_internal().speed * dt;
		}
		else
		{
			backlog_internal().pos -= backlog_internal().speed * dt;
		}
		backlog_internal().pos = wi::math::Clamp(backlog_internal().pos, -canvas.GetLogicalHeight(), 0);
	}
	void Draw(const wi::Canvas& canvas, CommandList cmd)
	{
		if (backlog_internal().pos > -canvas.GetLogicalHeight())
		{
			if (!backlog_internal().backgroundTex.IsValid())
			{
				const uint8_t colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
				wi::texturehelper::CreateTexture(backlog_internal().backgroundTex, colorData, 1, 2);
			}

			wi::image::Params fx = wi::image::Params((float)canvas.GetLogicalWidth(), (float)canvas.GetLogicalHeight());
			fx.pos = XMFLOAT3(0, backlog_internal().pos, 0);
			fx.opacity = wi::math::Lerp(1, 0, -backlog_internal().pos / canvas.GetLogicalHeight());
			wi::image::Draw(&backlog_internal().backgroundTex, fx, cmd);

			wi::font::Params params = wi::font::Params(10, canvas.GetLogicalHeight() - 10, wi::font::WIFONTSIZE_DEFAULT, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_BOTTOM);
			params.h_wrap = canvas.GetLogicalWidth() - params.posX;
			params.v_align = wi::font::WIFALIGN_BOTTOM;
			wi::font::Draw(backlog_internal().inputArea, params, cmd);

			backlog_internal().font.SetText(getText());
			if (backlog_internal().refitscroll)
			{
				backlog_internal().refitscroll = false;
				float textheight = backlog_internal().font.TextHeight();
				float limit = canvas.GetLogicalHeight() * 0.9f;
				if (backlog_internal().scroll + textheight > limit)
				{
					backlog_internal().scroll = limit - textheight;
				}
			}
			backlog_internal().font.params.posX = 50;
			backlog_internal().font.params.posY = backlog_internal().pos + backlog_internal().scroll;
			backlog_internal().font.params.h_wrap = canvas.GetLogicalWidth() - backlog_internal().font.params.posX;
			Rect rect;
			rect.left = 0;
			rect.right = (int32_t)canvas.GetPhysicalWidth();
			rect.top = 0;
			rect.bottom = int32_t(canvas.GetPhysicalHeight() * 0.9f);
			wi::graphics::GetDevice()->BindScissorRects(1, &rect, cmd);
			backlog_internal().font.Draw(cmd);
			rect.left = -std::numeric_limits<int>::max();
			rect.right = std::numeric_limits<int>::max();
			rect.top = -std::numeric_limits<int>::max();
			rect.bottom = std::numeric_limits<int>::max();
			wi::graphics::GetDevice()->BindScissorRects(1, &rect, cmd);
		}
	}


	std::string getText()
	{
		std::scoped_lock lock(backlog_internal().logLock);
		std::string retval;
		for (auto& x : backlog_internal().stream)
		{
			retval += x;
		}
		return retval;
	}
	void clear() 
	{
		std::scoped_lock lock(backlog_internal().logLock);
		backlog_internal().stream.clear();
		backlog_internal().scroll = 0;
	}
	void post(const std::string& input, LogLevel level)
	{
		if (backlog_internal().logLevel > level)
		{
			return;
		}

		// This is explicitly scoped for scoped_lock!
		{
			std::scoped_lock lock(backlog_internal().logLock);

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
			backlog_internal().stream.push_back(str);
			if (backlog_internal().stream.size() > backlog_internal().deletefromline)
			{
				backlog_internal().stream.pop_front();
			}
			backlog_internal().refitscroll = true;

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
			backlog_internal().logwriter.write_logfile(); // will lock mutex
		}
	}
	void input(const char input) 
	{
		std::scoped_lock lock(backlog_internal().logLock);
		backlog_internal().inputArea += input;
	}
	void acceptInput() 
	{
		backlog_internal().historyPos = 0;
		post(backlog_internal().inputArea.c_str());
		backlog_internal().history.push_back(backlog_internal().inputArea);
		if (backlog_internal().history.size() > backlog_internal().deletefromline)
		{
			backlog_internal().history.pop_front();
		}
		if (!backlog_internal().blockLuaExec)
		{
			wi::lua::RunText(backlog_internal().inputArea);
		}
		else
		{
			post("Lua execution is disabled", LogLevel::Error);
		}
		backlog_internal().inputArea.clear();
	}
	void deletefromInput() 
	{
		std::scoped_lock lock(backlog_internal().logLock);
		if (!backlog_internal().inputArea.empty())
		{
			backlog_internal().inputArea.pop_back();
		}
	}

	void historyPrev() 
	{
		std::scoped_lock lock(backlog_internal().logLock);
		if (!backlog_internal().history.empty())
		{
			backlog_internal().inputArea = backlog_internal().history[backlog_internal().history.size() - 1 - backlog_internal().historyPos];
			if ((size_t)backlog_internal().historyPos < backlog_internal().history.size() - 1)
			{
				backlog_internal().historyPos++;
			}
		}
	}
	void historyNext() 
	{
		std::scoped_lock lock(backlog_internal().logLock);
		if (!backlog_internal().history.empty())
		{
			if (backlog_internal().historyPos > 0)
			{
				backlog_internal().historyPos--;
			}
			backlog_internal().inputArea = backlog_internal().history[backlog_internal().history.size() - 1 - backlog_internal().historyPos];
		}
	}

	void setBackground(Texture* texture)
	{
		backlog_internal().backgroundTex = *texture;
	}
	void setFontSize(int value)
	{
		backlog_internal().font.params.size = value;
	}
	void setFontRowspacing(float value)
	{
		backlog_internal().font.params.spacingY = value;
	}

	bool isActive() { return backlog_internal().enabled; }

	void Lock()
	{
		backlog_internal().locked = true;
		backlog_internal().enabled = false;
	}
	void Unlock()
	{
		backlog_internal().locked = false;
	}

	void BlockLuaExecution()
	{
		backlog_internal().blockLuaExec = true;
	}
	void UnblockLuaExecution()
	{
		backlog_internal().blockLuaExec = false;
	}

	void SetLogLevel(LogLevel newLevel)
	{
		backlog_internal().logLevel = newLevel;
	}
}
