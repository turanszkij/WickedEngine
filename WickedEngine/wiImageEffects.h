#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

enum BLENDMODE{
	BLENDMODE_OPAQUE,
	BLENDMODE_ALPHA,
	BLENDMODE_ADDITIVE,
	BLENDMODE_MAX,
};
enum SAMPLEMODE{
	SAMPLEMODE_CLAMP,
	SAMPLEMODE_WRAP,
	SAMPLEMODE_MIRROR,
};
enum QUALITY{
	QUALITY_NEAREST,
	QUALITY_BILINEAR,
	QUALITY_ANISOTROPIC,
};
enum ImageType{
	SCREEN,
	WORLD,
};
enum Pivot{
	UPPERLEFT,
	CENTER,
};

class wiImageEffects {
public:
	XMFLOAT3 pos;
	XMFLOAT2 siz;
	XMFLOAT2 scale;
	XMFLOAT4 drawRec;
	XMFLOAT2 texOffset;
	XMFLOAT2 offset;
	XMFLOAT2 sunPos;
	XMFLOAT4 lookAt; //.w is for enabled
	float mirror;
	float blur, blurDir;
	float fade;
	float opacity;
	float rotation;
	bool extractNormalMap;
	float mipLevel;
	unsigned int stencilRef;
	int stencilComp;

	// don't set anything, just fill the whole screen
	bool presentFullScreen;

	BLENDMODE blendFlag;
	ImageType typeFlag;
	SAMPLEMODE sampleFlag;
	QUALITY quality;
	Pivot pivotFlag;

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

		void clear() { active = motionBlur = outline = fxaa = ssao = linDepth = colorGrade = ssr = false; dofStrength = 0; ssss = XMFLOAT2(0, 0); }
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
		Processing() { clear(); }
	};
	Processing process;
	bool deferred;
	TextureView maskMap, distortionMap, refractionSource;
	// Generic texture
	void setMaskMap(TextureView view) { maskMap = view; }
	// The normalmap texture which should distort the refraction source
	void setDistortionMap(TextureView view) { distortionMap = view; }
	// The texture which should be distorted
	void setRefractionSource(TextureView view) { refractionSource = view; }

	void init() {
		pos = XMFLOAT3(0, 0, 0);
		siz = XMFLOAT2(1, 1);
		scale = XMFLOAT2(1, 1);
		drawRec = XMFLOAT4(0, 0, 0, 0);
		texOffset = XMFLOAT2(0, 0);
		offset = XMFLOAT2(0, 0);
		sunPos = XMFLOAT2(0, 0);
		lookAt = XMFLOAT4(0, 0, 0, 0);
		mirror = 1.0f;
		blur = blurDir = fade = rotation = 0.0f;
		opacity = 1.0f;
		extractNormalMap = false;
		mipLevel = 0.f;
		stencilRef = 0;
		stencilComp = 0;
		presentFullScreen = false;
		blendFlag = BLENDMODE_ALPHA;
		typeFlag = SCREEN;
		sampleFlag = SAMPLEMODE_MIRROR;
		quality = QUALITY_BILINEAR;
		pivotFlag = UPPERLEFT;
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

