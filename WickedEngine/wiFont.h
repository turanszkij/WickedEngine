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
	int posX, posY;
	int size = WIFONTSIZE_DEFAULT; // line height in pixels
	float scaling = 1;
	int spacingX, spacingY;
	wiFontAlign h_align, v_align;
	wiColor color;
	wiColor shadowColor;
	int h_wrap = -1; // wrap start in pixels (-1 default for no wrap)
	int style = 0;

	wiFontParams(int posX = 0, int posY = 0, int size = WIFONTSIZE_DEFAULT, wiFontAlign h_align = WIFALIGN_LEFT, wiFontAlign v_align = WIFALIGN_TOP
		, int spacingX = 0, int spacingY = 0, wiColor color = wiColor(255, 255, 255, 255), wiColor shadowColor = wiColor(0,0,0,0))
		:posX(posX), posY(posY), size(size), h_align(h_align), v_align(v_align), spacingX(spacingX), spacingY(spacingY), color(color), shadowColor(shadowColor)
	{}
};

namespace wiFont
{
	void Initialize();

	void LoadShaders();
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

	int textWidth(const char* text, const wiFontParams& params);
	int textWidth(const wchar_t* text, const wiFontParams& params);
	int textWidth(const std::string& text, const wiFontParams& params);
	int textWidth(const std::wstring& text, const wiFontParams& params);

	int textHeight(const char* text, const wiFontParams& params);
	int textHeight(const wchar_t* text, const wiFontParams& params);
	int textHeight(const std::string& text, const wiFontParams& params);
	int textHeight(const std::wstring& text, const wiFontParams& params);

};
