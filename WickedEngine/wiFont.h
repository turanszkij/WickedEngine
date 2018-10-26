#pragma once 
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "wiColor.h"


// Do not alter order because it is bound to lua manually
enum wiFontAlign
{
	WIFALIGN_LEFT,
	// same as mid
	WIFALIGN_CENTER,
	// same as center
	WIFALIGN_MID,
	WIFALIGN_RIGHT,
	WIFALIGN_TOP,
	WIFALIGN_BOTTOM,
	WIFALIGN_COUNT,
};

class wiFontProps
{
public:
	int size;
	int spacingX, spacingY;
	int posX, posY;
	wiFontAlign h_align, v_align;
	wiColor color;
	wiColor shadowColor;

	wiFontProps(int posX = 0, int posY = 0, int size = -1, wiFontAlign h_align = WIFALIGN_LEFT, wiFontAlign v_align = WIFALIGN_TOP
		, int spacingX = 2, int spacingY = 1, const wiColor& color = wiColor(255, 255, 255, 255), const wiColor& shadowColor = wiColor(0,0,0,0))
		:posX(posX), posY(posY), size(size), h_align(h_align), v_align(v_align), spacingX(spacingX), spacingY(spacingY), color(color), shadowColor(shadowColor)
	{}
};

class wiFont
{
protected:
	struct Vertex
	{
		XMFLOAT2 Pos;
		XMHALF2 Tex;
	};
	
private:



	static void ModifyGeo(volatile Vertex* vertexList, const std::wstring& text, wiFontProps props, int style);

public:
	static void Initialize();
	static void CleanUp();

	static void LoadShaders();
	static void BindPersistentState(GRAPHICSTHREAD threadID);

	std::wstring text;
	wiFontProps props;
	int style;

	wiFont(const std::string& text = "", wiFontProps props = wiFontProps(), int style = 0);
	wiFont(const std::wstring& text, wiFontProps props = wiFontProps(), int style = 0);
	~wiFont();

	
	void Draw(GRAPHICSTHREAD threadID);


	int textWidth();
	int textHeight();

	static void addFontStyle( const std::string& toAdd );
	static int getFontStyleByName( const std::string& get );

	void SetText(const std::string& text);
	void SetText(const std::wstring& text);
	std::wstring GetText();
	std::string GetTextA();

	static std::string& GetFontPath();
};
