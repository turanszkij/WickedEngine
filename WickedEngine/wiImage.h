#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiImageEffects;
enum BLENDMODE;

class wiImage
{
private:
	//static mutex MUTEX;
protected:
	GFX_STRUCT ImageCB
	{
		XMMATRIX	mViewProjection;
		XMMATRIX	mTrans;
		XMFLOAT4	mDimensions;
		XMFLOAT4	mDrawRec;
		XMFLOAT4	mTexMulAdd;
		UINT		mPivot;
		UINT		mMask;
		UINT		mDistort;
		UINT		mNormalmapSeparate;
		float		mOffsetX;
		float		mOffsetY;
		float		mMirror;
		float		mFade;
		float		mOpacity;
		float		mMipLevel;
		float pad[2];

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
	
	static wiGraphicsTypes::BlendState		blendState, blendStateAdd, blendStateNoBlend, blendStateAvg;
	static wiGraphicsTypes::BufferResource           constantBuffer,processCb;

	static wiGraphicsTypes::VertexShader     vertexShader,screenVS;
	static wiGraphicsTypes::PixelShader      pixelShader,blurHPS,blurVPS,shaftPS,outlinePS,dofPS,motionBlurPS,bloomSeparatePS
		,fxaaPS,ssaoPS,ssssPS,deferredPS,linDepthPS,colorGradePS,ssrPS, screenPS, stereogramPS;
	

	
	static wiGraphicsTypes::RasterizerState		rasterizerState;
	static wiGraphicsTypes::DepthStencilState	depthStencilStateGreater,depthStencilStateLess,depthNoStencilState;

public:
	static void LoadShaders();
	static void BindPersistentState(GRAPHICSTHREAD threadID);
private:
	static void LoadBuffers();
	static void SetUpStates();

public:
	wiImage();
	
	static void Draw(wiGraphicsTypes::Texture2D* texture, const wiImageEffects& effects);
	static void Draw(wiGraphicsTypes::Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID);

	static void DrawDeferred(wiGraphicsTypes::Texture2D* texture
		, wiGraphicsTypes::Texture2D* depth, wiGraphicsTypes::Texture2D* lightmap, wiGraphicsTypes::Texture2D* normal
		, wiGraphicsTypes::Texture2D* ao, GRAPHICSTHREAD threadID, int stencilref = 0);

	static void Load();
	static void CleanUp();
};

