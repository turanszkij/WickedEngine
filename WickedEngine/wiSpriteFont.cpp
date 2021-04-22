#include "wiSpriteFont.h"
#include "wiHelper.h"

using namespace wiGraphics;

void wiSpriteFont::FixedUpdate()
{
	if (IsDisableUpdate())
		return;
}
void wiSpriteFont::Update(float dt)
{
	if (IsDisableUpdate())
		return;
}

void wiSpriteFont::Draw(CommandList cmd) const
{
	if (IsHidden())
		return;
	wiFont::Draw(text, params, cmd);
}

float wiSpriteFont::textWidth() const
{
	return wiFont::textWidth(text, params);
}
float wiSpriteFont::textHeight() const
{
	return wiFont::textHeight(text, params);
}

void wiSpriteFont::SetText(const std::string& value)
{
	wiHelper::StringConvert(value, text);
}
void wiSpriteFont::SetText(const std::wstring& value)
{
	text = value;
}

std::string wiSpriteFont::GetTextA() const
{
	std::string retVal;
	wiHelper::StringConvert(text, retVal);
	return retVal;
}
const std::wstring& wiSpriteFont::GetText() const
{
	return text;
}
