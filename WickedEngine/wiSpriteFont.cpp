#include "wiSpriteFont.h"
#include "wiHelper.h"

using namespace wi::graphics;

namespace wi
{

	void SpriteFont::FixedUpdate()
	{
		if (IsDisableUpdate())
			return;
	}
	void SpriteFont::Update(float dt)
	{
		if (IsDisableUpdate())
			return;
	}

	void SpriteFont::Draw(CommandList cmd) const
	{
		if (IsHidden())
			return;
		wi::font::Draw(text, params, cmd);
	}

	float SpriteFont::TextWidth() const
	{
		return wi::font::TextWidth(text, params);
	}
	float SpriteFont::TextHeight() const
	{
		return wi::font::TextHeight(text, params);
	}

	void SpriteFont::SetText(const std::string& value)
	{
		wi::helper::StringConvert(value, text);
	}
	void SpriteFont::SetText(std::string&& value)
	{
		wi::helper::StringConvert(value, text);
	}
	void SpriteFont::SetText(const std::wstring& value)
	{
		text = value;
	}
	void SpriteFont::SetText(std::wstring&& value)
	{
		text = std::move(value);
	}

	std::string SpriteFont::GetTextA() const
	{
		std::string retVal;
		wi::helper::StringConvert(text, retVal);
		return retVal;
	}
	const std::wstring& SpriteFont::GetText() const
	{
		return text;
	}

}
