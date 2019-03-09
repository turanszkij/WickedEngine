#include "wiBackLog.h"
#include "wiMath.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiTextureHelper.h"
#include "wiSpinLock.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiLua.h"

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
	int scroll = 0;
	stringstream inputArea;
	int historyPos = 0;
	wiFont font;
	wiSpinLock logLock;

	std::unique_ptr<Texture2D> backgroundTex;

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
	void Draw() 
	{
		if (state != DISABLED) 
		{
			if (backgroundTex == nullptr)
			{
				const uint8_t colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
				backgroundTex.reset(new Texture2D);
				HRESULT hr = wiTextureHelper::CreateTexture(*backgroundTex.get(), colorData, 1, 2);
				assert(SUCCEEDED(hr));
			}

			wiImageParams fx = wiImageParams((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
			fx.pos = XMFLOAT3(0, pos, 0);
			fx.opacity = wiMath::Lerp(1, 0, -pos / wiRenderer::GetDevice()->GetScreenHeight());
			wiImage::Draw(backgroundTex.get(), fx, GRAPHICSTHREAD_IMMEDIATE);
			font.SetText(getText());
			font.params.posX = 50;
			font.params.posY = (int)pos + (int)scroll;
			font.Draw(GRAPHICSTHREAD_IMMEDIATE);
			wiFont(inputArea.str().c_str(), wiFontParams(10, wiRenderer::GetDevice()->GetScreenHeight() - 10, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_BOTTOM)).Draw(GRAPHICSTHREAD_IMMEDIATE);
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
		logLock.lock();
		stringstream ss("");
		ss << input << endl;
		stream.push_back(ss.str().c_str());
		if (stream.size() > deletefromline) {
			stream.pop_front();
		}
		logLock.unlock();
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
		wiLua::GetGlobal()->RunText(inputArea.str());
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

	void setBackground(Texture2D* texture)
	{
		backgroundTex.reset(texture);
	}
	void setFontSize(int value)
	{
		font.params.size = value;
	}
	void setFontRowspacing(int value)
	{
		font.params.spacingY = value;
	}

	bool isActive() { return state == IDLE; }

}
