#pragma once
#include "CommonInclude.h"
#include "wiImageEffects.h"
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiEnums.h"
#include "wiMath.h"
#include "wiFrustum.h"
#include "wiTransform.h"
#include "wiIntersectables.h"
#include "ShaderInterop.h"

#include <vector>
#include <map>
#include <unordered_set>
#include <list>
#include <deque>
#include <sstream>

struct HitSphere;
class wiParticle;
class wiEmittedParticle;
class wiHairParticle;
class wiRenderTarget;

struct Armature;
struct Bone;
struct Mesh;
struct Material;
struct Object;
struct Model;

typedef std::map<std::string,Mesh*> MeshCollection;
typedef std::map<std::string,Material*> MaterialCollection;

class wiArchive;

struct ModelChild
{
	Model* parentModel = nullptr;
};

GFX_STRUCT Instance
{
	XMFLOAT4A mat0;
	XMFLOAT4A mat1;
	XMFLOAT4A mat2;
	XMFLOAT4A color_dither; //rgb:color, a:dither

	Instance(){}
	Instance(const XMFLOAT4X4& matIn, float dither = 0.0f, const XMFLOAT3& color = XMFLOAT3(1, 1, 1)){
		Create(matIn, dither, color);
	}
	inline void Create(const XMFLOAT4X4& matIn, float dither = 0.0f, const XMFLOAT3& color = XMFLOAT3(1, 1, 1)) volatile
	{
		//mat0 = XMFLOAT4A(matIn._11, matIn._21, matIn._31, matIn._41);
		//mat1 = XMFLOAT4A(matIn._12, matIn._22, matIn._32, matIn._42);
		//mat2 = XMFLOAT4A(matIn._13, matIn._23, matIn._33, matIn._43);
		//color_dither = XMFLOAT4A(color.x, color.y, color.z, dither);

		mat0.x = matIn._11;
		mat0.y = matIn._21;
		mat0.z = matIn._31;
		mat0.w = matIn._41;

		mat1.x = matIn._12;
		mat1.y = matIn._22;
		mat1.z = matIn._32;
		mat1.w = matIn._42;

		mat2.x = matIn._13;
		mat2.y = matIn._23;
		mat2.z = matIn._33;
		mat2.w = matIn._43;

		color_dither.x = color.x;
		color_dither.y = color.y;
		color_dither.z = color.z;
		color_dither.w = dither;
	}

	ALIGN_16
};
GFX_STRUCT InstancePrev
{
	XMFLOAT4A mat0;
	XMFLOAT4A mat1;
	XMFLOAT4A mat2;

	InstancePrev(){}
	InstancePrev(const XMFLOAT4X4& matIn)
	{
		Create(matIn);
	}
	inline void Create(const XMFLOAT4X4& matIn) volatile
	{
		//mat0 = XMFLOAT4A(matIn._11, matIn._21, matIn._31, matIn._41);
		//mat1 = XMFLOAT4A(matIn._12, matIn._22, matIn._32, matIn._42);
		//mat2 = XMFLOAT4A(matIn._13, matIn._23, matIn._33, matIn._43);

		mat0.x = matIn._11;
		mat0.y = matIn._21;
		mat0.z = matIn._31;
		mat0.w = matIn._41;

		mat1.x = matIn._12;
		mat1.y = matIn._22;
		mat1.z = matIn._32;
		mat1.w = matIn._42;

		mat2.x = matIn._13;
		mat2.y = matIn._23;
		mat2.z = matIn._33;
		mat2.w = matIn._43;
	}

	ALIGN_16
};
CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	XMFLOAT4 baseColor; // + alpha (.w)
	XMFLOAT4 texMulAdd;
	float roughness;
	float reflectance;
	float metalness;
	float emissive;
	float refractionIndex;
	float subsurfaceScattering;
	float normalMapStrength;
	float parallaxOcclusionMapping;

	MaterialCB() {};
	MaterialCB(const Material& mat) { Create(mat); };
	void Create(const Material& mat);
};
struct Material
{
private:
	STENCILREF engineStencilRef;
	uint8_t userStencilRef;

public:

	std::string name;
	XMFLOAT3 diffuseColor;

	std::string surfaceMapName;
	wiGraphicsTypes::Texture2D* surfaceMap;

	std::string textureName;
	wiGraphicsTypes::Texture2D* texture;
	bool premultipliedTexture;
	BLENDMODE blendFlag;
		
	std::string normalMapName;
	wiGraphicsTypes::Texture2D* normalMap;

	std::string displacementMapName;
	wiGraphicsTypes::Texture2D* displacementMap;

	std::string specularMapName;
	wiGraphicsTypes::Texture2D* specularMap;

	MaterialCB gpuData;
	wiGraphicsTypes::GPUBuffer constantBuffer;
	static wiGraphicsTypes::GPUBuffer* constantBuffer_Impostor;
	static void CreateImpostorMaterialCB();

	bool toonshading;
	bool water, shadeless;
	float enviroReflection;

	XMFLOAT4 specular;
	int specular_power;
	XMFLOAT3 movingTex;
	float framesToWaitForTexCoordOffset;
	XMFLOAT4 texMulAdd;
	bool isSky;

	bool cast_shadow;

	// PBR properties
	XMFLOAT3 baseColor;
	float alpha;
	float roughness;
	float reflectance;
	float metalness;
	float emissive;
	float refractionIndex;
	float subsurfaceScattering;
	float normalMapStrength;
	float parallaxOcclusionMapping;

	bool planar_reflections;

	float alphaRef;

	struct CustomShader
	{
		std::string name;

		struct CustomShaderPass
		{
			wiGraphicsTypes::GraphicsPSO* pso = nullptr;

			~CustomShaderPass()
			{
				SAFE_DELETE(pso);
			}
		};

		CustomShaderPass passes[SHADERTYPE_COUNT];
	};
	CustomShader* customShader = nullptr;
	static std::vector<Material::CustomShader*> customShaderPresets;

	Material()
	{
		init();
	}
	Material(const std::string& newName){
		name=newName;
		init();
	}
	~Material();
	void init();

	void Update();

	bool IsTransparent() const { return alpha < 1.0f || customShader != nullptr; }
	bool IsWater() const { return water; }
	bool HasPlanarReflection() const { return planar_reflections || IsWater(); }
	bool IsCastingShadow() const { return cast_shadow; }
	bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }
	RENDERTYPE GetRenderType() const
	{
		if (customShader != nullptr)
			return RENDERTYPE_TRANSPARENT;
		if (IsWater())
			return RENDERTYPE_WATER;
		if (IsTransparent())
			return RENDERTYPE_TRANSPARENT;
		return RENDERTYPE_OPAQUE;
	}
	void ConvertToPhysicallyBasedMaterial();
	// User stencil ref could be anything from 0-127, greater will be truncated when using 8 bit stencil buffer!
	void SetUserStencilRef(uint8_t value)
	{
		assert(value < 128);
		userStencilRef = value & 0x0F;
	}
	UINT GetStencilRef()
	{
		return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
	}
	wiGraphicsTypes::Texture2D* GetBaseColorMap() const;
	wiGraphicsTypes::Texture2D* GetNormalMap() const;
	wiGraphicsTypes::Texture2D* GetSurfaceMap() const;
	wiGraphicsTypes::Texture2D* GetDisplacementMap() const;
	void Serialize(wiArchive& archive);

	ALIGN_16
};
struct RibbonVertex
{
	XMFLOAT3 pos;
	XMFLOAT2 tex;
	XMFLOAT4 col;
	float fade;
	//XMFLOAT3 vel;

	RibbonVertex(){pos=/*vel=*/XMFLOAT3(0,0,0);tex=XMFLOAT2(0,0);col=XMFLOAT4(0,0,0,0);fade=0.06f;}
	RibbonVertex(const XMFLOAT3& newPos, const XMFLOAT2& newTex, const XMFLOAT4& newCol, float newFade){
		pos=newPos;
		tex=newTex;
		col=newCol;
		fade=newFade;
		//vel=XMFLOAT3(0,0,0);
	}
};
struct RibbonTrail
{
	int base, tip;
	RibbonTrail(){base=tip=-1;}
};
struct VertexRef{
	int index;
	float weight;
	VertexRef(){index=0;weight=0;}
	VertexRef(int i, float w){index=i;weight=w;}
};
struct VertexGroup{
	std::string name;
	std::map<int,float> vertices;
	VertexGroup(){name="";}
	VertexGroup(const std::string& n){name=n;}
	void addVertex(const VertexRef& vRef){vertices.insert(std::pair<int,float>(vRef.index,vRef.weight));}
	void Serialize(wiArchive& archive);
};
struct MeshSubset
{
	Material* material;
	UINT indexBufferOffset;

	std::vector<uint32_t> subsetIndices;

	MeshSubset();
	~MeshSubset();
};
struct Mesh
{
public:
	struct Vertex_FULL
	{
		XMFLOAT4 pos; //pos, wind
		XMFLOAT4 nor; //normal, unused
		XMFLOAT4 tex; //tex, matIndex, unused
		XMFLOAT4 ind; //bone indices
		XMFLOAT4 wei; //bone weights
	
		Vertex_FULL(){
			pos=XMFLOAT4(0,0,0,0);
			nor=XMFLOAT4(0,0,0,1); 
			tex=XMFLOAT4(0,0,0,0);
			ind=XMFLOAT4(0,0,0,0);
			wei=XMFLOAT4(0,0,0,0);
		};
		Vertex_FULL(const XMFLOAT3& newPos){
			pos=XMFLOAT4(newPos.x,newPos.y,newPos.z,1);
			nor=XMFLOAT4(0,0,0,1);
			tex=XMFLOAT4(0,0,0,0);
			ind=XMFLOAT4(0,0,0,0);
			wei=XMFLOAT4(0,0,0,0);
		}
	};
	struct Vertex_POS
	{
		XMFLOAT3 pos;
		uint32_t normal_wind;

		Vertex_POS() :pos(XMFLOAT3(0.0f, 0.0f, 0.0f)), normal_wind(0) {}
		Vertex_POS(const Vertex_FULL& vert)
		{
			pos.x = vert.pos.x;
			pos.y = vert.pos.y;
			pos.z = vert.pos.z;
			NORWINDFromFloat(XMFLOAT3(vert.nor.x, vert.nor.y, vert.nor.z), vert.pos.w);
		}
		inline XMVECTOR LoadPOS() const
		{
			return XMLoadFloat3(&pos);
		}
		inline XMVECTOR LoadNOR() const
		{
			return XMLoadFloat3(&GetNor_FULL());
		}
		inline void NORWINDFromFloat(const XMFLOAT3& normal, float wind)
		{
			normal_wind = 0;

			normal_wind |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
			normal_wind |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
			normal_wind |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
			normal_wind |= (uint32_t)(wind * 255.0f) << 24;
		}
		inline XMFLOAT3 GetNor_FULL() const
		{
			XMFLOAT3 nor_FULL(0, 0, 0);

			nor_FULL.x = (float)((normal_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor_FULL.y = (float)((normal_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor_FULL.z = (float)((normal_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;

			return nor_FULL;
		}

		static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R32G32B32A32_FLOAT;
	};
	struct Vertex_TEX
	{
		XMHALF2 tex;

		Vertex_TEX() :tex(XMHALF2(0.0f, 0.0f)) {}
		Vertex_TEX(const Vertex_FULL& vert)
		{
			tex = XMHALF2(vert.tex.x, vert.tex.y);
		}

		static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R16G16_FLOAT;
	};
	struct Vertex_BON
	{
		uint64_t ind;
		uint64_t wei;

		Vertex_BON()
		{
			ind = 0;
			wei = 0;
		}
		Vertex_BON(const Vertex_FULL& vert)
		{
			ind = 0;
			wei = 0;

			ind |= (uint64_t)vert.ind.x << 0;
			ind |= (uint64_t)vert.ind.y << 16;
			ind |= (uint64_t)vert.ind.z << 32;
			ind |= (uint64_t)vert.ind.w << 48;

			wei |= (uint64_t)(vert.wei.x * 65535.0f) << 0;
			wei |= (uint64_t)(vert.wei.y * 65535.0f) << 16;
			wei |= (uint64_t)(vert.wei.z * 65535.0f) << 32;
			wei |= (uint64_t)(vert.wei.w * 65535.0f) << 48;
		}
		inline XMFLOAT4 GetInd_FULL() const
		{
			XMFLOAT4 ind_FULL(0, 0, 0, 0);

			ind_FULL.x = (float)((ind >> 0)  & 0x0000FFFF);
			ind_FULL.y = (float)((ind >> 16)  & 0x0000FFFF);
			ind_FULL.z = (float)((ind >> 32) & 0x0000FFFF);
			ind_FULL.w = (float)((ind >> 48) & 0x0000FFFF);

			return ind_FULL;
		}
		inline XMFLOAT4 GetWei_FULL() const
		{
			XMFLOAT4 wei_FULL(0, 0, 0, 0);

			wei_FULL.x = (float)((wei >> 0)  & 0x0000FFFF) / 65535.0f;
			wei_FULL.y = (float)((wei >> 16) & 0x0000FFFF) / 65535.0f;
			wei_FULL.z = (float)((wei >> 32) & 0x0000FFFF) / 65535.0f;
			wei_FULL.w = (float)((wei >> 48) & 0x0000FFFF) / 65535.0f;

			return wei_FULL;
		}
	};

	std::string name;
	std::string parent;
	std::vector<Vertex_FULL>	vertices_FULL;
	std::vector<Vertex_POS>		vertices_POS; // position(xyz), normal+wind(w as uint)
	std::vector<Vertex_TEX>		vertices_TEX; // texcoords
	std::vector<Vertex_BON>		vertices_BON; // bone indices, bone weights
	std::vector<Vertex_POS>		vertices_Transformed_POS; // for soft body simulation
	std::vector<Vertex_POS>		vertices_Transformed_PRE; // for soft body simulation
	std::vector<uint32_t>		indices;
	std::vector<XMFLOAT3>		physicsverts;
	std::vector<uint32_t>		physicsindices;
	std::vector<int>			physicalmapGP;
	std::vector<MeshSubset>		subsets;
	std::vector<std::string>	materialNames;

	wiGraphicsTypes::GPUBuffer*	indexBuffer;
	wiGraphicsTypes::GPUBuffer*	vertexBuffer_POS;
	wiGraphicsTypes::GPUBuffer*	vertexBuffer_TEX;
	wiGraphicsTypes::GPUBuffer*	vertexBuffer_BON;
	wiGraphicsTypes::GPUBuffer*	streamoutBuffer_POS;
	wiGraphicsTypes::GPUBuffer*	streamoutBuffer_PRE;

	// Dynamic vertexbuffers write into a global pool, these will be the offsets into that:
	UINT bufferOffset_POS;
	UINT bufferOffset_PRE;

	wiGraphicsTypes::INDEXBUFFER_FORMAT indexFormat;

	bool renderable,doubleSided;

	bool calculatedAO;

	std::string armatureName;
	Armature* armature;

	AABB aabb;

	//RIBBONTRAIL VERTS
	RibbonTrail trailInfo;

	bool isBillboarded;
	XMFLOAT3 billboardAxis;

	std::vector<VertexGroup> vertexGroups;
	bool softBody;
	float mass, friction;
	int massVG,goalVG,softVG; //vertexGroupID
	std::vector<XMFLOAT3> goalPositions,goalNormals;

	wiRenderTarget	impostorTarget;
	float impostorDistance;
	static wiGraphicsTypes::GPUBuffer impostorVB_POS;
	static wiGraphicsTypes::GPUBuffer impostorVB_TEX;

	float tessellationFactor;

	bool optimized;
	bool renderDataComplete;

	Mesh(const std::string& newName = "");
	~Mesh();
	void LoadFromFile(const std::string& newName, const std::string& fname, const MaterialCollection& materialColl, const std::unordered_set<Armature*>& armatures);
	void Optimize();
	void CreateRenderData();
	static void CreateImpostorVB();
	void ComputeNormals(bool smooth = false);
	void FlipCulling();
	void FlipNormals();
	void init();
	
	bool hasArmature() const { return armature != nullptr; }
	bool hasImpostor() const { return impostorTarget.IsInitialized(); }
	bool hasDynamicVB() const { return softBody; }
	float getTessellationFactor() { return tessellationFactor; }
	int GetRenderTypes() const;
	wiGraphicsTypes::INDEXBUFFER_FORMAT GetIndexFormat() const { return indexFormat; }
	void Serialize(wiArchive& archive);
};
struct Cullable
{
public:
	Cullable();
	AABB bounds;
	void Serialize(wiArchive& archive);
};
struct Object : public Cullable, public Transform, public ModelChild
{
	std::string meshName;
	Mesh* mesh;

	bool renderable;
	int cascadeMask = 0; // which shadow cascades to skip (0: skip none, 1: skip first, etc...)

	//PARTICLE
	std::vector< wiEmittedParticle* > eParticleSystems;
	std::vector< wiHairParticle* > hParticleSystems;

	float transparency;

	XMFLOAT3 color;

	
	//PHYSICS
	bool rigidBody, kinematic;
	std::string collisionShape, physicsType;
	float mass, friction, restitution, damping;
	
	
	//RIBBON TRAIL
	std::deque<RibbonVertex> trail;
	wiGraphicsTypes::Texture2D* trailDistortTex;
	wiGraphicsTypes::Texture2D* trailTex;

	int physicsObjectID;

	// occlusion result history bitfield (32 bit->32 frame history)
	uint32_t occlusionHistory;
	// occlusion query pool index
	int occlusionQueryID;

	// Is it deformed with an armature?
	bool isArmatureDeformed() const
	{
		return mesh->hasArmature() && boneParent.empty();
	}
	// Is it controlled by the physics engine or an Armature?
	bool isDynamic() const
	{
		return ((physicsObjectID >= 0 && !kinematic) || mesh->softBody || isArmatureDeformed());
	}

	Object(const std::string& newName = "");
	Object(const Object& other);
	virtual ~Object();
	void init();
	void EmitTrail(const XMFLOAT3& color, float fadeSpeed = 0.06f);
	void FadeTrail();
	bool IsCastingShadow() const;
	bool IsReflector() const;
	int GetRenderTypes() const;
	bool IsOccluded() const;
	virtual void UpdateTransform();
	void UpdateObject();
	XMMATRIX GetOBB() const;
	void Serialize(wiArchive& archive);

	Object operator=(const Object& other) { return Object(other); }
};
// AutoSerialized!
struct KeyFrame
{
	XMFLOAT4 data;
	int frameI;

	KeyFrame(){
		frameI=0;
		data=XMFLOAT4(0,0,0,1);
	}
	KeyFrame(int newFrameI, float x, float y, float z, float w){
		frameI=newFrameI;
		data=XMFLOAT4(x,y,z,w);
	}
	void Serialize(wiArchive& archive);
};
struct Action
{
	std::string name;
	int frameCount;

	Action(){
		name="";
		frameCount=0;
	}
};
struct ActionFrames
{
	std::vector< KeyFrame > keyframesRot;
	std::vector< KeyFrame > keyframesPos;
	std::vector< KeyFrame > keyframesSca;

	ActionFrames(){
	}
};
struct Bone : public Transform
{
	std::vector<std::string> childrenN;
	std::vector<Bone*> childrenI;

	XMFLOAT4X4 restInv;
	std::vector<ActionFrames> actionFrames;

	XMFLOAT4X4 recursivePose, recursiveRest, recursiveRestInv;

	// These will be used in the skinning process to transform verts
	XMFLOAT4X4 boneRelativity;

	float length;
	bool connected;

	Bone():length(0),connected(false){};
	Bone(const std::string& newName):Transform(){
		name=newName;
		childrenN.clear();
		childrenI.clear();
		actionFrames.clear();
		// create identity action
		actionFrames.push_back(ActionFrames());
		actionFrames.back().keyframesPos.push_back(KeyFrame(1, 0, 0, 0, 1));
		actionFrames.back().keyframesRot.push_back(KeyFrame(1, 0, 0, 0, 1));
		actionFrames.back().keyframesSca.push_back(KeyFrame(1, 1, 1, 1, 0));
		length=1.0f;
		connected = false;
	}

	XMMATRIX getMatrix(int getTranslation = 1, int getRotation = 1, int getScale = 1);
	virtual void UpdateTransform();
	void Serialize(wiArchive& archive);
};
struct AnimationLayer
{
	std::string name;

	int activeAction, prevAction;
	float currentFrame;

	bool playing;

	// for the calculation of the blendFact
	float blendCurrentFrame;
	// how many frames should the blend finish in
	float blendFrames;
	// [0,1] 0: prev; 1: current
	float blendFact;
	// animate prev action and blend with current bone set
	float currentFramePrevAction;
	// How much the layer will contribute
	float weight;

	enum ANIMATIONLAYER_TYPE
	{
		ANIMLAYER_TYPE_PRIMARY,
		ANIMLAYER_TYPE_ADDITIVE,
	}type;

	bool looped;

	AnimationLayer();

	void ChangeAction(int actionIndex, float blendFrames = 0.0f, float weight = 1.0f);
	void ResetAction();
	void ResetActionPrev();
	void PauseAction();
	void StopAction();
	void PlayAction();

	void Serialize(wiArchive& archive);
};
struct Armature : public Transform, public ModelChild
{
public:
	std::vector<Bone*> boneCollection;
	std::vector<Bone*> rootbones;

	std::list<AnimationLayer*> animationLayers;
	std::vector<Action> actions;

	GFX_STRUCT ShaderBoneType
	{
		XMFLOAT4A pose0;
		XMFLOAT4A pose1;
		XMFLOAT4A pose2; 
		
		void Create(const XMFLOAT4X4& matIn)
		{
			pose0 = XMFLOAT4A(matIn._11, matIn._21, matIn._31, matIn._41);
			pose1 = XMFLOAT4A(matIn._12, matIn._22, matIn._32, matIn._42);
			pose2 = XMFLOAT4A(matIn._13, matIn._23, matIn._33, matIn._43);
		}

		ALIGN_16
	};
	std::vector<ShaderBoneType> boneData;
	wiGraphicsTypes::GPUBuffer boneBuffer;

	Armature() :Transform(){
		init();
	};
	Armature(const std::string& newName):Transform(){
		name=newName;
		init();
	}
	virtual ~Armature();
	void init()
	{
		boneCollection.clear();
		rootbones.clear();
		actions.clear();
		actions.push_back(Action());
		actions.back().name = "[WickedEngine-Default]{IdentityAction}";
		actions.back().frameCount = 1;
		XMStoreFloat4x4(&world, XMMatrixIdentity());
		animationLayers.clear();
		animationLayers.push_back(new AnimationLayer());
		animationLayers.back()->type = AnimationLayer::ANIMLAYER_TYPE_PRIMARY;
	}

	inline AnimationLayer* GetPrimaryAnimation() { 
		return *animationLayers.begin(); 
	}

	// Empty actionName will be the Identity Action (T-pose)
	void ChangeAction(const std::string& actionName = "", float blendFrames = 0.0f, const std::string& animLayer = "", float weight = 1.0f);
	AnimationLayer* GetAnimLayer(const std::string& name);
	void AddAnimLayer(const std::string& name);
	void DeleteAnimLayer(const std::string& name);
	void RecursiveRest(Bone* bone);
	virtual void UpdateTransform();
	void UpdateArmature();
	void CreateFamily();
	void CreateBuffers();
	Bone* GetBone(const std::string& name);
	void Serialize(wiArchive& archive);

	ALIGN_16

private:
	enum KeyFrameType {
		ROTATIONKEYFRAMETYPE,
		POSITIONKEYFRAMETYPE,
		SCALARKEYFRAMETYPE,
	};
	static void RecursiveBoneTransform(Armature* armature, Bone* bone, const XMMATRIX& parentCombinedMat);
	static XMVECTOR InterPolateKeyFrames(float currentFrame, const int frameCount, const std::vector<KeyFrame>& keyframes, KeyFrameType type);
};
struct SHCAM{	
	XMFLOAT4X4 View,Projection;
	XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...
	XMFLOAT3 Eye,At,Up;
	float nearplane,farplane,size;

	SHCAM(){
		nearplane = 0.1f; farplane = 200, size = 0;
		Init(XMQuaternionIdentity());
		Create_Perspective(XM_PI/2.0f);
	}
	//orthographic
	SHCAM(float size, const XMVECTOR& dir, float nearP, float farP){
		nearplane=nearP;
		farplane=farP;
		Init(dir);
		Create_Ortho(size);
	};
	//perspective
	SHCAM(const XMFLOAT4& dir, float newNear, float newFar, float newFov){
		size = 0;
		nearplane=newNear;
		farplane=newFar;
		Init(XMLoadFloat4(&dir));
		Create_Perspective(newFov);
	};
	void Init(const XMVECTOR& dir){
		XMMATRIX rot = XMMatrixRotationQuaternion( dir );
		XMVECTOR rEye = XMVectorSet(0,0,0,0);
		XMVECTOR rAt = XMVector3Transform( XMVectorSet( 0.0f, -1.0f, 0.0f, 0.0f ), rot);
		XMVECTOR rUp = XMVector3Transform( XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f ), rot);
		XMMATRIX rView = XMMatrixLookAtLH(rEye,rAt,rUp);
	
		XMStoreFloat3( &Eye, rEye );
		XMStoreFloat3( &At, rAt );
		XMStoreFloat3( &Up, rUp );
		XMStoreFloat4x4( &View, rView);
	}
	void Create_Ortho(float size){
		XMMATRIX rProjection = XMMatrixOrthographicOffCenterLH(-size*0.5f,size*0.5f,-size*0.5f,size*0.5f,farplane,nearplane);
		XMStoreFloat4x4( &Projection, rProjection);
		rProjection = XMMatrixOrthographicOffCenterLH(-size*0.5f, size*0.5f, -size*0.5f, size*0.5f, nearplane, farplane);
		XMStoreFloat4x4(&realProjection, rProjection);
		this->size=size;
	}
	void Create_Perspective(float fov){
		XMMATRIX rProjection = XMMatrixPerspectiveFovLH(fov,1,farplane,nearplane);
		XMStoreFloat4x4( &Projection, rProjection); 
		rProjection = XMMatrixPerspectiveFovLH(fov, 1, nearplane, farplane);
		XMStoreFloat4x4(&realProjection, rProjection);
	}
	void Update(const XMVECTOR& pos){
		XMStoreFloat4x4( &View , XMMatrixTranslationFromVector(-pos) 
			* XMMatrixLookAtLH(XMLoadFloat3(&Eye),XMLoadFloat3(&At),XMLoadFloat3(&Up))
			);
	}
	void Update(const XMMATRIX& mat){
		XMVECTOR sca,rot,tra;
		XMMatrixDecompose(&sca,&rot,&tra,mat);

		XMMATRIX mRot = XMMatrixRotationQuaternion( rot );

		XMVECTOR rEye = XMVectorAdd( XMLoadFloat3(&Eye),tra );
		XMVECTOR rAt = XMVectorAdd( XMVector3Transform( XMLoadFloat3(&At),mRot ),tra );
		XMVECTOR rUp = XMVector3Transform( XMLoadFloat3(&Up),mRot );

		XMStoreFloat4x4( &View , 
			XMMatrixLookAtLH(rEye,rAt,rUp)
			);
	}
	void Update(const XMMATRIX& rot, const XMVECTOR& tra)
	{
		XMVECTOR rEye = XMVectorAdd(XMLoadFloat3(&Eye), tra);
		XMVECTOR rAt = XMVectorAdd(XMVector3Transform(XMLoadFloat3(&At), rot), tra);
		XMVECTOR rUp = XMVector3Transform(XMLoadFloat3(&Up), rot);

		XMStoreFloat4x4(&View,
			XMMatrixLookAtLH(rEye, rAt, rUp)
		);
	}
	XMMATRIX getVP(){
		return XMMatrixTranspose(XMLoadFloat4x4(&View)*XMLoadFloat4x4(&Projection));
	}
};
struct Light : public Cullable , public Transform, public ModelChild
{
	enum LightType {
		DIRECTIONAL			= ENTITY_TYPE_DIRECTIONALLIGHT,
		POINT				= ENTITY_TYPE_POINTLIGHT,
		SPOT				= ENTITY_TYPE_SPOTLIGHT,
		SPHERE				= ENTITY_TYPE_SPHERELIGHT,
		DISC				= ENTITY_TYPE_DISCLIGHT,
		RECTANGLE			= ENTITY_TYPE_RECTANGLELIGHT,
		TUBE				= ENTITY_TYPE_TUBELIGHT,
		LIGHTTYPE_COUNT,
	};

	XMFLOAT4 color;
	XMFLOAT4 enerDis;
	bool volumetrics = false;
	bool noHalo;
	bool shadow;
	std::vector<wiGraphicsTypes::Texture2D*> lensFlareRimTextures;
	std::vector<std::string> lensFlareNames;

	static wiGraphicsTypes::Texture2D* shadowMapArray_2D;
	static wiGraphicsTypes::Texture2D* shadowMapArray_Cube;
	static wiGraphicsTypes::Texture2D* shadowMapArray_Transparent;
	int shadowMap_index;
	int entityArray_index;

	std::vector<SHCAM> shadowCam_pointLight;
	std::vector<SHCAM> shadowCam_dirLight;
	std::vector<SHCAM> shadowCam_spotLight;

	float shadowBias;

	// area light props:
	float radius, width, height;

	XMFLOAT3 GetDirection() const;
	float GetRange() const;

	Light();
	virtual ~Light();
	virtual void UpdateTransform();
	void UpdateLight();
	void SetType(LightType type);
	LightType GetType() const { return type; }
	void Serialize(wiArchive& archive);

	bool IsActive() { return abs(enerDis.x + enerDis.y) > 0; }

private:
	LightType type;
};
struct Decal : public Cullable, public Transform, public ModelChild
{
	std::string texName,norName;
	wiGraphicsTypes::Texture2D* texture,*normal;
	XMFLOAT3 front;
	float life,fadeStart;
	XMFLOAT4 atlasMulAdd;
	XMFLOAT4 color;
	float emissive;

	Decal(const XMFLOAT3& tra=XMFLOAT3(0,0,0), const XMFLOAT3& sca=XMFLOAT3(1,1,1), const XMFLOAT4& rot=XMFLOAT4(0,0,0,1), const std::string& tex="", const std::string& nor="");
	virtual ~Decal();
	
	void addTexture(const std::string& tex);
	void addNormal(const std::string& nor);
	virtual void UpdateTransform();
	void UpdateDecal();
	float GetOpacity() const;
	void Serialize(wiArchive& archive);
};
struct WorldInfo{
	XMFLOAT3 horizon;
	XMFLOAT3 zenith;
	XMFLOAT3 ambient;
	XMFLOAT3 fogSEH;
	XMFLOAT4 water;
	float cloudiness;
	float cloudScale;
	float cloudSpeed;

	WorldInfo(){
		horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		zenith = XMFLOAT3(0.00f, 0.00f, 0.0f);
		ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		fogSEH = XMFLOAT3(100, 1000, 0);
		water = XMFLOAT4(0, 0, 0, 0);
		cloudiness = 0.0f;
		cloudScale = 0.0003f;
		cloudSpeed = 0.1f;
	}
};
struct Wind{
	XMFLOAT3 direction;
	float randomness;
	float waveSize;
	Wind():direction(XMFLOAT3(0,0,0)),randomness(5),waveSize(1){}
};
struct Camera : public Transform, public ModelChild
{
	XMFLOAT4X4 View, Projection, VP;
	XMFLOAT3 At, Up;
	float width, height;
	float zNearP, zFarP;
	float fov;
	Frustum frustum;
	XMFLOAT4X4 InvView, InvProjection, InvVP;
	XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...
	
	Camera():Transform(){
		width = height = zNearP = zFarP = fov = 0;
	}
	Camera(const XMFLOAT3& newPos, const XMFLOAT4& newRot,
		const std::string& newName):Transform()
	{
		width = height = zNearP = zFarP = fov = 0;
		translation_rest=newPos;
		rotation_rest=newRot;
		name=newName;
	}
	void SetUp(float newWidth, float newHeight, float newNear, float newFar, float fov = XM_PI / 3.0f)
	{
		XMMATRIX View = XMMATRIX();
		XMVECTOR At = XMVectorSet(0,0,1,0), Up = XMVectorSet(0,1,0,0), Eye = this->GetEye();

		zNearP = newNear;
		zFarP = newFar;

		width = newWidth;
		height = newHeight;

		this->fov = fov;

		UpdateProjection();

		XMStoreFloat4x4(&this->View, View);
		XMStoreFloat3(&this->At, At);
		XMStoreFloat3(&this->Up, Up);

		UpdateTransform();
	}
	void UpdateProps()
	{
		XMMATRIX View = XMMATRIX();
		XMVECTOR At = XMVectorSet(0, 0, 1, 0), Up = XMVectorSet(0, 1, 0, 0), Eye = this->GetEye();

		XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
		At = XMVector3Transform(At, camRot);
		Up = XMVector3Transform(Up, camRot);

		View = XMMatrixLookToLH(Eye, At, Up);

		XMStoreFloat4x4(&this->View, View);
		XMStoreFloat3(&this->At, At);
		XMStoreFloat3(&this->Up, Up);

		frustum.ConstructFrustum(zFarP, realProjection, this->View);

		XMMATRIX VP = XMMatrixMultiply(View, GetProjection());
		XMStoreFloat4x4(&this->VP, VP);
		XMStoreFloat4x4(&InvView, XMMatrixInverse(nullptr, View));
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, VP));
	}
	void Move(const XMVECTOR& movevector)
	{
		XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
		XMVECTOR rotVect = XMVector3Transform(movevector, camRot);
		XMVECTOR Eye = XMLoadFloat3(&translation_rest);
		Eye += rotVect;

		XMStoreFloat3(&translation_rest, Eye);
	}
	void Reflect(Camera* toReflect, const XMFLOAT4& plane = XMFLOAT4(0,1,0,0))
	{
		*this = *toReflect;

		XMMATRIX reflectMatrix = XMMatrixReflect(XMLoadFloat4(&plane));

		XMMATRIX View;
		XMVECTOR At = XMVectorSet(0, 0, 1, 0), Up = XMVectorSet(0, 1, 0, 0), Eye = this->GetEye();

		XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
		At = XMVector3TransformNormal(At, camRot);
		Up = XMVector3TransformNormal(Up, camRot);

		At = XMVector3TransformNormal(At, reflectMatrix);
		Up = XMVector3TransformNormal(Up, reflectMatrix);
		Eye = XMVectorSetW(Eye, 1);
		Eye = XMVector4Transform(Eye, reflectMatrix);

		View = XMMatrixLookToLH(Eye, At, Up);

		XMStoreFloat4x4(&this->View, View);
		XMStoreFloat3(&this->At, At);
		XMStoreFloat3(&this->Up, Up);

		frustum.ConstructFrustum(zFarP, realProjection, this->View);

		XMMATRIX VP = XMMatrixMultiply(View, GetProjection());
		XMStoreFloat4x4(&this->VP, VP);
		XMStoreFloat4x4(&InvView, XMMatrixInverse(nullptr, View));
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, VP));
	}
	void UpdateProjection()
	{
		XMMATRIX P = XMMatrixPerspectiveFovLH(fov, width / height, zFarP, zNearP); // reverse zbuffer!
		XMStoreFloat4x4(&realProjection, XMMatrixPerspectiveFovLH(fov, width / height, zNearP, zFarP));
		XMMATRIX InvP = XMMatrixInverse(nullptr, P);
		XMStoreFloat4x4(&this->Projection, P);
		XMStoreFloat4x4(&this->InvProjection, InvP);
	}
	void BakeMatrices()
	{
		XMMATRIX V = XMLoadFloat4x4(&this->View);
		XMMATRIX P = XMLoadFloat4x4(&this->Projection);
		XMMATRIX VP = XMMatrixMultiply(V, P);
		XMStoreFloat4x4(&this->VP, VP);
		XMStoreFloat4x4(&InvView, XMMatrixInverse(nullptr, V));
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, VP));
		XMMATRIX InvP = XMMatrixInverse(nullptr, P);
		XMStoreFloat4x4(&this->Projection, P);
		XMStoreFloat4x4(&this->InvProjection, InvP);
	}

	void Lerp(const Camera* a, const Camera* b, float t);
	void CatmullRom(const Camera* a, const Camera* b, const Camera* c, const Camera* d, float t);

	XMVECTOR GetEye() const
	{
		return XMLoadFloat3(&translation);
	}
	XMVECTOR GetAt() const
	{
		return XMLoadFloat3(&At);
	}
	XMVECTOR GetUp() const
	{
		return XMLoadFloat3(&Up);
	}
	XMVECTOR GetRight() const
	{
		return XMVector3Cross(GetAt(), GetUp());
	}
	XMMATRIX GetView() const
	{
		return XMLoadFloat4x4(&View);
	}
	XMMATRIX GetInvView() const
	{
		return XMLoadFloat4x4(&InvView);
	}
	XMMATRIX GetProjection() const
	{
		return XMLoadFloat4x4(&Projection);
	}
	XMMATRIX GetInvProjection() const
	{
		return XMLoadFloat4x4(&InvProjection);
	}
	XMMATRIX GetViewProjection() const
	{
		return XMLoadFloat4x4(&VP);
	}
	XMMATRIX GetInvViewProjection() const
	{
		return XMLoadFloat4x4(&InvVP);
	}

	// when the projection matrix is modified for reverse zbuffering, this returns the normal projection
	XMMATRIX GetRealProjection() const
	{
		return XMLoadFloat4x4(&realProjection);
	}
	virtual void UpdateTransform();

	void Serialize(wiArchive& archive);
};
struct EnvironmentProbe : public Transform, public Cullable, public ModelChild
{
	int textureIndex;
	bool realTime;
	bool isUpToDate;

	EnvironmentProbe() : textureIndex(-1), realTime(false), isUpToDate(false) {}

	void UpdateEnvProbe();

	void Serialize(wiArchive& archive);
};
struct ForceField : public Transform, public ModelChild
{
	int type;
	float gravity; // negative = deflector, positive = attractor
	float range; // affection range

	ForceField() : Transform()
	{
		type = ENTITY_TYPE_FORCEFIELD_POINT;
		gravity = 0;
		range = 0;
	}

	void Serialize(wiArchive& archive);
};

struct Model : public Transform
{
	std::unordered_set<Object*> objects;
	MeshCollection meshes;
	MaterialCollection materials;
	std::unordered_set<Armature*> armatures;
	std::unordered_set<Light*> lights;
	std::unordered_set<Decal*> decals;
	std::unordered_set<ForceField*> forces;
	std::list<EnvironmentProbe*> environmentProbes;
	std::list<Camera*> cameras;

	Model();
	virtual ~Model();
	void CleanUp();
	void LoadFromDisk(const std::string& fileName);
	void FinishLoading();
	void UpdateModel();
	void Add(Object* value);
	void Add(Armature* value);
	void Add(Light* value);
	void Add(Decal* value);
	void Add(ForceField* value);
	void Add(EnvironmentProbe* value);
	void Add(Camera* value);
	// merge
	void Add(Model* value);
	void Serialize(wiArchive& archive);
};

struct Scene
{
	// First is always the world node
	std::vector<Model*> models;
	WorldInfo worldInfo;
	Wind wind;

	Scene();
	~Scene();
	void ClearWorld();

	Model* GetWorldNode();
	void AddModel(Model* model);
	void Update();
};

// Load world info from file:
void LoadWiWorldInfo(const std::string& fileName, WorldInfo& worldInfo, Wind& wind);

