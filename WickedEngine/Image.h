#pragma once
#include "WickedEngine.h"

class wiImage
{
private:
	static mutex MUTEX;
protected:
	struct ConstantBuffer
	{
		XMMATRIX mViewProjection;
		XMMATRIX mTrans;
		XMFLOAT4 mDimensions;
		XMFLOAT4 mOffsetMirFade;
		XMFLOAT4 mDrawRec;
		XMFLOAT4 mBlurOpaPiv;
		XMFLOAT4 mTexOffset;
	};
	struct PSConstantBuffer
	{
		XMFLOAT4 mMaskFadOpaDis;
		XMFLOAT4 mDimension;
		PSConstantBuffer(){mMaskFadOpaDis=mDimension=XMFLOAT4(0,0,0,0);}
	};
	struct ProcessBuffer
	{
		XMFLOAT4 mPostProcess;
		XMFLOAT4 mBloom;
		ProcessBuffer(){mPostProcess=mBloom=XMFLOAT4(0,0,0,0);}
	};
	struct LightShaftBuffer
	{
		XMFLOAT4 mProperties;
		XMFLOAT2 mLightShaft; XMFLOAT2 mPadding;
		LightShaftBuffer(){mProperties=XMFLOAT4(0,0,0,0);mLightShaft=XMFLOAT2(0,0);}
	};
	struct DeferredBuffer
	{
		XMFLOAT3 mAmbient; float pad;
		XMFLOAT3 mHorizon; float pad1;
		XMMATRIX mViewProjInv;
		XMFLOAT3 mFogSEH; float pad2;
	};
	struct BlurBuffer
	{
		XMVECTOR mWeight;
		XMFLOAT4 mWeightTexelStrenMip;
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
	static int RENDERWIDTH,RENDERHEIGHT;

public:
	wiImage();
	
	static void Draw(ID3D11ShaderResourceView* texture, const ImageEffects& effects);
	inline static void Draw(ID3D11ShaderResourceView* texture, const ImageEffects& effects,ID3D11DeviceContext* context);

	static void DrawDeferred(ID3D11ShaderResourceView* texture
		, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* lightmap, ID3D11ShaderResourceView* normal
		, ID3D11ShaderResourceView* ao, ID3D11DeviceContext* context, int stencilref = 0);



	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT3& pos, const XMFLOAT2& siz, ID3D11DeviceContext* context);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz,const float&rotation);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz,const float&rotation, const float&opacity, ID3D11DeviceContext* context);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror, const float&blur, const float&blurStrength, const float&fade, const float&opacity);
	static void Draw(ID3D11ShaderResourceView *texture, const XMFLOAT4& newPosSiz, const XMFLOAT4& newDrawRec, const float&newMirror, const float&newBlur, const float&newBlurStrength, const float&newFade, const float&newOpacity, ID3D11DeviceContext* context);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror, const float&blur, const float&blurStrength, const float&fade, const float&opacity, const float&rotation);
	static void Draw(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror, const float&blur, const float&blurStrength, const float&fade, const float&opacity, const float&rotation, ID3D11DeviceContext* context);
	static void Draw(ID3D11ShaderResourceView* resourceView, ID3D11ShaderResourceView* mask, const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror, const float&blur, const float&blurStrength, const float&fade, const float&opacity, const float&rotation, XMFLOAT2 texOffset, BLENDMODE blendFlag, ID3D11DeviceContext* context);
	
	static void DrawModifiedTexCoords(ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror, XMFLOAT2 texOffset, const float&opacity, BLENDMODE blendFlag);
	static void DrawModifiedTexCoords(ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, const float&mirror, XMFLOAT2 texOffset, const float&opacity, BLENDMODE blendFlag, ID3D11DeviceContext* context);
	static void DrawOffset(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz,XMFLOAT2 offset);
	static void DrawOffset(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz, const XMFLOAT4& drawRec, XMFLOAT2 offset);
	static void DrawAdditive(ID3D11ShaderResourceView* resourceView,const XMFLOAT4& posSiz,const XMFLOAT4& drawRec=XMFLOAT4(0,0,0,0));

	static void BatchBegin();
	static void BatchBegin(ID3D11DeviceContext* context);
	static void BatchBegin(ID3D11DeviceContext* context, unsigned int stencilref, bool stencilOpLess=true);

	static void Load();
	static void CleanUp();

	static void SetScreenResolution(int width, int height){RENDERHEIGHT=height;RENDERWIDTH=width;};

	/*
	struct ImageResource{
		string name;
		ID3D11ShaderResourceView* texture;

		ImageResource(){name="";texture=NULL;};
		ImageResource(const string& newDirectory, const string& newName); //in wiRenderer.cpp
		void CleanUp(){
			if(texture) texture->Release(); texture=NULL;
		}
	};
	static map<string,ImageResource> images;
	static ID3D11ShaderResourceView* getImageByName(const string& get);
	static void addImageResource(const string& dir, const string& name);*/
};


#include "Sprite.h"
