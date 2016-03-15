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
	
	static BlendState		blendState, blendStateAdd, blendStateNoBlend, blendStateAvg;
	static BufferResource           constantBuffer,processCb;

	static VertexShader     vertexShader,screenVS;
	static PixelShader      pixelShader,blurHPS,blurVPS,shaftPS,outlinePS,dofPS,motionBlurPS,bloomSeparatePS
		,fxaaPS,ssaoPS,ssssPS,deferredPS,linDepthPS,colorGradePS,ssrPS, screenPS, stereogramPS;
	

	
	static RasterizerState		rasterizerState;
	static DepthStencilState	depthStencilStateGreater,depthStencilStateLess,depthNoStencilState;

public:
	static void LoadShaders();
	static void BindPersistentState(DeviceContext context);
private:
	static void LoadBuffers();
	static void SetUpStates();

public:
	wiImage();
	
	static void Draw(TextureView texture, const wiImageEffects& effects);
	static void Draw(TextureView texture, const wiImageEffects& effects,DeviceContext context);

	static void DrawDeferred(TextureView texture
		, TextureView depth, TextureView lightmap, TextureView normal
		, TextureView ao, DeviceContext context, int stencilref = 0);

	static void Load();
	static void CleanUp();
};

