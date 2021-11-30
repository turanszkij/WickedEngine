#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"

#include <string>

namespace wi::backlog
{
	// Do not modify the order, as this is exposed to LUA scripts as int!
	enum class LogLevel
	{
		None,
		Default,
		Warning,
		Error,
	};

	void Toggle();
	void Scroll(int direction);
	void Update(const wi::Canvas& canvas, float dt = 1.0f / 60.0f);
	void Draw(const wi::Canvas& canvas, wi::graphics::CommandList cmd);

	std::string getText();
	void clear();
	void post(const std::string& input, LogLevel level = LogLevel::Default);
	void input(const char input);
	void acceptInput();
	void deletefromInput();

	void historyPrev();
	void historyNext();

	bool isActive();

	void setBackground(wi::graphics::Texture* texture);
	void setFontSize(int value);
	void setFontRowspacing(float value);

	void Lock();
	void Unlock();

	void BlockLuaExecution();
	void UnblockLuaExecution();

	void SetLogLevel(LogLevel newLevel);
};
