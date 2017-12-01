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
public:
	static std::string FONTPATH;
protected:
	struct Vertex
	{
		XMFLOAT2 Pos;
		XMHALF2 Tex;
	};
	static wiGraphicsTypes::GPURingBuffer       *vertexBuffer;
	static wiGraphicsTypes::GPUBuffer           *indexBuffer;

	static wiGraphicsTypes::VertexLayout		*vertexLayout;
	static wiGraphicsTypes::VertexShader		*vertexShader;
	static wiGraphicsTypes::PixelShader			*pixelShader;
	static wiGraphicsTypes::BlendState			*blendState;
	static wiGraphicsTypes::RasterizerState		*rasterizerState;
	static wiGraphicsTypes::DepthStencilState	*depthStencilState;
	static wiGraphicsTypes::GraphicsPSO			*PSO;
	
	static void SetUpStates();
public:
	static void LoadShaders();
private:
	static void LoadVertexBuffer();
	static void LoadIndices();



	struct wiFontStyle{
		std::string name;
		wiGraphicsTypes::Texture2D* texture;
		
		struct LookUp{
			int ascii;
			char character;
			float left;
			float right;
			int pixelWidth;
		};
		LookUp lookup[128];
		int texWidth, texHeight;
		int lineHeight;

		wiFontStyle(){}
		wiFontStyle(const std::string& newName);
		void CleanUp();
	};
	static std::vector<wiFontStyle> fontStyles;


	static void ModifyGeo(volatile Vertex* vertexList, const std::wstring& text, wiFontProps props, int style);

public:
	static void Initialize();
	static void SetUpStaticComponents();
	static void CleanUpStatic();

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

	void CleanUp();
};
