#pragma once
#include "wiFont.h"

#include <string>

class wiSpriteFont
{
private:
	enum FLAGS
	{
		EMPTY = 0,
		HIDDEN = 1 << 0,
		DISABLE_UPDATE = 1 << 1,
	};
	uint32_t _flags = EMPTY;
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
	virtual void Draw(wiGraphics::CommandList cmd) const;

	constexpr void SetHidden(bool value = true) { if (value) { _flags |= HIDDEN; } else { _flags &= ~HIDDEN; } }
	constexpr bool IsHidden() const { return _flags & HIDDEN; }
	constexpr void SetDisableUpdate(bool value = true) { if (value) { _flags |= DISABLE_UPDATE; } else { _flags &= ~DISABLE_UPDATE; } }
	constexpr bool IsDisableUpdate() const { return _flags & DISABLE_UPDATE; }

	float textWidth() const;
	float textHeight() const;

	void SetText(const std::string& value);
	void SetText(const std::wstring& value);

	std::string GetTextA() const;
	const std::wstring& GetText() const;
};
