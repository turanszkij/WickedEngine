#pragma once
#include "WickedEngine.h"
#include "PHYSICS.h"
#include "Lines.h"
#include "Cube.h"

struct Transform;
struct Vertex;
struct Material;
struct Object;
struct BoneShaderBuffer;
struct Mesh;
struct Armature;
struct Bone;
struct KeyFrame;
struct SHCAM;
struct Light;
struct Decal;
struct WorldInfo;
struct Wind;
struct ActionCamera;
struct HitSphere;
struct RAY;


class  Lines;
class  Cube;
class  wiParticle;
class  wiEmittedParticle;
class  wiHairParticle;
class  wiSprite;
class  wiSPTree;
class  TaskThread;
struct Cullable;
class  PHYSICS;
class  Camera;

typedef map<string,Mesh*> MeshCollection;
typedef map<string,Material*> MaterialCollection;

class wiRenderer
{
public:
#define API_CALL_STRICTNESS
#ifndef WINSTORE_SUPPORT
	typedef IDXGISwapChain*				SwapChain;
#else
	typedef IDXGISwapChain1*				SwapChain; 
#endif
	typedef IUnknown*					APIInterface;
	typedef ID3D11DeviceContext*		DeviceContext;
	typedef	ID3D11Device*				GraphicsDevice;
	typedef	D3D11_VIEWPORT				ViewPort;
	typedef ID3D11CommandList*			CommandList;
	typedef ID3D11RenderTargetView*		RenderTargetView;
	typedef ID3D11ShaderResourceView*	TextureView;
	typedef ID3D11SamplerState*			Sampler;
	typedef ID3D11Resource*				APIResource;
	typedef ID3D11Buffer*				BufferResource;
	typedef ID3D11VertexShader*			VertexShader;
	typedef ID3D11PixelShader*			PixelShader;
	typedef ID3D11GeometryShader*		GeometryShader;
	typedef ID3D11HullShader*			HullShader;
	typedef ID3D11DomainShader*			DomainShader;
	typedef ID3D11InputLayout*			VertexLayout;
	typedef D3D11_INPUT_ELEMENT_DESC    VertexLayoutDesc;
	typedef ID3D11BlendState*			BlendState;
	typedef ID3D11DepthStencilState*	DepthStencilState;
	typedef ID3D11RasterizerState*		RasterizerState;


	enum PRIMITIVETOPOLOGY{
		TRIANGLELIST,
		TRIANGLESTRIP,
		POINTLIST,
		LINELIST,
		PATCHLIST,
	};

	struct VertexShaderInfo{
		VertexShader vertexShader;
		VertexLayout vertexLayout;

		VertexShaderInfo(){
			vertexShader = nullptr;
			vertexLayout = VertexLayout();
		}
	};

	static D3D_DRIVER_TYPE				driverType;
	static D3D_FEATURE_LEVEL			featureLevel;
	static SwapChain					swapChain;
	static RenderTargetView				renderTargetView;
	static ViewPort						viewPort;
	static GraphicsDevice				graphicsDevice;
	static DeviceContext				immediateContext;
	static bool DX11,VSYNC;
	static const int NUM_DCONTEXT = 5;
	static DeviceContext deferredContexts[NUM_DCONTEXT];
	static CommandList commandLists[NUM_DCONTEXT];
	static mutex graphicsMutex;
#ifndef WINSTORE_SUPPORT
	static HRESULT InitDevice(HWND window, int screenW, int screenH, bool windowed, short& requestMultiThreading);
#else
	static HRESULT InitDevice(Windows::UI::Core::CoreWindow^ window, short& requestMultiThreading);
#endif
	static void DestroyDevice();
	static void Present(function<void()> drawToScreen1=nullptr,function<void()> drawToScreen2=nullptr,function<void()> drawToScreen3=nullptr);
	static void wiRenderer::ReleaseCommandLists();
	static void wiRenderer::ExecuteDeferredContexts();
	static void FinishCommandList(int THREAD);

	static map<DeviceContext,long> drawCalls;
	static long getDrawCallCount();

	
	static Sampler ssClampLin,ssClampPoi,ssMirrorLin,ssMirrorPoi,ssWrapLin,ssWrapPoi
		,ssClampAni,ssWrapAni,ssMirrorAni,ssComp;

	
	static int RENDERWIDTH,RENDERHEIGHT,SCREENWIDTH,SCREENHEIGHT;
	static int SHADOWMAPRES,SOFTSHADOW,POINTLIGHTSHADOW,POINTLIGHTSHADOWRES;
	static bool HAIRPARTICLEENABLED, EMITTERSENABLED;

protected:
	struct ConstantBuffer
	{
		XMVECTOR mDisplace;
	};
	struct StaticCB
	{
		XMMATRIX mViewProjection;
		XMMATRIX mRefViewProjection;
		XMVECTOR mCamPos;
		//XMFLOAT4A mMotionBlur;
		XMFLOAT4 mClipPlane;
		XMFLOAT3 mWind; float time;
		float windRandomness;
		float windWaveSize;
		float padding[2];
	};
	struct PixelCB
	{
		//XMFLOAT4A mFx;
		XMFLOAT3 mHorizon; float pad;
		XMVECTOR mSun;
		XMFLOAT3 mAmbient; float pad1;
		XMFLOAT4 mSunColor;
		XMFLOAT3 mFogSEH; float pad2;
	};
	struct FxCB{
		XMFLOAT4 mFx;
		XMFLOAT4 colorMask;
	};
	struct MaterialCB
	{
		XMFLOAT4 difColor;
		UINT hasRef,hasNor,hasTex,hasSpe;
		XMFLOAT4 specular;
		float refractionIndex;
		XMFLOAT2 movingTex;
		float metallic;
		UINT shadeless;
		UINT specular_power;
		UINT toon;
		UINT matIndex;
		float emit;
		float padding[3];

		MaterialCB(const Material& mat,UINT materialIndex);
		void* operator new(size_t size){ void* result = _aligned_malloc(size,16); return result; }
		void operator delete(void* p){ if(p) _aligned_free(p); }
	};
	struct ForShadowMapCB
	{
		XMMATRIX mViewProjection;
		XMFLOAT3 mWind; float time;
		float windRandomness;
		float windWaveSize;
	};
	struct CubeShadowCb{
		XMMATRIX mViewProjection[6];
	};
	struct TessBuffer
	{
		XMVECTOR  g_f4Eye;
		XMFLOAT4A g_f4TessFactors;
	};
	struct dLightBuffer
	{
		XMVECTOR direction;
		XMFLOAT4 col;
		XMFLOAT4 mBiasResSoftshadow;
		XMMATRIX mShM[3];
	};
	struct pLightBuffer
	{
		XMFLOAT3 pos; float pad;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;
	};
	struct sLightBuffer
	{
		XMMATRIX world;
		XMVECTOR direction;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;
		XMFLOAT4 mBiasResSoftshadow;
		XMMATRIX mShM;
	};
	struct vLightBuffer
	{
		XMMATRIX world;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;
	};
	struct LightStaticCB{
		XMMATRIX mProjInv;
	};
	struct LineBuffer{
		XMMATRIX mWorldViewProjection;
		XMFLOAT4A color;
	};
	struct SkyBuffer
	{
		XMMATRIX mV;
		XMMATRIX mP;
	};
	struct DecalCBVS
	{
		XMMATRIX mWVP;
	};
	struct DecalCBPS
	{
		XMMATRIX mDecalVP;
		int hasTexNor;
		XMFLOAT3 eye;
		float opacity;
		XMFLOAT3 front;
	};
	struct ViewPropCB
	{
		XMMATRIX matView;
		XMMATRIX matProj;
		float mZNearP;
		float mZFarP;
		float padding[2];
	};


	void UpdateSpheres();
	static void SetUpBoneLines();
	static void UpdateBoneLines();
	static void SetUpCubes();
	static void UpdateCubes();
	


	static bool						wireRender, debugSpheres, debugLines, debugBoxes;
	static ID3D11BlendState*		blendState, *blendStateTransparent, *blendStateAdd;
	static ID3D11Buffer*			constantBuffer, *staticCb, *shCb, *pixelCB, *matCb, *lightCb[3], *tessBuf
		, *lineBuffer, *trailCB, *lightStaticCb, *vLightCb,*cubeShCb,*fxCb,*skyCb,*decalCbVS,*decalCbPS,*viewPropCB;
	static ID3D11VertexShader*		vertexShader10, *vertexShader,*shVS,*lineVS,*trailVS,*waterVS
		,*lightVS[3],*vSpotLightVS,*vPointLightVS,*cubeShVS,*sOVS,*decalVS;
	static ID3D11PixelShader*		pixelShader,*shPS,*linePS,*trailPS,*simplestPS,*blackoutPS,*textureonlyPS,*waterPS,*transparentPS
		,*lightPS[3],*vLightPS,*cubeShPS,*decalPS,*fowardSimplePS;
	static ID3D11GeometryShader* cubeShGS,*sOGS;
	static ID3D11HullShader*		hullShader;
	static ID3D11DomainShader*		domainShader;
	static ID3D11InputLayout*       vertexLayout, *lineIL, *trailIL, *sOIL;
	static ID3D11SamplerState*		texSampler, *mapSampler, *comparisonSampler, *mirSampler, *pointSampler;
	static ID3D11RasterizerState*	rasterizerState,*rssh,*nonCullRSsh,*wireRS,*nonCullRS,*nonCullWireRS,*backFaceRS;
	static ID3D11DepthStencilState* depthStencilState,*xRayStencilState,*depthReadStencilState,*stencilReadState,*stencilReadMatch;
	static ID3D11PixelShader*		skyPS;
	static ID3D11VertexShader*		skyVS;
	static ID3D11SamplerState*		skySampler;
	static TextureView noiseTex,trailDistortTex,enviroMap,colorGrading;
	static void LoadBuffers();
	static void LoadBasicShaders();
	static void LoadLineShaders();
	static void LoadTessShaders();
	static void LoadSkyShaders();
	static void LoadShadowShaders();
	static void LoadWaterShaders();
	static void LoadTrailShaders();
	static void SetUpStates();
	static int vertexCount;
	static int visibleCount;


	static void addVertexCount(const int& toadd){vertexCount+=toadd;}

	enum KeyFrameType{
		ROTATIONKEYFRAMETYPE,
		POSITIONKEYFRAMETYPE,
		SCALARKEYFRAMETYPE,
	};


	static void RecursiveBoneTransform(Armature* armature, Bone* bone, const XMMATRIX& parentCombinedMat);
	inline static XMVECTOR InterPolateKeyFrames(const float& currentFrame, const int& frameCount,const std::vector<KeyFrame>& keyframes, KeyFrameType type);
	static Vertex TransformVertex(const Mesh* mesh, const int& vertexI, const XMMATRIX& mat=XMMatrixIdentity());
	XMFLOAT3 VertexVelocity(const Mesh* mesh, const int& vertexI);

	Armature* getArmatureByName(const string& get);
	int getActionByName(Armature* armature, const string& get);
	int getBoneByName(Armature* armature, const string& get);
	Material* getMaterialByName(const string& get);
	HitSphere* getSphereByName(const string& get);

	static float GameSpeed,overrideGameSpeed;

public:
	wiRenderer();
	void CleanUp();
	static void SetUpStaticComponents();
	static void CleanUpStatic();
	
	static void Update(float amount = GetGameSpeed());
	static void UpdateRenderInfo(ID3D11DeviceContext* context);
	static void UpdateObjects();
	static void UpdateSoftBodyPinning();
	static void UpdateSkinnedVB();
	static void UpdatewiSPTree(wiSPTree*& tree);
	static void UpdateImages();
	static void ManageImages();
	static void PutWaterRipple(const string& image, const XMFLOAT3& pos, const XMFLOAT4& plane);
	static void ManageWaterRipples();
	static void DrawWaterRipples(ID3D11DeviceContext* context);
	static void SetGameSpeed(float value){GameSpeed=value; if(GameSpeed<0) GameSpeed=0;};
	static float GetGameSpeed();
	static void ChangeRasterizer(){wireRender=!wireRender;};
	static bool GetRasterizer(){return wireRender;};
	static WorldInfo worldInfo;
	static float shBias;
	static void ToggleDebugSpheres(){debugSpheres=!debugSpheres;}
	static void SetToDrawDebugSpheres(bool param){debugSpheres=param;}
	static void SetToDrawDebugLines(bool param){debugLines=param;}
	static void SetToDrawDebugBoxes(bool param){debugBoxes=param;}
	static bool GetToDrawDebugSpheres(){return debugSpheres;};
	static bool GetToDrawDebugBoxes(){return debugBoxes;};
	static TextureView GetColorGrading(){return colorGrading;};
	static void SetColorGrading(TextureView tex){colorGrading=tex?tex:colorGrading;};
	static void SetEnviromentMap(TextureView tex){ enviroMap = tex; }
	static TextureView GetEnviromentMap(){ return enviroMap; }
	static void SetNoiseTexture(TextureView tex){ noiseTex = tex; }
	static TextureView GetNoiseTexture(){ return noiseTex; }
	
private:
	static vector<TextureView> textureSlotsPS;
	static PixelShader boundPS;
public:
	inline static void BindTexturePS(TextureView texture, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && texture!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView t;
			context->PSGetShaderResources(slot,1,&t);
			if(t!=texture)
		#endif
				context->PSSetShaderResources(slot,1,&texture);
		#ifdef API_CALL_STRICTNESS
			if(t!=nullptr)
				t->Release();
		#endif
		}
	}
	inline static void BindTexturesPS(TextureView textures[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && textures!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView *t = new TextureView[num];
			context->PSGetShaderResources(slot,num,t);
			if(t[0]!=textures[0])
		#endif
				context->PSSetShaderResources(slot,num,textures);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(t[i]!=nullptr)
					t[i]->Release();
		#endif
		}
	}
	inline static void BindTextureVS(TextureView texture, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && texture!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView t;
			context->VSGetShaderResources(slot,1,&t);
			if(t!=texture)
		#endif
				context->VSSetShaderResources(slot,1,&texture);
		#ifdef API_CALL_STRICTNESS
			if(t!=nullptr)
				t->Release();
		#endif
		}
	}
	inline static void BindTexturesVS(TextureView textures[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && textures!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView *t = new TextureView[num];
			context->VSGetShaderResources(slot,num,t);
			if(t[0]!=textures[0])
		#endif
				context->VSSetShaderResources(slot,num,textures);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(t[i]!=nullptr)
					t[i]->Release();
		#endif
		}
	}
	inline static void BindTextureGS(TextureView texture, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && texture!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView t;
			context->GSGetShaderResources(slot,1,&t);
			if(t!=texture)
		#endif
				context->GSSetShaderResources(slot,1,&texture);
		#ifdef API_CALL_STRICTNESS
			if(t!=nullptr)
				t->Release();
		#endif
		}
	}
	inline static void BindTexturesGS(TextureView textures[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && textures!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView *t = new TextureView[num];
			context->GSGetShaderResources(slot,num,t);
			if(t[0]!=textures[0])
		#endif
				context->GSSetShaderResources(slot,num,textures);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(t[i]!=nullptr)
					t[i]->Release();
		#endif
		}
	}
	inline static void BindTextureDS(TextureView texture, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && texture!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView t;
			context->DSGetShaderResources(slot,1,&t);
			if(t!=texture)
		#endif
				context->DSSetShaderResources(slot,1,&texture);
		#ifdef API_CALL_STRICTNESS
			if(t!=nullptr)
				t->Release();
		#endif
		}
	}
	inline static void BindTexturesDS(TextureView textures[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && textures!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView *t = new TextureView[num];
			context->DSGetShaderResources(slot,num,t);
			if(t[0]!=textures[0])
		#endif
				context->DSSetShaderResources(slot,num,textures);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(t[i]!=nullptr)
					t[i]->Release();
		#endif
		}
	}
	inline static void BindTextureHS(TextureView texture, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && texture!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView t;
			context->HSGetShaderResources(slot,1,&t);
			if(t!=texture)
		#endif
				context->HSSetShaderResources(slot,1,&texture);
		#ifdef API_CALL_STRICTNESS
			if(t!=nullptr)
				t->Release();
		#endif
		}
	}
	inline static void BindTexturesHS(TextureView textures[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && textures!=nullptr){
		#ifdef API_CALL_STRICTNESS
			TextureView *t = new TextureView[num];
			context->HSGetShaderResources(slot,num,t);
			if(t[0]!=textures[0])
		#endif
				context->HSSetShaderResources(slot,num,textures);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(t[i]!=nullptr)
					t[i]->Release();
		#endif
		}
	}
	inline static void BindSamplerPS(Sampler sampler, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && sampler!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler s;
			context->PSGetSamplers(slot,1,&s);
			if(s!=sampler)
		#endif
				context->PSSetSamplers(slot,1,&sampler);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr)
				s->Release();
		#endif
		}
	}
	inline static void BindSamplersPS(Sampler samplers[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && samplers!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler *s = new Sampler[num];
			context->PSGetSamplers(slot,num,s);
			if(s[0]!=samplers[0])
		#endif
				context->PSSetSamplers(slot,num,samplers);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(s[i]!=nullptr)
					s[i]->Release();
		#endif
		}
	}
	inline static void BindSamplerVS(Sampler sampler, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && sampler!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler s;
			context->VSGetSamplers(slot,1,&s);
			if(s!=sampler)
		#endif
				context->VSSetSamplers(slot,1,&sampler);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr)
				s->Release();
		#endif
		}
	}
	inline static void BindSamplersVS(Sampler samplers[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && samplers!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler *s = new Sampler[num];
			context->VSGetSamplers(slot,num,s);
			if(s[0]!=samplers[0])
		#endif
				context->VSSetSamplers(slot,num,samplers);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(s[i]!=nullptr)
					s[i]->Release();
		#endif
		}
	}
	inline static void BindSamplerGS(Sampler sampler, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && sampler!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler s;
			context->GSGetSamplers(slot,1,&s);
			if(s!=sampler)
		#endif
				context->GSSetSamplers(slot,1,&sampler);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr)
				s->Release();
		#endif
		}
	}
	inline static void BindSamplersGS(Sampler samplers[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && samplers!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler *s = new Sampler[num];
			context->GSGetSamplers(slot,num,s);
			if(s[0]!=samplers[0])
		#endif
				context->GSSetSamplers(slot,num,samplers);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(s[i]!=nullptr)
					s[i]->Release();
		#endif
		}
	}
	inline static void BindSamplerHS(Sampler sampler, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && sampler!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler s;
			context->HSGetSamplers(slot,1,&s);
			if(s!=sampler)
		#endif
				context->HSSetSamplers(slot,1,&sampler);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr)
				s->Release();
		#endif
		}
	}
	inline static void BindSamplersHS(Sampler samplers[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && samplers!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler *s = new Sampler[num];
			context->HSGetSamplers(slot,num,s);
			if(s[0]!=samplers[0])
		#endif
				context->HSSetSamplers(slot,num,samplers);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(s[i]!=nullptr)
					s[i]->Release();
		#endif
		}
	}
	inline static void BindSamplerDS(Sampler sampler, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr && sampler!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler s;
			context->DSGetSamplers(slot,1,&s);
			if(s!=sampler)
		#endif
				context->DSSetSamplers(slot,1,&sampler);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr)
				s->Release();
		#endif
		}
	}
	inline static void BindSamplersDS(Sampler samplers[], int slot, int num, DeviceContext context=immediateContext){
		if(context!=nullptr && samplers!=nullptr){
		#ifdef API_CALL_STRICTNESS
			Sampler *s = new Sampler[num];
			context->DSGetSamplers(slot,num,s);
			if(s[0]!=samplers[0])
		#endif
				context->DSSetSamplers(slot,num,samplers);
		#ifdef API_CALL_STRICTNESS
			for(int i=0;i<num;++i)
				if(s[i]!=nullptr)
					s[i]->Release();
		#endif
		}
	}
	inline static void BindConstantBufferPS(BufferResource buffer, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			BufferResource b;
			context->PSGetConstantBuffers(slot,1,&b);
			if(b!=buffer)
		#endif
				context->PSSetConstantBuffers(slot,1,&buffer);
		#ifdef API_CALL_STRICTNESS
			if(b!=nullptr)
				b->Release();
		#endif
		}
	}
	inline static void BindConstantBufferVS(BufferResource buffer, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			BufferResource b;
			context->VSGetConstantBuffers(slot,1,&b);
			if(b!=buffer)
		#endif
				context->VSSetConstantBuffers(slot,1,&buffer);
		#ifdef API_CALL_STRICTNESS
			if(b!=nullptr)
				b->Release();
		#endif
		}
	}
	inline static void BindConstantBufferGS(BufferResource buffer, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			BufferResource b;
			context->GSGetConstantBuffers(slot,1,&b);
			if(b!=buffer)
		#endif
				context->GSSetConstantBuffers(slot,1,&buffer);
		#ifdef API_CALL_STRICTNESS
			if(b!=nullptr)
				b->Release();
		#endif
		}
	}
	inline static void BindConstantBufferDS(BufferResource buffer, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			BufferResource b;
			context->DSGetConstantBuffers(slot,1,&b);
			if(b!=buffer)
		#endif
				context->DSSetConstantBuffers(slot,1,&buffer);
		#ifdef API_CALL_STRICTNESS
			if(b!=nullptr)
				b->Release();
		#endif
		}
	}
	inline static void BindConstantBufferHS(BufferResource buffer, int slot, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			BufferResource b;
			context->HSGetConstantBuffers(slot,1,&b);
			if(b!=buffer)
		#endif
				context->HSSetConstantBuffers(slot,1,&buffer);
		#ifdef API_CALL_STRICTNESS
			if(b!=nullptr)
				b->Release();
		#endif
		}
	}
	inline static void BindVertexBuffer(BufferResource vertexBuffer, int slot, UINT stride, DeviceContext context=immediateContext){
		if(context!=nullptr){
			UINT offset = 0;
			context->IASetVertexBuffers(slot,1,&vertexBuffer,&stride,&offset);
		}
	}
	inline static void BindIndexBuffer(BufferResource indexBuffer, DeviceContext context=immediateContext){
		if(context!=nullptr){
			context->IASetIndexBuffer(indexBuffer,DXGI_FORMAT_R32_UINT,0);
		}
	}
	inline static void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, DeviceContext context=immediateContext){
		if(context!=nullptr){
			D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			switch (type)
			{
			case wiRenderer::TRIANGLELIST:
				topo=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
			case wiRenderer::TRIANGLESTRIP:
				topo=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
				break;
			case wiRenderer::POINTLIST:
				topo=D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
				break;
			case wiRenderer::LINELIST:
				topo=D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
				break;
			case wiRenderer::PATCHLIST:
				topo=D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
				break;
			default:
				break;
			}
		#ifdef API_CALL_STRICTNESS
			D3D11_PRIMITIVE_TOPOLOGY p;
			context->IAGetPrimitiveTopology(&p);
			if(p!=topo)
		#endif
				context->IASetPrimitiveTopology(topo);
		}
	}
	inline static void BindVertexLayout(VertexLayout layout, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			VertexLayout v;
			context->IAGetInputLayout(&v);
			if(v!=layout)
		#endif
				context->IASetInputLayout(layout);
		#ifdef API_CALL_STRICTNESS
			if(v!=nullptr) v->Release();
		#endif
		}
	}
	inline static void BindBlendState(BlendState state, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			BlendState b;
			context->OMGetBlendState(&b,nullptr,nullptr);
			if(b!=state){
		#endif
				float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				UINT sampleMask   = 0xffffffff;
				context->OMSetBlendState(state,blendFactor,sampleMask);
		#ifdef API_CALL_STRICTNESS
			}
			if(b!=nullptr) b->Release();
		#endif
		}
	}
	inline static void BindDepthStencilState(DepthStencilState state, UINT stencilRef, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			DepthStencilState b;
			UINT s;
			context->OMGetDepthStencilState(&b,&s);
			if(b!=state || s!=stencilRef){
		#endif
				context->OMSetDepthStencilState(state,stencilRef);
		#ifdef API_CALL_STRICTNESS
			}
			if(b!=nullptr) b->Release();
		#endif
		}
	}
	inline static void BindRasterizerState(RasterizerState state, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			RasterizerState b;
			context->RSGetState(&b);
			if(b!=state){
		#endif
				context->RSSetState(state);
		#ifdef API_CALL_STRICTNESS
			}
			if(b!=nullptr) b->Release();
		#endif
		}
	}
	inline static void BindStreamOutTarget(BufferResource buffer, DeviceContext context=immediateContext){
		if(context!=nullptr){
			UINT offsetSO[1] = {0};
			context->SOSetTargets(1,&buffer,offsetSO);
		}
	}
	inline static void BindPS(PixelShader shader, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			PixelShader s;
			context->PSGetShader(&s,nullptr,0);
			if(s!=shader)
		#endif
				context->PSSetShader(shader, nullptr, 0);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr) s->Release();
		#endif
		}
	}
	inline static void BindVS(VertexShader shader, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			VertexShader s;
			context->VSGetShader(&s,nullptr,0);
			if(s!=shader)
		#endif
				context->VSSetShader(shader, nullptr, 0);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr) s->Release();
		#endif
		}
	}
	inline static void BindGS(GeometryShader shader, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			GeometryShader s;
			context->GSGetShader(&s,nullptr,0);
			if(s!=shader)
		#endif
				context->GSSetShader(shader, nullptr, 0);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr) s->Release();
		#endif
		}
	}
	inline static void BindHS(HullShader shader, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			HullShader s;
			context->HSGetShader(&s,nullptr,0);
			if(s!=shader)
		#endif
				context->HSSetShader(shader, nullptr, 0);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr) s->Release();
		#endif
		}
	}
	inline static void BindDS(DomainShader shader, DeviceContext context=immediateContext){
		if(context!=nullptr){
		#ifdef API_CALL_STRICTNESS
			DomainShader s;
			context->DSGetShader(&s,nullptr,0);
			if(s!=shader)
		#endif
				context->DSSetShader(shader, nullptr, 0);
		#ifdef API_CALL_STRICTNESS
			if(s!=nullptr) s->Release();
		#endif
		}
	}
	inline static void Draw(int vertexCount, DeviceContext context=immediateContext){
		if(context!=nullptr){
			context->Draw(vertexCount,0);
			++drawCalls[context];
		}
	}
	inline static void DrawIndexed(int indexCount, DeviceContext context=immediateContext){
		if(context!=nullptr){
			context->DrawIndexed(indexCount,0,0);
			++drawCalls[context];
		}
	}
	inline static void DrawIndexedInstanced(int indexCount, int instanceCount, DeviceContext context=immediateContext){
		if(context!=nullptr){
			context->DrawIndexedInstanced(indexCount,instanceCount,0,0,0);
			++drawCalls[context];
		}
	}
	//<value>Comment: specify dataSize param if you are uploading multiple instances of data (eg. sizeof(Vertex)*vertices.count())
	template<typename T>
	inline static void ResizeBuffer(BufferResource& buffer, int dataCount){
		if(buffer!=nullptr){
			D3D11_BUFFER_DESC desc;
			buffer->GetDesc(&desc);
			int dataSize = sizeof(T)*dataCount;
			if((int)desc.ByteWidth<dataSize){
				buffer->Release();
				desc.ByteWidth=dataSize;
				graphicsDevice->CreateBuffer( &desc, nullptr, &buffer );
			}
		}
	}
	template<typename T>
	inline static void UpdateBuffer(BufferResource& buffer, const T* data,DeviceContext context=immediateContext, int dataSize = -1)
	{
		if(buffer != nullptr && data!=nullptr && context != nullptr){
			D3D11_BUFFER_DESC desc;
			buffer->GetDesc(&desc);
			HRESULT hr;
			if(dataSize>(int)desc.ByteWidth){ //recreate the buffer if new datasize exceeds buffer size [SLOW][TEST!!!!]
				buffer->Release();
				desc.ByteWidth=dataSize*2;
				hr=graphicsDevice->CreateBuffer( &desc, nullptr, &buffer );
			}
			if(desc.Usage == D3D11_USAGE_DYNAMIC){
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				void* dataPtr;
				context->Map(buffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
				dataPtr = (void*)mappedResource.pData;
				memcpy(dataPtr,data,(dataSize>=0?dataSize:desc.ByteWidth));
				context->Unmap(buffer,0);
			}
			else{
				context->UpdateSubresource( buffer, 0, nullptr, data, 0, 0 );
			}
		}
	}
	template<typename T>
	inline static void SafeRelease(T& resource){
		if(resource != nullptr){
			resource->Release();
			resource=nullptr;
		}
	}

	static void UpdatePerWorldCB(ID3D11DeviceContext* context);
	static void UpdatePerFrameCB(ID3D11DeviceContext* context);
	static void UpdatePerRenderCB(ID3D11DeviceContext* context, int tessF);
	static void UpdatePerViewCB(ID3D11DeviceContext* context, const XMMATRIX& newView, const XMMATRIX& newRefView, const XMMATRIX& newProjection
							 , const XMVECTOR& newEye, const XMFLOAT4& newClipPlane = XMFLOAT4(0,0,0,0));
	static void UpdatePerEffectCB(ID3D11DeviceContext* context, const XMFLOAT4& blackoutBlackWhiteInvCol, const XMFLOAT4 colorMask);

	enum SHADED_TYPE
	{
		SHADED_NONE=0,
		SHADED_DEFERRED=1,
		SHADED_FORWARD_SIMPLE=2
	};
	
	static void DrawSky(const XMVECTOR&, ID3D11DeviceContext* context);
	static void DrawWorld(const XMMATRIX&, bool DX11Eff, int tessF, ID3D11DeviceContext* context
		, bool BlackOut, SHADED_TYPE shaded, ID3D11ShaderResourceView* refRes, bool grass, int passIdentifier = 1); //passidentifier is >1 (0 is for shadows)
	static void DrawForSO(ID3D11DeviceContext* context);
	static void ClearShadowMaps(ID3D11DeviceContext* context);
	static void DrawForShadowMap(ID3D11DeviceContext* context);
	static void DrawWorldWater(const XMMATRIX& newView, ID3D11ShaderResourceView* refracRes, ID3D11ShaderResourceView* refRes
		, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* nor, ID3D11DeviceContext* context, int passIdentifier=1);
	static void DrawWorldTransparent(const XMMATRIX& newView, ID3D11ShaderResourceView* refracRes, ID3D11ShaderResourceView* refRes
		, ID3D11ShaderResourceView* depth, ID3D11DeviceContext* context, int passIdentifier=1);
	void DrawDebugSpheres(const XMMATRIX&, ID3D11DeviceContext* context);
	static void DrawDebugLines(const XMMATRIX&, ID3D11DeviceContext* context);
	static void DrawDebugBoxes(const XMMATRIX&, ID3D11DeviceContext* context);
	static void DrawSoftParticles(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, bool dark = false);
	static void DrawSoftPremulParticles(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, bool dark = false);
	static void DrawTrails(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes);
	static void DrawImagesAdd(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes);
	//alpha-opaque
	static void DrawImages(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes);
	static void DrawImagesNormals(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes);
	static void DrawLights(const XMMATRIX&, ID3D11DeviceContext* context
		, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* normal, ID3D11ShaderResourceView* material
		, unsigned int stencilRef = 2);
	static void DrawVolumeLights(const XMMATRIX&, ID3D11DeviceContext* context);
	static void DrawLensFlares(ID3D11DeviceContext* context, ID3D11ShaderResourceView* depth, const int& RENDERWIDTH, const int& RENDERHEIGHT);
	static void DrawDecals(const XMMATRIX& newView, DeviceContext context, TextureView depth);
	
	inline static XMVECTOR GetSunPosition();
	inline static XMFLOAT4 GetSunColor();
	static Wind wind;
	static int getVertexCount(){return vertexCount;}
	static void resetVertexCount(){vertexCount=0;}
	static int getVisibleObjectCount(){return visibleCount;}
	static void resetVisibleObjectCount(){visibleCount=0;}
	static void setRenderResolution(int w, int h){RENDERWIDTH=w;RENDERHEIGHT=h;};

	static void FinishLoading();
	static wiSPTree* spTree;
	static wiSPTree* spTree_trans;
	static wiSPTree* spTree_water;
	static wiSPTree* spTree_lights;
	
	static vector<Object*> everyObject;
	static vector<Object*> objects;
	static vector<Object*> objects_trans;
	static vector<Object*> objects_water;	
	static MeshCollection meshes;
	static MaterialCollection materials;
	static vector<Armature*> armatures;
	static vector<Lines> boneLines;
	static vector<Cube> cubes;
	static vector<Light*> lights;
	static map<string,vector<wiEmittedParticle*>> emitterSystems;
	vector<ActionCamera> cameras;
	vector<HitSphere*> spheres;
	static map<string,Transform*> transforms;
	static list<Decal*> decals;
	
	static deque<wiSprite*> images;
	static deque<wiSprite*> waterRipples;
	static void CleanUpStaticTemp();
	
	static void SetUpLights();
	static void UpdateLights();


	static wiRenderTarget normalMapRT, imagesRT, imagesRTAdd;
	
	static Camera* cam;


	float getSphereRadius(const int& index);

	string DIRECTORY;

	enum PICKTYPE
	{
		PICK_OPAQUE,
		PICK_TRANSPARENT,
		PICK_WATER,
	};
	struct Picked
	{
		Object* object;
		XMFLOAT3 position,normal;

		Picked():object(nullptr){}
	};
	static Picked Pick(long cursorX, long cursorY, PICKTYPE pickType = PICKTYPE::PICK_OPAQUE);
	static RAY getPickRay(long cursorX, long cursorY);

	static PHYSICS* physicsEngine;
	static void SychronizeWithPhysicsEngine();

	static void LoadModel(const string& dir, const string& name, const XMMATRIX& transform = XMMatrixIdentity(), const string& ident = "common", PHYSICS* physicsEngine = nullptr);
};

