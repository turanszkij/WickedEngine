#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"
#include "wiColor.h"
#include "wiCanvas.h"

namespace wi::image
{
	// Do not alter order or value because it is bound to lua manually!
	enum STENCILMODE
	{
		STENCILMODE_DISABLED,
		STENCILMODE_EQUAL,
		STENCILMODE_LESS,
		STENCILMODE_LESSEQUAL,
		STENCILMODE_GREATER,
		STENCILMODE_GREATEREQUAL,
		STENCILMODE_NOT,
		STENCILMODE_ALWAYS,
		STENCILMODE_COUNT
	};
	enum STENCILREFMODE
	{
		STENCILREFMODE_ENGINE,
		STENCILREFMODE_USER,
		STENCILREFMODE_ALL,
		STENCILREFMODE_COUNT
	};
	enum SAMPLEMODE
	{
		SAMPLEMODE_CLAMP,
		SAMPLEMODE_WRAP,
		SAMPLEMODE_MIRROR,
		SAMPLEMODE_COUNT
	};
	enum QUALITY
	{
		QUALITY_NEAREST,
		QUALITY_LINEAR,
		QUALITY_ANISOTROPIC,
		QUALITY_COUNT
	};

	struct Params
	{
		enum FLAGS
		{
			EMPTY = 0,
			DRAWRECT = 1 << 0,
			DRAWRECT2 = 1 << 1,
			MIRROR = 1 << 2,
			EXTRACT_NORMALMAP = 1 << 3,
			FULLSCREEN = 1 << 4,
			BACKGROUND = 1 << 5,
			OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1 << 6,
			OUTPUT_COLOR_SPACE_LINEAR = 1 << 7,
		};
		uint32_t _flags = EMPTY;

		XMFLOAT3 pos = XMFLOAT3(0, 0, 0);
		XMFLOAT2 siz = XMFLOAT2(1, 1);
		XMFLOAT2 scale = XMFLOAT2(1, 1);
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 drawRect = XMFLOAT4(0, 0, 0, 0);
		XMFLOAT4 drawRect2 = XMFLOAT4(0, 0, 0, 0);
		XMFLOAT2 texOffset = XMFLOAT2(0, 0);
		XMFLOAT2 texOffset2 = XMFLOAT2(0, 0);
		XMFLOAT2 pivot = XMFLOAT2(0, 0); // (0,0) : upperleft, (0.5,0.5) : center, (1,1) : bottomright
		float rotation = 0;
		float fade = 0;
		float opacity = 1;
		float hdr_scaling = 1.0f;

		// you can deform the image by its corners (0: top left, 1: top right, 2: bottom left, 3: bottom right)
		XMFLOAT2 corners[4] = {
			XMFLOAT2(0, 0), XMFLOAT2(1, 0),
			XMFLOAT2(0, 1), XMFLOAT2(1, 1)
		};
		const XMMATRIX* customRotation = nullptr;
		const XMMATRIX* customProjection = nullptr;

		uint8_t stencilRef = 0;
		STENCILMODE stencilComp = STENCILMODE_DISABLED;
		STENCILREFMODE stencilRefMode = STENCILREFMODE_ALL;
		wi::enums::BLENDMODE blendFlag = wi::enums::BLENDMODE_ALPHA;
		SAMPLEMODE sampleFlag = SAMPLEMODE_MIRROR;
		QUALITY quality = QUALITY_LINEAR;

		const wi::graphics::Texture* maskMap = nullptr;
		// Generic texture
		constexpr void setMaskMap(const wi::graphics::Texture* tex) { maskMap = tex; }

		constexpr bool isDrawRectEnabled() const { return _flags & DRAWRECT; }
		constexpr bool isDrawRect2Enabled() const { return _flags & DRAWRECT2; }
		constexpr bool isMirrorEnabled() const { return _flags & MIRROR; }
		constexpr bool isExtractNormalMapEnabled() const { return _flags & EXTRACT_NORMALMAP; }
		constexpr bool isFullScreenEnabled() const { return _flags & FULLSCREEN; }
		constexpr bool isBackgroundEnabled() const { return _flags & BACKGROUND; }
		constexpr bool isHDR10OutputMappingEnabled() const { return _flags & OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		constexpr bool isLinearOutputMappingEnabled() const { return _flags & OUTPUT_COLOR_SPACE_LINEAR; }

		// enables draw rectangle for base texture (cutout texture outside draw rectangle)
		constexpr void enableDrawRect(const XMFLOAT4& rect) { _flags |= DRAWRECT; drawRect = rect; }
		// enables draw rectangle for mask texture (cutout texture outside draw rectangle)
		constexpr void enableDrawRect2(const XMFLOAT4& rect) { _flags |= DRAWRECT2; drawRect2 = rect; }
		// mirror the image horizontally
		constexpr void enableMirror() { _flags |= MIRROR; }
		// enable normal map extraction shader that will perform texcolor * 2 - 1 (preferably onto a signed render target)
		constexpr void enableExtractNormalMap() { _flags |= EXTRACT_NORMALMAP; }
		// enable full screen override. It just draws texture onto the full screen, disabling any other setup except sampler and stencil)
		constexpr void enableFullScreen() { _flags |= FULLSCREEN; }
		// enable background blur, which samples a background screen texture on a specified mip level on transparent areas instead of alpha blending
		//	the background tex should be bound with wi::image::SetBackground() beforehand
		constexpr void enableBackground() { _flags |= BACKGROUND; }
		// enable HDR10 output mapping, if this image can be interpreted in linear space and converted to HDR10 display format
		//	this only works together with the enableFullScreen() override!
		constexpr void enableHDR10OutputMapping() { _flags |= OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		// enable linear output mapping, which means removing gamma curve and outputting in linear space (useful for blending in HDR space)
		//	this only works together with the enableFullScreen() override!
		constexpr void enableLinearOutputMapping(float scaling = 1.0f) { _flags |= OUTPUT_COLOR_SPACE_LINEAR; hdr_scaling = scaling; }

		// disable draw rectangle for base texture (whole texture will be drawn, no cutout)
		constexpr void disableDrawRect() { _flags &= ~DRAWRECT; }
		// disable draw rectangle for mask texture (whole texture will be drawn, no cutout)
		constexpr void disableDrawRect2() { _flags &= ~DRAWRECT2; }
		constexpr void disableMirror() { _flags &= ~MIRROR; }
		constexpr void disableExtractNormalMap() { _flags &= ~EXTRACT_NORMALMAP; }
		constexpr void disableFullScreen() { _flags &= ~FULLSCREEN; }
		constexpr void disableBackground() { _flags &= ~BACKGROUND; }
		constexpr void disableHDR10OutputMapping() { _flags &= ~OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		constexpr void disableLinearOutputMapping() { _flags &= ~OUTPUT_COLOR_SPACE_LINEAR; }

		Params() = default;
		Params(float width, float height) :
			siz(width, height)
		{}
		Params(float posX, float posY, float width, float height, const XMFLOAT4& color = XMFLOAT4(1, 1, 1, 1)) :
			pos(posX, posY, 0),
			siz(width, height),
			color(color)
		{}
	};


	// Set canvas to handle DPI-aware image rendering (applied to all image rendering commands on this CommandList)
	void SetCanvas(const wi::Canvas& canvas, wi::graphics::CommandList cmd);

	// Set a background texture (applied to all image rendering commands on this CommandList that used enableBackground())
	void SetBackground(const wi::graphics::Texture& texture, wi::graphics::CommandList cmd);

	// Draw the specified texture with the specified parameters
	void Draw(const wi::graphics::Texture* texture, const Params& params, wi::graphics::CommandList cmd);

	// Initializes the image renderer
	void Initialize();

}
