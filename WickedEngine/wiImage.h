#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"
#include "wiColor.h"
#include "wiCanvas.h"
#include "wiPrimitive.h"

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
			CORNER_ROUNDING = 1 << 8,
			DEPTH_TEST = 1 << 9,
			ANGULAR_DOUBLESIDED = 1 << 10,
			ANGULAR_INVERSE = 1 << 11,
			DISTORTION_MASK = 1 << 12,
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
		float intensity = 1;
		float hdr_scaling = 1.0f; // a scaling value for use by linear output mapping
		float mask_alpha_range_start = 0; // constrain mask alpha to not go below this level
		float mask_alpha_range_end = 1; // constrain mask alpha to not go above this level
		XMFLOAT2 angular_softness_direction = XMFLOAT2(0, 1);
		float angular_softness_inner_angle = 0;
		float angular_softness_outer_angle = 0;

		// you can deform the image by its corners (0: top left, 1: top right, 2: bottom left, 3: bottom right)
		XMFLOAT2 corners[4] = {
			XMFLOAT2(0, 0), XMFLOAT2(1, 0),
			XMFLOAT2(0, 1), XMFLOAT2(1, 1)
		};
		const XMMATRIX* customRotation = nullptr;
		const XMMATRIX* customProjection = nullptr;

		struct Rounding
		{
			float radius = 0; // the radius of corner (in logical pixel units)
			int segments = 18; // how many segments to add to smoothing curve
		} corners_rounding[4]; // specify rounding corners (0: top left, 1: top right, 2: bottom left, 3: bottom right)

		float border_soften = 0; // how much alpha softening to apply to image border in range [0, 1] (0: disable)

		uint8_t stencilRef = 0;
		STENCILMODE stencilComp = STENCILMODE_DISABLED;
		STENCILREFMODE stencilRefMode = STENCILREFMODE_ALL;
		wi::enums::BLENDMODE blendFlag = wi::enums::BLENDMODE_ALPHA;
		SAMPLEMODE sampleFlag = SAMPLEMODE_MIRROR;
		QUALITY quality = QUALITY_LINEAR;

		const wi::graphics::Texture* maskMap = nullptr;
		const wi::graphics::Texture* backgroundMap = nullptr;
		int image_subresource = -1;
		int mask_subresource = -1;
		int background_subresource = -1;

		// Set a mask map that will be used to multiply the base image
		constexpr void setMaskMap(const wi::graphics::Texture* tex) { maskMap = tex; }
		// Set a texture that will be used to blend to the transparent part of the image with screen coordinates
		//	Will be used if using enableBackground()
		//	If you don't set this per image, then wi::image::SetBackground() will be used instead
		constexpr void setBackgroundMap(const wi::graphics::Texture* tex) { backgroundMap = tex; }

		constexpr bool isDrawRectEnabled() const { return _flags & DRAWRECT; }
		constexpr bool isDrawRect2Enabled() const { return _flags & DRAWRECT2; }
		constexpr bool isMirrorEnabled() const { return _flags & MIRROR; }
		constexpr bool isExtractNormalMapEnabled() const { return _flags & EXTRACT_NORMALMAP; }
		constexpr bool isFullScreenEnabled() const { return _flags & FULLSCREEN; }
		constexpr bool isBackgroundEnabled() const { return _flags & BACKGROUND; }
		constexpr bool isHDR10OutputMappingEnabled() const { return _flags & OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		constexpr bool isLinearOutputMappingEnabled() const { return _flags & OUTPUT_COLOR_SPACE_LINEAR; }
		constexpr bool isCornerRoundingEnabled() const { return _flags & CORNER_ROUNDING; }
		constexpr bool isDepthTestEnabled() const { return _flags & DEPTH_TEST; }
		constexpr bool isAngularSoftnessDoubleSided() const { return _flags & ANGULAR_DOUBLESIDED; }
		constexpr bool isAngularSoftnessInverse() const { return _flags & ANGULAR_INVERSE; }
		constexpr bool isDistortionMaskEnabled() const { return _flags & DISTORTION_MASK; }

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
		// enable background, which samples a background screen texture on transparent areas instead of alpha blending
		//	the background tex should be bound with wi::image::SetBackground() beforehand
		constexpr void enableBackground() { _flags |= BACKGROUND; }
		// enable HDR10 output mapping, if this image can be interpreted in linear space and converted to HDR10 display format
		constexpr void enableHDR10OutputMapping() { _flags |= OUTPUT_COLOR_SPACE_HDR10_ST2084; }
		// enable linear output mapping, which means removing gamma curve and outputting in linear space (useful for blending in HDR space)
		constexpr void enableLinearOutputMapping(float scaling = 1.0f) { _flags |= OUTPUT_COLOR_SPACE_LINEAR; hdr_scaling = scaling; }
		constexpr void enableCornerRounding() { _flags |= CORNER_ROUNDING; }
		constexpr void enableDepthTest() { _flags |= DEPTH_TEST; }
		constexpr void enableAngularSoftnessDoubleSided() { _flags |= ANGULAR_DOUBLESIDED; }
		constexpr void enableAngularSoftnessInverse() { _flags |= ANGULAR_INVERSE; }
		// Mask texture RG will be used for distortion of screen UVs for background image, A will be used as opacity
		constexpr void enableDistortionMask() { _flags |= DISTORTION_MASK; }

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
		constexpr void disableCornerRounding() { _flags &= ~CORNER_ROUNDING; }
		constexpr void disableDepthTest() { _flags &= ~DEPTH_TEST; }
		constexpr void disableAngularSoftnessDoubleSided() { _flags &= ~ANGULAR_DOUBLESIDED; }
		constexpr void disableAngularSoftnessInverse() { _flags &= ~ANGULAR_INVERSE; }
		constexpr void disableDistortionMask() { _flags &= ~DISTORTION_MASK; }

		Params() = default;

		Params(
			float width,
			float height
		) :
			siz(width, height)
		{}

		Params(
			float posX,
			float posY,
			float width,
			float height,
			const XMFLOAT4& color = XMFLOAT4(1, 1, 1, 1)
		) :
			pos(posX, posY, 0),
			siz(width, height),
			color(color)
		{}

		Params(
			const XMFLOAT4& color,
			wi::enums::BLENDMODE blendFlag = wi::enums::BLENDMODE_ALPHA,
			bool background = false
		) :
			color(color),
			blendFlag(blendFlag)
		{
			if (background)
			{
				enableBackground();
			}
		}

		Params(
			const wi::primitive::Hitbox2D& hitbox, const XMFLOAT4& color = XMFLOAT4(1, 1, 1, 1)
		) :
			pos(XMFLOAT3(hitbox.pos.x, hitbox.pos.y, 0)),
			siz(hitbox.siz),
			color(color)
		{}
	};


	// Set canvas to handle DPI-aware image rendering (applied to all image rendering commands on the current thread)
	void SetCanvas(const wi::Canvas& current_canvas);

	// Set a background texture (applied to all image rendering commands on the current thread that used enableBackground())
	void SetBackground(const wi::graphics::Texture& texture);

	// Draw the specified texture with the specified parameters
	void Draw(const wi::graphics::Texture* texture, const Params& params, wi::graphics::CommandList cmd);

	// Initializes the image renderer
	void Initialize();

}
