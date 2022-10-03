#include "wiEmittedParticle.h"
#include "wiScene.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiPrimitive.h"
#include "wiRandom.h"
#include "wiArchive.h"
#include "wiTextureHelper.h"
#include "wiGPUSortLib.h"
#include "wiProfiler.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiTimer.h"
#include "wiVector.h"

#include <algorithm>

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::enums;

namespace wi
{
	static Shader vertexShader;
	static Shader meshShader;
	static Shader pixelShader[EmittedParticleSystem::PARTICLESHADERTYPE_COUNT];
	static Shader kickoffUpdateCS;
	static Shader finishUpdateCS;
	static Shader emitCS;
	static Shader emitCS_VOLUME;
	static Shader emitCS_FROMMESH;
	static Shader sphpartitionCS;
	static Shader sphpartitionoffsetsCS;
	static Shader sphpartitionoffsetsresetCS;
	static Shader sphdensityCS;
	static Shader sphforceCS;
	static Shader simulateCS;
	static Shader simulateCS_SORTING;
	static Shader simulateCS_DEPTHCOLLISIONS;
	static Shader simulateCS_SORTING_DEPTHCOLLISIONS;

	static BlendState			blendStates[BLENDMODE_COUNT];
	static RasterizerState		rasterizerState;
	static RasterizerState		wireFrameRS;
	static DepthStencilState	depthStencilState;
	static PipelineState		PSO[BLENDMODE_COUNT][EmittedParticleSystem::PARTICLESHADERTYPE_COUNT];
	static PipelineState		PSO_wire;

	static bool ALLOW_MESH_SHADER = false;


	void EmittedParticleSystem::SetMaxParticleCount(uint32_t value)
	{
		MAX_PARTICLES = value;
		counterBuffer = {}; // will be recreated
	}

	void EmittedParticleSystem::CreateSelfBuffers()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		if (particleBuffer.desc.size < MAX_PARTICLES * sizeof(Particle))
		{
			BLAS = {};

			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;

			// Particle buffer:
			bd.stride = sizeof(Particle);
			bd.size = bd.stride * MAX_PARTICLES;
			device->CreateBuffer(&bd, nullptr, &particleBuffer);
			device->SetName(&particleBuffer, "EmittedParticleSystem::particleBuffer");

			// Alive index lists (double buffered):
			bd.stride = sizeof(uint32_t);
			bd.size = bd.stride * MAX_PARTICLES;
			device->CreateBuffer(&bd, nullptr, &aliveList[0]);
			device->SetName(&aliveList[0], "EmittedParticleSystem::aliveList[0]");
			device->CreateBuffer(&bd, nullptr, &aliveList[1]);
			device->SetName(&aliveList[1], "EmittedParticleSystem::aliveList[1]");
			device->CreateBuffer(&bd, nullptr, &culledIndirectionBuffer);
			device->SetName(&culledIndirectionBuffer, "EmittedParticleSystem::culledIndirectionBuffer");
			device->CreateBuffer(&bd, nullptr, &culledIndirectionBuffer2);
			device->SetName(&culledIndirectionBuffer2, "EmittedParticleSystem::culledIndirectionBuffer2");

			// Dead index list:
			wi::vector<uint32_t> indices(MAX_PARTICLES);
			for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
			{
				indices[i] = i;
			}
			device->CreateBuffer(&bd, indices.data(), &deadList);
			device->SetName(&deadList, "EmittedParticleSystem::deadList");


			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}
			bd.stride = sizeof(MeshComponent::Vertex_POS);
			bd.size = bd.stride * 4 * MAX_PARTICLES;
			wi::vector<MeshComponent::Vertex_POS> positionData(4 * MAX_PARTICLES);
			std::fill(positionData.begin(), positionData.end(), MeshComponent::Vertex_POS());
			device->CreateBuffer(&bd, positionData.data(), &vertexBuffer_POS);
			device->SetName(&vertexBuffer_POS, "EmittedParticleSystem::vertexBuffer_POS");

			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(MeshComponent::Vertex_UVS);
			bd.size = bd.stride * 4 * MAX_PARTICLES;
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_UVS);
			device->SetName(&vertexBuffer_UVS, "EmittedParticleSystem::vertexBuffer_UVS");

			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(MeshComponent::Vertex_COL);
			bd.size = bd.stride * 4 * MAX_PARTICLES;
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_COL);
			device->SetName(&vertexBuffer_COL, "EmittedParticleSystem::vertexBuffer_COL");

			bd.bind_flags = BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::NONE;
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}
			bd.format = Format::R32_UINT;
			bd.stride = sizeof(uint);
			bd.size = bd.stride * 6 * MAX_PARTICLES;
			wi::vector<uint> primitiveData(6 * MAX_PARTICLES);
			for (uint particleID = 0; particleID < MAX_PARTICLES; ++particleID)
			{
				uint v0 = particleID * 4;
				uint i0 = particleID * 6;
				primitiveData[i0 + 0] = v0 + 0;
				primitiveData[i0 + 1] = v0 + 1;
				primitiveData[i0 + 2] = v0 + 2;
				primitiveData[i0 + 3] = v0 + 2;
				primitiveData[i0 + 4] = v0 + 1;
				primitiveData[i0 + 5] = v0 + 3;
			}
			device->CreateBuffer(&bd, primitiveData.data(), &primitiveBuffer);
			device->SetName(&primitiveBuffer, "EmittedParticleSystem::primitiveBuffer");
		}

		if (IsSorted() && distanceBuffer.desc.size < MAX_PARTICLES * sizeof(float))
		{
			// Distance buffer:
			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			bd.stride = sizeof(float);
			bd.size = bd.stride * MAX_PARTICLES;
			wi::vector<float> distances(MAX_PARTICLES);
			std::fill(distances.begin(), distances.end(), 0.0f);
			device->CreateBuffer(&bd, distances.data(), &distanceBuffer);
			device->SetName(&distanceBuffer, "EmittedParticleSystem::distanceBuffer");
		}

		if (IsSPHEnabled())
		{
			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;

			if (sphPartitionCellIndices.desc.size < MAX_PARTICLES * sizeof(float))
			{
				// SPH Partitioning grid indices per particle:
				bd.stride = sizeof(float); // really, it is uint, but sorting is performing comparisons on floats, so whateva
				bd.size = bd.stride * MAX_PARTICLES;
				device->CreateBuffer(&bd, nullptr, &sphPartitionCellIndices);
				device->SetName(&sphPartitionCellIndices, "EmittedParticleSystem::sphPartitionCellIndices");

				// Density buffer (for SPH simulation):
				bd.stride = sizeof(float);
				bd.size = bd.stride * MAX_PARTICLES;
				device->CreateBuffer(&bd, nullptr, &densityBuffer);
				device->SetName(&densityBuffer, "EmittedParticleSystem::densityBuffer");
			}

			if (sphPartitionCellOffsets.desc.size < SPH_PARTITION_BUCKET_COUNT * sizeof(uint32_t))
			{
				// SPH Partitioning grid cell offsets into particle index list:
				bd.stride = sizeof(uint32_t);
				bd.size = bd.stride * SPH_PARTITION_BUCKET_COUNT;
				device->CreateBuffer(&bd, nullptr, &sphPartitionCellOffsets);
				device->SetName(&sphPartitionCellOffsets, "EmittedParticleSystem::sphPartitionCellOffsets");
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
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			bd.size = sizeof(counters);
			bd.stride = sizeof(counters);
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&bd, &counters, &counterBuffer);
			device->SetName(&counterBuffer, "EmittedParticleSystem::counterBuffer");
		}

		if (!indirectBuffers.IsValid())
		{
			GPUBufferDesc bd;

			// Indirect Execution buffer:
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
			bd.size =
				sizeof(wi::graphics::IndirectDispatchArgs) +
				sizeof(wi::graphics::IndirectDispatchArgs) +
				sizeof(wi::graphics::IndirectDrawArgsInstanced);
			device->CreateBuffer(&bd, nullptr, &indirectBuffers);
			device->SetName(&indirectBuffers, "EmittedParticleSystem::indirectBuffers");

			// Constant buffer:
			bd.usage = Usage::DEFAULT;
			bd.size = sizeof(EmittedParticleCB);
			bd.bind_flags = BindFlag::CONSTANT_BUFFER;
			bd.misc_flags = ResourceMiscFlag::NONE;
			device->CreateBuffer(&bd, nullptr, &constantBuffer);
			device->SetName(&constantBuffer, "EmittedParticleSystem::constantBuffer");

			// Debug information CPU-readback buffer:
			{
				GPUBufferDesc debugBufDesc = counterBuffer.GetDesc();
				debugBufDesc.usage = Usage::READBACK;
				debugBufDesc.bind_flags = BindFlag::NONE;
				debugBufDesc.misc_flags = ResourceMiscFlag::NONE;
				for (int i = 0; i < arraysize(statisticsReadbackBuffer); ++i)
				{
					device->CreateBuffer(&debugBufDesc, nullptr, &statisticsReadbackBuffer[i]);
					device->SetName(&statisticsReadbackBuffer[i], "EmittedParticleSystem::statisticsReadbackBuffer");
				}
			}
		}

	}

	void EmittedParticleSystem::CreateRaytracingRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING) && !BLAS.IsValid() && primitiveBuffer.IsValid())
		{
			RaytracingAccelerationStructureDesc desc;
			desc.type = RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL;
			desc.flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
			desc.flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;

			desc.bottom_level.geometries.emplace_back();
			auto& geometry = desc.bottom_level.geometries.back();
			geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES;
			geometry.triangles.vertex_buffer = vertexBuffer_POS;
			geometry.triangles.index_buffer = primitiveBuffer;
			geometry.triangles.index_format = IndexBufferFormat::UINT32;
			geometry.triangles.index_count = (uint32_t)(primitiveBuffer.desc.size / primitiveBuffer.desc.stride);
			geometry.triangles.index_offset = 0;
			geometry.triangles.vertex_count = (uint32_t)(vertexBuffer_POS.desc.size / vertexBuffer_POS.desc.stride);
			geometry.triangles.vertex_format = Format::R32G32B32_FLOAT;
			geometry.triangles.vertex_stride = sizeof(MeshComponent::Vertex_POS);

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
			assert(success);
			device->SetName(&BLAS, "EmittedParticleSystem::BLAS");
		}
	}

	uint64_t EmittedParticleSystem::GetMemorySizeInBytes() const
	{
		if (!particleBuffer.IsValid())
			return 0;

		uint64_t retVal = 0;

		retVal += particleBuffer.GetDesc().size;
		retVal += aliveList[0].GetDesc().size;
		retVal += aliveList[1].GetDesc().size;
		retVal += deadList.GetDesc().size;
		retVal += distanceBuffer.GetDesc().size;
		retVal += sphPartitionCellIndices.GetDesc().size;
		retVal += sphPartitionCellOffsets.GetDesc().size;
		retVal += densityBuffer.GetDesc().size;
		retVal += counterBuffer.GetDesc().size;
		retVal += indirectBuffers.GetDesc().size;
		retVal += constantBuffer.GetDesc().size;
		retVal += vertexBuffer_POS.GetDesc().size;
		retVal += vertexBuffer_UVS.GetDesc().size;
		retVal += vertexBuffer_COL.GetDesc().size;
		retVal += primitiveBuffer.GetDesc().size;
		retVal += culledIndirectionBuffer.GetDesc().size;
		retVal += culledIndirectionBuffer2.GetDesc().size;

		return retVal;
	}

	void EmittedParticleSystem::UpdateCPU(const TransformComponent& transform, float dt)
	{
		CreateSelfBuffers();

		if (IsPaused())
			return;

		emit = std::max(0.0f, emit - std::floor(emit));

		center = transform.GetPosition();

		emit += (float)count * dt;

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
	void EmittedParticleSystem::Burst(int num)
	{
		if (IsPaused())
			return;

		burst += num;
	}
	void EmittedParticleSystem::Restart()
	{
		SetPaused(false);
		counterBuffer = {}; // will be recreated
	}

	void EmittedParticleSystem::UpdateGPU(uint32_t instanceIndex, const TransformComponent& transform, const MeshComponent* mesh, CommandList cmd) const
	{
		if (!particleBuffer.IsValid())
		{
			return;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("UpdateEmittedParticles", cmd);

		if (!IsPaused())
		{
			EmittedParticleCB cb;
			cb.xEmitterWorld = transform.world;
			cb.xEmitCount = (uint32_t)emit;
			cb.xEmitterMeshIndexCount = mesh == nullptr ? 0 : (uint32_t)mesh->indices.size();
			cb.xEmitterMeshVertexPositionStride = sizeof(MeshComponent::Vertex_POS);
			cb.xEmitterRandomness = wi::random::GetRandom(0, 1000) * 0.001f;
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
			cb.xEmitterRestitution = restitution;
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
			cb.xEmitterInstanceIndex = instanceIndex;

			cb.xEmitterOptions = 0;
			if (IsSPHEnabled())
			{
				cb.xEmitterOptions |= EMITTER_OPTION_BIT_SPH_ENABLED;
			}
			if (IsFrameBlendingEnabled())
			{
				cb.xEmitterOptions |= EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED;
			}
			if (ALLOW_MESH_SHADER && device->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
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
			cb.xSPH_visc_constant = (45.0f / (XM_PI * h6));
			cb.xSPH_K = SPH_K;
			cb.xSPH_p0 = SPH_p0;
			cb.xSPH_e = SPH_e;

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Buffer(&constantBuffer, ResourceState::CONSTANT_BUFFER, ResourceState::COPY_DST),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}
			device->UpdateBuffer(&constantBuffer, &cb, cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Buffer(&constantBuffer, ResourceState::COPY_DST, ResourceState::CONSTANT_BUFFER),
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
				&vertexBuffer_POS,
				&vertexBuffer_UVS,
				&vertexBuffer_COL,
				&culledIndirectionBuffer,
				&culledIndirectionBuffer2,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			if (mesh != nullptr)
			{
				device->BindResource(&mesh->generalBuffer, 0, cmd, mesh->ib.subresource_srv);
				if (mesh->streamoutBuffer.IsValid())
				{
					device->BindResource(&mesh->streamoutBuffer, 1, cmd, mesh->so_pos_nor_wind.subresource_srv);
				}
				else
				{
					device->BindResource(&mesh->generalBuffer, 1, cmd, mesh->vb_pos_nor_wind.subresource_srv);
				}
			}

			GPUBarrier barrier_indirect_uav = GPUBarrier::Buffer(&indirectBuffers, ResourceState::INDIRECT_ARGUMENT, ResourceState::UNORDERED_ACCESS);
			GPUBarrier barrier_uav_indirect = GPUBarrier::Buffer(&indirectBuffers, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT);
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
				auto range = wi::profiler::BeginRangeGPU("SPH - Simulation", cmd);

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
				wi::gpusortlib::Sort(MAX_PARTICLES, sphPartitionCellIndices, counterBuffer, PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveList[0], cmd);

				// 3.) Reset grid cell offset buffer with invalid offsets (max uint):
				device->EventBegin("PartitionOffsetsReset", cmd);
				device->BindComputeShader(&sphpartitionoffsetsresetCS, cmd);
				const GPUResource* uav_partitionoffsets[] = {
					&sphPartitionCellOffsets,
				};
				device->BindUAVs(uav_partitionoffsets, 0, arraysize(uav_partitionoffsets), cmd);
				device->Dispatch((SPH_PARTITION_BUCKET_COUNT + THREADCOUNT_SIMULATION - 1) / THREADCOUNT_SIMULATION, 1, 1, cmd);
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

				wi::profiler::EndRange(range);
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
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Buffer(&counterBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
					GPUBarrier::Buffer(&distanceBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}
			device->EventEnd(cmd);

		}

		if (IsSorted())
		{
			wi::gpusortlib::Sort(MAX_PARTICLES, distanceBuffer, counterBuffer, PARTICLECOUNTER_OFFSET_CULLEDCOUNT, culledIndirectionBuffer, cmd);
		}

		if (!IsPaused())
		{
			// finish updating, update draw argument buffer:
			device->EventBegin("FinishUpdate", cmd);
			device->BindComputeShader(&finishUpdateCS, cmd);

			const GPUResource* res[] = {
				&counterBuffer,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			const GPUResource* uavs[] = {
				&indirectBuffers,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Buffer(&indirectBuffers, ResourceState::INDIRECT_ARGUMENT, ResourceState::UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->Dispatch(1, 1, 1, cmd);

			device->EventEnd(cmd);

		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Buffer(&counterBuffer, ResourceState::SHADER_RESOURCE, ResourceState::COPY_SRC),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		// Statistics is copied to readback:
		device->CopyResource(&statisticsReadbackBuffer[(statisticsReadBackIndex - 1) % arraysize(statisticsReadbackBuffer)], &counterBuffer, cmd);

		{
			const GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&indirectBuffers, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT),
				GPUBarrier::Buffer(&counterBuffer, ResourceState::COPY_SRC, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&particleBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&aliveList[1], ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&vertexBuffer_POS, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&vertexBuffer_UVS, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&vertexBuffer_COL, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&culledIndirectionBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
				GPUBarrier::Buffer(&culledIndirectionBuffer2, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->EventEnd(cmd);
	}


	void EmittedParticleSystem::Draw(const MaterialComponent& material, CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("EmittedParticle", cmd);

		if (wi::renderer::IsWireRender())
		{
			device->BindPipelineState(&PSO_wire, cmd);
		}
		else
		{
			const wi::enums::BLENDMODE blendMode = material.GetBlendMode();
			device->BindPipelineState(&PSO[blendMode][shaderType], cmd);

			device->BindShadingRate(material.shadingRate, cmd);
		}

		device->BindConstantBuffer(&constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);

		const GPUResource* res[] = {
			&counterBuffer,
			&particleBuffer,
			&culledIndirectionBuffer,
			&culledIndirectionBuffer2,
		};
		device->BindResources(res, 0, arraysize(res), cmd);

		if (ALLOW_MESH_SHADER && device->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
		{
			device->DispatchMeshIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, cmd);
		}
		else
		{
			device->DrawInstancedIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, cmd);
		}

		device->EventEnd(cmd);
	}


	namespace EmittedParticleSystem_Internal
	{
		void LoadShaders()
		{
			wi::renderer::LoadShader(ShaderStage::VS, vertexShader, "emittedparticleVS.cso");

			if (ALLOW_MESH_SHADER && wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
			{
				wi::renderer::LoadShader(ShaderStage::MS, meshShader, "emittedparticleMS.cso");
			}

			wi::renderer::LoadShader(ShaderStage::PS, pixelShader[EmittedParticleSystem::SOFT], "emittedparticlePS_soft.cso");
			wi::renderer::LoadShader(ShaderStage::PS, pixelShader[EmittedParticleSystem::SOFT_DISTORTION], "emittedparticlePS_soft_distortion.cso");
			wi::renderer::LoadShader(ShaderStage::PS, pixelShader[EmittedParticleSystem::SIMPLE], "emittedparticlePS_simple.cso");
			wi::renderer::LoadShader(ShaderStage::PS, pixelShader[EmittedParticleSystem::SOFT_LIGHTING], "emittedparticlePS_soft_lighting.cso");

			wi::renderer::LoadShader(ShaderStage::CS, kickoffUpdateCS, "emittedparticle_kickoffUpdateCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, finishUpdateCS, "emittedparticle_finishUpdateCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, emitCS, "emittedparticle_emitCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, emitCS_VOLUME, "emittedparticle_emitCS_volume.cso");
			wi::renderer::LoadShader(ShaderStage::CS, emitCS_FROMMESH, "emittedparticle_emitCS_FROMMESH.cso");
			wi::renderer::LoadShader(ShaderStage::CS, sphpartitionCS, "emittedparticle_sphpartitionCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, sphpartitionoffsetsCS, "emittedparticle_sphpartitionoffsetsCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, sphpartitionoffsetsresetCS, "emittedparticle_sphpartitionoffsetsresetCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, sphdensityCS, "emittedparticle_sphdensityCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, sphforceCS, "emittedparticle_sphforceCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, simulateCS, "emittedparticle_simulateCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, simulateCS_SORTING, "emittedparticle_simulateCS_SORTING.cso");
			wi::renderer::LoadShader(ShaderStage::CS, simulateCS_DEPTHCOLLISIONS, "emittedparticle_simulateCS_DEPTHCOLLISIONS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, simulateCS_SORTING_DEPTHCOLLISIONS, "emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.cso");


			GraphicsDevice* device = wi::graphics::GetDevice();

			for (int i = 0; i < BLENDMODE_COUNT; ++i)
			{
				PipelineStateDesc desc;
				desc.pt = PrimitiveTopology::TRIANGLESTRIP;
				if (ALLOW_MESH_SHADER && wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
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

				for (int j = 0; j < EmittedParticleSystem::PARTICLESHADERTYPE_COUNT; ++j)
				{
					desc.ps = &pixelShader[j];
					device->CreatePipelineState(&desc, &PSO[i][j]);
				}
			}

			{
				PipelineStateDesc desc;
				desc.pt = PrimitiveTopology::TRIANGLESTRIP;
				if (ALLOW_MESH_SHADER && wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
				{
					desc.ms = &meshShader;
				}
				else
				{
					desc.vs = &vertexShader;
				}
				desc.ps = &pixelShader[EmittedParticleSystem::SIMPLE];
				desc.bs = &blendStates[BLENDMODE_ALPHA];
				desc.rs = &wireFrameRS;
				desc.dss = &depthStencilState;

				device->CreatePipelineState(&desc, &PSO_wire);
			}

		}
	}

	void EmittedParticleSystem::Initialize()
	{
		wi::Timer timer;

		RasterizerState rs;
		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rasterizerState = rs;


		rs.fill_mode = FillMode::WIREFRAME;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		wireFrameRS = rs;


		DepthStencilState dsd;
		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::GREATER_EQUAL;
		dsd.stencil_enable = false;
		depthStencilState = dsd;


		BlendState bd;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ALPHA] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::ONE;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ZERO;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ADDITIVE] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::ONE;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_PREMULTIPLIED] = bd;

		bd.render_target[0].blend_enable = false;
		blendStates[BLENDMODE_OPAQUE] = bd;

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { EmittedParticleSystem_Internal::LoadShaders(); });
		EmittedParticleSystem_Internal::LoadShaders();

		wi::backlog::post("wi::EmittedParticleSystem Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}


	void EmittedParticleSystem::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> (uint32_t&)shaderType;
			wi::ecs::SerializeEntity(archive, meshID, seri);
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

			if (archive.GetVersion() >= 74)
			{
				archive >> restitution;
			}

		}
		else
		{
			archive << _flags;
			archive << (uint32_t)shaderType;
			wi::ecs::SerializeEntity(archive, meshID, seri);
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

			if (archive.GetVersion() >= 74)
			{
				archive << restitution;
			}
		}
	}

}
