#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"

struct wiImageParams;

namespace wiImage
{
	void Draw(wiGraphicsTypes::Texture2D* texture, const wiImageParams& params, GRAPHICSTHREAD threadID);

	void DrawDeferred(wiGraphicsTypes::Texture2D* lightmap_diffuse, wiGraphicsTypes::Texture2D* lightmap_specular,
		wiGraphicsTypes::Texture2D* ao, GRAPHICSTHREAD threadID, int stencilref = 0);

	void BindPersistentState(GRAPHICSTHREAD threadID);

	void LoadShaders();
	void Initialize();
};

enum STENCILMODE
{
	STENCILMODE_DISABLED,
	STENCILMODE_GREATER,
	STENCILMODE_LESS,
	STENCILMODE_EQUAL,
	STENCILMODE_COUNT
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
	QUALITY_BICUBIC,
	QUALITY_COUNT
};
enum ImageType
{
	SCREEN,
	WORLD,
};

struct wiImageParams
{
	enum FLAGS
	{
		EMPTY = 0,
		DRAWRECT = 1 << 0,
		MIRROR = 1 << 1,
		EXTRACT_NORMALMAP = 1 << 2,
		HDR = 1 << 3,
		FULLSCREEN = 1 << 4,
	};
	uint32_t _flags = EMPTY;

	XMFLOAT3 pos;
	XMFLOAT2 siz;
	XMFLOAT2 scale;
	XMFLOAT4 col;
	XMFLOAT4 drawRect;
	XMFLOAT4 lookAt;			//(x,y,z) : direction, (w) :isenabled?
	XMFLOAT2 texOffset;
	XMFLOAT2 pivot;				// (0,0) : upperleft, (0.5,0.5) : center, (1,1) : bottomright
	float rotation;
	float mipLevel;
	float fade;
	float opacity;
	XMFLOAT2 corners[4];		// you can deform the image by its corners (0: top left, 1: top right, 2: bottom left, 3: bottom right)

	UINT stencilRef;
	STENCILMODE stencilComp;
	BLENDMODE blendFlag;
	ImageType typeFlag;
	SAMPLEMODE sampleFlag;
	QUALITY quality;

	wiGraphicsTypes::Texture2D* maskMap;
	wiGraphicsTypes::Texture2D* distortionMap;
	wiGraphicsTypes::Texture2D* refractionSource;
	// Generic texture
	void setMaskMap(wiGraphicsTypes::Texture2D* view) { maskMap = view; }
	// The normalmap texture which should distort the refraction source
	void setDistortionMap(wiGraphicsTypes::Texture2D* view) { distortionMap = view; }
	// The texture which should be distorted
	void setRefractionSource(wiGraphicsTypes::Texture2D* view) { refractionSource = view; }

	struct PostProcess 
	{
		enum POSTPROCESS
		{
			DISABLED,
			BLUR,
			LIGHTSHAFT,
			OUTLINE,
			DEPTHOFFIELD,
			MOTIONBLUR,
			BLOOMSEPARATE,
			FXAA,
			SSAO,
			SSSS,
			SSR,
			COLORGRADE,
			STEREOGRAM,
			TONEMAP,
			REPROJECTDEPTHBUFFER,
			DOWNSAMPLEDEPTHBUFFER,
			TEMPORALAA,
			SHARPEN,
			LINEARDEPTH,
			POSTPROCESS_COUNT
		} type = DISABLED;

		union PostProcessParams
		{
			struct Blur
			{
				float x;
				float y;
			} blur;
			Blur ssss;
			Blur sun;
			float dofStrength;
			float sharpen;
			float exposure;
			float bloomThreshold;
		} params;

		bool isActive() const { return type != DISABLED; }
		void clear() { type = DISABLED; }
		void setBlur(const XMFLOAT2& direction) { type = BLUR; params.blur.x = direction.x; params.blur.y = direction.y; }
		void setBloom(float threshold) { type = BLOOMSEPARATE; params.bloomThreshold = threshold; }
		void setDOF(float value) { if (value > 0) { type = DEPTHOFFIELD; params.dofStrength = value; } }
		void setMotionBlur() { type = MOTIONBLUR; }
		void setOutline() { type = OUTLINE; }
		void setFXAA() { type = FXAA; }
		void setSSAO() { type = SSAO; }
		void setLinDepth() { type = LINEARDEPTH; }
		void setColorGrade() { type = COLORGRADE; }
		void setSSSS(const XMFLOAT2& value) { type = SSSS; params.ssss.x = value.x; params.ssss.y = value.y; }
		void setSSR() { type = SSR; }
		void setStereogram() { type = STEREOGRAM; }
		void setToneMap(float exposure) { type = TONEMAP; params.exposure = exposure; }
		void setDepthBufferReprojection() { type = REPROJECTDEPTHBUFFER; }
		void setDepthBufferDownsampling() { type = DOWNSAMPLEDEPTHBUFFER; }
		void setTemporalAAResolve() { type = TEMPORALAA; }
		void setSharpen(float value) { if (value > 0) { type = SHARPEN; params.sharpen = value; } }
		void setLightShaftCenter(const XMFLOAT2& pos) { type = LIGHTSHAFT; params.sun.x = pos.x; params.sun.y = pos.y; }
	};
	PostProcess process;

	void init() 
	{
		_flags = EMPTY;
		pos = XMFLOAT3(0, 0, 0);
		siz = XMFLOAT2(1, 1);
		col = XMFLOAT4(1, 1, 1, 1);
		scale = XMFLOAT2(1, 1);
		drawRect = XMFLOAT4(0, 0, 0, 0);
		texOffset = XMFLOAT2(0, 0);
		lookAt = XMFLOAT4(0, 0, 0, 0);
		pivot = XMFLOAT2(0, 0);
		fade = rotation = 0.0f;
		opacity = 1.0f;
		mipLevel = 0.f;
		stencilRef = 0;
		stencilComp = STENCILMODE_DISABLED;
		blendFlag = BLENDMODE_ALPHA;
		typeFlag = SCREEN;
		sampleFlag = SAMPLEMODE_MIRROR;
		quality = QUALITY_LINEAR;
		maskMap = nullptr;
		distortionMap = nullptr;
		refractionSource = nullptr;
		corners[0] = XMFLOAT2(0, 0);
		corners[1] = XMFLOAT2(1, 0);
		corners[2] = XMFLOAT2(0, 1);
		corners[3] = XMFLOAT2(1, 1);
	}

	constexpr bool isDrawRectEnabled() const { return _flags & DRAWRECT; }
	constexpr bool isMirrorEnabled() const { return _flags & MIRROR; }
	constexpr bool isExtractNormalMapEnabled() const { return _flags & EXTRACT_NORMALMAP; }
	constexpr bool isHDREnabled() const { return _flags & HDR; }
	constexpr bool isFullScreenEnabled() const { return _flags & FULLSCREEN; }

	// enables draw rectangle (cutout texture outside draw rectangle)
	void enableDrawRect(const XMFLOAT4& rect) { _flags |= DRAWRECT; drawRect = rect; }
	// mirror the image horizontally
	void enableMirror() { _flags |= MIRROR; }
	// enable normal map extraction shader that will perform texcolor * 2 - 1 (preferably onto a signed render target)
	void enableExtractNormalMap() { _flags |= EXTRACT_NORMALMAP; }
	// enable HDR blending to float render target
	void enableHDR() { _flags |= HDR; }
	// enable full screen override. It just draws texture onto the full screen, disabling any other setup except sampler and stencil)
	void enableFullScreen() { _flags |= FULLSCREEN; }

	// disable draw rectangle (whole texture will be drawn, no cutout)
	void disableDrawRect() { _flags &= ~DRAWRECT; }
	// disable mirroring
	void disableMirror() { _flags &= ~MIRROR; }
	// disable normal map extraction shader
	void disableExtractNormalMap() { _flags &= ~EXTRACT_NORMALMAP; }
	// if you want to render onto normalized render target
	void disableHDR() { _flags &= ~HDR; }
	// disable full screen override
	void disableFullScreen() { _flags &= ~FULLSCREEN; }


	wiImageParams() {
		init();
	}
	wiImageParams(float width, float height) {
		init();
		siz = XMFLOAT2(width, height);
	}
	wiImageParams(float posX, float posY, float width, float height) {
		init();
		pos.x = posX;
		pos.y = posY;
		siz = XMFLOAT2(width, height);
	}
};
