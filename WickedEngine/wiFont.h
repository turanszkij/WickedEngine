#pragma once 
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

#include <string>

// Do not alter order because it is bound to lua manually
enum wiFontAlign
{
	WIFALIGN_LEFT,
	WIFALIGN_CENTER,
	WIFALIGN_RIGHT,
	WIFALIGN_TOP,
	WIFALIGN_BOTTOM
};

static const int WIFONTSIZE_DEFAULT = 16;

struct wiFontParams
{
	float posX, posY;
	int size = WIFONTSIZE_DEFAULT; // line height in DPI scaled units
	float scaling = 1;
	float spacingX = 1, spacingY = 1; // minimum spacing between characters
	wiFontAlign h_align, v_align;
	wiColor color;
	wiColor shadowColor;
	float h_wrap = -1; // wrap start width (-1 default for no wrap)
	int style = 0;

	wiFontParams(float posX = 0, float posY = 0, int size = WIFONTSIZE_DEFAULT, wiFontAlign h_align = WIFALIGN_LEFT, wiFontAlign v_align = WIFALIGN_TOP
		, wiColor color = wiColor(255, 255, 255, 255), wiColor shadowColor = wiColor(0,0,0,0))
		:posX(posX), posY(posY), size(size), h_align(h_align), v_align(v_align), color(color), shadowColor(shadowColor)
	{}
};

namespace wiFont
{
	void Initialize();

	const wiGraphics::Texture* GetAtlas();

	// Returns the font directory
	const std::string& GetFontPath();
	// Sets the font directory
	void SetFontPath(const std::string& path);

	// Create a font. Returns fontStyleID that is reusable. If font already exists, just return its ID
	int AddFontStyle(const std::string& fontName);

	void Draw(const char* text, const wiFontParams& params, wiGraphics::CommandList cmd);
	void Draw(const wchar_t* text, const wiFontParams& params, wiGraphics::CommandList cmd);
	void Draw(const std::string& text, const wiFontParams& params, wiGraphics::CommandList cmd);
	void Draw(const std::wstring& text, const wiFontParams& params, wiGraphics::CommandList cmd);

	float textWidth(const char* text, const wiFontParams& params);
	float textWidth(const wchar_t* text, const wiFontParams& params);
	float textWidth(const std::string& text, const wiFontParams& params);
	float textWidth(const std::wstring& text, const wiFontParams& params);

	float textHeight(const char* text, const wiFontParams& params);
	float textHeight(const wchar_t* text, const wiFontParams& params);
	float textHeight(const std::string& text, const wiFontParams& params);
	float textHeight(const std::wstring& text, const wiFontParams& params);

};
