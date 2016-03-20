#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiGraphicsAPI.h"
#include "skinningDEF.h"
#include "SamplerMapping.h"
#include "wiSPTree.h"

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


class wiRenderer
{
public:
	static GraphicsDevice* graphicsDevice;
	static GraphicsDevice* GetDevice() { return graphicsDevice; }

#ifndef WINSTORE_SUPPORT
	static void InitDevice(HWND window, int screenW, int screenH, bool windowed);
#else
	static void InitDevice(Windows::UI::Core::CoreWindow^ window);
#endif
	static void Present(function<void()> drawToScreen1=nullptr,function<void()> drawToScreen2=nullptr,function<void()> drawToScreen3=nullptr);

	
	static Sampler samplers[SSLOT_COUNT];

	
	static int SHADOWMAPRES,SOFTSHADOW,POINTLIGHTSHADOW,POINTLIGHTSHADOWRES,SPOTLIGHTSHADOW,SPOTLIGHTSHADOWRES;
	static bool HAIRPARTICLEENABLED, EMITTERSENABLED;

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
	static BlendState			blendStates[BSTYPE_LAST];


	void UpdateSpheres();
	static void SetUpBoneLines();
	static void UpdateBoneLines();
	static void SetUpCubes();
	static void UpdateCubes();
	


	static bool	wireRender, debugSpheres, debugBoneLines, debugBoxes;


	static Texture2D* enviroMap,*colorGrading;
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
	static void UpdateRenderData(GRAPHICSTHREAD threadID);
	//static void UpdateSoftBodyPinning();
	static void UpdateSPTree(wiSPTree*& tree);
	static void UpdateImages();
	static void ManageImages();
	static void PutDecal(Decal* decal);
	static void PutWaterRipple(const string& image, const XMFLOAT3& pos, const wiWaterPlane& waterPlane);
	static void ManageWaterRipples();
	static void DrawWaterRipples(GRAPHICSTHREAD threadID);
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
	static Texture2D* GetColorGrading(){return colorGrading;};
	static void SetColorGrading(Texture2D* tex){colorGrading=tex;};
	static void SetEnviromentMap(Texture2D* tex){ enviroMap = tex; }
	static Texture2D* GetEnviromentMap(){ return enviroMap; }


	static Transform* getTransformByName(const string& name);
	static Armature* getArmatureByName(const string& get);
	static int getActionByName(Armature* armature, const string& get);
	static int getBoneByName(Armature* armature, const string& get);
	static Material* getMaterialByName(const string& get);
	HitSphere* getSphereByName(const string& get);
	static Object* getObjectByName(const string& name);
	static Light* getLightByName(const string& name);

	static void ReloadShaders(const string& path = "");
	static void BindPersistentState(GRAPHICSTHREAD threadID);
	static void RebindPersistentState(GRAPHICSTHREAD threadID);

	static Vertex TransformVertex(const Mesh* mesh, int vertexI, const XMMATRIX& mat = XMMatrixIdentity());
	static Vertex TransformVertex(const Mesh* mesh, const SkinnedVertex& vertex, const XMMATRIX& mat = XMMatrixIdentity());
	static XMFLOAT3 VertexVelocity(const Mesh* mesh, const int& vertexI);
	
public:
	

	static void UpdateWorldCB(GRAPHICSTHREAD threadID);
	static void UpdateFrameCB(GRAPHICSTHREAD threadID);
	static void UpdateCameraCB(GRAPHICSTHREAD threadID);
	static void SetClipPlane(XMFLOAT4 clipPlane, GRAPHICSTHREAD threadID);
	static void UpdateGBuffer(Texture2D* slot0, Texture2D* slot1, Texture2D* slot2, Texture2D* slot3, Texture2D* slot4, GRAPHICSTHREAD threadID);
	static void UpdateDepthBuffer(Texture2D* depth, Texture2D* linearDepth, GRAPHICSTHREAD threadID);
	
	static void DrawSky(GRAPHICSTHREAD threadID, bool isReflection = false);
	static void DrawSun(GRAPHICSTHREAD threadID);
	static void DrawWorld(Camera* camera, bool DX11Eff, int tessF, GRAPHICSTHREAD threadID
		, bool BlackOut, bool isReflection, SHADERTYPE shaded, Texture2D* refRes, bool grass, GRAPHICSTHREAD thread);
	static void ClearShadowMaps(GRAPHICSTHREAD threadID);
	static void DrawForShadowMap(GRAPHICSTHREAD threadID);
	static void DrawWorldTransparent(Camera* camera, Texture2D* refracRes, Texture2D* refRes
		, Texture2D* waterRippleNormals, GRAPHICSTHREAD threadID);
	void DrawDebugSpheres(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugBoneLines(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugLines(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugBoxes(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawSoftParticles(Camera* camera, GRAPHICSTHREAD threadID, bool dark = false);
	static void DrawSoftPremulParticles(Camera* camera, GRAPHICSTHREAD threadID, bool dark = false);
	static void DrawTrails(GRAPHICSTHREAD threadID, Texture2D* refracRes);
	static void DrawImagesAdd(GRAPHICSTHREAD threadID, Texture2D* refracRes);
	//alpha-opaque
	static void DrawImages(GRAPHICSTHREAD threadID, Texture2D* refracRes);
	static void DrawImagesNormals(GRAPHICSTHREAD threadID, Texture2D* refracRes);
	static void DrawLights(Camera* camera, GRAPHICSTHREAD threadID, unsigned int stencilRef = 2);
	static void DrawVolumeLights(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawLensFlares(GRAPHICSTHREAD threadID);
	static void DrawDecals(Camera* camera, GRAPHICSTHREAD threadID);
	
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

	static void PutEnvProbe(const XMFLOAT3& position, int resolution = 256);
};

