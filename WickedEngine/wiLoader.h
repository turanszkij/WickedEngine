#pragma once
#include "CommonInclude.h"
#include "skinningDEF.h"
#include "wiImageEffects.h"
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiGraphicsThreads.h"
#include "wiStencilRef.h"
#include "wiMath.h"
#include "wiFrustum.h"

struct HitSphere;
class wiParticle;
class wiEmittedParticle;
class wiHairParticle;
class wiRenderTarget;

struct Armature;
struct Bone;
struct Mesh;
struct Material;

typedef map<string,Mesh*> MeshCollection;
typedef map<string,Material*> MaterialCollection;

struct SkinnedVertex
{
	XMFLOAT4 pos; //pos, wind
	XMFLOAT4 nor; //normal, vertex ao
	XMFLOAT4 tex; //tex, matIndex, padding
	XMFLOAT4 bon; //bone indices
	XMFLOAT4 wei; //bone weights


	SkinnedVertex(){
		pos=XMFLOAT4(0,0,0,1);
		nor=XMFLOAT4(0,0,0,1); 
		tex=XMFLOAT4(0,0,0,0);
		bon=XMFLOAT4(0,0,0,0);
		wei=XMFLOAT4(0,0,0,0);
	};
	SkinnedVertex(const XMFLOAT3& newPos){
		pos=XMFLOAT4(newPos.x,newPos.y,newPos.z,1);
		nor=XMFLOAT4(0,0,0,1);
		tex=XMFLOAT4(0,0,0,0);
		bon=XMFLOAT4(0,0,0,0);
		wei=XMFLOAT4(0,0,0,0);
	}
};
struct Vertex
{
	XMFLOAT4 pos; //pos, wind
	XMFLOAT4 nor; //normal, vertex ao
	XMFLOAT4 tex; //tex, matIndex, padding
	XMFLOAT4 pre; //previous frame position

	Vertex(){
		pos=XMFLOAT4(0,0,0,1);
		nor=XMFLOAT4(0,0,0,1);
		tex=XMFLOAT4(0,0,0,0);
		pre=XMFLOAT4(0,0,0,0);
	};
	Vertex(const XMFLOAT3& newPos){
		pos=XMFLOAT4(newPos.x,newPos.y,newPos.z,1);
		nor=XMFLOAT4(0,0,0,1);
		tex=XMFLOAT4(0,0,0,0);
		pre=XMFLOAT4(0,0,0,0);
	}
};
struct Instance
{
	XMFLOAT4A mat0;
	XMFLOAT4A mat1;
	XMFLOAT4A mat2;
	XMFLOAT4A color_dither; //rgb:color, a:dither

	Instance(float dither = 0.0f, const XMFLOAT3& color = XMFLOAT3(1, 1, 1)){
		mat0 = XMFLOAT4A(1, 0, 0, 0);
		mat0 = XMFLOAT4A(0, 1, 0, 0);
		mat0 = XMFLOAT4A(0, 0, 1, 0);
		color_dither = XMFLOAT4A(color.x, color.y, color.z, dither);
	}
	Instance(const XMMATRIX& matIn, float dither = 0.0f, const XMFLOAT3& color = XMFLOAT3(1, 1, 1)){
		Create(matIn, dither, color);
	}
	void Create(const XMMATRIX& matIn, float dither = 0.0f, const XMFLOAT3& color = XMFLOAT3(1, 1, 1))
	{
		XMStoreFloat4A(&mat0, matIn.r[0]);
		XMStoreFloat4A(&mat1, matIn.r[1]);
		XMStoreFloat4A(&mat2, matIn.r[2]);
		color_dither = XMFLOAT4A(color.x, color.y, color.z, dither);
	}

	ALIGN_16
};
struct Material
{
	string name;
	XMFLOAT3 diffuseColor;

	bool hasRefMap;
	string refMapName;
	ID3D11ShaderResourceView* refMap;

	bool hasTexture;
	string textureName;
	ID3D11ShaderResourceView* texture;
	bool premultipliedTexture;
	BLENDMODE blendFlag;
		
	bool hasNormalMap;
	string normalMapName;
	ID3D11ShaderResourceView* normalMap;

	bool hasDisplacementMap;
	string displacementMapName;
	ID3D11ShaderResourceView* displacementMap;

	bool hasSpecularMap;
	string specularMapName;
	ID3D11ShaderResourceView* specularMap;

	bool subsurface_scattering;
	bool toonshading;
	bool transparent;
	bool water,shadeless;
	float alpha;
	float refraction_index;
	float enviroReflection;
	float emit;

	XMFLOAT4 specular;
	int specular_power;
	XMFLOAT3 movingTex;
	float framesToWaitForTexCoordOffset;
	XMFLOAT2 texOffset;
	bool isSky;

	bool cast_shadow;


	Material(){}
	Material(string newName){
		name=newName;
		diffuseColor = XMFLOAT3(0,0,0);

		hasRefMap=false;
		refMapName="";
		refMap=0;

		hasTexture=false;
		textureName="";
		texture=0;
		premultipliedTexture=false;
		blendFlag=BLENDMODE::BLENDMODE_ALPHA;

		hasNormalMap=false;
		normalMapName="";
		normalMap=0;

		hasDisplacementMap=false;
		displacementMapName="";
		displacementMap=0;

		
		hasSpecularMap=false;
		specularMapName="";
		specularMap=0;

		subsurface_scattering=toonshading=transparent=water=false;
		alpha=1.0f;
		refraction_index=0.0f;
		enviroReflection=0.0f;
		emit=0.0f;

		specular=XMFLOAT4(0,0,0,0);
		specular_power=50;
		movingTex=XMFLOAT3(0,0,0);
		framesToWaitForTexCoordOffset=0;
		texOffset=XMFLOAT2(0,0);
		isSky=water=shadeless=false;
		cast_shadow=true;
	}
	void CleanUp();
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
struct SPHERE;
struct RAY;
struct AABB{
	enum INTERSECTION_TYPE{
		OUTSIDE,
		INTERSECTS,
		INSIDE,
	};

	XMFLOAT3 corners[8];

	AABB();
	AABB(const XMFLOAT3& min, const XMFLOAT3& max);
	void create(const XMFLOAT3& min, const XMFLOAT3& max);
	void createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth);
	AABB get(const XMMATRIX& mat);
	AABB get(const XMFLOAT4X4& mat);
	XMFLOAT3 getMin() const;
	XMFLOAT3 getMax() const;
	XMFLOAT3 getCenter() const;
	XMFLOAT3 getHalfWidth() const;
	float getArea() const;
	float getRadius() const;
	INTERSECTION_TYPE intersects(const AABB& b) const;
	bool intersects(const XMFLOAT3& p) const;
	bool intersects(const RAY& ray) const;
	AABB operator* (float a);
};
struct SPHERE{
	float radius;
	XMFLOAT3 center;
	SPHERE():center(XMFLOAT3(0,0,0)),radius(0){}
	SPHERE(const XMFLOAT3& c, float r):center(c),radius(r){}
	bool intersects(const AABB& b);
	bool intersects(const SPHERE& b);
};
struct RAY{
	XMFLOAT3 origin,direction,direction_inverse;
	
	RAY(const XMFLOAT3& newOrigin=XMFLOAT3(0,0,0), const XMFLOAT3& newDirection=XMFLOAT3(0,0,1)):origin(newOrigin),direction(newDirection){}
	RAY(const XMVECTOR& newOrigin, const XMVECTOR& newDirection){
		XMStoreFloat3(&origin,newOrigin);
		XMStoreFloat3(&direction,newDirection);
		XMStoreFloat3(&direction_inverse,XMVectorDivide(XMVectorSet(1,1,1,1),newDirection));
	}
	bool intersects(const AABB& box) const;
};
struct BoneShaderBuffer
{
#ifdef USE_GPU_SKINNING
	XMMATRIX pose[MAXBONECOUNT];
	XMMATRIX prev[MAXBONECOUNT];
#endif
};
struct VertexRef{
	int index;
	float weight;
	VertexRef(){index=0;weight=0;}
	VertexRef(int i, float w){index=i;weight=w;}
};
struct VertexGroup{
	string name;
	//vector<VertexRef> vertices;
	//index,weight
	map<int,float> vertices;
	VertexGroup(){name="";}
	VertexGroup(string n){name=n;}
	void addVertex(const VertexRef& vRef){vertices.insert(pair<int,float>(vRef.index,vRef.weight));}
};
struct Mesh{
	string name;
	string parent;
	vector<SkinnedVertex> vertices;
	vector<Vertex> skinnedVertices;
	vector<unsigned int>    indices;
	vector<XMFLOAT3>		physicsverts;
	vector<unsigned int>	physicsindices;
	vector<int>				physicalmapGP;

	static thread_local vector<Instance>		instances;

	ID3D11Buffer* meshVertBuff;
	static ID3D11Buffer* meshInstanceBuffer;
	ID3D11Buffer* meshIndexBuff;
	ID3D11Buffer* boneBuffer;
	ID3D11Buffer* sOutBuffer;

	vector<string> materialNames;
	vector<int> materialIndices;
	vector<Material*> materials;
	bool renderable,doubleSided;
	STENCILREF stencilRef;
	//vector<int> usedBy;

	bool calculatedAO;

	int armatureIndex;
	Armature* armature;

	AABB aabb;

	//RIBBONTRAIL VERTS
	RibbonTrail trailInfo;

	bool isBillboarded;
	XMFLOAT3 billboardAxis;

	vector<VertexGroup> vertexGroups;
	bool softBody;
	float mass, friction;
	int massVG,goalVG,softVG; //vertexGroupID
	vector<XMFLOAT3> goalPositions,goalNormals;

	Mesh(){
		init();
	}
	Mesh(string newName){
		name=newName;
		init();
	}
	void CleanUp(){
		if(meshVertBuff) {meshVertBuff->Release(); meshVertBuff=NULL; }
		if(meshInstanceBuffer) {meshInstanceBuffer->Release(); meshInstanceBuffer=NULL;}
		if(meshIndexBuff) {meshIndexBuff->Release(); meshIndexBuff=NULL; }
		if(sOutBuffer) {sOutBuffer->Release(); sOutBuffer=NULL; }
		vertices.clear();
		skinnedVertices.clear();
		indices.clear();
		vertexGroups.clear();
		goalPositions.clear();
		goalNormals.clear();
	}
	void LoadFromFile(const string& newName, const string& fname
		, const MaterialCollection& materialColl, vector<Armature*> armatures, const string& identifier="");
	bool buffersComplete;
	void Optimize();
	void CreateBuffers();
	void CreateVertexArrays();
	static void AddRenderableInstance(const Instance& instance, int numerator);
	static void UpdateRenderableInstances(int count, ID3D11DeviceContext* context);
	void init(){
		parent="";
		vertices.resize(0);
		skinnedVertices.resize(0);
		indices.resize(0);
		meshVertBuff=NULL;
		meshInstanceBuffer=NULL;
		meshIndexBuff=NULL;
		boneBuffer=NULL;
		sOutBuffer=NULL;
		materialNames.resize(0);
		materialIndices.resize(0);
		renderable=false;
		doubleSided=false;
		stencilRef = STENCILREF::STENCILREF_DEFAULT;
		//usedBy.resize(0);
		aabb=AABB();
		trailInfo=RibbonTrail();
		armatureIndex=0;
		armature=NULL;
		isBillboarded=false;
		billboardAxis=XMFLOAT3(0,0,0);
		vertexGroups.resize(0);
		softBody=false;
		mass=friction=1;
		massVG=-1;
		goalVG=-1;
		softVG=-1;
		goalPositions.resize(0);
		goalNormals.resize(0);
		buffersComplete=false;
		calculatedAO = false;
	}
	
	bool hasArmature()const{return armature!=nullptr;}
};
struct Node
{
	string name;

	Node(){
		name="";
	}


	string GetID()
	{
		auto x = name.find_last_of('_');
		if (x != string::npos)
		{
			return name.substr(x + 1);
		}
		return "";
	}
};
struct Load_Debug_Properties
{
	string parentName;

	Load_Debug_Properties(){
		parentName="";
	}
};
struct Transform : public Node, public Load_Debug_Properties
{
	Transform* parent;
	set<Transform*> children;
	
	XMFLOAT3 translation_rest, translation, translationPrev; 
	XMFLOAT4 rotation_rest, rotation, rotationPrev;
	XMFLOAT3 scale_rest, scale, scalePrev;
	XMFLOAT4X4 world_rest, world, worldPrev;
	XMFLOAT4X4 *boneRelativity, *boneRelativityPrev; //for bones only (calculating efficiency)

	XMFLOAT4X4 parent_inv_rest;
	int copyParentT,copyParentR,copyParentS;

	Transform():Node(){
		Clear();
	}
	~Transform(){
		if(boneRelativity!=nullptr){
			delete boneRelativity;
			boneRelativity=nullptr;
		}
		if(boneRelativityPrev!=nullptr){
			delete boneRelativityPrev;
			boneRelativityPrev=nullptr;
		}
	};
	void Clear()
	{
		parent = nullptr;
		copyParentT = copyParentR = copyParentS = 1;
		boneRelativity = boneRelativityPrev = nullptr;
		translation_rest = translation = translationPrev = XMFLOAT3(0, 0, 0);
		scale_rest = scale = scalePrev = XMFLOAT3(1, 1, 1);
		rotation_rest = rotation = rotationPrev = XMFLOAT4(0, 0, 0, 1);
		world_rest = world = worldPrev = parent_inv_rest = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	}

	virtual XMMATRIX getTransform(int getTranslation = 1, int getRotation = 1, int getScale = 1);
	//attach to parent
	void attachTo(Transform* newParent, int copyTranslation=1, int copyRotation=1, int copyScale=1);
	//detach child - detach all if no parameters
	void detachChild(Transform* child = nullptr);
	//detach from parent
	void detach(int erasefromParent = 1);
	void applyTransform(int t=1, int r=1, int s=1);
	void transform(const XMFLOAT3& t = XMFLOAT3(0,0,0), const XMFLOAT4& r = XMFLOAT4(0,0,0,1), const XMFLOAT3& s = XMFLOAT3(1,1,1));
	void transform(const XMMATRIX& m = XMMatrixIdentity());
	void Translate(const XMFLOAT3& value);
	void RotateRollPitchYaw(const XMFLOAT3& value);
	void Scale(const XMFLOAT3& value);
};
struct Cullable
{
public:
	Cullable();
	AABB bounds;
	long lastSquaredDistMulThousand; //for early depth sort
	bool operator() (const Cullable* a, const Cullable* b) const{
		return a->lastSquaredDistMulThousand<b->lastSquaredDistMulThousand;
	}
};
struct Streamable : public Cullable
{
public:
	Streamable();
	string directory;
	string meshfile;
	string materialfile;
	bool loaded;
	Mesh* mesh;

	void StreamIn();
	void StreamOut();
};
struct Object : public Streamable, public Transform
{
	bool armatureDeform;

	//PARTICLE
	enum wiParticleEmitter{
		NO_EMITTER,
		EMITTER_VISIBLE,
		EMITTER_INVISIBLE,
	};
	wiParticleEmitter particleEmitter;
	vector< wiEmittedParticle* > eParticleSystems;
	vector< wiHairParticle* > hwiParticleSystems;

	float transparency;

	XMFLOAT3 color;

	
	//PHYSICS
	bool rigidBody, kinematic;
	string collisionShape, physicsType;
	float mass, friction, restitution, damping;
	
	
	//RIBBON TRAIL
	deque<RibbonVertex> trail;
	ID3D11Buffer* trailBuff;
	ID3D11ShaderResourceView* trailDistortTex;

	int physicsObjectI;

	bool isDynamic()
	{
		return ((physicsObjectI >= 0 && !kinematic) || mesh->hasArmature() || mesh->softBody);
	}

	Object(){};
	Object(string newName):Transform(){
		armatureDeform=false;
		name=newName;
		XMStoreFloat4x4( &world,XMMatrixIdentity() );
		XMStoreFloat4x4( &worldPrev,XMMatrixIdentity() );
		trail.resize(0);
		trailBuff=NULL;
		particleEmitter = NO_EMITTER;
		eParticleSystems.resize(0);
		hwiParticleSystems.resize(0);
		mesh=nullptr;
		rigidBody=kinematic=false;
		collisionShape="";
		mass = friction = restitution = damping = 1.0f;
		physicsType="ACTIVE";
		physicsObjectI=-1;
		transparency = 0.0f;
		color = XMFLOAT3(1, 1, 1);
		trailDistortTex = nullptr;
	}

	void CleanUp(){
		trail.clear();
		if(trailBuff) {trailBuff->Release(); trailBuff=NULL;}
	}
	void EmitTrail(const XMFLOAT3& color, float fadeSpeed = 0.06f);
	void FadeTrail();
};
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
};
struct Action
{
	string name;
	int frameCount;

	Action(){
		name="";
		frameCount=0;
	}
};
struct ActionFrames
{
	vector< KeyFrame > keyframesRot;
	vector< KeyFrame > keyframesPos;
	vector< KeyFrame > keyframesSca;

	ActionFrames(){
		keyframesRot.resize(0);
		keyframesPos.resize(0);
		keyframesSca.resize(0);
	}
	void CleanUp(){
		keyframesRot.clear();
		keyframesPos.clear();
		keyframesSca.clear();
	}
};
struct Bone : public Transform
{
	vector< string > childrenN;
	vector< Bone* > childrenI;

	XMFLOAT4X4 restInv;
	vector<ActionFrames> actionFrames;

	XMFLOAT4X4 recursivePose, recursiveRest, recursiveRestInv;

	float length;
	bool connected;

	Bone(){};
	Bone(string newName):Transform(){
		name=newName;
		childrenN.resize(0);
		childrenI.resize(0);
		actionFrames.resize(0);
		length=1.0f;
		boneRelativity=new XMFLOAT4X4();
		boneRelativityPrev=new XMFLOAT4X4();
		connected = false;
	}
	void CleanUp(){
		childrenN.clear();
		childrenI.clear();

		for(unsigned int i=0;i<actionFrames.size();i++)
			actionFrames[i].CleanUp();
		actionFrames.clear();
	}

	XMMATRIX getTransform(int getTranslation = 1, int getRotation = 1, int getScale = 1);
};
struct AnimationLayer
{
	string name;

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

	AnimationLayer()
	{
		name = "";

		activeAction = prevAction = 0;
		ResetAction();
		ResetActionPrev();

		playing = true;
		blendCurrentFrame = 0.0f;
		blendFrames = 0.0f;
		blendFact = 0.0f;
		currentFramePrevAction = 0.0f;
		weight = 1.0f;
		type = ANIMLAYER_TYPE_ADDITIVE;

		looped = true;
	}

	void ChangeAction(int actionIndex, float blendFrames = 0.0f, float weight = 1.0f)
	{
		currentFramePrevAction = currentFrame;
		ResetAction();
		prevAction = activeAction;
		activeAction = actionIndex;
		this->blendFrames = blendFrames;
		blendFact = 0.0f;
		blendCurrentFrame = 0.0f;
		this->weight = weight;
	}
	void ResetAction()
	{
		currentFrame = 1;
	}
	void ResetActionPrev()
	{
		currentFramePrevAction = 1;
	}
	void PauseAction()
	{
		playing = false;
	}
	void StopAction()
	{
		ResetAction();
		PauseAction();
	}
	void PlayAction()
	{
		playing = true;
	}
};
struct Armature : public Transform
{
public:
	string unidentified_name;
	vector<Bone*> boneCollection;
	vector<Bone*> rootbones;

	list<AnimationLayer*> animationLayers;
	vector<Action> actions;
	

	Armature() = delete;
	Armature(string newName,string identifier):Transform(){
		unidentified_name=newName;
		stringstream ss("");
		ss<<newName<<identifier;
		name=ss.str();
		boneCollection.resize(0);
		rootbones.resize(0);
		actions.resize(0);
		XMStoreFloat4x4(&world,XMMatrixIdentity());
		animationLayers.clear();
		animationLayers.push_back(new AnimationLayer());
		animationLayers.back()->type = AnimationLayer::ANIMLAYER_TYPE_PRIMARY;
	}
	void CleanUp(){
		actions.clear();
		for (Bone* b : boneCollection)
		{
			b->CleanUp();
			delete b;
		}
		boneCollection.clear();
		rootbones.clear();
		for (auto& x : animationLayers)
		{
			delete x;
		}
		animationLayers.clear();
	}

	inline AnimationLayer* GetPrimaryAnimation() { 
		return *animationLayers.begin(); 
	}

	bool ChangeAction(const string& actionName, float blendFrames = 0.0f, const string& animLayer = "")
	{
		AnimationLayer* anim = GetPrimaryAnimation();
		if (animLayer.length() > 0)
		{
			anim = GetAnimLayer(animLayer);
		}

		int i = 0;
		for (Action& x : actions)
		{
			if (!x.name.compare(actionName))
			{
				anim->ChangeAction(i, blendFrames);
				return true;
			}
			i++;
		}
		return false;
	}
	AnimationLayer* GetAnimLayer(const string& name)
	{
		for (auto& x : animationLayers)
		{
			if (!x->name.compare(name))
			{
				return x;
			}
		}
		animationLayers.push_back(new AnimationLayer);
		animationLayers.back()->name = name;
		return animationLayers.back();
	}
	void AddAnimLayer(const string& name)
	{
		for (auto& x : animationLayers)
		{
			if (!x->name.compare(name))
			{
				return;
			}
		}
		animationLayers.push_back(new AnimationLayer);
		animationLayers.back()->name = name;
	}
	void DeleteAnimLayer(const string& name)
	{
		auto i = animationLayers.begin();
		while (i != animationLayers.end())
		{
			if ((*i)->type != AnimationLayer::ANIMLAYER_TYPE_PRIMARY && !(*i)->name.compare(name))
			{
				animationLayers.erase(i++);
			}
			else
			{
				++i;
			}
		}
	}
};
struct SHCAM{	
	XMFLOAT4X4 View,Projection;
	XMFLOAT3 Eye,At,Up;
	float nearplane,farplane,size;

	SHCAM(){
		Init(XMQuaternionIdentity());
		Create_Perspective(XM_PI/2.0f);
		nearplane=0.1f;farplane=200,size=0;
	}
	//orthographic
	SHCAM(float size, const XMVECTOR& dir, float newNear, float newFar){
		nearplane=newNear;
		farplane=newFar;
		Init(dir);
		Create_Ortho(size);
	};
	//perspective
	SHCAM(const XMFLOAT4& dir, float newNear, float newFar, float newFov){
		nearplane=newNear;
		farplane=newFar;
		Init(XMLoadFloat4(&dir));
		Create_Perspective(newFov);
	};
	void Init(const XMVECTOR& dir){
		XMMATRIX rot = XMMatrixIdentity();
		rot = XMMatrixRotationQuaternion( dir );
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
		XMMATRIX rProjection = XMMatrixOrthographicLH(size,size,-farplane,farplane);
		XMStoreFloat4x4( &Projection, rProjection);
		this->size=size;
	}
	void Create_Perspective(float fov){
		XMMATRIX rProjection = XMMatrixPerspectiveFovLH(fov,1,nearplane,farplane);
		XMStoreFloat4x4( &Projection, rProjection);
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
	XMMATRIX getVP(){
		return XMMatrixTranspose(XMLoadFloat4x4(&View)*XMLoadFloat4x4(&Projection));
	}
};
struct Light : public Cullable , public Transform
{
	XMFLOAT4 color;
	XMFLOAT4 enerDis;
	bool noHalo;
	bool shadow;
	vector<ID3D11ShaderResourceView*> lensFlareRimTextures;
	vector<string> lensFlareNames;

	//vector<wiRenderTarget> shadowMap;
	static vector<wiRenderTarget> shadowMaps_pointLight;
	static vector<wiRenderTarget> shadowMaps_spotLight;
	vector<wiRenderTarget> shadowMaps_dirLight;
	int shadowMap_index;

	vector<SHCAM> shadowCam;

	float shadowBias;

	enum LightType{
		DIRECTIONAL,
		POINT,
		SPOT,
	};
	LightType type;

	Light():Transform(){
		color=XMFLOAT4(0,0,0,0);
		enerDis=XMFLOAT4(0,0,0,0);
		type=LightType::POINT;
		//shadowMap.resize(0);
		shadow = false;
		shadowCam.resize(0);
		noHalo=false;
		lensFlareRimTextures.resize(0);
		lensFlareNames.resize(0);
		shadowMap_index = -1;
		shadowBias = 0.00001f;
	}
	void CleanUp();
};
struct Decal : public Cullable, public Transform
{
	string texName,norName;
	ID3D11ShaderResourceView* texture,*normal;
	XMFLOAT4X4 view,projection;
	XMFLOAT3 front;
	float life,fadeStart;

	Decal(const XMFLOAT3& tra=XMFLOAT3(0,0,0), const XMFLOAT3& sca=XMFLOAT3(1,1,1), const XMFLOAT4& rot=XMFLOAT4(0,0,0,1), const string& tex="", const string& nor="");
	void Update();
	void addTexture(const string& tex);
	void addNormal(const string& nor);
	void CleanUp();
};
struct WorldInfo{
	XMFLOAT3 horizon;
	XMFLOAT3 zenith;
	XMFLOAT3 ambient;
	XMFLOAT3 fogSEH;

	WorldInfo(){
		horizon=zenith=ambient=XMFLOAT3(0,0,0);
		fogSEH=XMFLOAT3(100,1000,0);
	}
};
struct Wind{
	XMFLOAT3 direction;
	float time;
	float randomness;
	float waveSize;
	Wind():direction(XMFLOAT3(0,0,0)),time(0),randomness(5),waveSize(1){}
};
struct Camera:public Transform{
	XMFLOAT4X4 View, Projection;
	XMFLOAT3 At, Up;
	float width, height;
	float zNearP, zFarP;
	Frustum frustum;
	
	Camera():Transform(){
	}
	Camera(const XMFLOAT3& newPos, const XMFLOAT4& newRot,
		const string& newName):Transform()
	{
		translation_rest=newPos;
		rotation_rest=newRot;
		name=newName;
		getTransform();
	}
	void SetUp(int newWidth, int newHeight, float newNear, float newFar)
	{
		XMMATRIX View, Projection;
		XMVECTOR At = XMVectorSet(0,0,1,0), Up = XMVectorSet(0,1,0,0), Eye = this->GetEye();

		zNearP = newNear;
		zFarP = newFar;

		width = (float)newWidth;
		height = (float)newHeight;


		Projection = XMMatrixPerspectiveFovLH(XM_PI / 3.0f, (float)width / (float)height, zNearP, zFarP);


		XMStoreFloat4x4(&this->View, View);
		XMStoreFloat4x4(&this->Projection, Projection);
		XMStoreFloat3(&this->At, At);
		XMStoreFloat3(&this->Up, Up);

		Update();
	}
	void Update()
	{
		getTransform();

		UpdateProps();
	}
	void UpdateProps()
	{
		XMMATRIX View;
		XMVECTOR At = XMVectorSet(0, 0, 1, 0), Up = XMVectorSet(0, 1, 0, 0), Eye = this->GetEye();

		XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
		At = XMVector3Transform(At, camRot);
		Up = XMVector3Transform(Up, camRot);

		View = XMMatrixLookToLH(Eye, At, Up);

		XMStoreFloat4x4(&this->View, View);
		XMStoreFloat3(&this->At, At);
		XMStoreFloat3(&this->Up, Up);

		frustum.ConstructFrustum(zFarP, Projection, this->View);
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
		At = XMVector3Transform(At, camRot);
		Up = XMVector3Transform(Up, camRot);

		At = XMVector3Transform(At, reflectMatrix);
		Up = XMVector3Transform(Up, reflectMatrix);
		Eye = XMVectorSetW(Eye, 1);
		Eye = XMVector4Transform(Eye, reflectMatrix);

		View = XMMatrixLookToLH(Eye, At, Up);

		XMStoreFloat4x4(&this->View, View);
		XMStoreFloat3(&this->At, At);
		XMStoreFloat3(&this->Up, Up);

		frustum.ConstructFrustum(zFarP, Projection, this->View);
	}

	XMVECTOR GetEye()
	{
		return XMLoadFloat3(&translation);
	}
	XMVECTOR GetAt()
	{
		return XMLoadFloat3(&At);
	}
	XMVECTOR GetUp()
	{
		return XMLoadFloat3(&Up);
	}
	XMMATRIX GetView()
	{
		return XMLoadFloat4x4(&View);
	}
	XMMATRIX GetProjection()
	{
		return XMLoadFloat4x4(&Projection);
	}
	XMMATRIX GetViewProjection()
	{
		return XMMatrixMultiply(GetView(),GetProjection());
	}
};
struct HitSphere:public SPHERE, public Transform{
	float radius_saved, radius;
	static ID3D11Buffer *vertexBuffer;
	enum HitSphereType{
		HITTYPE,
		ATKTYPE,
		INVTYPE,
	};
	HitSphereType TYPE_SAVED,TYPE;

	HitSphere():Transform(){}
	HitSphere(string newName, float newRadius, const XMFLOAT3& location, 
		Transform* newParent, string newProperty):Transform(){

			name=newName;
			radius_saved=newRadius;
			translation_rest=location;
			if(newParent!=nullptr){
				parentName=newParent->name;
				parent=newParent;
			}
			TYPE=HitSphereType::HITTYPE;
			if(newProperty.find("inv")!=string::npos) TYPE=HitSphereType::INVTYPE;
			else if(newProperty.find("atk")!=string::npos) TYPE=HitSphereType::ATKTYPE;
			TYPE_SAVED=TYPE;
	}
	void Reset(){
		TYPE=TYPE_SAVED;
		radius=radius_saved;
	}

static const int RESOLUTION = 36;
	static void SetUpStatic();
	static void CleanUpStatic();

};



// Create rest positions for bone tree
void RecursiveRest(Armature* armature, Bone* bone);

void LoadWiArmatures(const string& directory, const string& filename, const string& identifier, vector<Armature*>& armatures
					 , map<string,Transform*>& transforms);
void LoadWiMaterialLibrary(const string& directory, const string& filename, const string& identifier, const string& texturesDir, MaterialCollection& materials);
void LoadWiObjects(const string& directory, const string& filename, const string& identifier, vector<Object*>& objects_norm
				   , vector<Object*>& objects_trans, vector<Object*>& objects_water, vector<Armature*>& armatures, MeshCollection& meshes
				   , map<string,Transform*>& transforms, const MaterialCollection& materials);
void LoadWiMeshes(const string& directory, const string& filename, const string& identifier, MeshCollection& meshes, const vector<Armature*>& armatures, const MaterialCollection& materials);
void LoadWiActions(const string& directory, const string& filename, const string& identifier, vector<Armature*>& armatures);
void LoadWiLights(const string& directory, const string& filename, const string& identifier, vector<Light*>& lights
				  , const vector<Armature*>& armatures
				  , map<string,Transform*>& transforms);
void LoadWiHitSpheres(const string& directory, const string& name, const string& identifier, vector<HitSphere*>& spheres
					  ,const vector<Armature*>& armatures, map<string,Transform*>& transforms);
void LoadWiWorldInfo(const string&directory, const string& name, WorldInfo& worldInfo, Wind& wind);
void LoadWiCameras(const string&directory, const string& name, const string& identifier, vector<Camera>& cameras
				   ,const vector<Armature*>& armatures, map<string,Transform*>& transforms);
void LoadWiDecals(const string&directory, const string& name, const string& texturesDir, list<Decal*>& decals);

void LoadFromDisk(const string& dir, const string& name, const string& identifier
				  , vector<Armature*>& armatures
				  , MaterialCollection& materials
				  , vector<Object*>& objects_norm, vector<Object*>& objects_trans, vector<Object*>& objects_water
				  , MeshCollection& meshes
				  , vector<Light*>& lights
				  , vector<HitSphere*>& spheres
				  , WorldInfo& worldInfo, Wind& wind
				  , vector<Camera>& cameras
				  , vector<Armature*>& l_armatures
				  , vector<Object*>& l_objects
				  , map<string,Transform*>& transforms
				  , list<Decal*>& decals
				  );



#include "wiSPTree.h"
class wiSPTree;
#define SPTREE_GENERATE_QUADTREE 0
#define SPTREE_GENERATE_OCTREE 1
void GenerateSPTree(wiSPTree*& tree, vector<Cullable*>& objects, int type = SPTREE_GENERATE_QUADTREE);
