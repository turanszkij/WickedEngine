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

		if (anim.typewriter.time > 0)
		{
			size_t text_length = text.length();
			size_t text_length_prev = std::min(text_length, size_t(wi::math::Lerp(float(std::min(text_length, anim.typewriter.character_start)), float(text_length + 1), anim.typewriter.elapsed / anim.typewriter.time)));
			anim.typewriter.elapsed += dt;
			size_t text_length_next = std::min(text_length, size_t(wi::math::Lerp(float(std::min(text_length, anim.typewriter.character_start)), float(text_length + 1), anim.typewriter.elapsed / anim.typewriter.time)));

			if (anim.typewriter.soundinstance.IsValid())
			{
				if (!anim.typewriter.IsFinished() && wi::audio::IsEnded(&anim.typewriter.soundinstance) && text_length_prev != text_length_next && text[text_length_next - 1] != ' ' && anim.typewriter.soundinstance.IsValid())
				{
					wi::audio::Stop(&anim.typewriter.soundinstance);
					if (!IsHidden())
					{
						wi::audio::Play(&anim.typewriter.soundinstance);
					}
				}
				wi::audio::ExitLoop(&anim.typewriter.soundinstance);
			}

			if (anim.typewriter.looped && anim.typewriter.elapsed > anim.typewriter.time)
			{
				anim.typewriter.reset();
			}
		}
	}

	void SpriteFont::Draw(CommandList cmd) const
	{
		if (IsHidden())
			return;

		size_t text_length = text.length();

		if (anim.typewriter.time > 0)
		{
			text_length = std::min(text_length, size_t(wi::math::Lerp(float(std::min(text_length, anim.typewriter.character_start)), float(text_length + 1), anim.typewriter.elapsed / anim.typewriter.time)));
		}

		wi::font::Draw(text.c_str(), text_length, params, cmd);
	}

	XMFLOAT2 SpriteFont::TextSize() const
	{
		return wi::font::TextSize(text, params);
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
