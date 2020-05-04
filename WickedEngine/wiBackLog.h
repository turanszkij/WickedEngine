#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

#include <string>
#include <fstream>

namespace wiBackLog
{
	void Toggle();
	void Scroll(int direction);
	void Update();
	void Draw(wiGraphics::CommandList cmd);

	std::string getText();
	void clear();
	void post(const char* input);
	void input(const char& input);
	void acceptInput();
	void deletefromInput();
	void save(std::ofstream& file);

	void historyPrev();
	void historyNext();

	bool isActive();

	void setBackground(wiGraphics::Texture* texture);
	void setFontSize(int value);
	void setFontRowspacing(float value);
};
