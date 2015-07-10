#pragma once 
#include "CommonInclude.h"
#define MAX_TEXT 20000

class wiRenderer;

static class wiFont
{
private:
	static mutex MUTEX;
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
		
		void* operator new(size_t size)
		{

			void* result = _aligned_malloc(size,16);
			return result;
		}	
		void operator delete(void* p)
		{
			if(p) _aligned_free(p);
		}
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



	static UINT textlen;
	static SHORT line,pos;
	static BOOL toDraw;
	static DWORD counter;


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


	static void ModifyGeo(const wchar_t* text,XMFLOAT2 sizSpacing,const int& style, ID3D11DeviceContext* context = nullptr);
public:
	static void Initialize();
	static void SetUpStaticComponents();
	static void CleanUpStatic();
	
	static void DrawBlink(wchar_t* text,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);
	static void Draw(const wchar_t* text,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);
	static void DrawBlink(const std::string& text,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);
	static void Draw(const std::string& text,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);

	static void DrawBlink(const string& text,const char* fontStyle,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);
	static void DrawBlink(const wchar_t* text,const char* fontStyle,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);
	static void Draw(const string& text,const char* fontStyle,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);
	static void Draw(const wchar_t* text,const char* fontStyle,XMFLOAT4 posSizSpacing=XMFLOAT4(0,0,0,0), const char* Halign="left", const char* Valign="top", ID3D11DeviceContext* context = nullptr);

	static void Blink(DWORD perframe,DWORD invisibleTime);

	static int textWidth(const wchar_t*,FLOAT siz,const int& style);
	static int textHeight(const wchar_t*,FLOAT siz,const int& style);

	static void addFontStyle( const string& toAdd );
	static int getFontStyleByName( const string& get );

	void CleanUp();
};
