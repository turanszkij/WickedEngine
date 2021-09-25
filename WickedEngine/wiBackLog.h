#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"

#include <string>
#include <fstream>

namespace wiBackLog
{
	enum LogLevel
	{
		None,
		Default,
		Warning,
		Error
	};

	void Toggle();
	void Scroll(int direction);
	void Update(const wiCanvas& canvas);
	void Draw(const wiCanvas& canvas, wiGraphics::CommandList cmd);

	std::string getText();
	void clear();
	void post(const std::string& input);
	void post(const std::string& input, LogLevel level);
	void input(const char input);
	void acceptInput();
	void deletefromInput();

	void historyPrev();
	void historyNext();

	bool isActive();

	void setBackground(wiGraphics::Texture* texture);
	void setFontSize(int value);
	void setFontRowspacing(float value);

	void Lock();
	void Unlock();

	void BlockLuaExecution();
	void UnblockLuaExecution();

	void SetLogLevel(LogLevel newLevel);
};
