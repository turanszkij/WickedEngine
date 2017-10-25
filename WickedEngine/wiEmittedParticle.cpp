#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiLoader.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"

using namespace std;
using namespace wiGraphicsTypes;

VertexShader  *wiEmittedParticle::vertexShader = nullptr;
PixelShader   *wiEmittedParticle::pixelShader = nullptr,*wiEmittedParticle::simplestPS = nullptr;
GPURingBuffer *wiEmittedParticle::dynamicPool = nullptr;
GPUBuffer           *wiEmittedParticle::constantBuffer = nullptr;
BlendState		*wiEmittedParticle::blendStateAlpha = nullptr,*wiEmittedParticle::blendStateAdd = nullptr;
RasterizerState		*wiEmittedParticle::rasterizerState = nullptr,*wiEmittedParticle::wireFrameRS = nullptr;
DepthStencilState	*wiEmittedParticle::depthStencilState = nullptr;

#define MAX_PARTICLES 10000

static const int NUM_POS_SAMPLES = 30;
static const float INV_NUM_POS_SAMPLES = 1.0f / NUM_POS_SAMPLES;

wiEmittedParticle::wiEmittedParticle()
{
	name = "";
	object = nullptr;
	materialName = "";
	light = nullptr;
	lightName = "";
	particleBufferOffset = 0;
}
wiEmittedParticle::wiEmittedParticle(const std::string& newName, const std::string& newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot)
{
	name=newName;
	object=newObject;
	materialName = newMat;
	for (MeshSubset& subset : object->mesh->subsets)
	{
		if (!newMat.compare(subset.material->name)) {
			material = subset.material;
			break;
		}
	}

	size=newSize;
	random_factor=newRandomFac;
	normal_factor=newNormalFac;

	count=newCount;
	points.resize(0);
	life=newLife;
	random_life=newRandLife;
	emit=0;
	
	scaleX=newScaleX;
	scaleY=newScaleY;
	rotation = newRot;

	light=nullptr;
	CreateLight();

	XMFLOAT4X4 transform = XMFLOAT4X4(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
	transform4 = transform;
	transform3 = XMFLOAT3X3(
		transform._11,transform._12,transform._13
		,transform._21,transform._22,transform._23
		,transform._31,transform._32,transform._33
		);

	motionBlurAmount = 0.0f;
	particleBufferOffset = 0;

	SetupLightInterpolators();
}
int wiEmittedParticle::getCount(){return (int)points.size();}

void wiEmittedParticle::SetupLightInterpolators()
{
	// for smooth light interpolation
	currentSample = 0;
	posSamples = new XMFLOAT3[NUM_POS_SAMPLES];
	radSamples = new float[NUM_POS_SAMPLES];
	energySamples = new float[NUM_POS_SAMPLES];
	for (int i = 0; i < NUM_POS_SAMPLES; ++i)
	{
		radSamples[i] = 0.0f;
		energySamples[i] = 0.0f;
		posSamples[i] = XMFLOAT3(0, 0, 0);
	}
}

int wiEmittedParticle::getRandomPointOnEmitter(){ return wiRandom::getRandom((int)object->mesh->indices.size()-1); }

void wiEmittedParticle::CreateLight()
{
	if (light == nullptr && material->blendFlag == BLENDMODE_ADDITIVE)
	{
		light = new Light();
		light->color.x = material->baseColor.x;
		light->color.y = material->baseColor.y;
		light->color.z = material->baseColor.z;
		light->SetType(Light::POINT);
		light->name = name + "_pslight";
		light->shadow = true;
		light->enerDis = XMFLOAT4(0, 0, 0, 0); // will be filled on Update()
		lightName = light->name;
	}
}

void wiEmittedParticle::addPoint(const XMMATRIX& t4, const XMMATRIX& t3)
{
	int gen[3];
	gen[0] = getRandomPointOnEmitter();
	switch(gen[0]%3)
	{
	case 0:
		gen[1]=gen[0]+1;
		gen[2]=gen[0]+2;
		break;
	case 1:
		gen[0]=gen[0]-1;
		gen[1]=gen[0]+1;
		gen[2]=gen[0]+2;
		break;
	case 2:
		gen[0]=gen[0]-2;
		gen[1]=gen[0]+1;
		gen[2]=gen[0]+2;
		break;
	default:
		break;
	}
	float f = wiRandom::getRandom(0, 1000) * 0.001f, g = wiRandom::getRandom(0, 1000) * 0.001f;
	if (f + g > 1)
	{
		f = 1 - f;
		g = 1 - g;
	}

	XMFLOAT3 pos;
	XMFLOAT3 vel;
	XMVECTOR& vbar = XMVectorBaryCentric(
		object->mesh->vertices_POS[object->mesh->indices[gen[0]]].Load(),
		object->mesh->vertices_POS[object->mesh->indices[gen[1]]].Load(),
		object->mesh->vertices_POS[object->mesh->indices[gen[2]]].Load(),
		f,
		g);
	XMVECTOR& nbar = XMVectorBaryCentric(
		XMLoadFloat4(&object->mesh->vertices_NOR[object->mesh->indices[gen[0]]].GetNor_FULL()),
		XMLoadFloat4(&object->mesh->vertices_NOR[object->mesh->indices[gen[1]]].GetNor_FULL()),
		XMLoadFloat4(&object->mesh->vertices_NOR[object->mesh->indices[gen[2]]].GetNor_FULL()),
		f,
		g);
	XMStoreFloat3( &pos, XMVector3Transform( vbar, t4 ) );
	XMStoreFloat3( &vel, XMVector3Normalize( XMVector3Transform( nbar, t3 ) ));
			
	float vrand = (normal_factor*getNewVelocityModifier())/60.0f;

	vel.x*=vrand;
	vel.y*=vrand;
	vel.z*=vrand;


	points.push_back(Point(pos, XMFLOAT4(size, 1, (float)wiRandom::getRandom(0, 1), (float)wiRandom::getRandom(0, 1)), vel/*, XMFLOAT3(1,1,1)*/, getNewLifeSpan()
		,rotation*getNewRotationModifier(),scaleX,scaleY ) );
}
void wiEmittedParticle::Update(float dt)
{
	float gamespeed = wiRenderer::GetGameSpeed() * dt * 60; // it was created for 60 FPS in mind...

	XMFLOAT3 minP=XMFLOAT3(FLOAT32_MAX,FLOAT32_MAX,FLOAT32_MAX)
		,maxP=XMFLOAT3(-FLOAT32_MAX,-FLOAT32_MAX,-FLOAT32_MAX);

	for (unsigned int i = 0; i<points.size(); ++i){
		Point &point = points[i];

		point.pos.x += point.vel.x*gamespeed;
		point.pos.y += point.vel.y*gamespeed;
		point.pos.z += point.vel.z*gamespeed;
		point.rot += point.rotVel*gamespeed;

		point.life -= gamespeed;
		point.life=wiMath::Clamp(point.life,0,point.maxLife);

		float lifeLerp = 1 - point.life/point.maxLife;
		point.sizOpaMir.x=wiMath::Lerp(point.sizBeginEnd.x,point.sizBeginEnd.y,lifeLerp);
		point.sizOpaMir.y=wiMath::Lerp(1,0,lifeLerp);

		
		minP=wiMath::Min(XMFLOAT3(point.pos.x-point.sizOpaMir.x,point.pos.y-point.sizOpaMir.x,point.pos.z-point.sizOpaMir.x),minP);
		maxP=wiMath::Max(XMFLOAT3(point.pos.x+point.sizOpaMir.x,point.pos.y+point.sizOpaMir.x,point.pos.z+point.sizOpaMir.x),maxP);
	}
	bounding_box.create(minP,maxP);

	while(!points.empty() && points.front().life<=0)
		points.pop_front();


	XMFLOAT4X4& transform = object->world;
	transform4 = transform;
	transform3 = XMFLOAT3X3(
		transform._11,transform._12,transform._13
		,transform._21,transform._22,transform._23
		,transform._31,transform._32,transform._33
		);
	XMMATRIX t4=XMLoadFloat4x4(&transform4), t3=XMLoadFloat3x3(&transform3);
	
	emit += (float)count/60.0f*gamespeed;

	bool clearSpace=false;
	if(points.size()+emit>=MAX_PARTICLES)
		clearSpace=true;

	for(int i=0;i<(int)emit;++i)
	{
		if(clearSpace)
			points.pop_front();

		addPoint(t4,t3);
	}
	if((int)emit>0)
		emit=0;
	
}
void wiEmittedParticle::Burst(float num)
{
	XMMATRIX t4=XMLoadFloat4x4(&transform4), t3=XMLoadFloat3x3(&transform3);

	static float burst = 0;
	burst+=num;
	for(int i=0;i<(int)burst;++i)
		addPoint(t4,t3);
	burst-=(int)burst;
}

void wiEmittedParticle::UpdateAttachedLight(float dt)
{
	if (light != nullptr)
	{
		// smooth light position to eliminate jitter:

		posSamples[currentSample] = bounding_box.getCenter();
		radSamples[currentSample] = bounding_box.getRadius() * 2;
		energySamples[currentSample] = sqrt((float)points.size());

		XMFLOAT3 pos = XMFLOAT3(0, 0, 0);
		float rad = 0.0f;
		float energy = 0.0f;
		for (int i = 0; i < NUM_POS_SAMPLES; ++i)
		{
			pos.x += posSamples[i].x;
			pos.y += posSamples[i].y;
			pos.z += posSamples[i].z;
			rad += radSamples[i];
			energy += energySamples[i];
		}
		pos.x *= INV_NUM_POS_SAMPLES;
		pos.y *= INV_NUM_POS_SAMPLES;
		pos.z *= INV_NUM_POS_SAMPLES;
		rad *= INV_NUM_POS_SAMPLES;
		energy *= INV_NUM_POS_SAMPLES;
		currentSample = (currentSample + 1) % NUM_POS_SAMPLES;

		light->translation_rest = pos;

		light->enerDis = XMFLOAT4(energy, rad, 0, 0);
		light->UpdateLight();
	}
}

void wiEmittedParticle::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	if (!points.empty())
	{
		static std::vector<Point> renderPoints[GRAPHICSTHREAD_COUNT];
		if (renderPoints[threadID].size() < points.size())
		{
			renderPoints[threadID].resize((points.size() + 1) * 2);
		}
		renderPoints[threadID].insert(renderPoints[threadID].begin(), points.begin(), points.end());
		particleBufferOffset = wiRenderer::GetDevice()->AppendRingBuffer(dynamicPool, renderPoints[threadID].data(), sizeof(Point)* renderPoints[threadID].size(), threadID);
	}
}

void wiEmittedParticle::Draw(GRAPHICSTHREAD threadID)
{
	if(!points.empty())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("EmittedParticle", threadID);

		bool additive = (material->blendFlag==BLENDMODE_ADDITIVE || material->premultipliedTexture);

		device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,threadID);
		device->BindVertexLayout(nullptr, threadID);
		device->BindPS(wiRenderer::IsWireRender() ? simplestPS : pixelShader, threadID);
		device->BindVS(vertexShader,threadID);

		ConstantBuffer cb;
		cb.mAdd.x = additive ? 1.0f : 0.0f;
		cb.mAdd.y = 0.0f;
		cb.mMotionBlurAmount = motionBlurAmount;
		cb.mBufferOffset = particleBufferOffset / sizeof(Point);
		

		device->UpdateBuffer(constantBuffer,&cb,threadID);
		device->BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(ConstantBuffer),threadID);

		device->BindRasterizerState(wiRenderer::IsWireRender() ? wireFrameRS : rasterizerState, threadID);
		device->BindDepthStencilState(depthStencilState, 1, threadID);
	
		device->BindBlendState((additive ? blendStateAdd : blendStateAlpha), threadID);

		device->BindResourceVS(dynamicPool, 0, threadID);

		if (!wiRenderer::IsWireRender() && material->texture)
		{
			device->BindResourcePS(material->texture, TEXSLOT_ONDEMAND0, threadID);
		}

		device->Draw((int)points.size() * 6,threadID);


		device->EventEnd(threadID);
	}
}


void wiEmittedParticle::CleanUp()
{

	points.clear();

	SAFE_DELETE_ARRAY(posSamples);
	SAFE_DELETE_ARRAY(radSamples);
	SAFE_DELETE_ARRAY(energySamples);
}



void wiEmittedParticle::LoadShaders()
{
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticleVS.cso", wiResourceManager::VERTEXSHADER));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
	}


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS.cso", wiResourceManager::PIXELSHADER));
	simplestPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));



}
void wiEmittedParticle::LoadBuffers()
{
	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	constantBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, constantBuffer);

	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Point) * 1024 * 1024;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(Point);
	dynamicPool = new GPURingBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, dynamicPool);
}
void wiEmittedParticle::SetUpStates()
{
	RasterizerStateDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	rasterizerState = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,rasterizerState);

	
	rs.FillMode=FILL_WIREFRAME;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	wireFrameRS = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,wireFrameRS);




	
	DepthStencilStateDesc dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER;

	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_INCR;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_DECR;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;

	// Create the depth stencil state.
	depthStencilState = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilState);


	
	BlendStateDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable=false;
	blendStateAlpha = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateAlpha);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable=false;
	blendStateAdd = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendStateAdd);
}
void wiEmittedParticle::SetUpStatic()
{
	LoadShaders();
	LoadBuffers();
	SetUpStates();
}
void wiEmittedParticle::CleanUpStatic()
{
	SAFE_DELETE(vertexShader);
	SAFE_DELETE(pixelShader);
	SAFE_DELETE(simplestPS);
	SAFE_DELETE(constantBuffer);
	SAFE_DELETE(blendStateAlpha);
	SAFE_DELETE(blendStateAdd);
	SAFE_DELETE(rasterizerState);
	SAFE_DELETE(wireFrameRS);
	SAFE_DELETE(depthStencilState);
}

void wiEmittedParticle::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> emit;
		archive >> transform4;
		archive >> transform3;
		archive >> name;
		archive >> materialName;
		archive >> size;
		archive >> random_factor;
		archive >> normal_factor;
		archive >> count;
		archive >> life;
		archive >> random_life;
		archive >> scaleX;
		archive >> scaleY;
		archive >> rotation;
		archive >> motionBlurAmount;
		archive >> lightName;

		SetupLightInterpolators();
	}
	else
	{
		archive << emit;
		archive << transform4;
		archive << transform3;
		archive << name;
		archive << materialName;
		archive << size;
		archive << random_factor;
		archive << normal_factor;
		archive << count;
		archive << life;
		archive << random_life;
		archive << scaleX;
		archive << scaleY;
		archive << rotation;
		archive << motionBlurAmount;
		archive << lightName;
	}
}
