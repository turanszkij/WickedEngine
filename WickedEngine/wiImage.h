#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "wiImageEffects.h"

enum BLENDMODE;

class wiImage
{
protected:
	CBUFFER(ImageCB, CBSLOT_IMAGE_IMAGE)
	{
		XMMATRIX	mTransform;
		XMFLOAT4	mTexMulAdd;
		XMFLOAT4	mColor;
		XMFLOAT2	mPivot;
		UINT		mMirror;
		float		mMipLevel;
	};
	CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
	{
		float params0[4];
		float params1[4];
	};
	
	static wiGraphicsTypes::GPUBuffer       constantBuffer, processCb;

	static wiGraphicsTypes::VertexShader*   vertexShader;
	static wiGraphicsTypes::VertexShader*   screenVS;

	enum IMAGE_SHADER
	{
		IMAGE_SHADER_STANDARD,
		IMAGE_SHADER_SEPARATENORMALMAP,
		IMAGE_SHADER_DISTORTION,
		IMAGE_SHADER_DISTORTION_MASKED,
		IMAGE_SHADER_MASKED,
		IMAGE_SHADER_FULLSCREEN,
		IMAGE_SHADER_COUNT
	};
	static wiGraphicsTypes::PixelShader* imagePS[IMAGE_SHADER_COUNT];

	enum POSTPROCESS
	{
		POSTPROCESS_BLUR_H,
		POSTPROCESS_BLUR_V,
		POSTPROCESS_LIGHTSHAFT,
		POSTPROCESS_OUTLINE,
		POSTPROCESS_DEPTHOFFIELD,
		POSTPROCESS_MOTIONBLUR,
		POSTPROCESS_BLOOMSEPARATE,
		POSTPROCESS_FXAA,
		POSTPROCESS_SSAO,
		POSTPROCESS_SSSS,
		POSTPROCESS_SSR,
		POSTPROCESS_COLORGRADE,
		POSTPROCESS_STEREOGRAM,
		POSTPROCESS_TONEMAP,
		POSTPROCESS_REPROJECTDEPTHBUFFER,
		POSTPROCESS_DOWNSAMPLEDEPTHBUFFER,
		POSTPROCESS_TEMPORALAA,
		POSTPROCESS_SHARPEN,
		POSTPROCESS_LINEARDEPTH,
		POSTPROCESS_COUNT
	};
	static wiGraphicsTypes::PixelShader* postprocessPS[POSTPROCESS_COUNT];
	static wiGraphicsTypes::PixelShader* deferredPS;


	static wiGraphicsTypes::BlendState			blendStates[BLENDMODE_COUNT];
	static wiGraphicsTypes::BlendState			blendStateDisableColor;
	static wiGraphicsTypes::RasterizerState		rasterizerState;
	static wiGraphicsTypes::DepthStencilState	depthStencilStates[STENCILMODE_COUNT];
	static wiGraphicsTypes::DepthStencilState	depthStencilStateDepthWrite;

	enum IMAGE_HDR
	{
		IMAGE_HDR_DISABLED,
		IMAGE_HDR_ENABLED,
		IMAGE_HDR_COUNT
	};
	static wiGraphicsTypes::GraphicsPSO imagePSO[IMAGE_SHADER_COUNT][BLENDMODE_COUNT][STENCILMODE_COUNT][IMAGE_HDR_COUNT];
	static wiGraphicsTypes::GraphicsPSO postprocessPSO[POSTPROCESS_COUNT];
	static wiGraphicsTypes::GraphicsPSO deferredPSO;


public:
	static void LoadShaders();
	static void BindPersistentState(GRAPHICSTHREAD threadID);
private:
	static void LoadBuffers();
	static void SetUpStates();

public:
	wiImage();
	
	static void Draw(wiGraphicsTypes::Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID);

	static void DrawDeferred(wiGraphicsTypes::Texture2D* lightmap_diffuse, wiGraphicsTypes::Texture2D* lightmap_specular, 
		wiGraphicsTypes::Texture2D* ao, GRAPHICSTHREAD threadID, int stencilref = 0);

	static void Initialize();
	static void CleanUp();
};

