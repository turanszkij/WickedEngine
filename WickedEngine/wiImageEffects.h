#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

enum BLENDMODE
{
	BLENDMODE_OPAQUE,
	BLENDMODE_ALPHA,
	BLENDMODE_PREMULTIPLIED,
	BLENDMODE_ADDITIVE,
	BLENDMODE_COUNT
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
	QUALITY_BILINEAR,
	QUALITY_ANISOTROPIC,
	QUALITY_COUNT
};
enum ImageType
{
	SCREEN,
	WORLD,
};

class wiImageEffects {
public:
	XMFLOAT3 pos;
	XMFLOAT2 siz;
	XMFLOAT4 col;
	XMFLOAT2 scale;
	XMFLOAT4 drawRec;
	XMFLOAT2 texOffset;
	XMFLOAT2 sunPos;
	//(x,y,z) : direction, (w) :isenabled?
	XMFLOAT4 lookAt;
	// (0,0) : upperleft, (0.5,0.5) : center, (1,1) : bottomright
	XMFLOAT2 pivot;
	// mirror horizontally
	bool mirror;
	float blur, blurDir;
	float fade;
	float opacity;
	float rotation;
	bool extractNormalMap;
	float mipLevel;
	bool hdr;
	UINT stencilRef;
	STENCILMODE stencilComp;

	// don't set anything, just fill the whole screen
	bool presentFullScreen;

	BLENDMODE blendFlag;
	ImageType typeFlag;
	SAMPLEMODE sampleFlag;
	QUALITY quality;

	struct Bloom {
		bool  separate;
		float threshold;
		float saturation;

		void clear() { separate = false; threshold = saturation = 0; }
		Bloom() { clear(); }
	};
	Bloom bloom;
	struct Processing {
		bool active;
		bool motionBlur;
		bool outline;
		float dofStrength;
		bool fxaa;
		bool ssao;
		XMFLOAT2 ssss;
		bool ssr;
		bool linDepth;
		bool colorGrade;
		bool stereogram;
		bool tonemap;
		bool reprojectDepthBuffer;
		bool downsampleDepthBuffer4x;
		bool temporalAAResolve;
		float sharpen;

		void clear() { 
			active = motionBlur = outline = fxaa = ssao = linDepth = colorGrade = ssr = stereogram = tonemap = reprojectDepthBuffer = downsampleDepthBuffer4x = temporalAAResolve = false; 
			dofStrength = sharpen = 0; ssss = XMFLOAT2(0, 0); 
		}
		void setDOF(float value) { dofStrength = value; active = value > FLT_EPSILON; }
		void setMotionBlur(bool value) { motionBlur = value; active = value; }
		void setOutline(bool value) { outline = value; active = value; }
		void setFXAA(bool value) { fxaa = value; active = value; }
		void setSSAO(bool value) { ssao = value; active = value; }
		void setLinDepth(bool value) { linDepth = value; active = value; }
		void setColorGrade(bool value) { colorGrade = value; active = value; }
		//direction*Properties
		void setSSSS(const XMFLOAT2& value) { ssss = value; active = value.x || value.y; }
		void setSSR(bool value) { ssr = value; active = value; }
		void setStereogram(bool value) { stereogram = value; active = value; }
		void setToneMap(bool value) { tonemap = value; active = value; }
		void setDepthBufferReprojection(bool value) { reprojectDepthBuffer = value; active = value; }
		void setDepthBufferDownsampling(bool value) { downsampleDepthBuffer4x = value; active = value; }
		void setTemporalAAResolve(bool value) { temporalAAResolve = value; active = value; }
		void setSharpen(float value) { sharpen = value; active = value > FLT_EPSILON; }
		Processing() { clear(); }
	};
	Processing process;
	bool deferred;
	wiGraphicsTypes::Texture2D* maskMap, *distortionMap, *refractionSource;
	// Generic texture
	void setMaskMap(wiGraphicsTypes::Texture2D* view) { maskMap = view; }
	// The normalmap texture which should distort the refraction source
	void setDistortionMap(wiGraphicsTypes::Texture2D* view) { distortionMap = view; }
	// The texture which should be distorted
	void setRefractionSource(wiGraphicsTypes::Texture2D* view) { refractionSource = view; }

	void init() {
		pos = XMFLOAT3(0, 0, 0);
		siz = XMFLOAT2(1, 1);
		col = XMFLOAT4(1, 1, 1, 1);
		scale = XMFLOAT2(1, 1);
		drawRec = XMFLOAT4(0, 0, 0, 0);
		texOffset = XMFLOAT2(0, 0);
		sunPos = XMFLOAT2(0, 0);
		lookAt = XMFLOAT4(0, 0, 0, 0);
		pivot = XMFLOAT2(0, 0);
		mirror = false;
		blur = blurDir = fade = rotation = 0.0f;
		opacity = 1.0f;
		extractNormalMap = false;
		hdr = false;
		mipLevel = 0.f;
		stencilRef = 0;
		stencilComp = STENCILMODE_DISABLED;
		presentFullScreen = false;
		blendFlag = BLENDMODE_ALPHA;
		typeFlag = SCREEN;
		sampleFlag = SAMPLEMODE_MIRROR;
		quality = QUALITY_BILINEAR;
		bloom = Bloom();
		process = Processing();
		deferred = false;
		maskMap = nullptr;
		distortionMap = nullptr;
		refractionSource = nullptr;
	}


	wiImageEffects() {
		init();
	}
	wiImageEffects(float width, float height) {
		init();
		siz = XMFLOAT2(width, height);
	}
	wiImageEffects(float posX, float posY, float width, float height) {
		init();
		pos.x = posX;
		pos.y = posY;
		siz = XMFLOAT2(width, height);
	}
};

