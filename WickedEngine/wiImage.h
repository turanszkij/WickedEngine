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
	GFX_STRUCT ConstantBuffer
	{
		XMMATRIX mViewProjection;
		XMMATRIX mTrans;
		XMFLOAT4 mDimensions;
		XMFLOAT4 mOffsetMirFade;
		XMFLOAT4 mDrawRec;
		XMFLOAT4 mBlurOpaPiv;
		XMFLOAT4 mTexOffset;

		ALIGN_16
	};
	GFX_STRUCT PSConstantBuffer
	{
		XMFLOAT4 mMaskFadOpaDis;
		XMFLOAT4 mDimension;
		PSConstantBuffer(){mMaskFadOpaDis=mDimension=XMFLOAT4(0,0,0,0);}

		ALIGN_16
	};
	GFX_STRUCT ProcessBuffer
	{
		XMFLOAT4 mPostProcess;
		XMFLOAT4 mBloom;
		ProcessBuffer(){mPostProcess=mBloom=XMFLOAT4(0,0,0,0);}

		ALIGN_16
	};
	GFX_STRUCT LightShaftBuffer
	{
		XMFLOAT4 mProperties;
		XMFLOAT2 mLightShaft; XMFLOAT2 mPadding;
		LightShaftBuffer(){mProperties=XMFLOAT4(0,0,0,0);mLightShaft=XMFLOAT2(0,0);}

		ALIGN_16
	};
	GFX_STRUCT DeferredBuffer
	{
		XMFLOAT3 mAmbient; float pad;
		XMFLOAT3 mHorizon; float pad1;
		XMMATRIX mViewProjInv;
		XMFLOAT3 mFogSEH; float pad2;

		ALIGN_16
	};
	GFX_STRUCT BlurBuffer
	{
		XMVECTOR mWeight;
		XMFLOAT4 mWeightTexelStrenMip;

		ALIGN_16
	};
	
	static BlendState		blendState, blendStateAdd, blendStateNoBlend, blendStateAvg;
	static BufferResource           constantBuffer,PSCb,blurCb,processCb,shaftCb,deferredCb;

	static VertexShader     vertexShader,screenVS;
	static PixelShader      pixelShader,blurHPS,blurVPS,shaftPS,outlinePS,dofPS,motionBlurPS,bloomSeparatePS
		,fxaaPS,ssaoPS,ssssPS,deferredPS,linDepthPS,colorGradePS,ssrPS;
	

	
	static RasterizerState		rasterizerState;
	static DepthStencilState	depthStencilStateGreater,depthStencilStateLess,depthNoStencilState;

public:
	static void LoadShaders();
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


	//// DEPRECATED
	//static void BatchBegin();
	//// DEPRECATED
	//static void BatchBegin(DeviceContext context);
	//// DEPRECATED
	//static void BatchBegin(DeviceContext context, unsigned int stencilref, bool stencilOpLess=true);

	static void Load();
	static void CleanUp();
};

