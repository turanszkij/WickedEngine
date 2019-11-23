#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiSceneSystem.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiIntersect.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "wiTextureHelper.h"
#include "wiGPUSortLib.h"
#include "wiProfiler.h"
#include "wiBackLog.h"

#include <algorithm>

using namespace std;
using namespace wiGraphics;

namespace wiSceneSystem
{

static const VertexShader*		vertexShader = nullptr;
static const PixelShader*		pixelShader[wiEmittedParticle::PARTICLESHADERTYPE_COUNT] = {};
static const ComputeShader*		kickoffUpdateCS = nullptr;
static const ComputeShader*		finishUpdateCS = nullptr;
static const ComputeShader*		emitCS = nullptr;
static const ComputeShader*		emitCS_FROMMESH = nullptr;
static const ComputeShader*		sphpartitionCS = nullptr;
static const ComputeShader*		sphpartitionoffsetsCS = nullptr;
static const ComputeShader*		sphpartitionoffsetsresetCS = nullptr;
static const ComputeShader*		sphdensityCS = nullptr;
static const ComputeShader*		sphforceCS = nullptr;
static const ComputeShader*		simulateCS = nullptr;
static const ComputeShader*		simulateCS_SORTING = nullptr;
static const ComputeShader*		simulateCS_DEPTHCOLLISIONS = nullptr;
static const ComputeShader*		simulateCS_SORTING_DEPTHCOLLISIONS = nullptr;

static BlendState			blendStates[BLENDMODE_COUNT];
static RasterizerState		rasterizerState;
static RasterizerState		wireFrameRS;
static DepthStencilState	depthStencilState;
static PipelineState		PSO[BLENDMODE_COUNT][wiEmittedParticle::PARTICLESHADERTYPE_COUNT];
static PipelineState		PSO_wire;


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
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
	bd.ByteWidth = 
		sizeof(wiGraphics::IndirectDispatchArgs) + 
		sizeof(wiGraphics::IndirectDispatchArgs) + 
		sizeof(wiGraphics::IndirectDrawArgsInstanced);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, indirectBuffers.get());

	// Constant buffer:
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(EmittedParticleCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
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

void wiEmittedParticle::UpdateCPU(const TransformComponent& transform, float dt)
{
	if (IsPaused())
		return;

	emit = std::max(0.0f, emit - floorf(emit));

	CreateSelfBuffers();

	center = transform.GetPosition();

	emit += (float)count*dt;

	emit += burst;
	burst = 0;

	// Swap CURRENT alivelist with NEW alivelist
	aliveList[0].swap(aliveList[1]);


	if (IsDebug())
	{
		wiRenderer::GetDevice()->DownloadResource(counterBuffer.get(), debugDataReadbackBuffer.get(), &debugData);
	}
}
void wiEmittedParticle::Burst(int num)
{
	if (IsPaused())
		return;

	burst += num;
}
void wiEmittedParticle::Restart()
{
	buffersUpToDate = false;
	SetPaused(false);
}

//#define DEBUG_SORTING // slow but great for debug!!
void wiEmittedParticle::UpdateGPU(const TransformComponent& transform, const MaterialComponent& material, const MeshComponent* mesh, CommandList cmd) const
{
	if (particleBuffer == nullptr)
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();

	if (!IsPaused())
	{
		device->EventBegin("UpdateEmittedParticles", cmd);

		EmittedParticleCB cb;
		cb.xEmitterWorld = transform.world;
		cb.xEmitCount = (uint32_t)emit;
		cb.xEmitterMeshIndexCount = mesh == nullptr ? 0 : (uint32_t)mesh->indices.size();
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
		cb.xParticleEmissive = material.emissiveColor.w;
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
		const float h9 = h6 * cb.xSPH_h3;
		cb.xSPH_poly6_constant = (315.0f / (64.0f * XM_PI * h9));
		cb.xSPH_spiky_constant = (-45.0f / (XM_PI * h6));
		cb.xSPH_K = SPH_K;
		cb.xSPH_p0 = SPH_p0;
		cb.xSPH_e = SPH_e;
		cb.xSPH_ENABLED = IsSPHEnabled() ? 1 : 0;

		device->UpdateBuffer(constantBuffer.get(), &cb, cmd);
		device->BindConstantBuffer(CS, constantBuffer.get(), CB_GETBINDSLOT(EmittedParticleCB), cmd);

		GPUResource* uavs[] = {
			particleBuffer.get(),
			aliveList[0].get(), // CURRENT alivelist
			aliveList[1].get(), // NEW alivelist
			deadList.get(),
			counterBuffer.get(),
			indirectBuffers.get(),
			distanceBuffer.get(),
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		GPUResource* resources[] = {
			mesh == nullptr ? nullptr : mesh->indexBuffer.get(),
			mesh == nullptr ? nullptr : (mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS.get() : mesh->vertexBuffer_POS.get()),
		};
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, arraysize(resources), cmd);

		device->Barrier(&GPUBarrier::Buffer(indirectBuffers.get(), BUFFER_STATE_INDIRECT_ARGUMENT, BUFFER_STATE_UNORDERED_ACCESS), 1, cmd);

		// kick off updating, set up state
		device->EventBegin("KickOff Update", cmd);
		device->BindComputeShader(kickoffUpdateCS, cmd);
		device->Dispatch(1, 1, 1, cmd);
		device->Barrier(&GPUBarrier::Memory(), 1, cmd);
		device->EventEnd(cmd);

		device->Barrier(&GPUBarrier::Buffer(indirectBuffers.get(), BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT), 1, cmd);

		// emit the required amount if there are free slots in dead list
		device->EventBegin("Emit", cmd);
		device->BindComputeShader(mesh == nullptr ? emitCS : emitCS_FROMMESH, cmd);
		device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHEMIT, cmd);
		device->Barrier(&GPUBarrier::Memory(), 1, cmd);
		device->EventEnd(cmd);

		if (IsSPHEnabled())
		{
			auto range = wiProfiler::BeginRangeGPU("SPH - Simulation", cmd);

			// Smooth Particle Hydrodynamics:
			device->EventBegin("SPH - Simulation", cmd);

#ifdef SPH_USE_ACCELERATION_GRID
			// 1.) Assign particles into partitioning grid:
			device->EventBegin("Partitioning", cmd);
			device->BindComputeShader(sphpartitionCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			GPUResource* res_partition[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				particleBuffer.get(),
			};
			device->BindResources(CS, res_partition, 0, arraysize(res_partition), cmd);
			GPUResource* uav_partition[] = {
				sphPartitionCellIndices.get(),
			};
			device->BindUAVs(CS, uav_partition, 0, arraysize(uav_partition), cmd);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&GPUBarrier::Memory(), 1, cmd);
			device->EventEnd(cmd);

			// 2.) Sort particle index list based on partition grid cell index:
			wiGPUSortLib::Sort(MAX_PARTICLES, *sphPartitionCellIndices.get(), *counterBuffer.get(), PARTICLECOUNTER_OFFSET_ALIVECOUNT, *aliveList[0].get(), cmd);

			// 3.) Reset grid cell offset buffer with invalid offsets (max uint):
			device->EventBegin("PartitionOffsetsReset", cmd);
			device->BindComputeShader(sphpartitionoffsetsresetCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			GPUResource* uav_partitionoffsets[] = {
				sphPartitionCellOffsets.get(),
			};
			device->BindUAVs(CS, uav_partitionoffsets, 0, arraysize(uav_partitionoffsets), cmd);
			device->Dispatch((uint32_t)ceilf((float)SPH_PARTITION_BUCKET_COUNT / (float)THREADCOUNT_SIMULATION), 1, 1, cmd);
			device->Barrier(&GPUBarrier::Memory(), 1, cmd);
			device->EventEnd(cmd);

			// 4.) Assemble grid cell offsets from the sorted particle index list <--> grid cell index list connection:
			device->EventBegin("PartitionOffsets", cmd);
			device->BindComputeShader(sphpartitionoffsetsCS, cmd);
			GPUResource* res_partitionoffsets[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				sphPartitionCellIndices.get(),
			};
			device->BindResources(CS, res_partitionoffsets, 0, arraysize(res_partitionoffsets), cmd);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&GPUBarrier::Memory(), 1, cmd);
			device->EventEnd(cmd);

#endif // SPH_USE_ACCELERATION_GRID

			// 5.) Compute particle density field:
			device->EventBegin("Density Evaluation", cmd);
			device->BindComputeShader(sphdensityCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			GPUResource* res_density[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				particleBuffer.get(),
				sphPartitionCellIndices.get(),
				sphPartitionCellOffsets.get(),
			};
			device->BindResources(CS, res_density, 0, arraysize(res_density), cmd);
			GPUResource* uav_density[] = {
				densityBuffer.get()
			};
			device->BindUAVs(CS, uav_density, 0, arraysize(uav_density), cmd);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&GPUBarrier::Memory(), 1, cmd);
			device->EventEnd(cmd);

			// 6.) Compute particle pressure forces:
			device->EventBegin("Force Evaluation", cmd);
			device->BindComputeShader(sphforceCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			GPUResource* res_force[] = {
				aliveList[0].get(), // CURRENT alivelist
				counterBuffer.get(),
				densityBuffer.get(),
				sphPartitionCellIndices.get(),
				sphPartitionCellOffsets.get(),
			};
			device->BindResources(CS, res_force, 0, arraysize(res_force), cmd);
			GPUResource* uav_force[] = {
				particleBuffer.get(),
			};
			device->BindUAVs(CS, uav_force, 0, arraysize(uav_force), cmd);
			device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&GPUBarrier::Memory(), 1, cmd);
			device->EventEnd(cmd);

			device->UnbindResources(0, 3, cmd);
			device->UnbindUAVs(0, 8, cmd);

			device->EventEnd(cmd);

			wiProfiler::EndRange(range);
		}

		device->EventBegin("Simulate", cmd);
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, arraysize(resources), cmd);

		// update CURRENT alive list, write NEW alive list
		if (IsSorted())
		{
			if (IsDepthCollisionEnabled())
			{
				device->BindComputeShader(simulateCS_SORTING_DEPTHCOLLISIONS, cmd);
			}
			else
			{
				device->BindComputeShader(simulateCS_SORTING, cmd);
			}
		}
		else
		{
			if (IsDepthCollisionEnabled())
			{
				device->BindComputeShader(simulateCS_DEPTHCOLLISIONS, cmd);
			}
			else
			{
				device->BindComputeShader(simulateCS, cmd);
			}
		}
		device->DispatchIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
		device->Barrier(&GPUBarrier::Memory(), 1, cmd);
		device->EventEnd(cmd);


		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(resources), cmd);

		device->EventEnd(cmd);

	}

	if (IsSorted())
	{
#ifdef DEBUG_SORTING
		vector<uint32_t> before(MAX_PARTICLES);
		device->DownloadResource(aliveList[1].get(), debugDataReadbackIndexBuffer.get(), before.data());

		device->DownloadResource(counterBuffer.get(), debugDataReadbackBuffer.get(), &debugData);
		uint32_t particleCount = debugData.aliveCount_afterSimulation;
#endif // DEBUG_SORTING


		wiGPUSortLib::Sort(MAX_PARTICLES, *distanceBuffer.get(), *counterBuffer.get(), PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, *aliveList[1].get(), cmd);


#ifdef DEBUG_SORTING
		vector<uint32_t> after(MAX_PARTICLES);
		device->DownloadResource(aliveList[1].get(), debugDataReadbackIndexBuffer.get(), after.data());

		vector<float> distances(MAX_PARTICLES);
		device->DownloadResource(distanceBuffer.get(), debugDataReadbackDistanceBuffer.get(), distances.data());

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
				device->UpdateBuffer(aliveList[1].get(), before.data(), cmd);
			}
		}
#endif // DEBUG_SORTING
	}

	if (!IsPaused())
	{
		// finish updating, update draw argument buffer:
		device->EventBegin("FinishUpdate", cmd);
		device->BindComputeShader(finishUpdateCS, cmd);

		GPUResource* res[] = {
			counterBuffer.get(),
		};
		device->BindResources(CS, res, 0, arraysize(res), cmd);

		GPUResource* uavs[] = {
			indirectBuffers.get(),
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);
		device->Barrier(&GPUBarrier::Memory(), 1, cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(0, arraysize(res), cmd);
		device->EventEnd(cmd);
	}
}


void wiEmittedParticle::Draw(const CameraComponent& camera, const MaterialComponent& material, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("EmittedParticle", cmd);

	if (wiRenderer::IsWireRender())
	{
		device->BindPipelineState(&PSO_wire, cmd);
	}
	else
	{
		const BLENDMODE blendMode = material.GetBlendMode();
		device->BindPipelineState(&PSO[blendMode][shaderType], cmd);
		device->BindResource(PS, material.GetBaseColorMap(), TEXSLOT_ONDEMAND0, cmd);
	}

	device->BindConstantBuffer(VS, constantBuffer.get(), CB_GETBINDSLOT(EmittedParticleCB), cmd);
	device->BindConstantBuffer(PS, constantBuffer.get(), CB_GETBINDSLOT(EmittedParticleCB), cmd);

	GPUBarrier barriers[] = {
		GPUBarrier::Buffer(particleBuffer.get(), BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
		GPUBarrier::Buffer(aliveList[1].get(), BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	const GPUResource* res[] = {
		particleBuffer.get(),
		aliveList[1].get() // NEW aliveList
	};
	device->BindResources(VS, res, 0, arraysize(res), cmd);

	device->DrawInstancedIndirect(indirectBuffers.get(), ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, cmd);

	device->EventEnd(cmd);
}



void wiEmittedParticle::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	vertexShader = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticleVS.cso", wiResourceManager::VERTEXSHADER));
	
	pixelShader[SOFT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticlePS_soft.cso", wiResourceManager::PIXELSHADER));
	pixelShader[SOFT_DISTORTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticlePS_soft_distortion.cso", wiResourceManager::PIXELSHADER));
	pixelShader[SIMPLEST] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));
	
	kickoffUpdateCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_kickoffUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	finishUpdateCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_finishUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	emitCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_emitCS.cso", wiResourceManager::COMPUTESHADER));
	emitCS_FROMMESH = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_emitCS_FROMMESH.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphpartitionCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionoffsetsCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphpartitionoffsetsCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionoffsetsresetCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphpartitionoffsetsresetCS.cso", wiResourceManager::COMPUTESHADER));
	sphdensityCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphdensityCS.cso", wiResourceManager::COMPUTESHADER));
	sphforceCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_sphforceCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_SORTING = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS_SORTING.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_DEPTHCOLLISIONS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS_DEPTHCOLLISIONS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_SORTING_DEPTHCOLLISIONS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.cso", wiResourceManager::COMPUTESHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	for (int i = 0; i < BLENDMODE_COUNT; ++i)
	{
		PipelineStateDesc desc;
		desc.vs = vertexShader;
		desc.bs = &blendStates[i];
		desc.rs = &rasterizerState;
		desc.dss = &depthStencilState;

		desc.ps = pixelShader[SOFT];
		device->CreatePipelineState(&desc, &PSO[i][SOFT]);
		desc.ps = pixelShader[SOFT_DISTORTION];
		device->CreatePipelineState(&desc, &PSO[i][SOFT_DISTORTION]);
		desc.ps = pixelShader[SIMPLEST];
		device->CreatePipelineState(&desc, &PSO[i][SIMPLEST]);
	}

	{
		PipelineStateDesc desc;
		desc.vs = vertexShader;
		desc.ps = pixelShader[SIMPLEST];
		desc.bs = &blendStates[BLENDMODE_ALPHA];
		desc.rs = &wireFrameRS;
		desc.dss = &depthStencilState;

		device->CreatePipelineState(&desc, &PSO_wire);
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
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER_EQUAL;
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
