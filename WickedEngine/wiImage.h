#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"

class wiImageEffects;
enum BLENDMODE;

class wiImage
{
private:
	//static mutex MUTEX;
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
	
	static wiGraphicsTypes::BlendState		*blendState, *blendStateAdd, *blendStateNoBlend, *blendStateMax, *blendStateDisable;
	static wiGraphicsTypes::GPUBuffer       *constantBuffer, *processCb;

	static wiGraphicsTypes::VertexShader     *vertexShader,*screenVS;
	static wiGraphicsTypes::PixelShader		 *imagePS, *imagePS_separatenormalmap, *imagePS_distortion, *imagePS_distortion_masked, *imagePS_masked;
	static wiGraphicsTypes::PixelShader      *blurHPS,*blurVPS,*shaftPS,*outlinePS,*dofPS,*motionBlurPS,*bloomSeparatePS
		,*fxaaPS,*ssaoPS,*ssssPS,*deferredPS,*linDepthPS,*colorGradePS,*ssrPS, *screenPS, *stereogramPS, *tonemapPS, *reprojectDepthBufferPS, *downsampleDepthBufferPS
		,*temporalAAResolvePS, *sharpenPS;
	

	
	static wiGraphicsTypes::RasterizerState		*rasterizerState;
	static wiGraphicsTypes::DepthStencilState	*depthStencilStateGreater, *depthStencilStateLess, *depthStencilStateEqual,*depthNoStencilState, *depthStencilStateDepthWrite;

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

	static void Load();
	static void CleanUp();
};

