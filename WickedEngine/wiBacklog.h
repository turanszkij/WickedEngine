#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"
#include "wiColor.h"
#include "wiHelper.h"

#include <string>
#include <cassert>

#define wilog_level(str,level,...) {char text[1024]; snprintf(text, sizeof(text), str, ## __VA_ARGS__); wi::backlog::post(text, level);}
#define wilog_messagebox(str,...) {char text[1024]; snprintf(text, sizeof(text), str, ## __VA_ARGS__); wi::backlog::post(text, wi::backlog::LogLevel::Error); wi::helper::messageBox(text, "Error!");}
#define wilog_warning(str,...) {wilog_level(str, wi::backlog::LogLevel::Warning, ## __VA_ARGS__);}
#define wilog_error(str,...) {wilog_level(str, wi::backlog::LogLevel::Error, ## __VA_ARGS__);}
#define wilog(str,...) {wilog_level(str, wi::backlog::LogLevel::Default, ## __VA_ARGS__);}
#define wilog_assert(cond,str,...) {if(!(cond)){wilog_error(str, ## __VA_ARGS__); assert(cond);}}

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
	void Draw(
		const wi::Canvas& canvas,
		wi::graphics::CommandList cmd,
		wi::graphics::ColorSpace colorspace = wi::graphics::ColorSpace::SRGB
	);
	void DrawOutputText(
		const wi::Canvas& canvas,
		wi::graphics::CommandList cmd,
		wi::graphics::ColorSpace colorspace = wi::graphics::ColorSpace::SRGB
	);

	std::string getText();
	void clear();
	void post(const char* input, LogLevel level = LogLevel::Default);
	void post(const std::string& input, LogLevel level = LogLevel::Default);

	void historyPrev();
	void historyNext();

	bool isActive();

	void setBackground(wi::graphics::Texture* texture);
	void setFontSize(int value);
	void setFontRowspacing(float value);
	void setFontColor(wi::Color color);

	void Lock();
	void Unlock();

	void BlockLuaExecution();
	void UnblockLuaExecution();

	void SetLogLevel(LogLevel newLevel);

	LogLevel GetUnseenLogLevelMax();

	// These are no longer used, but kept here to not break user code:
	[[deprecated("does nothing")]] inline void input(const char input) {}
	[[deprecated("does nothing")]] inline void acceptInput() {}
	[[deprecated("does nothing")]] inline void deletefromInput() {}
};
