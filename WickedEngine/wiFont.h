#pragma once 
#include "CommonInclude.h"
#define MAX_TEXT 20000

enum wiFontAlign
{
	WIFALIGN_LEFT,
	//same as mid
	WIFALIGN_CENTER,
	//same as center
	WIFALIGN_MID,
	WIFALIGN_RIGHT,
	WIFALIGN_TOP,
	WIFALIGN_BOTTOM,
	WIFALIGN_COUNT,
};

class wiFontProps
{
public:
	float size;
	float spacingX, spacingY;
	float posX, posY;
	wiFontAlign h_align, v_align;

	//zero-based properties: add or subtract from values
	wiFontProps(float posX = 0, float posY = 0, float size = 0, wiFontAlign h_align = WIFALIGN_LEFT, wiFontAlign v_align = WIFALIGN_TOP
		, float spacingX = 0, float spacingY = 0)
		:posX(posX), posY(posY), size(size), h_align(h_align), v_align(v_align), spacingX(spacingX), spacingY(spacingY)
	{}
};

class wiFont
{
protected:
	struct Vertex
	{
		XMFLOAT2 Pos;
		XMFLOAT2 Tex;
	};
	static std::vector<Vertex> vertexList;
	struct ConstantBuffer
	{
		XMMATRIX mProjection;
		XMMATRIX mTrans;
		XMFLOAT4 mDimensions;
		
		ALIGN_16
	};
	static ID3D11Buffer*           vertexBuffer,*indexBuffer;

	static ID3D11InputLayout   *vertexLayout;
	static ID3D11VertexShader  *vertexShader;
	static ID3D11PixelShader   *pixelShader;
	static ID3D11BlendState*		blendState;
	static ID3D11Buffer*           constantBuffer;
	static ID3D11SamplerState*			sampleState;
	static ID3D11RasterizerState*		rasterizerState;
	static ID3D11DepthStencilState*	depthStencilState;
	
	static void SetUpStates();
	static void SetUpCB();
	static void LoadShaders();
	static void LoadVertexBuffer();
	static void LoadIndices();



	struct wiFontStyle{
		string name;
		ID3D11ShaderResourceView *texture;
		
		struct LookUp{
			int code;
			int offX,offY,width;
		};
		LookUp lookup[127];
		int texWidth,texHeight,charSize,recSize;

		wiFontStyle(){}
		wiFontStyle(const string& newName);
		void CleanUp();
	};
	static vector<wiFontStyle> fontStyles;


	static void ModifyGeo(const wstring& text, wiFontProps props, int style, ID3D11DeviceContext* context = nullptr);

public:
	static void Initialize();
	static void SetUpStaticComponents();
	static void CleanUpStatic();

	wstring text;
	wiFontProps props;
	int style;

	wiFont(const string& text = "", wiFontProps props = wiFontProps(), int style = 0);
	wiFont(const wstring& text, wiFontProps props = wiFontProps(), int style = 0);
	~wiFont();

	
	void Draw(ID3D11DeviceContext* context = nullptr);


	int textWidth();
	int textHeight();

	static void addFontStyle( const string& toAdd );
	static int getFontStyleByName( const string& get );

	void SetText(const string& text);
	void SetText(const wstring& text);

	void CleanUp();
};
