#include "wiSpriteFont.h"
#include "wiHelper.h"

using namespace std;
using namespace wiGraphics;

void wiSpriteFont::FixedUpdate()
{

}
void wiSpriteFont::Update(float dt)
{

}

void wiSpriteFont::Draw(CommandList cmd) const
{
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

void wiSpriteFont::SetText(const string& value)
{
	wiHelper::StringConvert(value, text);
}
void wiSpriteFont::SetText(const wstring& value)
{
	text = value;
}

string wiSpriteFont::GetTextA() const
{
	string retVal;
	wiHelper::StringConvert(text, retVal);
	return retVal;
}
const wstring& wiSpriteFont::GetText() const
{
	return text;
}
