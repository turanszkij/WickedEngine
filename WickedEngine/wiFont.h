#pragma once 
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"
#include "wiCanvas.h"
#include "wiMath.h"

#include <string>

namespace wi::font
{
	// Do not alter order because it is bound to lua manually
	enum Alignment
	{
		WIFALIGN_LEFT,		// left alignment (horizontal)
		WIFALIGN_CENTER,	// center alignment (horizontal or vertical)
		WIFALIGN_RIGHT,		// right alignment (horizontal)
		WIFALIGN_TOP,		// top alignment (vertical)
		WIFALIGN_BOTTOM		// bottom alignment (vertical)
	};

	static constexpr int WIFONTSIZE_DEFAULT = 16;

	struct Cursor
	{
		XMFLOAT2 position = {}; // the next character's position offset from the first character (logical canvas units)
		XMFLOAT2 size = {}; // the written text's measurements from the first character (logical canvas units)
	};

	struct Params
	{
		union
		{
			XMFLOAT3 position = {}; // position in logical canvas units
			struct // back-compat aliasing
			{
				float posX; // position in horizontal direction (logical canvas units)
				float posY; // position in vertical direction (logical canvas units)
			};
		};
		int size = WIFONTSIZE_DEFAULT; // line height (logical canvas units)
		float scaling = 1; // this will apply upscaling to the text while keeping the same resolution (size) of the font
		float rotation = 0; // rotation around alignment anchor (in radians)
		float spacingX = 0, spacingY = 0; // minimum spacing between characters (logical canvas units)
		Alignment h_align = WIFALIGN_LEFT; // horizontal alignment
		Alignment v_align = WIFALIGN_TOP; // vertical alignment
		wi::Color color; // base color of the text characters
		wi::Color shadowColor; // transparent disables, any other color enables shadow under text
		float h_wrap = -1; // wrap start width (-1 default for no wrap) (logical canvas units)
		int style = 0; // 0: use default font style, other values can be taken from the wi::font::AddFontStyle() funtion's return value
		float softness = 0; // value in [0,1] range (requires SDF rendering to be enabled)
		float bolden = 0; // value in [0,1] range (requires SDF rendering to be enabled)
		float shadow_softness = 0.5f; // value in [0,1] range (requires SDF rendering to be enabled)
		float shadow_bolden = 0.1f; // value in [0,1] range (requires SDF rendering to be enabled)
		float shadow_offset_x = 0; // offset for shadow under the text in logical canvas coordinates
		float shadow_offset_y = 0; // offset for shadow under the text in logical canvas coordinates
		Cursor cursor; // cursor can be used to continue text drawing by taking the Draw's return value (optional)
		float hdr_scaling = 1.0f; // a scaling value for use by linear output mapping
		float intensity = 1.0f; // color multiplier
		float shadow_intensity = 1.0f; // shadow color multiplier
		const XMMATRIX* customProjection = nullptr;
		const XMMATRIX* customRotation = nullptr;

		enum FLAGS
		{
			EMPTY = 0,
			SDF_RENDERING = 1 << 0,
			OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1 << 1,
			OUTPUT_COLOR_SPACE_LINEAR = 1 << 2,
			DEPTH_TEST = 1 << 3,
		};
		uint32_t _flags = SDF_RENDERING;

		constexpr bool isSDFRenderingEnabled() const { return _flags & SDF_RENDERING; }
		constexpr bool isHDR10OutputMappingEnabled() const { return _flags & OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		constexpr bool isLinearOutputMappingEnabled() const { return _flags & OUTPUT_COLOR_SPACE_LINEAR; }
		constexpr bool isDepthTestEnabled() const { return _flags & DEPTH_TEST; }

		// enable Signed Distance Field (SDF) font rendering (enabled by default)
		constexpr void enableSDFRendering() { _flags |= SDF_RENDERING; }
		// enable HDR10 output mapping, if this image can be interpreted in linear space and converted to HDR10 display format
		constexpr void enableHDR10OutputMapping() { _flags |= OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		// enable linear output mapping, which means removing gamma curve and outputting in linear space (useful for blending in HDR space)
		constexpr void enableLinearOutputMapping(float scaling = 1.0f) { _flags |= OUTPUT_COLOR_SPACE_LINEAR; hdr_scaling = scaling; }
		constexpr void enableDepthTest() { _flags |= DEPTH_TEST; }

		constexpr void disableSDFRendering() { _flags &= ~SDF_RENDERING; }
		constexpr void disableHDR10OutputMapping() { _flags &= ~OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		constexpr void disableLinearOutputMapping() { _flags &= ~OUTPUT_COLOR_SPACE_LINEAR; }
		constexpr void disableDepthTest() { _flags &= ~DEPTH_TEST; }

		Params(
			float posX = 0,
			float posY = 0,
			int size = WIFONTSIZE_DEFAULT,
			Alignment h_align = WIFALIGN_LEFT,
			Alignment v_align = WIFALIGN_TOP,
			wi::Color color = wi::Color(255, 255, 255, 255),
			wi::Color shadowColor = wi::Color(0, 0, 0, 0)
		) :
			position(posX, posY, 0),
			size(size),
			h_align(h_align),
			v_align(v_align),
			color(color),
			shadowColor(shadowColor)
		{}

		Params(
			wi::Color color,
			wi::Color shadowColor = wi::Color(0, 0, 0, 0),
			float softness = 0.08f,
			float bolden = 0,
			float shadow_softness = 0.5f,
			float shadow_bolden = 0.1f,
			float shadow_offset_x = 0,
			float shadow_offset_y = 0
		) :
			color(color),
			shadowColor(shadowColor),
			softness(softness),
			bolden(bolden),
			shadow_softness(shadow_softness),
			shadow_bolden(shadow_bolden),
			shadow_offset_x(shadow_offset_x),
			shadow_offset_y(shadow_offset_y)
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
	//  copyData : whether data is copied away for storage. If false (default) developer must ensure that it is not deleted
	//	Returns fontStyleID that is reusable. If font already exists, just return its ID
	//	NOTE: When loading font with this method, the developer must ensure that font data is
	//	not deleted while the font is in use (unless copyData is specified as true)
	int AddFontStyle(const std::string& fontName, const uint8_t* data, size_t size, bool copyData = false);

	// Set canvas for the CommandList to handle DPI-aware font rendering on the current thread
	void SetCanvas(const wi::Canvas& current_canvas);
	// Call once per frame to update font atlas texture
	//	upscaling : this should be the DPI upscaling factor, otherwise there will be no upscaling. Upscaling will cause glyphs to be cached at higher resolution.
	void UpdateAtlas(float upscaling = 1.0f);

	// Draw text with specified parameters and return cursor for last word
	//	The next Draw() can continue from where this left off by using the return value of this function
	//	in wi::font::Params::cursor
	Cursor Draw(const char* text, size_t text_length, const Params& params, wi::graphics::CommandList cmd);
	Cursor Draw(const wchar_t* text, size_t text_length, const Params& params, wi::graphics::CommandList cmd);
	Cursor Draw(const char* text, const Params& params, wi::graphics::CommandList cmd);
	Cursor Draw(const wchar_t* text, const Params& params, wi::graphics::CommandList cmd);
	Cursor Draw(const std::string& text, const Params& params, wi::graphics::CommandList cmd);
	Cursor Draw(const std::wstring& text, const Params& params, wi::graphics::CommandList cmd);

	// Computes the text's size measurements in logical canvas coordinates
	XMFLOAT2 TextSize(const char* text, size_t text_length, const Params& params);
	XMFLOAT2 TextSize(const wchar_t* text, size_t text_length, const Params& params);
	XMFLOAT2 TextSize(const char* text, const Params& params);
	XMFLOAT2 TextSize(const wchar_t* text, const Params& params);
	XMFLOAT2 TextSize(const std::string& text, const Params& params);
	XMFLOAT2 TextSize(const std::wstring& text, const Params& params);

	// Computes the text's cursor coordinate for a given string
	Cursor TextCursor(const char* text, size_t text_length, const Params& params);
	Cursor TextCursor(const wchar_t* text, size_t text_length, const Params& params);
	Cursor TextCursor(const char* text, const Params& params);
	Cursor TextCursor(const wchar_t* text, const Params& params);
	Cursor TextCursor(const std::string& text, const Params& params);
	Cursor TextCursor(const std::wstring& text, const Params& params);

	// Computes the text's width in logical canvas coordinates
	//	Avoid calling TextWidth() and TextHeight() both, instead use TextSize() if you need both measurements!
	float TextWidth(const char* text, size_t text_length, const Params& params);
	float TextWidth(const wchar_t* text, size_t text_length, const Params& params);
	float TextWidth(const char* text, const Params& params);
	float TextWidth(const wchar_t* text, const Params& params);
	float TextWidth(const std::string& text, const Params& params);
	float TextWidth(const std::wstring& text, const Params& params);

	// Computes the text's height in logical canvas coordinates
	//	Avoid calling TextWidth() and TextHeight() both, instead use TextSize() if you need both measurements!
	float TextHeight(const char* text, size_t text_length, const Params& params);
	float TextHeight(const wchar_t* text, size_t text_length, const Params& params);
	float TextHeight(const char* text, const Params& params);
	float TextHeight(const wchar_t* text, const Params& params);
	float TextHeight(const std::string& text, const Params& params);
	float TextHeight(const std::wstring& text, const Params& params);

}
