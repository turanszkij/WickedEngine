#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ConstantBufferMapping.h"

class wiImageEffects;
enum BLENDMODE;

class wiImage
{
private:
	//static mutex MUTEX;
protected:
	GFX_STRUCT ImageCB
	{
		XMMATRIX	mTransform;
		XMFLOAT4	mTexMulAdd;
		XMFLOAT4	mColor;
		XMFLOAT2	mPivot;
		UINT		mMirror;

		UINT		mMask;
		UINT		mDistort;
		UINT		mNormalmapSeparate;
		float		mMipLevel;
		float		mFade;
		float		mOpacity;
		float		pad[3];

		CB_SETBINDSLOT(CBSLOT_IMAGE_IMAGE)

		ALIGN_16
	};
	GFX_STRUCT PostProcessCB
	{
		float params0[4];
		float params1[4];

		CB_SETBINDSLOT(CBSLOT_IMAGE_POSTPROCESS)

		ALIGN_16
	};
	
	static wiGraphicsTypes::BlendState		*blendState, *blendStateAdd, *blendStateNoBlend, *blendStateAvg;
	static wiGraphicsTypes::GPUBuffer       *constantBuffer, *processCb;

	static wiGraphicsTypes::VertexShader     *vertexShader,*screenVS;
	static wiGraphicsTypes::PixelShader      *pixelShader,*blurHPS,*blurVPS,*shaftPS,*outlinePS,*dofPS,*motionBlurPS,*bloomSeparatePS
		,*fxaaPS,*ssaoPS,*ssssPS,*deferredPS,*linDepthPS,*colorGradePS,*ssrPS, *screenPS, *stereogramPS, *tonemapPS;
	

	
	static wiGraphicsTypes::RasterizerState		*rasterizerState;
	static wiGraphicsTypes::DepthStencilState	*depthStencilStateGreater,*depthStencilStateLess,*depthNoStencilState;

public:
	static void LoadShaders();
	static void BindPersistentState(GRAPHICSTHREAD threadID);
private:
	static void LoadBuffers();
	static void SetUpStates();

public:
	wiImage();
	
	static void Draw(wiGraphicsTypes::Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);

	static void DrawDeferred(wiGraphicsTypes::Texture2D* lightmap_diffuse, wiGraphicsTypes::Texture2D* lightmap_specular, 
		wiGraphicsTypes::Texture2D* ao, GRAPHICSTHREAD threadID, int stencilref = 0);

	static void Load();
	static void CleanUp();
};

