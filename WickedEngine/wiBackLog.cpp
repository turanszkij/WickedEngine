#include "wiBackLog.h"
#include "wiMath.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiTextureHelper.h"
#include "wiSpinLock.h"
#include "wiFont.h"
#include "wiSpriteFont.h"
#include "wiImage.h"
#include "wiLua.h"
#include "wiInput.h"
#include "wiPlatform.h"

#include <mutex>
#include <deque>

using namespace std;
using namespace wiGraphics;


namespace wiBackLog
{
	bool enabled = false;
	deque<string> stream;
	deque<string> history;
	const float speed = 50.0f;
	unsigned int deletefromline = 500;
	float pos = -FLT_MAX;
	float scroll = 0;
	string inputArea;
	int historyPos = 0;
	wiSpriteFont font;
	wiSpinLock logLock;
	Texture backgroundTex;
	bool refitscroll = false;

	void Toggle() 
	{
		enabled = !enabled;
	}
	void Scroll(int dir) 
	{
		scroll += dir;
	}
	void Update() 
	{
		if (wiInput::Press(wiInput::KEYBOARD_BUTTON_HOME))
		{
			Toggle();
		}

		if (isActive())
		{
			if (wiInput::Press(wiInput::KEYBOARD_BUTTON_UP))
			{
				historyPrev();
			}
			if (wiInput::Press(wiInput::KEYBOARD_BUTTON_DOWN))
			{
				historyNext();
			}
			if (wiInput::Press(wiInput::KEYBOARD_BUTTON_ENTER))
			{
				acceptInput();
			}
			if (wiInput::Down(wiInput::KEYBOARD_BUTTON_PAGEUP))
			{
				Scroll(10);
			}
			if (wiInput::Down(wiInput::KEYBOARD_BUTTON_PAGEDOWN))
			{
				Scroll(-10);
			}
		}

		if (enabled)
		{
			pos += speed;
		}
		else
		{
			pos -= speed;
		}
		pos = wiMath::Clamp(pos, -wiRenderer::GetDevice()->GetScreenHeight(), 0);
	}
	void Draw(CommandList cmd)
	{
		if (pos > -wiRenderer::GetDevice()->GetScreenHeight())
		{
			if (!backgroundTex.IsValid())
			{
				const uint8_t colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
				wiTextureHelper::CreateTexture(backgroundTex, colorData, 1, 2);
			}

			wiImageParams fx = wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
			fx.pos = XMFLOAT3(0, pos, 0);
			fx.opacity = wiMath::Lerp(1, 0, -pos / wiRenderer::GetDevice()->GetScreenHeight());
			wiImage::Draw(&backgroundTex, fx, cmd);
			wiFont::Draw(inputArea, wiFontParams(10, wiRenderer::GetDevice()->GetScreenHeight() - 10, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_BOTTOM), cmd);


			font.SetText(getText());
			if (refitscroll)
			{
				refitscroll = false;
				float textheight = font.textHeight();
				float limit = wiRenderer::GetDevice()->GetScreenHeight() * 0.9f;
				if (scroll + textheight > limit)
				{
					scroll = limit - textheight;
				}
			}
			font.params.posX = 50;
			font.params.posY = pos + scroll;
			Rect rect;
			rect.left = 0;
			rect.right = (int32_t)wiRenderer::GetDevice()->GetResolutionWidth();
			rect.top = 0;
			rect.bottom = int32_t(wiRenderer::GetDevice()->GetResolutionHeight() * 0.9f);
			wiRenderer::GetDevice()->BindScissorRects(1, &rect, cmd);
			font.Draw(cmd);
			rect.left = -INT_MAX;
			rect.right = INT_MAX;
			rect.top = -INT_MAX;
			rect.bottom = INT_MAX;
			wiRenderer::GetDevice()->BindScissorRects(1, &rect, cmd);
		}
	}


	string getText() 
	{
		logLock.lock();
		string retval;
		for (auto& x : stream)
		{
			retval += x;
		}
		logLock.unlock();
		return retval;
	}
	void clear() 
	{
		logLock.lock();
		stream.clear();
		scroll = 0;
		logLock.unlock();
	}
	void post(const char* input) 
	{
		logLock.lock();
		string str;
		str = input;
		str += '\n';
		stream.push_back(str);
		if (stream.size() > deletefromline)
		{
			stream.pop_front();
		}
		refitscroll = true;
		logLock.unlock();

#ifdef _WIN32
		OutputDebugStringA(str.c_str());
#endif // _WIN32

        std::cout << str;
	}
	void input(const char input) 
	{
		inputArea += input;
	}
	void acceptInput() 
	{
		historyPos = 0;
		post(inputArea.c_str());
		history.push_back(inputArea);
		if (history.size() > deletefromline) {
			history.pop_front();
		}
		wiLua::RunText(inputArea);
		inputArea.clear();
	}
	void deletefromInput() 
	{
		if(!inputArea.empty())
			inputArea.pop_back();
	}

	void historyPrev() 
	{
		if (!history.empty()) 
		{
			inputArea = history[history.size() - 1 - historyPos];
			if ((size_t)historyPos < history.size() - 1)
				historyPos++;
		}
	}
	void historyNext() 
	{
		if (!history.empty()) 
		{
			if (historyPos > 0)
				historyPos--;
			inputArea = history[history.size() - 1 - historyPos];
		}
	}

	void setBackground(Texture* texture)
	{
		backgroundTex = *texture;
	}
	void setFontSize(int value)
	{
		font.params.size = value;
	}
	void setFontRowspacing(float value)
	{
		font.params.spacingY = value;
	}

	bool isActive() { return enabled; }

}
