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


static const int NUM_POS_SAMPLES = 30;
static const float INV_NUM_POS_SAMPLES = 1.0f / NUM_POS_SAMPLES;

wiEmittedParticle::wiEmittedParticle()
{
	name = "";
	object = nullptr;
	materialName = "";
	material = nullptr;

	size = 1;
	random_factor = 0;
	normal_factor = 1;

	count = 1;
	life = 60;
	random_life = 0;
	emit = 0;

	scaleX = 1;
	scaleY = 1;
	rotation = 0;

	motionBlurAmount = 0.0f;

	SAFE_INIT(particleBuffer);
	SAFE_INIT(aliveList[0]);
	SAFE_INIT(aliveList[1]);
	SAFE_INIT(deadList);
	SAFE_INIT(counterBuffer);
	SAFE_INIT(indirectDispatchBuffer);
	SAFE_INIT(indirectDrawBuffer);
	SAFE_INIT(constantBuffer);

	SetMaxParticleCount(10000);
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
	life=newLife;
	random_life=newRandLife;
	emit=0;
	
	scaleX=newScaleX;
	scaleY=newScaleY;
	rotation = newRot;

	motionBlurAmount = 0.0f;


	SAFE_INIT(particleBuffer);
	SAFE_INIT(aliveList[0]);
	SAFE_INIT(aliveList[1]);
	SAFE_INIT(deadList);
	SAFE_INIT(counterBuffer);
	SAFE_INIT(indirectDispatchBuffer);
	SAFE_INIT(indirectDrawBuffer);
	SAFE_INIT(constantBuffer);

	SetMaxParticleCount(10000);
}
wiEmittedParticle::wiEmittedParticle(const wiEmittedParticle& other)
{
	name = other.name + "0";
	object = other.object;
	materialName = other.materialName;
	material = other.material;
	size = other.size;
	random_factor = other.random_factor;
	normal_factor = other.normal_factor;
	count = other.count;
	life = other.life;
	random_life = other.random_life;
	emit = 0;
	scaleX = other.scaleX;
	scaleY = other.scaleY;
	rotation = other.rotation;
	motionBlurAmount = other.motionBlurAmount;


	SAFE_INIT(particleBuffer);
	SAFE_INIT(aliveList[0]);
	SAFE_INIT(aliveList[1]);
	SAFE_INIT(deadList);
	SAFE_INIT(counterBuffer);
	SAFE_INIT(indirectDispatchBuffer);
	SAFE_INIT(indirectDrawBuffer);
	SAFE_INIT(constantBuffer);

	SetMaxParticleCount(other.GetMaxParticleCount());
}

void wiEmittedParticle::SetMaxParticleCount(uint32_t value)
{
	MAX_PARTICLES = value;
	CreateSelfBuffers();
}

void wiEmittedParticle::CreateSelfBuffers()
{
	SAFE_DELETE(particleBuffer);
	SAFE_DELETE(aliveList[0]);
	SAFE_DELETE(aliveList[1]);
	SAFE_DELETE(deadList);
	SAFE_DELETE(counterBuffer);
	SAFE_DELETE(indirectDispatchBuffer);
	SAFE_DELETE(indirectDrawBuffer);
	SAFE_DELETE(constantBuffer);

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


	bd.BindFlags = BIND_UNORDERED_ACCESS;
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

uint32_t wiEmittedParticle::GetMemorySizeInBytes() const
{
	uint32_t retVal = 0;

	retVal += particleBuffer->GetDesc().ByteWidth;
	retVal += aliveList[0]->GetDesc().ByteWidth;
	retVal += aliveList[1]->GetDesc().ByteWidth;
	retVal += deadList->GetDesc().ByteWidth;
	retVal += counterBuffer->GetDesc().ByteWidth;
	retVal += indirectDispatchBuffer->GetDesc().ByteWidth;
	retVal += indirectDrawBuffer->GetDesc().ByteWidth;
	retVal += constantBuffer->GetDesc().ByteWidth;

	return retVal;
}

XMFLOAT3 wiEmittedParticle::GetPosition() const
{
	return object == nullptr ? XMFLOAT3(0, 0, 0) : object->translation;
}

void wiEmittedParticle::Update(float dt)
{
	float gamespeed = wiRenderer::GetGameSpeed() * dt;
	emit += (float)count*gamespeed;
}
void wiEmittedParticle::Burst(float num)
{
	emit += num;
}


void wiEmittedParticle::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("UpdateEmittedParticles", threadID);

	EmittedParticleCB cb;
	cb.xEmitterWorld = object->world;
	cb.xEmitterMeshIndexCount = (UINT)object->mesh->indices.size();
	cb.xEmitterMeshIndexStride = object->mesh->GetIndexFormat() == INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t);
	cb.xEmitterMeshVertexPositionStride = sizeof(Mesh::Vertex_POS);
	cb.xEmitterMeshVertexNormalStride = sizeof(Mesh::Vertex_NOR);
	cb.xEmitterRandomness = wiRandom::getRandom(0, 1000) * 0.001f;
	cb.xEmitCount = (UINT)emit;
	cb.xParticleLifeSpan = life / 60.0f;
	cb.xParticleLifeSpanRandomness = random_life;
	cb.xParticleNormalFactor = normal_factor;
	cb.xParticleRandomFactor = random_factor;
	cb.xParticleScaling = scaleX;
	cb.xParticleSize = size;
	cb.xParticleMotionBlurAmount = motionBlurAmount;
	cb.xParticleRotation = rotation;

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


	// kick off updating, set up state
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


	device->BindCS(nullptr, threadID);
	device->UnBindUnorderedAccessResources(0, ARRAYSIZE(uavs), threadID);
	device->UnBindResources(TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

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
		if (archive.GetVersion() < 9)
		{
			XMFLOAT4X4 transform4;
			XMFLOAT3X3 transform3;
			archive >> transform4;
			archive >> transform3;
		}
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
		if (archive.GetVersion() < 9)
		{
			string lightName;
			archive >> lightName;
		}

	}
	else
	{
		archive << emit;
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
	}
}
