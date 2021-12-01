#pragma once 
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"
#include "wiCanvas.h"

#include <string>

namespace wi::font
{
	// Do not alter order because it is bound to lua manually
	enum Alignment
	{
		WIFALIGN_LEFT,
		WIFALIGN_CENTER,
		WIFALIGN_RIGHT,
		WIFALIGN_TOP,
		WIFALIGN_BOTTOM
	};

	static constexpr int WIFONTSIZE_DEFAULT = 16;

	struct Params
	{
		float posX = 0;
		float posY = 0;
		int size = WIFONTSIZE_DEFAULT; // line height in DPI scaled units
		float scaling = 1;
		float spacingX = 1, spacingY = 1; // minimum spacing between characters
		Alignment h_align, v_align;
		wi::Color color;
		wi::Color shadowColor;
		float h_wrap = -1; // wrap start width (-1 default for no wrap)
		int style = 0;

		Params(
			float posX = 0,
			float posY = 0,
			int size = WIFONTSIZE_DEFAULT,
			Alignment h_align = WIFALIGN_LEFT,
			Alignment v_align = WIFALIGN_TOP,
			wi::Color color = wi::Color(255, 255, 255, 255),
			wi::Color shadowColor = wi::Color(0, 0, 0, 0)
		) :
			posX(posX),
			posY(posY),
			size(size),
			h_align(h_align),
			v_align(v_align),
			color(color),
			shadowColor(shadowColor)
		{}
	};

	// Initializes the font renderer
	void Initialize();

	// Get the texture that contains currently cached glyphs
	const wi::graphics::Texture* GetAtlas();

	// Create a font from a file. It must be an existing .ttf file.
	//	fontName : path to .ttf font
	//	Returns fontStyleID that is reusable. If font already exists, just return its ID
	int AddFontStyle(const std::string& fontName);

	// Create a font from binary data. It must be the binary data of a .ttf file
	//	fontName : name of the font (it doesn't need to be a path)
	//	data : binary data of the .ttf font
	//	size : size of the font binary data
	//	Returns fontStyleID that is reusable. If font already exists, just return its ID
	//	NOTE: When loading font with this method, the developer must ensure that font data is
	//	not deleted while the font is in use
	int AddFontStyle(const std::string& fontName, const uint8_t* data, size_t size);

	// Set canvas for the CommandList to handle DPI-aware font rendering
	void SetCanvas(const wi::Canvas& canvas, wi::graphics::CommandList cmd);

	void Draw(const char* text, const Params& params, wi::graphics::CommandList cmd);
	void Draw(const wchar_t* text, const Params& params, wi::graphics::CommandList cmd);
	void Draw(const std::string& text, const Params& params, wi::graphics::CommandList cmd);
	void Draw(const std::wstring& text, const Params& params, wi::graphics::CommandList cmd);

	float TextWidth(const char* text, const Params& params);
	float TextWidth(const wchar_t* text, const Params& params);
	float TextWidth(const std::string& text, const Params& params);
	float TextWidth(const std::wstring& text, const Params& params);

	float TextHeight(const char* text, const Params& params);
	float TextHeight(const wchar_t* text, const Params& params);
	float TextHeight(const std::string& text, const Params& params);
	float TextHeight(const std::wstring& text, const Params& params);

}
