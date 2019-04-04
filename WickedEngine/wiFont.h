#pragma once 
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"


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

	wiFontParams(int posX = 0, int posY = 0, int size = 16, wiFontAlign h_align = WIFALIGN_LEFT, wiFontAlign v_align = WIFALIGN_TOP
		, int spacingX = 0, int spacingY = 0, const wiColor& color = wiColor(255, 255, 255, 255), const wiColor& shadowColor = wiColor(0,0,0,0))
		:posX(posX), posY(posY), size(size), h_align(h_align), v_align(v_align), spacingX(spacingX), spacingY(spacingY), color(color), shadowColor(shadowColor)
	{}
};

class wiFont
{
public:
	static void Initialize();
	static void CleanUp();

	static void LoadShaders();
	static const wiGraphics::Texture2D* GetAtlas();

	// Returns the font path that can be modified
	static std::string& GetFontPath();

	// Create a font. Returns fontStyleID that is reusable. If font already exists, just return its ID
	static int AddFontStyle(const std::string& fontName);

	std::wstring text;
	wiFontParams params;
	int style;

	wiFont(const std::string& text = "", wiFontParams params = wiFontParams(), int style = 0);
	wiFont(const std::wstring& text, wiFontParams params = wiFontParams(), int style = 0);
	
	void Draw(GRAPHICSTHREAD threadID) const;

	int textWidth() const;
	int textHeight() const;

	void SetText(const std::string& text);
	void SetText(const std::wstring& text);
	std::wstring GetText() const;
	std::string GetTextA() const;

};
