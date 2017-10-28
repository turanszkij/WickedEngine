#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiLoader.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "wiTextureHelper.h"

using namespace std;
using namespace wiGraphicsTypes;

VertexShader  *wiEmittedParticle::vertexShader = nullptr;
PixelShader   *wiEmittedParticle::pixelShader = nullptr, *wiEmittedParticle::simplestPS = nullptr;
ComputeShader   *wiEmittedParticle::kickoffUpdateCS, *wiEmittedParticle::emitCS = nullptr, *wiEmittedParticle::simulateargsCS = nullptr, *wiEmittedParticle::drawargsCS = nullptr, *wiEmittedParticle::simulateCS = nullptr;
BlendState		*wiEmittedParticle::blendStateAlpha = nullptr,*wiEmittedParticle::blendStateAdd = nullptr;
RasterizerState		*wiEmittedParticle::rasterizerState = nullptr,*wiEmittedParticle::wireFrameRS = nullptr;
DepthStencilState	*wiEmittedParticle::depthStencilState = nullptr;

#define MAX_PARTICLES 10000

//static const int NUM_POS_SAMPLES = 30;
//static const float INV_NUM_POS_SAMPLES = 1.0f / NUM_POS_SAMPLES;

wiEmittedParticle::wiEmittedParticle()
{
	name = "";
	object = nullptr;
	materialName = "";
	light = nullptr;
	lightName = "";

	CreateSelfBuffers();
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
	//points.resize(0);
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

	SetupLightInterpolators();

	CreateSelfBuffers();
}
void wiEmittedParticle::CreateSelfBuffers()
{


	particleBuffer = new GPUBuffer;
	aliveList[0] = new GPUBuffer;
	aliveList[1] = new GPUBuffer;
	deadList = new GPUBuffer;
	counterBuffer = new GPUBuffer;
	indirectDispatchBuffer = new GPUBuffer;
	indirectDrawBuffer = new GPUBuffer;
	constantBuffer = new GPUBuffer;

	GPUBufferDesc bd;
	bd.Usage = USAGE_DEFAULT;
	bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	SubresourceData data;


	bd.ByteWidth = sizeof(Particle) * MAX_PARTICLES;
	bd.StructureByteStride = sizeof(Particle);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, particleBuffer);

	bd.ByteWidth = sizeof(uint32_t) * MAX_PARTICLES;
	bd.StructureByteStride = sizeof(uint32_t);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, aliveList[0]);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, aliveList[1]);

	uint32_t* indices = new uint32_t[MAX_PARTICLES];
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		indices[i] = i;
	}
	data.pSysMem = indices;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, deadList);
	SAFE_DELETE_ARRAY(indices);
	data.pSysMem = nullptr;


	uint32_t counters[] = { 0, MAX_PARTICLES, 0, 0 }; // aliveCount, deadCount, newAliveCount, realEmitCount
	data.pSysMem = counters;
	bd.ByteWidth = sizeof(counters);
	bd.StructureByteStride = sizeof(counters);
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, counterBuffer);
	data.pSysMem = nullptr;


	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_DRAWINDIRECT_ARGS;
	bd.ByteWidth = sizeof(wiGraphicsTypes::IndirectDrawArgsInstanced);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, indirectDrawBuffer);
	bd.ByteWidth = sizeof(wiGraphicsTypes::IndirectDispatchArgs);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, indirectDispatchBuffer);


	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(EmittedParticleCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, constantBuffer);
}

void wiEmittedParticle::SetupLightInterpolators()
{
	//// for smooth light interpolation
	//currentSample = 0;
	//posSamples = new XMFLOAT3[NUM_POS_SAMPLES];
	//radSamples = new float[NUM_POS_SAMPLES];
	//energySamples = new float[NUM_POS_SAMPLES];
	//for (int i = 0; i < NUM_POS_SAMPLES; ++i)
	//{
	//	radSamples[i] = 0.0f;
	//	energySamples[i] = 0.0f;
	//	posSamples[i] = XMFLOAT3(0, 0, 0);
	//}
}

int wiEmittedParticle::getRandomPointOnEmitter(){ return wiRandom::getRandom((int)object->mesh->indices.size()-1); }

void wiEmittedParticle::CreateLight()
{
	//if (light == nullptr && material->blendFlag == BLENDMODE_ADDITIVE)
	//{
	//	light = new Light();
	//	light->color.x = material->baseColor.x;
	//	light->color.y = material->baseColor.y;
	//	light->color.z = material->baseColor.z;
	//	light->SetType(Light::POINT);
	//	light->name = name + "_pslight";
	//	light->shadow = true;
	//	light->enerDis = XMFLOAT4(0, 0, 0, 0); // will be filled on Update()
	//	lightName = light->name;
	//}
}

void wiEmittedParticle::addPoint(const XMMATRIX& t4, const XMMATRIX& t3)
{
	//int gen[3];
	//gen[0] = getRandomPointOnEmitter();
	//switch(gen[0]%3)
	//{
	//case 0:
	//	gen[1]=gen[0]+1;
	//	gen[2]=gen[0]+2;
	//	break;
	//case 1:
	//	gen[0]=gen[0]-1;
	//	gen[1]=gen[0]+1;
	//	gen[2]=gen[0]+2;
	//	break;
	//case 2:
	//	gen[0]=gen[0]-2;
	//	gen[1]=gen[0]+1;
	//	gen[2]=gen[0]+2;
	//	break;
	//default:
	//	break;
	//}
	//float f = wiRandom::getRandom(0, 1000) * 0.001f, g = wiRandom::getRandom(0, 1000) * 0.001f;
	//if (f + g > 1)
	//{
	//	f = 1 - f;
	//	g = 1 - g;
	//}

	//XMFLOAT3 pos;
	//XMFLOAT3 vel;
	//XMVECTOR& vbar = XMVectorBaryCentric(
	//	object->mesh->vertices_POS[object->mesh->indices[gen[0]]].Load(),
	//	object->mesh->vertices_POS[object->mesh->indices[gen[1]]].Load(),
	//	object->mesh->vertices_POS[object->mesh->indices[gen[2]]].Load(),
	//	f,
	//	g);
	//XMVECTOR& nbar = XMVectorBaryCentric(
	//	XMLoadFloat4(&object->mesh->vertices_NOR[object->mesh->indices[gen[0]]].GetNor_FULL()),
	//	XMLoadFloat4(&object->mesh->vertices_NOR[object->mesh->indices[gen[1]]].GetNor_FULL()),
	//	XMLoadFloat4(&object->mesh->vertices_NOR[object->mesh->indices[gen[2]]].GetNor_FULL()),
	//	f,
	//	g);
	//XMStoreFloat3( &pos, XMVector3Transform( vbar, t4 ) );
	//XMStoreFloat3( &vel, XMVector3Normalize( XMVector3Transform( nbar, t3 ) ));
	//		
	//float vrand = (normal_factor*getNewVelocityModifier())/60.0f;

	//vel.x*=vrand;
	//vel.y*=vrand;
	//vel.z*=vrand;


	//points.push_back(Point(pos, XMFLOAT4(size, 1, (float)wiRandom::getRandom(0, 1), (float)wiRandom::getRandom(0, 1)), vel/*, XMFLOAT3(1,1,1)*/, getNewLifeSpan()
	//	,rotation*getNewRotationModifier(),scaleX,scaleY ) );
}
void wiEmittedParticle::Update(float dt)
{
	float gamespeed = wiRenderer::GetGameSpeed() * dt * 60; // it was created for 60 FPS in mind...

	//XMFLOAT3 minP=XMFLOAT3(FLOAT32_MAX,FLOAT32_MAX,FLOAT32_MAX)
	//	,maxP=XMFLOAT3(-FLOAT32_MAX,-FLOAT32_MAX,-FLOAT32_MAX);

	//for (unsigned int i = 0; i<points.size(); ++i){
	//	Point &point = points[i];

	//	point.pos.x += point.vel.x*gamespeed;
	//	point.pos.y += point.vel.y*gamespeed;
	//	point.pos.z += point.vel.z*gamespeed;
	//	point.rot += point.rotVel*gamespeed;

	//	point.life -= gamespeed;
	//	point.life=wiMath::Clamp(point.life,0,point.maxLife);

	//	float lifeLerp = 1 - point.life/point.maxLife;
	//	point.sizOpaMir.x=wiMath::Lerp(point.sizBeginEnd.x,point.sizBeginEnd.y,lifeLerp);
	//	point.sizOpaMir.y=wiMath::Lerp(1,0,lifeLerp);

	//	
	//	minP=wiMath::Min(XMFLOAT3(point.pos.x-point.sizOpaMir.x,point.pos.y-point.sizOpaMir.x,point.pos.z-point.sizOpaMir.x),minP);
	//	maxP=wiMath::Max(XMFLOAT3(point.pos.x+point.sizOpaMir.x,point.pos.y+point.sizOpaMir.x,point.pos.z+point.sizOpaMir.x),maxP);
	//}
	//bounding_box.create(minP,maxP);

	//while(!points.empty() && points.front().life<=0)
	//	points.pop_front();


	//XMFLOAT4X4& transform = object->world;
	//transform4 = transform;
	//transform3 = XMFLOAT3X3(
	//	transform._11,transform._12,transform._13
	//	,transform._21,transform._22,transform._23
	//	,transform._31,transform._32,transform._33
	//	);
	//XMMATRIX t4=XMLoadFloat4x4(&transform4), t3=XMLoadFloat3x3(&transform3);
	//
	emit += (float)count/60.0f*gamespeed;

	//bool clearSpace=false;
	//if(points.size()+emit>=MAX_PARTICLES)
	//	clearSpace=true;

	//for(int i=0;i<(int)emit;++i)
	//{
	//	if(clearSpace)
	//		points.pop_front();

	//	addPoint(t4,t3);
	//}
	//if((int)emit>0)
	//	emit=0;
	
}
void wiEmittedParticle::Burst(float num)
{
	//XMMATRIX t4=XMLoadFloat4x4(&transform4), t3=XMLoadFloat3x3(&transform3);

	//static float burst = 0;
	//burst+=num;
	//for(int i=0;i<(int)burst;++i)
	//	addPoint(t4,t3);
	//burst-=(int)burst;

	emit += num;
}

void wiEmittedParticle::UpdateAttachedLight(float dt)
{
	//if (light != nullptr)
	//{
	//	// smooth light position to eliminate jitter:

	//	posSamples[currentSample] = bounding_box.getCenter();
	//	radSamples[currentSample] = bounding_box.getRadius() * 4;
	//	energySamples[currentSample] = sqrt((float)points.size());

	//	XMFLOAT3 pos = XMFLOAT3(0, 0, 0);
	//	float rad = 0.0f;
	//	float energy = 0.0f;
	//	for (int i = 0; i < NUM_POS_SAMPLES; ++i)
	//	{
	//		pos.x += posSamples[i].x;
	//		pos.y += posSamples[i].y;
	//		pos.z += posSamples[i].z;
	//		rad += radSamples[i];
	//		energy += energySamples[i];
	//	}
	//	pos.x *= INV_NUM_POS_SAMPLES;
	//	pos.y *= INV_NUM_POS_SAMPLES;
	//	pos.z *= INV_NUM_POS_SAMPLES;
	//	rad *= INV_NUM_POS_SAMPLES;
	//	energy *= INV_NUM_POS_SAMPLES;
	//	currentSample = (currentSample + 1) % NUM_POS_SAMPLES;

	//	light->translation_rest = pos;

	//	light->enerDis = XMFLOAT4(energy, rad, 0, 0);
	//	light->UpdateLight();
	//}
}

void wiEmittedParticle::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("UpdateEmittedParticles", threadID);

	EmittedParticleCB cb;
	cb.xMotionBlurAmount = motionBlurAmount;
	cb.xEmitCount = (UINT)emit;
	cb.xMeshIndexCount = (UINT)object->mesh->indices.size();
	cb.xMeshIndexStride = object->mesh->GetIndexFormat() == INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t);
	cb.xMeshVertexPositionStride = sizeof(Mesh::Vertex_POS);
	cb.xMeshVertexNormalStride = sizeof(Mesh::Vertex_NOR);
	cb.xEmitterWorld = object->world;
	device->UpdateBuffer(constantBuffer, &cb, threadID);
	device->BindConstantBufferCS(constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), threadID);

	const GPUUnorderedResource* uavs[] = {
		static_cast<GPUUnorderedResource*>(particleBuffer),
		static_cast<GPUUnorderedResource*>(aliveList[0]), // last alivelist
		static_cast<GPUUnorderedResource*>(aliveList[1]), // new alivelist
		static_cast<GPUUnorderedResource*>(deadList),
		static_cast<GPUUnorderedResource*>(counterBuffer),
		static_cast<GPUUnorderedResource*>(indirectDispatchBuffer),
		static_cast<GPUUnorderedResource*>(indirectDrawBuffer),
	};
	device->BindUnorderedAccessResourcesCS(uavs, 0, ARRAYSIZE(uavs), threadID);
	
	const GPUResource* resources[] = {
		wiTextureHelper::getInstance()->getRandom64x64(),
		static_cast<GPUResource*>(&object->mesh->indexBuffer),
		static_cast<GPUResource*>(&object->mesh->vertexBuffer_POS),
		static_cast<GPUResource*>(&object->mesh->vertexBuffer_NOR),
	};
	device->BindResourcesCS(resources, TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

	// kick off updating
	device->BindCS(kickoffUpdateCS, threadID);
	device->Dispatch(1, 1, 1, threadID);

	// emit the required amount if there are free slots in dead list
	device->BindCS(emitCS, threadID);
	device->DispatchIndirect(indirectDispatchBuffer, 0, threadID);

	// kick off simulation based on OLD alivelist count
	device->BindCS(simulateargsCS, threadID);
	device->Dispatch(1, 1, 1, threadID);

	// update OLD alive list, write NEW alive list
	device->BindCS(simulateCS, threadID);
	device->DispatchIndirect(indirectDispatchBuffer, 0, threadID);

	// update the draw arguments
	device->BindCS(drawargsCS, threadID);
	device->Dispatch(1, 1, 1, threadID);


	device->UnBindUnorderedAccessResources(0, ARRAYSIZE(uavs), threadID);

	device->EventEnd(threadID);

	// Swap OLD alivelist with NEW alivelist
	SwapPtr(aliveList[0], aliveList[1]);
	emit -= (UINT)emit;
}

void wiEmittedParticle::Draw(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("EmittedParticle", threadID);

	bool additive = (material->blendFlag == BLENDMODE_ADDITIVE || material->premultipliedTexture);

	device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST, threadID);
	device->BindVertexLayout(nullptr, threadID);
	device->BindPS(wiRenderer::IsWireRender() ? simplestPS : pixelShader, threadID);
	device->BindVS(vertexShader, threadID);

	device->BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), threadID);

	device->BindRasterizerState(wiRenderer::IsWireRender() ? wireFrameRS : rasterizerState, threadID);
	device->BindDepthStencilState(depthStencilState, 1, threadID);

	device->BindBlendState((additive ? blendStateAdd : blendStateAlpha), threadID);

	device->BindResourceVS(particleBuffer, 0, threadID);
	device->BindResourceVS(aliveList[0], 1, threadID);

	if (!wiRenderer::IsWireRender() && material->texture)
	{
		device->BindResourcePS(material->texture, TEXSLOT_ONDEMAND0, threadID);
	}

	//device->Draw((int)points.size() * 6, threadID);
	device->DrawInstancedIndirect(indirectDrawBuffer, 0, threadID);

	device->EventEnd(threadID);
}


void wiEmittedParticle::CleanUp()
{
	SAFE_DELETE(particleBuffer);
	SAFE_DELETE(aliveList[0]);
	SAFE_DELETE(aliveList[1]);
	SAFE_DELETE(deadList);
	SAFE_DELETE(counterBuffer);
	SAFE_DELETE(indirectDispatchBuffer);
	SAFE_DELETE(indirectDrawBuffer);
	SAFE_DELETE(constantBuffer);

	//points.clear();

	//SAFE_DELETE_ARRAY(posSamples);
	//SAFE_DELETE_ARRAY(radSamples);
	//SAFE_DELETE_ARRAY(energySamples);
}



void wiEmittedParticle::LoadShaders()
{
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticleVS.cso", wiResourceManager::VERTEXSHADER));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
	}


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS.cso", wiResourceManager::PIXELSHADER));
	simplestPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));
	
	
	kickoffUpdateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_kickoffUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	emitCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_emitCS.cso", wiResourceManager::COMPUTESHADER));
	simulateargsCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_simulateargsCS.cso", wiResourceManager::COMPUTESHADER));
	drawargsCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_drawargsCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_simulateCS.cso", wiResourceManager::COMPUTESHADER));



}
void wiEmittedParticle::LoadBuffers()
{
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
	SAFE_DELETE(emitCS);
	SAFE_DELETE(simulateargsCS);
	SAFE_DELETE(drawargsCS);
	SAFE_DELETE(simulateCS);
	//SAFE_DELETE(constantBuffer);
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
