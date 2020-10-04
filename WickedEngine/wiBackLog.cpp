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
#include <sstream>
#include <deque>

using namespace std;
using namespace wiGraphics;


namespace wiBackLog
{

	enum State {
		DISABLED,
		IDLE,
		ACTIVATING,
		DEACTIVATING,
	};

	deque<string> stream;
	deque<string> history;
	State state = DISABLED;
	const float speed = 50.0f;
	unsigned int deletefromline = 100;
	float pos = -FLT_MAX;
	float scroll = 0;
	stringstream inputArea;
	int historyPos = 0;
	wiSpriteFont font;
	wiSpinLock logLock;

	std::unique_ptr<Texture> backgroundTex;

	void Toggle() 
	{
		switch (state) 
		{
		case IDLE:
			state = DEACTIVATING;
			break;
		case DISABLED:
			state = ACTIVATING;
			break;
		default:break;
		};
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

		if (state == DEACTIVATING)
			pos -= speed;
		else if (state == ACTIVATING)
			pos += speed;
		if (pos <= -wiRenderer::GetDevice()->GetScreenHeight())
		{
			state = DISABLED;
			pos = -(float)wiRenderer::GetDevice()->GetScreenHeight();
		}
		else if (pos > 0)
		{
			state = IDLE;
			pos = 0;
		}

		if (scroll + font.textHeight() > int(wiRenderer::GetDevice()->GetScreenHeight() * 0.8f))
		{
			scroll -= 2;
		}
	}
	void Draw(CommandList cmd)
	{
		if (state != DISABLED) 
		{
			if (backgroundTex == nullptr)
			{
				const uint8_t colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
				backgroundTex.reset(new Texture);
				wiTextureHelper::CreateTexture(*backgroundTex.get(), colorData, 1, 2);
			}

			wiImageParams fx = wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
			fx.pos = XMFLOAT3(0, pos, 0);
			fx.opacity = wiMath::Lerp(1, 0, -pos / wiRenderer::GetDevice()->GetScreenHeight());
			wiImage::Draw(backgroundTex.get(), fx, cmd);
			font.SetText(getText());
			font.params.posX = 50;
			font.params.posY = pos + scroll;
			font.Draw(cmd);
			wiFont::Draw(inputArea.str(), wiFontParams(10, wiRenderer::GetDevice()->GetScreenHeight() - 10, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_BOTTOM), cmd);
		}
	}


	string getText() 
	{
		logLock.lock();
		stringstream ss("");
		for (unsigned int i = 0; i < stream.size(); ++i)
			ss << stream[i];
		logLock.unlock();
		return ss.str();
	}
	void clear() 
	{
		logLock.lock();
		stream.clear();
		logLock.unlock();
	}
	void post(const char* input) 
	{
		stringstream ss("");
		ss << input << endl;

		logLock.lock();
		stream.push_back(ss.str().c_str());
		if (stream.size() > deletefromline) {
			stream.pop_front();
		}
		logLock.unlock();

#ifdef _WIN32
		OutputDebugStringA(ss.str().c_str());
#else
        std::cout << ss.str();
#endif // _WIN32
	}
	void input(const char& input) 
	{
		inputArea << input;
	}
	void acceptInput() 
	{
		historyPos = 0;
		stringstream commandStream("");
		commandStream << inputArea.str();
		post(inputArea.str().c_str());
		history.push_back(inputArea.str());
		if (history.size() > deletefromline) {
			history.pop_front();
		}
		wiLua::RunText(inputArea.str());
		inputArea.str("");
	}
	void deletefromInput() 
	{
		stringstream ss(inputArea.str().substr(0, inputArea.str().length() - 1));
		inputArea.str("");
		inputArea << ss.str();
	}
	void save(ofstream& file) 
	{
		for (deque<string>::iterator iter = stream.begin(); iter != stream.end(); ++iter)
			file << iter->c_str();
		file.close();
	}

	void historyPrev() 
	{
		if (!history.empty()) 
		{
			inputArea.str("");
			inputArea << history[history.size() - 1 - historyPos];
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
			inputArea.str("");
			inputArea << history[history.size() - 1 - historyPos];
		}
	}

	void setBackground(Texture* texture)
	{
		backgroundTex.reset(texture);
	}
	void setFontSize(int value)
	{
		font.params.size = value;
	}
	void setFontRowspacing(float value)
	{
		font.params.spacingY = value;
	}

	bool isActive() { return state == IDLE; }

}
