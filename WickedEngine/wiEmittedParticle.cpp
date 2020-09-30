#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiScene.h"
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
#include "wiEvent.h"

#include <algorithm>

using namespace std;
using namespace wiGraphics;

namespace wiScene
{

static Shader		vertexShader;
static Shader		meshShader;
static Shader		pixelShader[wiEmittedParticle::PARTICLESHADERTYPE_COUNT];
static Shader		kickoffUpdateCS;
static Shader		finishUpdateCS;
static Shader		emitCS;
static Shader		emitCS_VOLUME;
static Shader		emitCS_FROMMESH;
static Shader		sphpartitionCS;
static Shader		sphpartitionoffsetsCS;
static Shader		sphpartitionoffsetsresetCS;
static Shader		sphdensityCS;
static Shader		sphforceCS;
static Shader		simulateCS;
static Shader		simulateCS_SORTING;
static Shader		simulateCS_DEPTHCOLLISIONS;
static Shader		simulateCS_SORTING_DEPTHCOLLISIONS;

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
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &particleBuffer);

	// Alive index lists (double buffered):
	bd.StructureByteStride = sizeof(uint32_t);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &aliveList[0]);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &aliveList[1]);

	// Dead index list:
	std::vector<uint32_t> indices(MAX_PARTICLES);
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		indices[i] = i;
	}
	data.pSysMem = indices.data();
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, &deadList);
	data.pSysMem = nullptr;

	// Distance buffer:
	bd.StructureByteStride = sizeof(float);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	std::vector<float> distances(MAX_PARTICLES);
	std::fill(distances.begin(), distances.end(), 0.0f);
	data.pSysMem = distances.data();
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, &distanceBuffer);
	data.pSysMem = nullptr;

	// SPH Partitioning grid indices per particle:
	bd.StructureByteStride = sizeof(float); // really, it is uint, but sorting is performing comparisons on floats, so whateva
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &sphPartitionCellIndices);

	// SPH Partitioning grid cell offsets into particle index list:
	bd.StructureByteStride = sizeof(uint32_t);
	bd.ByteWidth = bd.StructureByteStride * SPH_PARTITION_BUCKET_COUNT;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &sphPartitionCellOffsets);

	// Density buffer (for SPH simulation):
	bd.StructureByteStride = sizeof(float);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &densityBuffer);

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
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, &counterBuffer);
	data.pSysMem = nullptr;

	// Indirect Execution buffer:
	bd.BindFlags = BIND_UNORDERED_ACCESS;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
	bd.ByteWidth = 
		sizeof(wiGraphics::IndirectDispatchArgs) + 
		sizeof(wiGraphics::IndirectDispatchArgs) + 
		sizeof(wiGraphics::IndirectDrawArgsInstanced);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &indirectBuffers);

	// Constant buffer:
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(EmittedParticleCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &constantBuffer);

	// Debug information CPU-readback buffer:
	{
		GPUBufferDesc debugBufDesc = counterBuffer.GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		for (int i = 0; i < arraysize(statisticsReadbackBuffer); ++i)
		{
			wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, &statisticsReadbackBuffer[i]);
		}
	}

}

uint32_t wiEmittedParticle::GetMemorySizeInBytes() const
{
	if (!particleBuffer.IsValid())
		return 0;

	uint32_t retVal = 0;

	retVal += particleBuffer.GetDesc().ByteWidth;
	retVal += aliveList[0].GetDesc().ByteWidth;
	retVal += aliveList[1].GetDesc().ByteWidth;
	retVal += deadList.GetDesc().ByteWidth;
	retVal += distanceBuffer.GetDesc().ByteWidth;
	retVal += sphPartitionCellIndices.GetDesc().ByteWidth;
	retVal += sphPartitionCellOffsets.GetDesc().ByteWidth;
	retVal += densityBuffer.GetDesc().ByteWidth;
	retVal += counterBuffer.GetDesc().ByteWidth;
	retVal += indirectBuffers.GetDesc().ByteWidth;
	retVal += constantBuffer.GetDesc().ByteWidth;

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
	std::swap(aliveList[0], aliveList[1]);

	// Read back statistics (with GPU delay):
	if (statisticsReadBackIndex > arraysize(statisticsReadbackBuffer))
	{
		const uint32_t oldest_stat_index = (statisticsReadBackIndex + 1) % arraysize(statisticsReadbackBuffer);
		GraphicsDevice* device = wiRenderer::GetDevice();
		Mapping mapping;
		mapping._flags = Mapping::FLAG_READ;
		mapping.size = sizeof(statistics);
		device->Map(&statisticsReadbackBuffer[oldest_stat_index], &mapping);
		if (mapping.data != nullptr)
		{
			memcpy(&statistics, mapping.data, sizeof(statistics));
			device->Unmap(&statisticsReadbackBuffer[oldest_stat_index]);
		}
	}
	statisticsReadBackIndex++;
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

void wiEmittedParticle::UpdateGPU(const TransformComponent& transform, const MaterialComponent& material, const MeshComponent* mesh, CommandList cmd) const
{
	if (!particleBuffer.IsValid())
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
		cb.xEmitterFramesXY = uint2(std::max(1u, framesX), std::max(1u, framesY));
		cb.xEmitterFrameCount = std::max(1u, frameCount);
		cb.xEmitterFrameStart = frameStart;
		cb.xEmitterTexMul = float2(1.0f / (float)cb.xEmitterFramesXY.x, 1.0f / (float)cb.xEmitterFramesXY.y);
		cb.xEmitterFrameRate = frameRate;

		cb.xEmitterOptions = 0;
		if (IsSPHEnabled())
		{
			cb.xEmitterOptions |= EMITTER_OPTION_BIT_SPH_ENABLED;
		}
		if (IsFrameBlendingEnabled())
		{
			cb.xEmitterOptions |= EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED;
		}
		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
		{
			cb.xEmitterOptions |= EMITTER_OPTION_BIT_MESH_SHADER_ENABLED;
		}

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

		device->UpdateBuffer(&constantBuffer, &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);

		const GPUResource* uavs[] = {
			&particleBuffer,
			&aliveList[0], // CURRENT alivelist
			&aliveList[1], // NEW alivelist
			&deadList,
			&counterBuffer,
			&indirectBuffers,
			&distanceBuffer,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const GPUResource* resources[] = {
			mesh == nullptr ? nullptr : &mesh->indexBuffer,
			mesh == nullptr ? nullptr : (mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS),
		};
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, arraysize(resources), cmd);

		GPUBarrier barrier_indirect_uav = GPUBarrier::Buffer(&indirectBuffers, BUFFER_STATE_INDIRECT_ARGUMENT, BUFFER_STATE_UNORDERED_ACCESS);
		GPUBarrier barrier_uav_indirect = GPUBarrier::Buffer(&indirectBuffers, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT);
		GPUBarrier barrier_memory = GPUBarrier::Memory();

		device->Barrier(&barrier_indirect_uav, 1, cmd);

		// kick off updating, set up state
		device->EventBegin("KickOff Update", cmd);
		device->BindComputeShader(&kickoffUpdateCS, cmd);
		device->Dispatch(1, 1, 1, cmd);
		device->Barrier(&barrier_memory, 1, cmd);
		device->EventEnd(cmd);

		device->Barrier(&barrier_uav_indirect, 1, cmd);

		// emit the required amount if there are free slots in dead list
		device->EventBegin("Emit", cmd);
		device->BindComputeShader(mesh == nullptr ? (IsVolumeEnabled() ? &emitCS_VOLUME : &emitCS) : &emitCS_FROMMESH, cmd);
		device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHEMIT, cmd);
		device->Barrier(&barrier_memory, 1, cmd);
		device->EventEnd(cmd);

		if (IsSPHEnabled())
		{
			auto range = wiProfiler::BeginRangeGPU("SPH - Simulation", cmd);

			// Smooth Particle Hydrodynamics:
			device->EventBegin("SPH - Simulation", cmd);

#ifdef SPH_USE_ACCELERATION_GRID
			// 1.) Assign particles into partitioning grid:
			device->EventBegin("Partitioning", cmd);
			device->BindComputeShader(&sphpartitionCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			const GPUResource* res_partition[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&particleBuffer,
			};
			device->BindResources(CS, res_partition, 0, arraysize(res_partition), cmd);
			const GPUResource* uav_partition[] = {
				&sphPartitionCellIndices,
			};
			device->BindUAVs(CS, uav_partition, 0, arraysize(uav_partition), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

			// 2.) Sort particle index list based on partition grid cell index:
			wiGPUSortLib::Sort(MAX_PARTICLES, sphPartitionCellIndices, counterBuffer, PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveList[0], cmd);

			// 3.) Reset grid cell offset buffer with invalid offsets (max uint):
			device->EventBegin("PartitionOffsetsReset", cmd);
			device->BindComputeShader(&sphpartitionoffsetsresetCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			const GPUResource* uav_partitionoffsets[] = {
				&sphPartitionCellOffsets,
			};
			device->BindUAVs(CS, uav_partitionoffsets, 0, arraysize(uav_partitionoffsets), cmd);
			device->Dispatch((uint32_t)ceilf((float)SPH_PARTITION_BUCKET_COUNT / (float)THREADCOUNT_SIMULATION), 1, 1, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

			// 4.) Assemble grid cell offsets from the sorted particle index list <--> grid cell index list connection:
			device->EventBegin("PartitionOffsets", cmd);
			device->BindComputeShader(&sphpartitionoffsetsCS, cmd);
			const GPUResource* res_partitionoffsets[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&sphPartitionCellIndices,
			};
			device->BindResources(CS, res_partitionoffsets, 0, arraysize(res_partitionoffsets), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

#endif // SPH_USE_ACCELERATION_GRID

			// 5.) Compute particle density field:
			device->EventBegin("Density Evaluation", cmd);
			device->BindComputeShader(&sphdensityCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			const GPUResource* res_density[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&particleBuffer,
				&sphPartitionCellIndices,
				&sphPartitionCellOffsets,
			};
			device->BindResources(CS, res_density, 0, arraysize(res_density), cmd);
			const GPUResource* uav_density[] = {
				&densityBuffer
			};
			device->BindUAVs(CS, uav_density, 0, arraysize(uav_density), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

			// 6.) Compute particle pressure forces:
			device->EventBegin("Force Evaluation", cmd);
			device->BindComputeShader(&sphforceCS, cmd);
			device->UnbindUAVs(0, 8, cmd);
			const GPUResource* res_force[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&densityBuffer,
				&sphPartitionCellIndices,
				&sphPartitionCellOffsets,
			};
			device->BindResources(CS, res_force, 0, arraysize(res_force), cmd);
			const GPUResource* uav_force[] = {
				&particleBuffer,
			};
			device->BindUAVs(CS, uav_force, 0, arraysize(uav_force), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
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
				device->BindComputeShader(&simulateCS_SORTING_DEPTHCOLLISIONS, cmd);
			}
			else
			{
				device->BindComputeShader(&simulateCS_SORTING, cmd);
			}
		}
		else
		{
			if (IsDepthCollisionEnabled())
			{
				device->BindComputeShader(&simulateCS_DEPTHCOLLISIONS, cmd);
			}
			else
			{
				device->BindComputeShader(&simulateCS, cmd);
			}
		}
		device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
		device->Barrier(&barrier_memory, 1, cmd);
		device->EventEnd(cmd);


		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(resources), cmd);

		device->EventEnd(cmd);

	}

	if (IsSorted())
	{
		wiGPUSortLib::Sort(MAX_PARTICLES, distanceBuffer, counterBuffer, PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, aliveList[1], cmd);
	}

	if (!IsPaused())
	{
		// finish updating, update draw argument buffer:
		device->EventBegin("FinishUpdate", cmd);
		device->BindComputeShader(&finishUpdateCS, cmd);

		const GPUResource* res[] = {
			&counterBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&indirectBuffers,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barrier_memory = GPUBarrier::Memory();
		device->Barrier(&barrier_memory, 1, cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(res), cmd);
		device->EventEnd(cmd);


		const GPUBarrier barriers[] = {
			GPUBarrier::Buffer(&particleBuffer, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
			GPUBarrier::Buffer(&aliveList[1], BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	// Statistics is copied to readback:
	device->CopyResource(&statisticsReadbackBuffer[(statisticsReadBackIndex - 1) % arraysize(statisticsReadbackBuffer)], &counterBuffer, cmd);
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
		device->BindShadingRate(material.shadingRate, cmd);
	}

	device->BindConstantBuffer(VS, &constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);
	device->BindConstantBuffer(PS, &constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
	{
		const GPUResource* res[] = {
			&counterBuffer,
			&particleBuffer,
			&aliveList[1], // NEW aliveList
		};
		device->BindResources(MS, res, TEXSLOT_ONDEMAND20, arraysize(res), cmd);
		device->DispatchMeshIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, cmd);
	}
	else
	{
		const GPUResource* res[] = {
			&particleBuffer,
			&aliveList[1] // NEW aliveList
		};
		device->BindResources(VS, res, TEXSLOT_ONDEMAND21, arraysize(res), cmd);
		device->DrawInstancedIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, cmd);
	}

	device->EventEnd(cmd);
}


namespace wiEmittedParticle_Internal
{
	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(VS, vertexShader, "emittedparticleVS.cso");

		if (wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
		{
			wiRenderer::LoadShader(MS, meshShader, "emittedparticleMS.cso");
		}

		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SOFT], "emittedparticlePS_soft.cso");
		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SOFT_DISTORTION], "emittedparticlePS_soft_distortion.cso");
		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SIMPLEST], "emittedparticlePS_simplest.cso");
		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SOFT_LIGHTING], "emittedparticlePS_soft_lighting.cso");

		wiRenderer::LoadShader(CS, kickoffUpdateCS, "emittedparticle_kickoffUpdateCS.cso");
		wiRenderer::LoadShader(CS, finishUpdateCS, "emittedparticle_finishUpdateCS.cso");
		wiRenderer::LoadShader(CS, emitCS, "emittedparticle_emitCS.cso");
		wiRenderer::LoadShader(CS, emitCS_VOLUME, "emittedparticle_emitCS_volume.cso");
		wiRenderer::LoadShader(CS, emitCS_FROMMESH, "emittedparticle_emitCS_FROMMESH.cso");
		wiRenderer::LoadShader(CS, sphpartitionCS, "emittedparticle_sphpartitionCS.cso");
		wiRenderer::LoadShader(CS, sphpartitionoffsetsCS, "emittedparticle_sphpartitionoffsetsCS.cso");
		wiRenderer::LoadShader(CS, sphpartitionoffsetsresetCS, "emittedparticle_sphpartitionoffsetsresetCS.cso");
		wiRenderer::LoadShader(CS, sphdensityCS, "emittedparticle_sphdensityCS.cso");
		wiRenderer::LoadShader(CS, sphforceCS, "emittedparticle_sphforceCS.cso");
		wiRenderer::LoadShader(CS, simulateCS, "emittedparticle_simulateCS.cso");
		wiRenderer::LoadShader(CS, simulateCS_SORTING, "emittedparticle_simulateCS_SORTING.cso");
		wiRenderer::LoadShader(CS, simulateCS_DEPTHCOLLISIONS, "emittedparticle_simulateCS_DEPTHCOLLISIONS.cso");
		wiRenderer::LoadShader(CS, simulateCS_SORTING_DEPTHCOLLISIONS, "emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.cso");


		GraphicsDevice* device = wiRenderer::GetDevice();

		for (int i = 0; i < BLENDMODE_COUNT; ++i)
		{
			PipelineStateDesc desc;
			if (wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
			{
				desc.ms = &meshShader;
			}
			else
			{
				desc.vs = &vertexShader;
			}
			desc.bs = &blendStates[i];
			desc.rs = &rasterizerState;
			desc.dss = &depthStencilState;

			for (int j = 0; j < wiEmittedParticle::PARTICLESHADERTYPE_COUNT; ++j)
			{
				desc.ps = &pixelShader[j];
				device->CreatePipelineState(&desc, &PSO[i][j]);
			}
		}

		{
			PipelineStateDesc desc;
			if (wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
			{
				desc.ms = &meshShader;
			}
			else
			{
				desc.vs = &vertexShader;
			}
			desc.ps = &pixelShader[wiEmittedParticle::SIMPLEST];
			desc.bs = &blendStates[BLENDMODE_ALPHA];
			desc.rs = &wireFrameRS;
			desc.dss = &depthStencilState;

			device->CreatePipelineState(&desc, &PSO_wire);
		}

	}
}

void wiEmittedParticle::Initialize()
{

	RasterizerStateDesc rs;
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
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

	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { wiEmittedParticle_Internal::LoadShaders(); });
	wiEmittedParticle_Internal::LoadShaders();

	wiBackLog::post("wiEmittedParticle Initialized");
}


void wiEmittedParticle::Serialize(wiArchive& archive, wiECS::Entity seed)
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

		if (archive.GetVersion() >= 45)
		{
			archive >> framesX;
			archive >> framesY;
			archive >> frameCount;
			archive >> frameStart;
			archive >> frameRate;
		}

		if (archive.GetVersion() == 48)
		{
			uint8_t shadingRate;
			archive >> shadingRate; // no longer needed
		}
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

		if (archive.GetVersion() >= 45)
		{
			archive << framesX;
			archive << framesY;
			archive << frameCount;
			archive << frameStart;
			archive << frameRate;
		}
	}
}

}
