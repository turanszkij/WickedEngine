#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiScene.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiIntersect.h"
#include "wiRandom.h"
#include "shaders/ResourceMapping.h"
#include "wiArchive.h"
#include "wiTextureHelper.h"
#include "wiGPUSortLib.h"
#include "wiProfiler.h"
#include "wiBackLog.h"
#include "wiEvent.h"
#include "wiTimer.h"

#include <algorithm>

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

static bool ALLOW_MESH_SHADER = false;


void wiEmittedParticle::SetMaxParticleCount(uint32_t value)
{
	MAX_PARTICLES = value;
	counterBuffer = {}; // will be recreated
}

void wiEmittedParticle::CreateSelfBuffers()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	if (particleBuffer.desc.Size < MAX_PARTICLES * sizeof(Particle))
	{
		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;

		// Particle buffer:
		bd.Stride = sizeof(Particle);
		bd.Size = bd.Stride * MAX_PARTICLES;
		device->CreateBuffer(&bd, nullptr, &particleBuffer);

		// Alive index lists (double buffered):
		bd.Stride = sizeof(uint32_t);
		bd.Size = bd.Stride * MAX_PARTICLES;
		device->CreateBuffer(&bd, nullptr, &aliveList[0]);
		device->CreateBuffer(&bd, nullptr, &aliveList[1]);

		// Dead index list:
		std::vector<uint32_t> indices(MAX_PARTICLES);
		for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
		{
			indices[i] = i;
		}
		device->CreateBuffer(&bd, indices.data(), &deadList);
	}

	if (IsSorted() && distanceBuffer.desc.Size < MAX_PARTICLES * sizeof(float))
	{
		// Distance buffer:
		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Stride = sizeof(float);
		bd.Size = bd.Stride * MAX_PARTICLES;
		std::vector<float> distances(MAX_PARTICLES);
		std::fill(distances.begin(), distances.end(), 0.0f);
		device->CreateBuffer(&bd, distances.data(), &distanceBuffer);
	}

	if (IsSPHEnabled())
	{
		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;

		if (sphPartitionCellIndices.desc.Size < MAX_PARTICLES * sizeof(float))
		{
			// SPH Partitioning grid indices per particle:
			bd.Stride = sizeof(float); // really, it is uint, but sorting is performing comparisons on floats, so whateva
			bd.Size = bd.Stride * MAX_PARTICLES;
			device->CreateBuffer(&bd, nullptr, &sphPartitionCellIndices);

			// Density buffer (for SPH simulation):
			bd.Stride = sizeof(float);
			bd.Size = bd.Stride * MAX_PARTICLES;
			device->CreateBuffer(&bd, nullptr, &densityBuffer);
		}

		if (sphPartitionCellOffsets.desc.Size < SPH_PARTITION_BUCKET_COUNT * sizeof(uint32_t))
		{
			// SPH Partitioning grid cell offsets into particle index list:
			bd.Stride = sizeof(uint32_t);
			bd.Size = bd.Stride * SPH_PARTITION_BUCKET_COUNT;
			device->CreateBuffer(&bd, nullptr, &sphPartitionCellOffsets);
		}
	}
	else
	{
		sphPartitionCellIndices = {};
		densityBuffer = {};
		sphPartitionCellOffsets = {};
	}

	if (!counterBuffer.IsValid())
	{
		// Particle System statistics:
		ParticleCounters counters;
		counters.aliveCount = 0;
		counters.deadCount = MAX_PARTICLES;
		counters.realEmitCount = 0;
		counters.aliveCount_afterSimulation = 0;

		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Size = sizeof(counters);
		bd.Stride = sizeof(counters);
		bd.MiscFlags = RESOURCE_MISC_BUFFER_RAW;
		device->CreateBuffer(&bd, &counters, &counterBuffer);
	}

	if(!indirectBuffers.IsValid())
	{
		GPUBufferDesc bd;

		// Indirect Execution buffer:
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_RAW | RESOURCE_MISC_INDIRECT_ARGS;
		bd.Size =
			sizeof(wiGraphics::IndirectDispatchArgs) +
			sizeof(wiGraphics::IndirectDispatchArgs) +
			sizeof(wiGraphics::IndirectDrawArgsInstanced);
		device->CreateBuffer(&bd, nullptr, &indirectBuffers);

		// Constant buffer:
		bd.Usage = USAGE_DEFAULT;
		bd.Size = sizeof(EmittedParticleCB);
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.MiscFlags = RESOURCE_MISC_NONE;
		device->CreateBuffer(&bd, nullptr, &constantBuffer);

		// Debug information CPU-readback buffer:
		{
			GPUBufferDesc debugBufDesc = counterBuffer.GetDesc();
			debugBufDesc.Usage = USAGE_READBACK;
			debugBufDesc.BindFlags = BIND_NONE;
			debugBufDesc.MiscFlags = RESOURCE_MISC_NONE;
			for (int i = 0; i < arraysize(statisticsReadbackBuffer); ++i)
			{
				device->CreateBuffer(&debugBufDesc, nullptr, &statisticsReadbackBuffer[i]);
			}
		}
	}

}

uint64_t wiEmittedParticle::GetMemorySizeInBytes() const
{
	if (!particleBuffer.IsValid())
		return 0;

	uint64_t retVal = 0;

	retVal += particleBuffer.GetDesc().Size;
	retVal += aliveList[0].GetDesc().Size;
	retVal += aliveList[1].GetDesc().Size;
	retVal += deadList.GetDesc().Size;
	retVal += distanceBuffer.GetDesc().Size;
	retVal += sphPartitionCellIndices.GetDesc().Size;
	retVal += sphPartitionCellOffsets.GetDesc().Size;
	retVal += densityBuffer.GetDesc().Size;
	retVal += counterBuffer.GetDesc().Size;
	retVal += indirectBuffers.GetDesc().Size;
	retVal += constantBuffer.GetDesc().Size;

	return retVal;
}

void wiEmittedParticle::UpdateCPU(const TransformComponent& transform, float dt)
{
	CreateSelfBuffers();

	if (IsPaused())
		return;

	emit = std::max(0.0f, emit - floorf(emit));

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
		memcpy(&statistics, statisticsReadbackBuffer[oldest_stat_index].mapped_data, sizeof(statistics));
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
	SetPaused(false);
	counterBuffer = {}; // will be recreated
}

void wiEmittedParticle::UpdateGPU(uint32_t materialIndex, const TransformComponent& transform, const MeshComponent* mesh, CommandList cmd) const
{
	if (!particleBuffer.IsValid())
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("UpdateEmittedParticles", cmd);

	if (!IsPaused())
	{
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
		cb.xParticleMass = mass;
		cb.xEmitterMaxParticleCount = MAX_PARTICLES;
		cb.xEmitterFixedTimestep = FIXED_TIMESTEP;
		cb.xEmitterFramesXY = uint2(std::max(1u, framesX), std::max(1u, framesY));
		cb.xEmitterFrameCount = std::max(1u, frameCount);
		cb.xEmitterFrameStart = frameStart;
		cb.xEmitterTexMul = float2(1.0f / (float)cb.xEmitterFramesXY.x, 1.0f / (float)cb.xEmitterFramesXY.y);
		cb.xEmitterFrameRate = frameRate;
		cb.xParticleGravity = gravity;
		cb.xParticleDrag = drag;
		XMStoreFloat3(&cb.xParticleVelocity, XMVector3TransformNormal(XMLoadFloat3(&velocity), XMLoadFloat4x4(&transform.world)));
		cb.xParticleRandomColorFactor = random_color;
		cb.xEmitterLayerMask = layerMask;
		cb.xEmitterMaterialIndex = materialIndex;

		cb.xEmitterOptions = 0;
		if (IsSPHEnabled())
		{
			cb.xEmitterOptions |= EMITTER_OPTION_BIT_SPH_ENABLED;
		}
		if (IsFrameBlendingEnabled())
		{
			cb.xEmitterOptions |= EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED;
		}
		if (ALLOW_MESH_SHADER && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&constantBuffer, RESOURCE_STATE_CONSTANT_BUFFER, RESOURCE_STATE_COPY_DST),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->UpdateBuffer(&constantBuffer, &cb, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&constantBuffer, RESOURCE_STATE_COPY_DST, RESOURCE_STATE_CONSTANT_BUFFER),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindConstantBuffer(&constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);

		const GPUResource* uavs[] = {
			&particleBuffer,
			&aliveList[0], // CURRENT alivelist
			&aliveList[1], // NEW alivelist
			&deadList,
			&counterBuffer,
			&indirectBuffers,
			&distanceBuffer,
		};
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		if (mesh != nullptr)
		{
			const GPUResource* resources[] = {
				&mesh->indexBuffer,
				(mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS),
			};
			device->BindResources(resources, TEXSLOT_ONDEMAND0, arraysize(resources), cmd);
		}

		GPUBarrier barrier_indirect_uav = GPUBarrier::Buffer(&indirectBuffers, RESOURCE_STATE_INDIRECT_ARGUMENT, RESOURCE_STATE_UNORDERED_ACCESS);
		GPUBarrier barrier_uav_indirect = GPUBarrier::Buffer(&indirectBuffers, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT);
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
			const GPUResource* res_partition[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&particleBuffer,
			};
			device->BindResources(res_partition, 0, arraysize(res_partition), cmd);
			const GPUResource* uav_partition[] = {
				&sphPartitionCellIndices,
			};
			device->BindUAVs(uav_partition, 0, arraysize(uav_partition), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

			// 2.) Sort particle index list based on partition grid cell index:
			wiGPUSortLib::Sort(MAX_PARTICLES, sphPartitionCellIndices, counterBuffer, PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveList[0], cmd);

			// 3.) Reset grid cell offset buffer with invalid offsets (max uint):
			device->EventBegin("PartitionOffsetsReset", cmd);
			device->BindComputeShader(&sphpartitionoffsetsresetCS, cmd);
			const GPUResource* uav_partitionoffsets[] = {
				&sphPartitionCellOffsets,
			};
			device->BindUAVs(uav_partitionoffsets, 0, arraysize(uav_partitionoffsets), cmd);
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
			device->BindResources(res_partitionoffsets, 0, arraysize(res_partitionoffsets), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

#endif // SPH_USE_ACCELERATION_GRID

			// 5.) Compute particle density field:
			device->EventBegin("Density Evaluation", cmd);
			device->BindComputeShader(&sphdensityCS, cmd);
			const GPUResource* res_density[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&particleBuffer,
				&sphPartitionCellIndices,
				&sphPartitionCellOffsets,
			};
			device->BindResources(res_density, 0, arraysize(res_density), cmd);
			const GPUResource* uav_density[] = {
				&densityBuffer
			};
			device->BindUAVs(uav_density, 0, arraysize(uav_density), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);

			// 6.) Compute particle pressure forces:
			device->EventBegin("Force Evaluation", cmd);
			device->BindComputeShader(&sphforceCS, cmd);
			const GPUResource* res_force[] = {
				&aliveList[0], // CURRENT alivelist
				&counterBuffer,
				&densityBuffer,
				&sphPartitionCellIndices,
				&sphPartitionCellOffsets,
			};
			device->BindResources(res_force, 0, arraysize(res_force), cmd);
			const GPUResource* uav_force[] = {
				&particleBuffer,
			};
			device->BindUAVs(uav_force, 0, arraysize(uav_force), cmd);
			device->DispatchIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, cmd);
			device->Barrier(&barrier_memory, 1, cmd);
			device->EventEnd(cmd);


			device->EventEnd(cmd);

			wiProfiler::EndRange(range);
		}

		device->EventBegin("Simulate", cmd);
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

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
		device->BindResources(res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&indirectBuffers,
		};
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Buffer(&counterBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&indirectBuffers, RESOURCE_STATE_INDIRECT_ARGUMENT, RESOURCE_STATE_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(1, 1, 1, cmd);

		device->EventEnd(cmd);

	}

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Buffer(&counterBuffer, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_COPY_SRC),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	// Statistics is copied to readback:
	device->CopyResource(&statisticsReadbackBuffer[(statisticsReadBackIndex - 1) % arraysize(statisticsReadbackBuffer)], &counterBuffer, cmd);

	{
		const GPUBarrier barriers[] = {
			GPUBarrier::Buffer(&indirectBuffers, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT),
			GPUBarrier::Buffer(&counterBuffer, RESOURCE_STATE_COPY_SRC, RESOURCE_STATE_SHADER_RESOURCE),
			GPUBarrier::Buffer(&particleBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE),
			GPUBarrier::Buffer(&aliveList[1], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->EventEnd(cmd);
}


void wiEmittedParticle::Draw(const MaterialComponent& material, CommandList cmd) const
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

		device->BindShadingRate(material.shadingRate, cmd);
	}

	device->BindConstantBuffer(&constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);

	if (ALLOW_MESH_SHADER && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
	{
		const GPUResource* res[] = {
			&counterBuffer,
			&particleBuffer,
			&aliveList[1], // NEW aliveList
		};
		device->BindResources(res, TEXSLOT_ONDEMAND20, arraysize(res), cmd);
		device->DispatchMeshIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, cmd);
	}
	else
	{
		const GPUResource* res[] = {
			&particleBuffer,
			&aliveList[1] // NEW aliveList
		};
		device->BindResources(res, TEXSLOT_ONDEMAND21, arraysize(res), cmd);
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

		if (ALLOW_MESH_SHADER && wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
		{
			wiRenderer::LoadShader(MS, meshShader, "emittedparticleMS.cso");
		}

		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SOFT], "emittedparticlePS_soft.cso");
		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SOFT_DISTORTION], "emittedparticlePS_soft_distortion.cso");
		wiRenderer::LoadShader(PS, pixelShader[wiEmittedParticle::SIMPLE], "emittedparticlePS_simple.cso");
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
			desc.pt = TRIANGLESTRIP;
			if (ALLOW_MESH_SHADER && wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
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
			desc.pt = TRIANGLESTRIP;
			if (ALLOW_MESH_SHADER && wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
			{
				desc.ms = &meshShader;
			}
			else
			{
				desc.vs = &vertexShader;
			}
			desc.ps = &pixelShader[wiEmittedParticle::SIMPLE];
			desc.bs = &blendStates[BLENDMODE_ALPHA];
			desc.rs = &wireFrameRS;
			desc.dss = &depthStencilState;

			device->CreatePipelineState(&desc, &PSO_wire);
		}

	}
}

void wiEmittedParticle::Initialize()
{
	wiTimer timer;

	RasterizerState rs;
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rasterizerState = rs;


	rs.FillMode = FILL_WIREFRAME;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	wireFrameRS = rs;


	DepthStencilState dsd;
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER_EQUAL;
	dsd.StencilEnable = false;
	depthStencilState = dsd;


	BlendState bd;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStates[BLENDMODE_ALPHA] = bd;

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStates[BLENDMODE_ADDITIVE] = bd;

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	blendStates[BLENDMODE_PREMULTIPLIED] = bd;

	bd.RenderTarget[0].BlendEnable = false;
	blendStates[BLENDMODE_OPAQUE] = bd;

	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { wiEmittedParticle_Internal::LoadShaders(); });
	wiEmittedParticle_Internal::LoadShaders();

	wiBackLog::post("wiEmittedParticle Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
}


void wiEmittedParticle::Serialize(wiArchive& archive, wiECS::EntitySerializer& seri)
{
	if (archive.IsReadMode())
	{
		archive >> _flags;
		archive >> (uint32_t&)shaderType;
		wiECS::SerializeEntity(archive, meshID, seri);
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

		if (archive.GetVersion() >= 64)
		{
			archive >> velocity;
			archive >> gravity;
			archive >> drag;
			archive >> random_color;
		}
		else
		{
			if (IsSPHEnabled())
			{
				gravity = XMFLOAT3(0, -9.8f * 2, 0);
				drag = 0.98f;
			}
		}
	}
	else
	{
		archive << _flags;
		archive << (uint32_t)shaderType;
		wiECS::SerializeEntity(archive, meshID, seri);
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

		if (archive.GetVersion() >= 64)
		{
			archive << velocity;
			archive << gravity;
			archive << drag;
			archive << random_color;
		}
	}
}

}
