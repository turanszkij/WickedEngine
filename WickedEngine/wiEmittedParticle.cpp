#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiSceneSystem.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "wiTextureHelper.h"
#include "wiGPUSortLib.h"
#include "wiProfiler.h"
#include "wiBackLog.h"

using namespace std;
using namespace wiGraphicsTypes;

namespace wiSceneSystem
{

static VertexShader*		vertexShader = nullptr;
static PixelShader*			pixelShader[wiEmittedParticle::PARTICLESHADERTYPE_COUNT] = {};
static ComputeShader*		kickoffUpdateCS = nullptr;
static ComputeShader*		finishUpdateCS = nullptr;
static ComputeShader*		emitCS = nullptr;
static ComputeShader*		emitCS_FROMMESH = nullptr;
static ComputeShader*		sphpartitionCS = nullptr;
static ComputeShader*		sphpartitionoffsetsCS = nullptr;
static ComputeShader*		sphpartitionoffsetsresetCS = nullptr;
static ComputeShader*		sphdensityCS = nullptr;
static ComputeShader*		sphforceCS = nullptr;
static ComputeShader*		simulateCS = nullptr;
static ComputeShader*		simulateCS_SORTING = nullptr;
static ComputeShader*		simulateCS_DEPTHCOLLISIONS = nullptr;
static ComputeShader*		simulateCS_SORTING_DEPTHCOLLISIONS = nullptr;

static BlendState			blendStates[BLENDMODE_COUNT];
static RasterizerState		rasterizerState;
static RasterizerState		wireFrameRS;
static DepthStencilState	depthStencilState;
static GraphicsPSO			PSO[BLENDMODE_COUNT][wiEmittedParticle::PARTICLESHADERTYPE_COUNT];
static GraphicsPSO			PSO_wire;
static ComputePSO			CPSO_kickoffUpdate;
static ComputePSO			CPSO_finishUpdate;
static ComputePSO			CPSO_emit;
static ComputePSO			CPSO_emit_FROMMESH;
static ComputePSO			CPSO_sphpartition;
static ComputePSO			CPSO_sphpartitionoffsets;
static ComputePSO			CPSO_sphpartitionoffsetsreset;
static ComputePSO			CPSO_sphdensity;
static ComputePSO			CPSO_sphforce;
static ComputePSO			CPSO_simulate;
static ComputePSO			CPSO_simulate_SORTING;
static ComputePSO			CPSO_simulate_DEPTHCOLLISIONS;
static ComputePSO			CPSO_simulate_SORTING_DEPTHCOLLISIONS;


void wiEmittedParticle::SetMaxParticleCount(uint32_t value)
{
	buffersUpToDate = false;
	MAX_PARTICLES = value;
}

void wiEmittedParticle::CreateSelfBuffers()
{
	if (buffersUpToDate)
	{
		return;
	}
	buffersUpToDate = true;

	particleBuffer.reset(new GPUBuffer);
	aliveList[0].reset(new GPUBuffer);
	aliveList[1].reset(new GPUBuffer);
	deadList.reset(new GPUBuffer);
	distanceBuffer.reset(new GPUBuffer);
	sphPartitionCellIndices.reset(new GPUBuffer);
	sphPartitionCellOffsets.reset(new GPUBuffer);
	densityBuffer.reset(new GPUBuffer);
	counterBuffer.reset(new GPUBuffer);
	indirectBuffers.reset(new GPUBuffer);
	constantBuffer.reset(new GPUBuffer);
	debugDataReadbackBuffer.reset(new GPUBuffer);
	debugDataReadbackIndexBuffer.reset(new GPUBuffer);
	debugDataReadbackDistanceBuffer.reset(new GPUBuffer);


	// GPU-local buffer descriptors:
	GPUBufferDesc bd;
	bd.Usage = USAGE_DEFAULT;
	bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	SubresourceData data;

	// Particle buffer:
	bd.StructureByteStride = sizeof(Particle);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, particleBuffer.get());

	// Alive index lists (double buffered):
	bd.StructureByteStride = sizeof(uint32_t);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, aliveList[0].get());
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, aliveList[1].get());

	// Dead index list:
	uint32_t* indices = new uint32_t[MAX_PARTICLES];
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		indices[i] = i;
	}
	data.pSysMem = indices;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, deadList.get());
	SAFE_DELETE_ARRAY(indices);
	data.pSysMem = nullptr;

	// Distance buffer:
	bd.StructureByteStride = sizeof(float);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	float* distances = new float[MAX_PARTICLES];
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		distances[i] = 0;
	}
	data.pSysMem = distances;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, distanceBuffer.get());
	SAFE_DELETE_ARRAY(distances);
	data.pSysMem = nullptr;

	// SPH Partitioning grid indices per particle:
	bd.StructureByteStride = sizeof(float); // really, it is uint, but sorting is performing comparisons on floats, so whateva
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, sphPartitionCellIndices.get());

	// SPH Partitioning grid cell offsets into particle index list:
	bd.StructureByteStride = sizeof(uint32_t);
	bd.ByteWidth = bd.StructureByteStride * SPH_PARTITION_BUCKET_COUNT;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, sphPartitionCellOffsets.get());

	// Density buffer (for SPH simulation):
	bd.StructureByteStride = sizeof(float);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, densityBuffer.get());

	// Particle System statistics:
	ParticleCounters counters;
	counters.aliveCount = 0;
	counters.deadCount = MAX_PARTICLES;
	counters.realEmitCount = 0;
	counters.aliveCount_afterSimulation = 0;

	data.pSysMem = &counters;
	bd.ByteWidth = sizeof(counters);
	bd.StructureByteStride = sizeof(counters);
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, counterBuffer.get());
	data.pSysMem = nullptr;

	// Indirect Execution buffer:
	bd.BindFlags = BIND_UNORDERED_ACCESS;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_DRAWINDIRECT_ARGS;
	bd.ByteWidth = 
		sizeof(wiGraphicsTypes::IndirectDispatchArgs) + 
		sizeof(wiGraphicsTypes::IndirectDispatchArgs) + 
		sizeof(wiGraphicsTypes::IndirectDrawArgsInstanced);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, indirectBuffers.get());

	// Constant buffer:
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(EmittedParticleCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, constantBuffer.get());

	// Debug information CPU-readback buffer:
	{
		GPUBufferDesc debugBufDesc = counterBuffer->GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, debugDataReadbackBuffer.get());
	}

	// Sorting debug buffers:
	{
		GPUBufferDesc debugBufDesc = aliveList[0]->GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, debugDataReadbackIndexBuffer.get());
	}
	{
		GPUBufferDesc debugBufDesc = distanceBuffer->GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, debugDataReadbackDistanceBuffer.get());
	}
}

uint32_t wiEmittedParticle::GetMemorySizeInBytes() const
{
	if (particleBuffer == nullptr)
		return 0;

	uint32_t retVal = 0;

	retVal += particleBuffer->GetDesc().ByteWidth;
	retVal += aliveList[0]->GetDesc().ByteWidth;
	retVal += aliveList[1]->GetDesc().ByteWidth;
	retVal += deadList->GetDesc().ByteWidth;
	retVal += distanceBuffer->GetDesc().ByteWidth;
	retVal += sphPartitionCellIndices->GetDesc().ByteWidth;
	retVal += sphPartitionCellOffsets->GetDesc().ByteWidth;
	retVal += densityBuffer->GetDesc().ByteWidth;
	retVal += counterBuffer->GetDesc().ByteWidth;
	retVal += indirectBuffers->GetDesc().ByteWidth;
	retVal += constantBuffer->GetDesc().ByteWidth;

	return retVal;
}

void wiEmittedParticle::Update(float dt)
{
	if (IsPaused())
		return;

	emit += (float)count*dt;
}
void wiEmittedParticle::Burst(float num)
{
	if (IsPaused())
		return;

	emit += num;
}
void wiEmittedParticle::Restart()
{
	buffersUpToDate = false;
	SetPaused(false);
}

//#define DEBUG_SORTING // slow but great for debug!!
void wiEmittedParticle::UpdateRenderData(const TransformComponent& transform, const MaterialComponent& material, const MeshComponent* mesh, GRAPHICSTHREAD threadID)
{
	center = transform.GetPosition();

	CreateSelfBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();


	if (!IsPaused())
	{

		device->EventBegin("UpdateEmittedParticles", threadID);

		EmittedParticleCB cb;
		cb.xEmitterWorld = transform.world;
		cb.xEmitCount = (UINT)emit;
		cb.xEmitterMeshIndexCount = mesh == nullptr ? 0 : (UINT)mesh->indices.size();
		cb.xEmitterMeshVertexPositionStride = sizeof(MeshComponent::Vertex_POS);
		cb.xEmitterRandomness = wiRandom::getRandom(0, 1000) * 0.001f;
		cb.xParticleLifeSpan = life;
		cb.xParticleLifeSpanRandomness = random_life;
		cb.xParticleNormalFactor = normal_factor;
		cb.xParticleRandomFactor = random_factor;
		cb.xParticleScaling = scaleX;
		cb.xParticleSize = size;
		cb.xParticleMotionBlurAmount = motionBlurAmount;
		cb.xParticleRotation = rotation * XM_PI * 60;
		cb.xParticleColor = wiMath::CompressColor(XMFLOAT4(material.baseColor.x, material.baseColor.y, material.baseColor.z, 1));
		cb.xEmitterOpacity = material.GetOpacity();
		cb.xParticleMass = mass;
		cb.xEmitterMaxParticleCount = MAX_PARTICLES;
		cb.xEmitterFixedTimestep = FIXED_TIMESTEP;

		// SPH:
		cb.xSPH_h = SPH_h;
		cb.xSPH_h_rcp = 1.0f / SPH_h;
		cb.xSPH_h2 = SPH_h * SPH_h;
		cb.xSPH_h3 = cb.xSPH_h2 * SPH_h;
		const float h6 = cb.xSPH_h2 * cb.xSPH_h2 * cb.xSPH_h2;
		const float h9 = cb.xSPH_h3 * cb.xSPH_h3;
		cb.xSPH_poly6_constant = (315.0f / (64.0f * XM_PI * h9));
		cb.xSPH_spiky_constant = (-45 / (XM_PI * h6));
		cb.xSPH_K = SPH_K;
		cb.xSPH_p0 = SPH_p0;
		cb.xSPH_e = SPH_e;
		cb.xSPH_ENABLED = IsSPHEnabled() ? 1 : 0;

		device->UpdateBuffer(constantBuffer.get(), &cb, threadID);
		device->BindConstantBuffer(CS, constantBuffer.get(), CB_GETBINDSLOT(EmittedParticleCB), threadID);

		GPUResource* uavs[] = {
			particleBuffer.get(),
			aliveList[0].get(), // CURRENT alivelist
			aliveList[1].get(), // NEW alivelist
			deadList.get(),
			counterBuffer.get(),
			indirectBuffers.get(),
			distanceBuffer.get(),
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		GPUResource* resources[] = {
			mesh == nullptr ? nullptr : mesh->indexBuffer.get(),
			mesh == nullptr ? nullptr : (mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS.get() : mesh->vertexBuffer_POS.get()),
		};
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

		GPUResource* indres[] = {
			indirectBuffers.get()
		};
		device->TransitionBarrier(indres, 1, RESOURCE_STATE_INDIRECT_ARGUMENT, RESOURCE_STATE_UNORDERED_ACCESS, threadID);

		// kick off updating, set up state
		device->EventBegin("KickOff Update", threadID);
		device->BindComputePSO(&CPSO_kickoffUpdate, threadID);
		device->Dispatch(1, 1, 1, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->EventEnd(threadID);

		device->TransitionBarrier(indres, 1, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT, threadID);

		// emit the required amount if there are free slots in dead list
		device->EventBegin("Emit", threadID);
		device->BindComputePSO(mesh == nullptr ? &CPSO_emit : &CPSO_emit_FROMMESH, threadID);
		device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHEMIT, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->EventEnd(threadID);

		if (IsSPHEnabled())
		{
			wiProfiler::BeginRange("SPH - Simulation", wiProfiler::DOMAIN_GPU, threadID);

			// Smooth Particle Hydrodynamics:
			device->EventBegin("SPH - Simulation", threadID);

#ifdef SPH_USE_ACCELERATION_GRID
			// 1.) Assign particles into partitioning grid:
			device->EventBegin("Partitioning", threadID);
			device->BindComputePSO(&CPSO_sphpartition, threadID);
			device->UnbindUAVs(0, 8, threadID);
			GPUResource* res_partition[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				particleBuffer.get(),
			};
			device->BindResources(CS, res_partition, 0, ARRAYSIZE(res_partition), threadID);
			GPUResource* uav_partition[] = {
				sphPartitionCellIndices.get(),
			};
			device->BindUAVs(CS, uav_partition, 0, ARRAYSIZE(uav_partition), threadID);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_partition, ARRAYSIZE(uav_partition), threadID);
			device->EventEnd(threadID);

			// 2.) Sort particle index list based on partition grid cell index:
			wiGPUSortLib::Sort(MAX_PARTICLES, sphPartitionCellIndices.get(), counterBuffer.get(), PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveList[0].get(), threadID);

			// 3.) Reset grid cell offset buffer with invalid offsets (max uint):
			device->EventBegin("PartitionOffsetsReset", threadID);
			device->BindComputePSO(&CPSO_sphpartitionoffsetsreset, threadID);
			device->UnbindUAVs(0, 8, threadID);
			GPUResource* uav_partitionoffsets[] = {
				sphPartitionCellOffsets.get(),
			};
			device->BindUAVs(CS, uav_partitionoffsets, 0, ARRAYSIZE(uav_partitionoffsets), threadID);
			device->Dispatch((UINT)ceilf((float)SPH_PARTITION_BUCKET_COUNT / (float)THREADCOUNT_SIMULATION), 1, 1, threadID);
			device->UAVBarrier(uav_partitionoffsets, ARRAYSIZE(uav_partitionoffsets), threadID);
			device->EventEnd(threadID);

			// 4.) Assemble grid cell offsets from the sorted particle index list <--> grid cell index list connection:
			device->EventBegin("PartitionOffsets", threadID);
			device->BindComputePSO(&CPSO_sphpartitionoffsets, threadID);
			GPUResource* res_partitionoffsets[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				sphPartitionCellIndices.get(),
			};
			device->BindResources(CS, res_partitionoffsets, 0, ARRAYSIZE(res_partitionoffsets), threadID);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_partitionoffsets, ARRAYSIZE(uav_partitionoffsets), threadID);
			device->EventEnd(threadID);

#endif // SPH_USE_ACCELERATION_GRID

			// 5.) Compute particle density field:
			device->EventBegin("Density Evaluation", threadID);
			device->BindComputePSO(&CPSO_sphdensity, threadID);
			device->UnbindUAVs(0, 8, threadID);
			GPUResource* res_density[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				particleBuffer.get(),
				sphPartitionCellIndices.get(),
				sphPartitionCellOffsets.get(),
			};
			device->BindResources(CS, res_density, 0, ARRAYSIZE(res_density), threadID);
			GPUResource* uav_density[] = {
				densityBuffer.get()
			};
			device->BindUAVs(CS, uav_density, 0, ARRAYSIZE(uav_density), threadID);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_density, ARRAYSIZE(uav_density), threadID);
			device->EventEnd(threadID);

			// 6.) Compute particle pressure forces:
			device->EventBegin("Force Evaluation", threadID);
			device->BindComputePSO(&CPSO_sphforce, threadID);
			device->UnbindUAVs(0, 8, threadID);
			GPUResource* res_force[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				densityBuffer.get(),
				sphPartitionCellIndices.get(),
				sphPartitionCellOffsets.get(),
			};
			device->BindResources(CS, res_force, 0, ARRAYSIZE(res_force), threadID);
			GPUResource* uav_force[] = {
				particleBuffer.get(),
			};
			device->BindUAVs(CS, uav_force, 0, ARRAYSIZE(uav_force), threadID);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_force, ARRAYSIZE(uav_force), threadID);
			device->EventEnd(threadID);

			device->UnbindResources(0, 3, threadID);
			device->UnbindUAVs(0, 8, threadID);

			device->EventEnd(threadID);

			wiProfiler::EndRange(threadID);
		}

		device->EventBegin("Simulate", threadID);
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

		// update CURRENT alive list, write NEW alive list
		if (IsSorted())
		{
			if (IsDepthCollisionEnabled())
			{
				device->BindComputePSO(&CPSO_simulate_SORTING_DEPTHCOLLISIONS, threadID);
			}
			else
			{
				device->BindComputePSO(&CPSO_simulate_SORTING, threadID);
			}
		}
		else
		{
			if (IsDepthCollisionEnabled())
			{
				device->BindComputePSO(&CPSO_simulate_DEPTHCOLLISIONS, threadID);
			}
			else
			{
				device->BindComputePSO(&CPSO_simulate, threadID);
			}
		}
		device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->EventEnd(threadID);


		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
		device->UnbindResources(TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

		device->EventEnd(threadID);

	}

	if (IsSorted())
	{
#ifdef DEBUG_SORTING
		vector<uint32_t> before(MAX_PARTICLES);
		device->DownloadResource(aliveList[1].get(), debugDataReadbackIndexBuffer.get(), before.data(), threadID);

		device->DownloadResource(counterBuffer.get(), debugDataReadbackBuffer.get(), &debugData, threadID);
		uint32_t particleCount = debugData.aliveCount_afterSimulation;
#endif // DEBUG_SORTING


		wiGPUSortLib::Sort(MAX_PARTICLES, distanceBuffer.get(), counterBuffer.get(), PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, aliveList[1].get(), threadID);


#ifdef DEBUG_SORTING
		vector<uint32_t> after(MAX_PARTICLES);
		device->DownloadResource(aliveList[1].get(), debugDataReadbackIndexBuffer.get(), after.data(), threadID);

		vector<float> distances(MAX_PARTICLES);
		device->DownloadResource(distanceBuffer.get(), debugDataReadbackDistanceBuffer.get(), distances.data(), threadID);

		if (particleCount > 1)
		{
			// CPU sort:
			for (uint32_t i = 0; i < particleCount - 1; ++i)
			{
				for (uint32_t j = i + 1; j < particleCount; ++j)
				{
					uint32_t particleIndexA = before[i];
					uint32_t particleIndexB = before[j];

					float distA = distances[particleIndexA];
					float distB = distances[particleIndexB];

					if (distA > distB)
					{
						before[i] = particleIndexB;
						before[j] = particleIndexA;
					}
				}
			}

			// Validate:
			bool valid = true;
			uint32_t i = 0;
			for (i = 0; i < particleCount; ++i)
			{
				if (before[i] != after[i])
				{
					if (distances[before[i]] != distances[after[i]]) // if distances are equal, we just don't care...
					{
						valid = false;
						break;
					}
				}
			}

			assert(valid && "Invalid GPU sorting result!");

			// Also we can reupload CPU sorted particles to verify:
			if (!valid)
			{
				device->UpdateBuffer(aliveList[1].get(), before.data(), threadID);
			}
		}
#endif // DEBUG_SORTING
	}

	if (!IsPaused())
	{
		// finish updating, update draw argument buffer:
		device->EventBegin("FinishUpdate", threadID);
		device->BindComputePSO(&CPSO_finishUpdate, threadID);

		GPUResource* res[] = {
			counterBuffer.get(),
		};
		device->BindResources(CS, res, 0, ARRAYSIZE(res), threadID);

		GPUResource* uavs[] = {
			indirectBuffers.get(),
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		device->Dispatch(1, 1, 1, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);

		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
		device->UnbindResources(0, ARRAYSIZE(res), threadID);
		device->EventEnd(threadID);


		// Swap CURRENT alivelist with NEW alivelist
		aliveList[0].swap(aliveList[1]);
		emit -= (UINT)emit;
	}

	if (IsDebug())
	{
		device->DownloadResource(counterBuffer.get(), debugDataReadbackBuffer.get(), &debugData, threadID);
	}
}


void wiEmittedParticle::Draw(const CameraComponent& camera, const MaterialComponent& material, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("EmittedParticle", threadID);

	if (wiRenderer::IsWireRender())
	{
		device->BindGraphicsPSO(&PSO_wire, threadID);
	}
	else
	{
		device->BindGraphicsPSO(&PSO[material.blendMode][shaderType], threadID);
		device->BindResource(PS, material.GetBaseColorMap(), TEXSLOT_ONDEMAND0, threadID);
	}

	device->BindConstantBuffer(VS, constantBuffer.get(), CB_GETBINDSLOT(EmittedParticleCB), threadID);

	GPUResource* res[] = {
		particleBuffer.get(),
		aliveList[0].get()
	};
	device->TransitionBarrier(res, ARRAYSIZE(res), RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, threadID);
	device->BindResources(VS, res, 0, ARRAYSIZE(res), threadID);

	device->DrawInstancedIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, threadID);

	device->EventEnd(threadID);
}



void wiEmittedParticle::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	vertexShader = static_cast<VertexShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticleVS.cso", wiResourceManager::VERTEXSHADER));
	
	pixelShader[SOFT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticlePS_soft.cso", wiResourceManager::PIXELSHADER));
	pixelShader[SOFT_DISTORTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticlePS_soft_distortion.cso", wiResourceManager::PIXELSHADER));
	pixelShader[SIMPLEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));
	
	kickoffUpdateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_kickoffUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	finishUpdateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_finishUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	emitCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_emitCS.cso", wiResourceManager::COMPUTESHADER));
	emitCS_FROMMESH = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_emitCS_FROMMESH.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphpartitionCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionoffsetsCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphpartitionoffsetsCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionoffsetsresetCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphpartitionoffsetsresetCS.cso", wiResourceManager::COMPUTESHADER));
	sphdensityCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphdensityCS.cso", wiResourceManager::COMPUTESHADER));
	sphforceCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphforceCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_SORTING = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS_SORTING.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_DEPTHCOLLISIONS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS_DEPTHCOLLISIONS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_SORTING_DEPTHCOLLISIONS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.cso", wiResourceManager::COMPUTESHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	for (int i = 0; i < BLENDMODE_COUNT; ++i)
	{
		GraphicsPSODesc desc;
		desc.vs = vertexShader;
		desc.bs = &blendStates[i];
		desc.rs = &rasterizerState;
		desc.dss = &depthStencilState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;

		desc.ps = pixelShader[SOFT];
		device->CreateGraphicsPSO(&desc, &PSO[i][SOFT]);
		desc.ps = pixelShader[SOFT_DISTORTION];
		device->CreateGraphicsPSO(&desc, &PSO[i][SOFT_DISTORTION]);
		desc.ps = pixelShader[SIMPLEST];
		device->CreateGraphicsPSO(&desc, &PSO[i][SIMPLEST]);
	}

	{
		GraphicsPSODesc desc;
		desc.vs = vertexShader;
		desc.ps = pixelShader[SIMPLEST];
		desc.bs = &blendStates[BLENDMODE_ALPHA];
		desc.rs = &wireFrameRS;
		desc.dss = &depthStencilState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;

		device->CreateGraphicsPSO(&desc, &PSO_wire);
	}

	{
		ComputePSODesc desc;

		desc.cs = kickoffUpdateCS;
		device->CreateComputePSO(&desc, &CPSO_kickoffUpdate);

		desc.cs = finishUpdateCS;
		device->CreateComputePSO(&desc, &CPSO_finishUpdate);

		desc.cs = emitCS;
		device->CreateComputePSO(&desc, &CPSO_emit);

		desc.cs = emitCS_FROMMESH;
		device->CreateComputePSO(&desc, &CPSO_emit_FROMMESH);

		desc.cs = sphpartitionCS;
		device->CreateComputePSO(&desc, &CPSO_sphpartition);

		desc.cs = sphpartitionoffsetsCS;
		device->CreateComputePSO(&desc, &CPSO_sphpartitionoffsets);

		desc.cs = sphpartitionoffsetsresetCS;
		device->CreateComputePSO(&desc, &CPSO_sphpartitionoffsetsreset);

		desc.cs = sphdensityCS;
		device->CreateComputePSO(&desc, &CPSO_sphdensity);

		desc.cs = sphforceCS;
		device->CreateComputePSO(&desc, &CPSO_sphforce);

		desc.cs = simulateCS;
		device->CreateComputePSO(&desc, &CPSO_simulate);

		desc.cs = simulateCS_SORTING;
		device->CreateComputePSO(&desc, &CPSO_simulate_SORTING);

		desc.cs = simulateCS_DEPTHCOLLISIONS;
		device->CreateComputePSO(&desc, &CPSO_simulate_DEPTHCOLLISIONS);

		desc.cs = simulateCS_SORTING_DEPTHCOLLISIONS;
		device->CreateComputePSO(&desc, &CPSO_simulate_SORTING_DEPTHCOLLISIONS);
	}

}
void wiEmittedParticle::Initialize()
{

	RasterizerStateDesc rs;
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_BACK;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs, &rasterizerState);


	rs.FillMode = FILL_WIREFRAME;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs, &wireFrameRS);


	DepthStencilStateDesc dsd;
	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilState);


	BlendStateDesc bd;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_ALPHA]);

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_ADDITIVE]);

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_PREMULTIPLIED]);

	bd.RenderTarget[0].BlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_OPAQUE]);

	LoadShaders();

	wiBackLog::post("wiEmittedParticle Initialized");
}
void wiEmittedParticle::CleanUp()
{
	SAFE_DELETE(vertexShader);
	for (int i = 0; i < ARRAYSIZE(pixelShader); ++i)
	{
		SAFE_DELETE(pixelShader[i]);
	}
	SAFE_DELETE(emitCS);
	SAFE_DELETE(emitCS_FROMMESH);
	SAFE_DELETE(simulateCS);
	SAFE_DELETE(simulateCS_SORTING);
	SAFE_DELETE(simulateCS_DEPTHCOLLISIONS);
	SAFE_DELETE(simulateCS_SORTING_DEPTHCOLLISIONS);
}


void wiEmittedParticle::Serialize(wiArchive& archive, uint32_t seed)
{
	if (archive.IsReadMode())
	{
		archive >> _flags;
		archive >> (uint32_t&)shaderType;
		wiECS::SerializeEntity(archive, meshID, seed);
		archive >> MAX_PARTICLES;
		archive >> FIXED_TIMESTEP;
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
		archive >> mass;
		archive >> SPH_h;
		archive >> SPH_K;
		archive >> SPH_p0;
		archive >> SPH_e;
	}
	else
	{
		archive << _flags;
		archive << (uint32_t)shaderType;
		wiECS::SerializeEntity(archive, meshID, seed);
		archive << MAX_PARTICLES;
		archive << FIXED_TIMESTEP;
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
		archive << mass;
		archive << SPH_h;
		archive << SPH_K;
		archive << SPH_p0;
		archive << SPH_e;
	}
}

}
