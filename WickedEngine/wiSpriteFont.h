#pragma once
#include "wiFont.h"

#include <string>

class wiSpriteFont
{
private:
public:
	std::wstring text;
	wiFontParams params;

	wiSpriteFont() = default;
	wiSpriteFont(const std::string& value, const wiFontParams& params = wiFontParams()) :params(params)
	{
		SetText(value);
	}

	virtual void FixedUpdate();
	virtual void Update(float dt);
	void Draw(wiGraphics::CommandList cmd) const;

	float textWidth() const;
	float textHeight() const;

	void SetText(const std::string& value);
	void SetText(const std::wstring& value);

	std::string GetTextA() const;
	const std::wstring& GetText() const;
};
