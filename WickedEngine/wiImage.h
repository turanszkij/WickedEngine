#pragma once
#include "CommonInclude.h"

class wiImageEffects;
enum BLENDMODE;

class wiImage
{
private:
	static mutex MUTEX;
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
	
	static ID3D11BlendState*		blendState, *blendStateAdd, *blendStateNoBlend, *blendStateAvg;
	static ID3D11Buffer*           constantBuffer,*PSCb,*blurCb,*processCb,*shaftCb,*deferredCb;

	static ID3D11VertexShader*     vertexShader,*screenVS;
	static ID3D11PixelShader*      pixelShader,*blurHPS,*blurVPS,*shaftPS,*outlinePS,*dofPS,*motionBlurPS,*bloomSeparatePS
		,*fxaaPS,*ssaoPS,*ssssPS,*deferredPS,*linDepthPS,*colorGradePS,*ssrPS;
	

	
	static ID3D11RasterizerState*		rasterizerState;
	static ID3D11DepthStencilState*	depthStencilStateGreater,*depthStencilStateLess,*depthNoStencilState;

	static void LoadShaders();
	static void LoadBuffers();
	static void SetUpStates();

public:
	wiImage();
	
	static void Draw(ID3D11ShaderResourceView* texture, const wiImageEffects& effects);
	static void Draw(ID3D11ShaderResourceView* texture, const wiImageEffects& effects,ID3D11DeviceContext* context);

	static void DrawDeferred(ID3D11ShaderResourceView* texture
		, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* lightmap, ID3D11ShaderResourceView* normal
		, ID3D11ShaderResourceView* ao, ID3D11DeviceContext* context, int stencilref = 0);


	//// DEPRECATED
	//static void BatchBegin();
	//// DEPRECATED
	//static void BatchBegin(ID3D11DeviceContext* context);
	//// DEPRECATED
	//static void BatchBegin(ID3D11DeviceContext* context, unsigned int stencilref, bool stencilOpLess=true);

	static void Load();
	static void CleanUp();
};

