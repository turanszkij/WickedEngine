#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiGraphicsAPI.h"
#include "skinningDEF.h"
#include "SamplerMapping.h"

struct Transform;
struct Vertex;
struct SkinnedVertex;
struct Material;
struct Object;
struct Mesh;
struct Armature;
struct Bone;
struct KeyFrame;
struct SHCAM;
struct Light;
struct Decal;
struct WorldInfo;
struct Wind;
struct Camera;
struct HitSphere;
struct RAY;
struct Camera;
struct Model;
struct Scene;

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
//class  Camera;
class  wiRenderTarget;
class  wiWaterPlane;

typedef map<string,Mesh*> MeshCollection;
typedef map<string, Material*> MaterialCollection;
typedef unordered_set<Cullable*> CulledList;


class wiRenderer
{
public:

	static D3D_DRIVER_TYPE				driverType;
	static D3D_FEATURE_LEVEL			featureLevel;
	static SwapChain					swapChain;
	static RenderTargetView				renderTargetView;
	static ViewPort						viewPort;
	static GraphicsDevice				graphicsDevice;
	static DeviceContext				immediateContext;
	static bool DX11,VSYNC,DEFERREDCONTEXT_SUPPORT;
	static DeviceContext deferredContexts[GRAPHICSTHREAD_COUNT];
	static CommandList commandLists[GRAPHICSTHREAD_COUNT];
	static mutex graphicsMutex;
#ifndef WINSTORE_SUPPORT
	static HRESULT InitDevice(HWND window, int screenW, int screenH, bool windowed);
#else
	static HRESULT InitDevice(Windows::UI::Core::CoreWindow^ window);
#endif
	static void DestroyDevice();
	static void Present(function<void()> drawToScreen1=nullptr,function<void()> drawToScreen2=nullptr,function<void()> drawToScreen3=nullptr);
	static void wiRenderer::ReleaseCommandLists();
	static void wiRenderer::ExecuteDeferredContexts();
	static void FinishCommandList(GRAPHICSTHREAD thread);

	static map<DeviceContext,long> drawCalls;
	static long getDrawCallCount();
	static bool getMultithreadingSupport(){ return DEFERREDCONTEXT_SUPPORT; }

	inline static DeviceContext getImmediateContext(){ return immediateContext; }
	inline static DeviceContext getDeferredContext(GRAPHICSTHREAD thread){ return deferredContexts[thread]; }

	inline static void Lock(){ graphicsMutex.lock(); }
	inline static void Unlock(){ graphicsMutex.unlock(); }
	
	static Sampler samplers[SSLOT_COUNT_PERSISTENT];

	
	static int SCREENWIDTH,SCREENHEIGHT;
	static int SHADOWMAPRES,SOFTSHADOW,POINTLIGHTSHADOW,POINTLIGHTSHADOWRES,SPOTLIGHTSHADOW,SPOTLIGHTSHADOWRES;
	static bool HAIRPARTICLEENABLED, EMITTERSENABLED;

	static int GetScreenWidth(){ return SCREENWIDTH; }
	static int GetScreenHeight(){ return SCREENHEIGHT; }
	static int SetScreenWidth(int value){ SCREENWIDTH = value; }
	static int SetScreenHeight(int value){ SCREENHEIGHT = value; }
	static void SetDirectionalLightShadowProps(int resolution, int softShadowQuality);
	static void SetPointLightShadowProps(int shadowMapCount, int resolution);
	static void SetSpotLightShadowProps(int count, int resolution);

protected:

	// Persistent buffers:
	GFX_STRUCT WorldCB
	{
		XMFLOAT4 mSun;
		XMFLOAT4 mSunColor;
		XMFLOAT3 mHorizon;				float pad0;
		XMFLOAT3 mZenith;				float pad1;
		XMFLOAT3 mAmbient;				float pad2;
		XMFLOAT3 mFog;					float pad3;
		XMFLOAT2 mScreenWidthHeight;
		float padding[2];

		CB_SETBINDSLOT(CBSLOT_RENDERER_WORLD)

		ALIGN_16
	};
	GFX_STRUCT FrameCB
	{
		XMFLOAT3 mWindDirection;		float pad0;
		float mWindTime;
		float mWindWaveSize;
		float mWindRandomness;
		float padding[1];
		
		CB_SETBINDSLOT(CBSLOT_RENDERER_FRAME)

		ALIGN_16
	};
	GFX_STRUCT CameraCB
	{
		XMMATRIX mView;
		XMMATRIX mProj;
		XMMATRIX mVP;
		XMMATRIX mPrevV;
		XMMATRIX mPrevP;
		XMMATRIX mPrevVP;
		XMMATRIX mReflVP;
		XMMATRIX mInvP;
		XMFLOAT3 mCamPos;		float pad0;
		XMFLOAT3 mAt;			float pad1;
		XMFLOAT3 mUp;			float pad2;
		float    mZNearP;
		float    mZFarP;
		float padding[2];

		CB_SETBINDSLOT(CBSLOT_RENDERER_CAMERA)

		ALIGN_16
	};
	GFX_STRUCT MaterialCB
	{
		XMFLOAT4 difColor;
		XMFLOAT4 specular;
		XMFLOAT4 texMulAdd;
		UINT hasRef;
		UINT hasNor;
		UINT hasTex;
		UINT hasSpe;
		UINT shadeless;
		UINT specular_power;
		UINT toon;
		UINT matIndex;
		float refractionIndex;
		float metallic;
		float emissive;
		float roughness;

		CB_SETBINDSLOT(CBSLOT_RENDERER_MATERIAL)

		MaterialCB() {};
		MaterialCB(const Material& mat, UINT materialIndex) { Create(mat,materialIndex); };
		void Create(const Material& mat, UINT materialIndex);

		ALIGN_16
	};
	GFX_STRUCT DirectionalLightCB
	{
		XMVECTOR direction;
		XMFLOAT4 col;
		XMFLOAT4 mBiasResSoftshadow;
		XMMATRIX mShM[3];

		CB_SETBINDSLOT(CBSLOT_RENDERER_DIRLIGHT)

		ALIGN_16
	};
	GFX_STRUCT MiscCB
	{
		XMMATRIX mTransform;
		XMFLOAT4 mColor;

		CB_SETBINDSLOT(CBSLOT_RENDERER_MISC)

		ALIGN_16
	};
	GFX_STRUCT ShadowCB
	{
		XMMATRIX mVP;

		CB_SETBINDSLOT(CBSLOT_RENDERER_SHADOW)

		ALIGN_16
	};

	// On demand buffers:
	GFX_STRUCT PointLightCB
	{
		XMFLOAT3 pos; float pad;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;

		CB_SETBINDSLOT(CBSLOT_RENDERER_POINTLIGHT)

		ALIGN_16
	};
	GFX_STRUCT SpotLightCB
	{
		XMMATRIX world;
		XMVECTOR direction;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;
		XMFLOAT4 mBiasResSoftshadow;
		XMMATRIX mShM;

		CB_SETBINDSLOT(CBSLOT_RENDERER_SPOTLIGHT)

		ALIGN_16
	};
	GFX_STRUCT VolumeLightCB
	{
		XMMATRIX world;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;

		CB_SETBINDSLOT(CBSLOT_RENDERER_VOLUMELIGHT)

		ALIGN_16
	};
	GFX_STRUCT DecalCB
	{
		XMMATRIX mDecalVP;
		int hasTexNor;
		XMFLOAT3 eye;
		float opacity;
		XMFLOAT3 front;

		CB_SETBINDSLOT(CBSLOT_RENDERER_DECAL)

		ALIGN_16
	};
	GFX_STRUCT CubeMapRenderCB
	{
		XMMATRIX mViewProjection[6];

		CB_SETBINDSLOT(CBSLOT_RENDERER_CUBEMAPRENDER)

		ALIGN_16
	};
	GFX_STRUCT BoneCB
	{
		XMMATRIX pose[MAXBONECOUNT];
		XMMATRIX prev[MAXBONECOUNT];

		CB_SETBINDSLOT(CBSLOT_RENDERER_BONEBUFFER)

		ALIGN_16
	};
	GFX_STRUCT APICB
	{
		XMFLOAT4 clipPlane;

		CB_SETBINDSLOT(CBSLOT_API)

		ALIGN_16
	};


	static BufferResource		constantBuffers[CBTYPE_LAST];
	static VertexShader			vertexShaders[VSTYPE_LAST];
	static PixelShader			pixelShaders[PSTYPE_LAST];
	static GeometryShader		geometryShaders[GSTYPE_LAST];
	static HullShader			hullShaders[HSTYPE_LAST];
	static DomainShader			domainShaders[DSTYPE_LAST];
	static RasterizerState		rasterizers[RSTYPE_LAST];
	static DepthStencilState	depthStencils[DSSTYPE_LAST];
	static VertexLayout			vertexLayouts[VLTYPE_LAST];


	void UpdateSpheres();
	static void SetUpBoneLines();
	static void UpdateBoneLines();
	static void SetUpCubes();
	static void UpdateCubes();
	


	static bool	wireRender, debugSpheres, debugBoneLines, debugBoxes;


	static BlendState blendState, blendStateTransparent, blendStateAdd;
	static TextureView enviroMap,colorGrading;
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


	static float GameSpeed, overrideGameSpeed;

	static Scene* scene;

public:
	static string SHADERPATH;

	wiRenderer();
	void CleanUp();
	static void SetUpStaticComponents();
	static void CleanUpStatic();
	
	static void Update();
	// Render data that needs to be updated on the main thread!
	static void UpdatePerFrameData();
	static void UpdateRenderData(DeviceContext context);
	//static void UpdateSoftBodyPinning();
	static void UpdateSPTree(wiSPTree*& tree);
	static void UpdateImages();
	static void ManageImages();
	static void PutDecal(Decal* decal);
	static void PutWaterRipple(const string& image, const XMFLOAT3& pos, const wiWaterPlane& waterPlane);
	static void ManageWaterRipples();
	static void DrawWaterRipples(DeviceContext context);
	static void SetGameSpeed(float value){GameSpeed=value; if(GameSpeed<0) GameSpeed=0;};
	static float GetGameSpeed();
	static void ChangeRasterizer(){wireRender=!wireRender;};
	static bool GetRasterizer(){return wireRender;};
	static void ToggleDebugSpheres(){debugSpheres=!debugSpheres;}
	static void SetToDrawDebugSpheres(bool param){debugSpheres=param;}
	static void SetToDrawDebugBoneLines(bool param){ debugBoneLines = param; }
	static void SetToDrawDebugBoxes(bool param){debugBoxes=param;}
	static bool GetToDrawDebugSpheres(){return debugSpheres;};
	static bool GetToDrawDebugBoxes(){return debugBoxes;};
	static TextureView GetColorGrading(){return colorGrading;};
	static void SetColorGrading(TextureView tex){colorGrading=tex;};
	static void SetEnviromentMap(TextureView tex){ enviroMap = tex; }
	static TextureView GetEnviromentMap(){ return enviroMap; }


	static Transform* getTransformByName(const string& name);
	static Armature* getArmatureByName(const string& get);
	static int getActionByName(Armature* armature, const string& get);
	static int getBoneByName(Armature* armature, const string& get);
	static Material* getMaterialByName(const string& get);
	HitSphere* getSphereByName(const string& get);
	static Object* getObjectByName(const string& name);
	static Light* getLightByName(const string& name);

	static void ReloadShaders(const string& path = "");

	static Vertex TransformVertex(const Mesh* mesh, int vertexI, const XMMATRIX& mat = XMMatrixIdentity());
	static Vertex TransformVertex(const Mesh* mesh, const SkinnedVertex& vertex, const XMMATRIX& mat = XMMatrixIdentity());
	static XMFLOAT3 VertexVelocity(const Mesh* mesh, const int& vertexI);
	
public:
	inline static void BindTexturePS(TextureView texture, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && texture != nullptr) {
			context->PSSetShaderResources(slot, 1, &texture);
		}
	}
	inline static void BindTexturesPS(TextureView textures[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && textures != nullptr) {
			context->PSSetShaderResources(slot, num, textures);
		}
	}
	inline static void BindTextureVS(TextureView texture, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && texture != nullptr) {
			context->VSSetShaderResources(slot, 1, &texture);
		}
	}
	inline static void BindTexturesVS(TextureView textures[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && textures != nullptr) {
			context->VSSetShaderResources(slot, num, textures);
		}
	}
	inline static void BindTextureGS(TextureView texture, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && texture != nullptr) {
			context->GSSetShaderResources(slot, 1, &texture);
		}
	}
	inline static void BindTexturesGS(TextureView textures[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && textures != nullptr) {
			context->GSSetShaderResources(slot, num, textures);
		}
	}
	inline static void BindTextureDS(TextureView texture, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && texture != nullptr) {
			context->DSSetShaderResources(slot, 1, &texture);
		}
	}
	inline static void BindTexturesDS(TextureView textures[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && textures != nullptr) {
			context->DSSetShaderResources(slot, num, textures);
		}
	}
	inline static void BindTextureHS(TextureView texture, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && texture != nullptr) {
			context->HSSetShaderResources(slot, 1, &texture);
		}
	}
	inline static void BindTexturesHS(TextureView textures[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && textures != nullptr) {
			context->HSSetShaderResources(slot, num, textures);
		}
	}
	inline static void UnbindTextures(int slot, int num, DeviceContext context = immediateContext)
	{
		assert(num <= 16 && "UnbindTextures limit of 16 reached!");
		if (context != nullptr)
		{
			static TextureView empties[16] = {
				nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
				nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
			};
			context->PSSetShaderResources(slot, num, empties);
			context->VSSetShaderResources(slot, num, empties);
			context->GSSetShaderResources(slot, num, empties);
			context->HSSetShaderResources(slot, num, empties);
			context->DSSetShaderResources(slot, num, empties);
		}
	}
	inline static void BindSamplerPS(Sampler sampler, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && sampler != nullptr) {
			context->PSSetSamplers(slot, 1, &sampler);
		}
	}
	inline static void BindSamplersPS(Sampler samplers[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && samplers != nullptr) {
			context->PSSetSamplers(slot, num, samplers);
		}
	}
	inline static void BindSamplerVS(Sampler sampler, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && sampler != nullptr) {
			context->VSSetSamplers(slot, 1, &sampler);
		}
	}
	inline static void BindSamplersVS(Sampler samplers[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && samplers != nullptr) {
			context->VSSetSamplers(slot, num, samplers);
		}
	}
	inline static void BindSamplerGS(Sampler sampler, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && sampler != nullptr) {
			context->GSSetSamplers(slot, 1, &sampler);
		}
	}
	inline static void BindSamplersGS(Sampler samplers[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && samplers != nullptr) {
			context->GSSetSamplers(slot, num, samplers);
		}
	}
	inline static void BindSamplerHS(Sampler sampler, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && sampler != nullptr) {
			context->HSSetSamplers(slot, 1, &sampler);
		}
	}
	inline static void BindSamplersHS(Sampler samplers[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && samplers != nullptr) {
			context->HSSetSamplers(slot, num, samplers);
		}
	}
	inline static void BindSamplerDS(Sampler sampler, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr && sampler != nullptr) {
			context->DSSetSamplers(slot, 1, &sampler);
		}
	}
	inline static void BindSamplersDS(Sampler samplers[], int slot, int num, DeviceContext context = immediateContext) {
		if (context != nullptr && samplers != nullptr) {
			context->DSSetSamplers(slot, num, samplers);
		}
	}
	inline static void BindConstantBufferPS(BufferResource buffer, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->PSSetConstantBuffers(slot, 1, &buffer);
		}
	}
	inline static void BindConstantBufferVS(BufferResource buffer, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->VSSetConstantBuffers(slot, 1, &buffer);

		}
	}
	inline static void BindConstantBufferGS(BufferResource buffer, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->GSSetConstantBuffers(slot, 1, &buffer);
		}
	}
	inline static void BindConstantBufferDS(BufferResource buffer, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr) {

			context->DSSetConstantBuffers(slot, 1, &buffer);
		}
	}
	inline static void BindConstantBufferHS(BufferResource buffer, int slot, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->HSSetConstantBuffers(slot, 1, &buffer);
		}
	}
	inline static void BindVertexBuffer(BufferResource vertexBuffer, int slot, UINT stride, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			UINT offset = 0;
			context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
		}
	}
	inline static void BindIndexBuffer(BufferResource indexBuffer, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		}
	}
	inline static void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)type);
		}
	}
	inline static void BindVertexLayout(VertexLayout layout, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->IASetInputLayout(layout);
		}
	}
	inline static void BindBlendState(BlendState state, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			static float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			static UINT sampleMask = 0xffffffff;
			context->OMSetBlendState(state, blendFactor, sampleMask);
		}
	}
	inline static void BindBlendStateEx(BlendState state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff,
		DeviceContext context = immediateContext) {
		if (context != nullptr) {
			float fblendFactor[4] = { blendFactor.x, blendFactor.y, blendFactor.z, blendFactor.w };
		}
	}
	inline static void BindDepthStencilState(DepthStencilState state, UINT stencilRef, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->OMSetDepthStencilState(state, stencilRef);
		}
	}
	inline static void BindRasterizerState(RasterizerState state, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->RSSetState(state);
		}
	}
	inline static void BindStreamOutTarget(BufferResource buffer, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			UINT offsetSO[1] = { 0 };
			context->SOSetTargets(1, &buffer, offsetSO);
		}
	}
	inline static void BindPS(PixelShader shader, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->PSSetShader(shader, nullptr, 0);
		}
	}
	inline static void BindVS(VertexShader shader, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->VSSetShader(shader, nullptr, 0);
		}
	}
	inline static void BindGS(GeometryShader shader, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->GSSetShader(shader, nullptr, 0);
		}
	}
	inline static void BindHS(HullShader shader, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->HSSetShader(shader, nullptr, 0);
		}
	}
	inline static void BindDS(DomainShader shader, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->DSSetShader(shader, nullptr, 0);
		}
	}
	inline static void Draw(int vertexCount, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->Draw(vertexCount, 0);
			++drawCalls[context];
		}
	}
	inline static void DrawIndexed(int indexCount, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->DrawIndexed(indexCount, 0, 0);
			++drawCalls[context];
		}
	}
	inline static void DrawIndexedInstanced(int indexCount, int instanceCount, DeviceContext context = immediateContext) {
		if (context != nullptr) {
			context->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
			++drawCalls[context];
		}
	}
	//<value>Comment: specify dataSize param if you are uploading multiple instances of data (eg. sizeof(Vertex)*vertices.count())
	template<typename T>
	inline static void ResizeBuffer(BufferResource& buffer, int dataCount) {
		if (buffer != nullptr) {
			static thread_local D3D11_BUFFER_DESC desc;
			buffer->GetDesc(&desc);
			int dataSize = sizeof(T)*dataCount;
			if ((int)desc.ByteWidth<dataSize) {
				buffer->Release();
				desc.ByteWidth = dataSize;
				graphicsDevice->CreateBuffer(&desc, nullptr, &buffer);
			}
		}
	}
	template<typename T>
	inline static void UpdateBuffer(BufferResource& buffer, const T* data, DeviceContext context = immediateContext, int dataSize = -1)
	{
		if (buffer != nullptr && data != nullptr && context != nullptr) {
			static thread_local D3D11_BUFFER_DESC desc;
			buffer->GetDesc(&desc);
			HRESULT hr;
			if (dataSize>(int)desc.ByteWidth) { //recreate the buffer if new datasize exceeds buffer size
				buffer->Release();
				desc.ByteWidth = dataSize * 2;
				hr = graphicsDevice->CreateBuffer(&desc, nullptr, &buffer);
			}
			if (desc.Usage == D3D11_USAGE_DYNAMIC) {
				static thread_local D3D11_MAPPED_SUBRESOURCE mappedResource;
				void* dataPtr;
				context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
				dataPtr = (void*)mappedResource.pData;
				memcpy(dataPtr, data, (dataSize >= 0 ? dataSize : desc.ByteWidth));
				context->Unmap(buffer, 0);
			}
			else {
				context->UpdateSubresource(buffer, 0, nullptr, data, 0, 0);
			}
		}
	}
	template<typename T>
	inline static void SafeRelease(T& resource) {
		if (resource != nullptr) {
			resource->Release();
			resource = nullptr;
		}
	}
	template<typename T>
	inline static void SafeInit(T& resource)
	{
		resource = nullptr;
	}

	static void UpdateWorldCB(DeviceContext context);
	static void UpdateFrameCB(DeviceContext context);
	static void UpdateCameraCB(DeviceContext context);
	static void SetClipPlane(XMFLOAT4 clipPlane, DeviceContext context);

	static void DrawSky(DeviceContext context, bool isReflection = false);
	static void DrawSun(DeviceContext context);
	static void DrawWorld(Camera* camera, bool DX11Eff, int tessF, DeviceContext context
		, bool BlackOut, bool isReflection, SHADERTYPE shaded, TextureView refRes, bool grass, GRAPHICSTHREAD thread);
	static void ClearShadowMaps(DeviceContext context);
	static void DrawForShadowMap(DeviceContext context);
	static void DrawWorldTransparent(Camera* camera, TextureView refracRes, TextureView refRes
		, TextureView waterRippleNormals, TextureView depth, DeviceContext context);
	void DrawDebugSpheres(Camera* camera, DeviceContext context);
	static void DrawDebugBoneLines(Camera* camera, DeviceContext context);
	static void DrawDebugLines(Camera* camera, DeviceContext context);
	static void DrawDebugBoxes(Camera* camera, DeviceContext context);
	static void DrawSoftParticles(Camera* camera, ID3D11DeviceContext *context, TextureView depth, bool dark = false);
	static void DrawSoftPremulParticles(Camera* camera, ID3D11DeviceContext *context, TextureView depth, bool dark = false);
	static void DrawTrails(DeviceContext context, TextureView refracRes);
	static void DrawImagesAdd(DeviceContext context, TextureView refracRes);
	//alpha-opaque
	static void DrawImages(DeviceContext context, TextureView refracRes);
	static void DrawImagesNormals(DeviceContext context, TextureView refracRes);
	static void DrawLights(Camera* camera, DeviceContext context
		, TextureView depth, TextureView normal, TextureView material
		, unsigned int stencilRef = 2);
	static void DrawVolumeLights(Camera* camera, DeviceContext context);
	static void DrawLensFlares(DeviceContext context, TextureView depth);
	static void DrawDecals(Camera* camera, DeviceContext context, TextureView depth);
	
	static XMVECTOR GetSunPosition();
	static XMFLOAT4 GetSunColor();
	static int getVertexCount(){return vertexCount;}
	static void resetVertexCount(){vertexCount=0;}
	static int getVisibleObjectCount(){return visibleCount;}
	static void resetVisibleObjectCount(){visibleCount=0;}

	static void FinishLoading();
	static wiSPTree* spTree;
	static wiSPTree* spTree_lights;

	// The scene holds all models, world information and wind information
	static Scene& GetScene();

	vector<Camera> cameras;
	vector<HitSphere*> spheres;
	static vector<Lines*> boneLines;
	static vector<Lines*> linesTemp;
	static vector<Cube> cubes;

	static vector<Object*> objectsWithTrails;
	static vector<wiEmittedParticle*> emitterSystems;
	
	static deque<wiSprite*> images;
	static deque<wiSprite*> waterRipples;
	static void CleanUpStaticTemp();


	static wiRenderTarget normalMapRT, imagesRT, imagesRTAdd;
	
	static Camera* cam, *refCam, *prevFrameCam;
	static Camera* getCamera(){ return cam; }
	static Camera* getRefCamera(){ return refCam; }

	float getSphereRadius(const int& index);

	string DIRECTORY;

	struct Picked
	{
		Object* object;
		XMFLOAT3 position,normal;
		float distance;

		Picked():object(nullptr),distance(0){}
	};

	// Pick closest object in the world
	// pickType: PICKTYPE enum values ocncatenated with | operator
	// layer : concatenated string of layers to check against, empty string : all layers will be checked
	// layerDisable : concatenated string of layers to NOT check against
	static Picked Pick(long cursorX, long cursorY, int pickType = PICK_OPAQUE, const string& layer = "", const string& layerDisable = "");
	static Picked Pick(RAY& ray, int pickType = PICK_OPAQUE, const string& layer = "", const string& layerDisable = "");
	static RAY getPickRay(long cursorX, long cursorY);
	static void RayIntersectMeshes(const RAY& ray, const CulledList& culledObjects, vector<Picked>& points,
		int pickType = PICK_OPAQUE, bool dynamicObjects = true, const string& layer = "", const string& layerDisable = "");
	static void CalculateVertexAO(Object* object);

	static PHYSICS* physicsEngine;
	static void SychronizeWithPhysicsEngine();

	static Model* LoadModel(const string& dir, const string& name, const XMMATRIX& transform = XMMatrixIdentity(), const string& ident = "common");
	static void LoadWorldInfo(const string& dir, const string& name);
};

