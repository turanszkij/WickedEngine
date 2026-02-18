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
	static Shader shadowPS;
	static Shader pixelShader[EmittedParticleSystem::PARTICLESHADERTYPE_COUNT];
	static Shader kickoffUpdateCS;
	static Shader finishUpdateCS;
	static Shader emitCS;
	static Shader emitCS_VOLUME;
	static Shader emitCS_FROMMESH;
	static Shader sphpartitionCS;
	static Shader sphcellallocationCS;
	static Shader sphbinningCS;
	static Shader sphdensityCS;
	static Shader sphforceCS;
	static Shader simulateCS;
	static Shader simulateCS_SORTING;
	static Shader simulateCS_DEPTHCOLLISIONS;
	static Shader simulateCS_SORTING_DEPTHCOLLISIONS;

	static BlendState			blendStates[BLENDMODE_COUNT];
	static RasterizerState		rasterizerState;
	static RasterizerState		rasterizerState_shadow;
	static RasterizerState		wireFrameRS;
	static DepthStencilState	depthStencilState;
	static DepthStencilState	depthStencilState_shadow;
	static PipelineState		PSO[BLENDMODE_COUNT][EmittedParticleSystem::PARTICLESHADERTYPE_COUNT];
	static PipelineState		PSO_wire;
	static PipelineState		PSO_shadow;

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
			auto fill_dead_indices = [&](void* dest) {
				uint32_t* indices = (uint32_t*)dest;
				for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
				{
					std::memcpy(indices + i, &i, sizeof(uint32_t));
				}
			};
			device->CreateBuffer2(&bd, fill_dead_indices, &deadList);
			device->SetName(&deadList, "EmittedParticleSystem::deadList");

			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::TYPED_FORMAT_CASTING | ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS;
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}
			const uint64_t alignment = device->GetMinOffsetAlignment(&bd);

			vb_pos.size = sizeof(MeshComponent::Vertex_POS32W) * 4 * MAX_PARTICLES;
			vb_nor.size = sizeof(MeshComponent::Vertex_NOR) * 4 * MAX_PARTICLES;
			vb_uvs.size = sizeof(MeshComponent::Vertex_UVS) * 4 * MAX_PARTICLES;
			vb_col.size = sizeof(MeshComponent::Vertex_COL) * 4 * MAX_PARTICLES;

			bd.size =
				align(vb_pos.size, alignment) +
				align(vb_nor.size, alignment) +
				align(vb_uvs.size, alignment) +
				align(vb_col.size, alignment)
			;

			device->CreateBuffer(&bd, nullptr, &generalBuffer);
			device->SetName(&generalBuffer, "EmittedParticleSystem::generalBuffer");

			uint64_t buffer_offset = 0ull;

			buffer_offset = align(buffer_offset, alignment);
			vb_pos.offset = buffer_offset;
			vb_pos.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_pos.offset, vb_pos.size, &MeshComponent::Vertex_POS32W::FORMAT);
			vb_pos.subresource_uav = device->CreateSubresource(&generalBuffer, SubresourceType::UAV, vb_pos.offset, vb_pos.size, &MeshComponent::Vertex_POS32W::FORMAT); // UAV can't have RGB32_F format!
			vb_pos.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_pos.subresource_srv);
			vb_pos.descriptor_uav = device->GetDescriptorIndex(&generalBuffer, SubresourceType::UAV, vb_pos.subresource_uav);
			buffer_offset += vb_pos.size;

			buffer_offset = align(buffer_offset, alignment);
			vb_nor.offset = buffer_offset;
			vb_nor.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_nor.offset, vb_nor.size, &MeshComponent::Vertex_NOR::FORMAT);
			vb_nor.subresource_uav = device->CreateSubresource(&generalBuffer, SubresourceType::UAV, vb_nor.offset, vb_nor.size, &MeshComponent::Vertex_NOR::FORMAT);
			vb_nor.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_nor.subresource_srv);
			vb_nor.descriptor_uav = device->GetDescriptorIndex(&generalBuffer, SubresourceType::UAV, vb_nor.subresource_uav);
			buffer_offset += vb_nor.size;

			buffer_offset = align(buffer_offset, alignment);
			vb_uvs.offset = buffer_offset;
			vb_uvs.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_uvs.offset, vb_uvs.size, &MeshComponent::Vertex_UVS::FORMAT);
			vb_uvs.subresource_uav = device->CreateSubresource(&generalBuffer, SubresourceType::UAV, vb_uvs.offset, vb_uvs.size, &MeshComponent::Vertex_UVS::FORMAT);
			vb_uvs.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_uvs.subresource_srv);
			vb_uvs.descriptor_uav = device->GetDescriptorIndex(&generalBuffer, SubresourceType::UAV, vb_uvs.subresource_uav);
			buffer_offset += vb_uvs.size;

			buffer_offset = align(buffer_offset, alignment);
			vb_col.offset = buffer_offset;
			vb_col.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_col.offset, vb_col.size, &MeshComponent::Vertex_COL::FORMAT);
			vb_col.subresource_uav = device->CreateSubresource(&generalBuffer, SubresourceType::UAV, vb_col.offset, vb_col.size, &MeshComponent::Vertex_COL::FORMAT);
			vb_col.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_col.subresource_srv);
			vb_col.descriptor_uav = device->GetDescriptorIndex(&generalBuffer, SubresourceType::UAV, vb_col.subresource_uav);
			buffer_offset += vb_col.size;

			primitiveBuffer = wi::renderer::GetIndexBufferForQuads(MAX_PARTICLES);
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
			auto init_distances = [&](void* dest) {
				float* distances = (float*)dest;
				std::fill(distances, distances + MAX_PARTICLES, 0.0f);
			};
			device->CreateBuffer2(&bd, init_distances, &distanceBuffer);
			device->SetName(&distanceBuffer, "EmittedParticleSystem::distanceBuffer");
		}

		if (IsSPHEnabled())
		{
			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;

			if (densityBuffer.desc.size < MAX_PARTICLES * sizeof(float))
			{
				// Density buffer (for SPH simulation):
				bd.stride = sizeof(float);
				bd.size = bd.stride * MAX_PARTICLES;
				device->CreateBuffer(&bd, nullptr, &densityBuffer);
				device->SetName(&densityBuffer, "EmittedParticleSystem::densityBuffer");
			}

			if (sphGridCells.desc.size < SPH_PARTITION_BUCKET_COUNT * sizeof(SPHGridCell))
			{
				// SPH Partitioning grid cell offsets,counts into particle index list:
				bd.stride = sizeof(SPHGridCell);
				bd.size = bd.stride * SPH_PARTITION_BUCKET_COUNT;
				device->CreateBuffer(&bd, nullptr, &sphGridCells);
				device->SetName(&sphGridCells, "EmittedParticleSystem::sphGridCells");
			}

			if (sphParticleCells.desc.size < MAX_PARTICLES * sizeof(uint))
			{
				// Compacted particle lists per grid cell  (for SPH simulation):
				bd.stride = sizeof(float);
				bd.size = bd.stride * MAX_PARTICLES;
				device->CreateBuffer(&bd, nullptr, &sphParticleCells);
				device->SetName(&sphParticleCells, "EmittedParticleSystem::sphParticleCells");
			}
		}
		else
		{
			densityBuffer = {};
			sphGridCells = {};
			sphParticleCells = {};
		}

		if (!counterBuffer.IsValid())
		{
			// Particle System statistics:
			ParticleCounters counters;
			counters.aliveCount = 0;
			counters.deadCount = MAX_PARTICLES;
			counters.realEmitCount = 0;
			counters.aliveCount_afterSimulation = 0;
			counters.cellAllocator = 0;

			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.stride = sizeof(counters);
			bd.size = bd.stride;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			device->CreateBuffer(&bd, &counters, &counterBuffer);
			device->SetName(&counterBuffer, "EmittedParticleSystem::counterBuffer");
		}

		if (!indirectBuffers.IsValid())
		{
			GPUBufferDesc bd;

			// Indirect Execution buffer:
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED | ResourceMiscFlag::INDIRECT_ARGS;
			bd.stride = sizeof(EmitterIndirectArgs);
			bd.size = bd.stride;
			device->CreateBufferZeroed(&bd, &indirectBuffers);
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
			geometry.triangles.vertex_buffer = generalBuffer;
			geometry.triangles.index_buffer = primitiveBuffer;
			geometry.triangles.index_format = GetIndexBufferFormat(primitiveBuffer.desc.format);
			geometry.triangles.index_count = MAX_PARTICLES * 6;
			geometry.triangles.index_offset = 0;
			geometry.triangles.vertex_count = (uint32_t)(vb_pos.size / sizeof(MeshComponent::Vertex_POS32W));
			geometry.triangles.vertex_format = Format::R32G32B32_FLOAT;
			geometry.triangles.vertex_stride = sizeof(MeshComponent::Vertex_POS32W);

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
		retVal += sphGridCells.GetDesc().size;
		retVal += sphParticleCells.GetDesc().size;
		retVal += densityBuffer.GetDesc().size;
		retVal += counterBuffer.GetDesc().size;
		retVal += indirectBuffers.GetDesc().size;
		retVal += constantBuffer.GetDesc().size;
		retVal += generalBuffer.GetDesc().size;
		retVal += culledIndirectionBuffer.GetDesc().size;
		retVal += culledIndirectionBuffer2.GetDesc().size;
		retVal += ComputeTextureMemorySizeInBytes(opacityCurveTex.GetDesc());

		return retVal;
	}

	void EmittedParticleSystem::UpdateCPU(const TransformComponent& transform, float dt)
	{
		this->dt = dt;
		CreateSelfBuffers();

		if (IsPaused() || dt == 0)
			return;

		active_frames <<= 1; // advance next frame

		emit = std::max(0.0f, emit - std::floor(emit));

		center = transform.GetPosition();

		emit += (float)count * dt;

		emit += burst;
		burst = 0;

		if ((uint)emit > 0)
		{
			EmitLocation& location = emit_locations.emplace_back();
			location.transform.init();
			location.count = (uint)emit;
			location.color = wi::Color::White();
			active_frames |= 1; // activate current frame
		}

		worldMatrix = transform.world;

		// Swap CURRENT alivelist with NEW alivelist
		std::swap(aliveList[0], aliveList[1]);

		// Read back statistics (with GPU delay):
		const uint32_t oldest_stat_index = wi::graphics::GetDevice()->GetBufferIndex();
		memcpy(&statistics, statisticsReadbackBuffer[oldest_stat_index].mapped_data, sizeof(statistics));

		if (statistics.aliveCount > 0 || statistics.aliveCount_afterSimulation > 0)
		{
			active_frames |= 1; // activate current frame
		}

		if (!opacityCurveTex.IsValid())
		{
			SetOpacityCurveControl(opacityCurveControlPeakStart, opacityCurveControlPeakEnd);
		}
	}
	void EmittedParticleSystem::Burst(int num)
	{
		if (IsPaused())
			return;

		burst += num;

		if (num > 0)
		{
			active_frames |= 1; // activate current frame
		}
	}
	void EmittedParticleSystem::Burst(int num, const XMFLOAT3& position, const wi::Color& color)
	{
		if (IsPaused() || num <= 0)
			return;

		XMFLOAT4X4 transform;
		XMStoreFloat4x4(&transform, XMMatrixTranslation(position.x, position.y, position.z));

		EmitLocation& location = emit_locations.emplace_back();
		location.count = (uint)num;
		location.transform.Create(transform);
		location.color = color;

		if (num > 0)
		{
			active_frames |= 1; // activate current frame
		}
	}
	void EmittedParticleSystem::Burst(int num, const XMFLOAT4X4& transform, const wi::Color& color)
	{
		if (IsPaused() || num <= 0)
			return;

		EmitLocation& location = emit_locations.emplace_back();
		location.count = (uint)num;
		location.transform.Create(transform);
		location.color = color;

		if (num > 0)
		{
			active_frames |= 1; // activate current frame
		}
	}
	void EmittedParticleSystem::Restart()
	{
		SetPaused(false);
		counterBuffer = {}; // will be recreated
	}

	void EmittedParticleSystem::UpdateGPU(uint32_t instanceIndex, const MeshComponent* mesh, CommandList cmd) const
	{
		if (IsInactive())
			return;
		if (!particleBuffer.IsValid())
			return;

		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("UpdateEmittedParticles", cmd);

		if (!IsPaused() && dt > 0)
		{
			auto alloc = device->AllocateGPU(sizeof(EmitLocation) * emit_locations.size(), cmd);
			const XMMATRIX W = XMLoadFloat4x4(&worldMatrix);
			for (size_t i = 0; i < emit_locations.size(); ++i)
			{
				EmitLocation& location = emit_locations[i];
				XMFLOAT4X4 mat = location.transform.GetMatrix();
				XMMATRIX M = XMMatrixTranspose(XMLoadFloat4x4(&mat));
				M = W * M;
				XMStoreFloat4x4(&mat, M);
				location.transform.Create(mat);
				std::memcpy((EmitLocation*)alloc.data + i, &location, sizeof(location));
			}
			device->BindResource(&alloc.buffer, 0, cmd);

			EmittedParticleCB cb;
			cb.xEmitBufferOffset = (uint)alloc.offset;
			if (mesh == nullptr || !IsFormatUnorm(mesh->position_format) || mesh->so_pos.IsValid())
			{
				cb.xEmitterBaseMeshUnormRemap.init();
			}
			else
			{
				XMFLOAT4X4 unormRemap;
				XMStoreFloat4x4(&unormRemap, mesh->aabb.getUnormRemapMatrix());
				cb.xEmitterBaseMeshUnormRemap.Create(unormRemap);
			}
			if (mesh == nullptr)
			{
				cb.xEmitterMeshGeometryOffset = 0;
				cb.xEmitterMeshGeometryCount = 0;
			}
			else
			{
				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh->GetLODSubsetRange(0, first_subset, last_subset);
				cb.xEmitterMeshGeometryOffset = mesh->geometryOffset + first_subset;
				cb.xEmitterMeshGeometryCount = last_subset - first_subset;
			}
			cb.xEmitterRandomness = wi::random::GetRandom(0.0f, 1.0f);
			cb.xParticleLifeSpan = life;
			cb.xParticleLifeSpanRandomness = random_life;
			cb.xParticleNormalFactor = normal_factor;
			cb.xParticleRandomFactor = random_factor;
			cb.xParticleScaling = scaleX;
			cb.xParticleSize = size;
			cb.xParticleMotionBlurAmount = motionBlurAmount;
			cb.xParticleRotation = rotation * XM_PI;
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
			XMStoreFloat3(&cb.xParticleVelocity, XMVector3TransformNormal(XMLoadFloat3(&velocity), XMLoadFloat4x4(&worldMatrix)));
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
			if (IsCollidersDisabled())
			{
				cb.xEmitterOptions |= EMITTER_OPTION_BIT_COLLIDERS_DISABLED;
			}
			if (_flags & FLAG_USE_RAIN_BLOCKER)
			{
				cb.xEmitterOptions |= EMITTER_OPTION_BIT_USE_RAIN_BLOCKER;
			}
			if (IsTakeColorFromMesh())
			{
				cb.xEmitterOptions |= EMITTER_OPTION_BIT_TAKE_COLOR_FROM_MESH;
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

			device->UpdateBuffer(&constantBuffer, &cb, cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Buffer(&constantBuffer, ResourceState::COPY_DST, ResourceState::CONSTANT_BUFFER),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->BindConstantBuffer(&constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), cmd);

			device->BindUAV(&particleBuffer, 0, cmd);
			device->BindUAV(&aliveList[0], 1, cmd);
			device->BindUAV(&aliveList[1], 2, cmd);
			device->BindUAV(&deadList, 3, cmd);
			device->BindUAV(&counterBuffer, 4, cmd);
			device->BindUAV(&indirectBuffers, 5, cmd);
			device->BindUAV(&distanceBuffer, 6, cmd);
			device->BindUAV(&generalBuffer, 7, cmd, vb_pos.subresource_uav);
			device->BindUAV(&generalBuffer, 8, cmd, vb_nor.subresource_uav);
			device->BindUAV(&generalBuffer, 9, cmd, vb_uvs.subresource_uav);
			device->BindUAV(&generalBuffer, 10, cmd, vb_col.subresource_uav);
			device->BindUAV(&culledIndirectionBuffer, 11, cmd);
			device->BindUAV(&culledIndirectionBuffer2, 12, cmd);

			GPUBarrier barrier_indirect_uav = GPUBarrier::Buffer(&indirectBuffers, ResourceState::INDIRECT_ARGUMENT, ResourceState::UNORDERED_ACCESS);
			GPUBarrier barrier_uav_indirect = GPUBarrier::Buffer(&indirectBuffers, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT);

			device->Barrier(&barrier_indirect_uav, 1, cmd);

			// emit the required amounts
			device->EventBegin("Emit", cmd);
			device->BindComputeShader(mesh == nullptr ? (IsVolumeEnabled() ? &emitCS_VOLUME : &emitCS) : &emitCS_FROMMESH, cmd);
			uint bufferoffset = (uint)alloc.offset;
			for (auto& location : emit_locations)
			{
				device->PushConstants(&bufferoffset, sizeof(bufferoffset), cmd);
				device->Dispatch((location.count + 63u) / 64u, 1, 1, cmd);
				bufferoffset += sizeof(EmitLocation);
			}
			emit_locations.clear();
			device->Barrier(GPUBarrier::Memory(), cmd);
			device->EventEnd(cmd);

			// kick off indirect updating
			device->EventBegin("KickOff Update", cmd);
			device->BindComputeShader(&kickoffUpdateCS, cmd);
			device->Dispatch(1, 1, 1, cmd);
			device->EventEnd(cmd);

			device->Barrier(&barrier_uav_indirect, 1, cmd);

			if (IsSPHEnabled())
			{
				auto range = wi::profiler::BeginRangeGPU("SPH - Simulation", cmd);

				// Smooth Particle Hydrodynamics:
				device->EventBegin("SPH - Simulation", cmd);

#ifdef SPH_USE_ACCELERATION_GRID
				// Reset grid cell offset buffer with invalid offsets (max uint):
				device->EventBegin("Grid - Reset", cmd);
				device->ClearUAV(&sphGridCells, 0, cmd);
				device->Barrier(GPUBarrier::Memory(), cmd);
				device->EventEnd(cmd);

				// Assign particles into partitioning grid:
				device->EventBegin("Particles - Grid Partitioning", cmd);
				device->BindComputeShader(&sphpartitionCS, cmd);
				const GPUResource* res_partition[] = {
					&aliveList[0], // CURRENT alivelist
					&particleBuffer,
				};
				device->BindResources(res_partition, 0, arraysize(res_partition), cmd);
				const GPUResource* uav_partition[] = {
					&counterBuffer,
					&sphGridCells,
				};
				device->BindUAVs(uav_partition, 0, arraysize(uav_partition), cmd);
				device->DispatchIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
				{
					GPUBarrier barriers[] = {
						GPUBarrier::Memory(&sphGridCells),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
				device->EventEnd(cmd);

				// Each grid cell allocates space for its own particle list:
				device->EventBegin("Grid - Cell Allocation", cmd);
				device->BindComputeShader(&sphcellallocationCS, cmd);
				const GPUResource* res_cellallocation[] = {
					&aliveList[0], // CURRENT alivelist
					&particleBuffer,
				};
				device->BindResources(res_cellallocation, 0, arraysize(res_cellallocation), cmd);
				const GPUResource* uav_cellallocation[] = {
					&counterBuffer,
					&sphGridCells,
				};
				device->BindUAVs(uav_cellallocation, 0, arraysize(uav_cellallocation), cmd);
				device->Dispatch((SPH_PARTITION_BUCKET_COUNT + THREADCOUNT_SIMULATION - 1) / THREADCOUNT_SIMULATION, 1, 1, cmd);
				{
					GPUBarrier barriers[] = {
						GPUBarrier::Memory(&sphGridCells),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
				device->EventEnd(cmd);

				// Each particle bins itself to a grid cell:
				device->EventBegin("Particles - Binning", cmd);
				device->BindComputeShader(&sphbinningCS, cmd);
				const GPUResource* res_binning[] = {
					&aliveList[0], // CURRENT alivelist
					&particleBuffer,
				};
				device->BindResources(res_binning, 0, arraysize(res_binning), cmd);
				const GPUResource* uav_binning[] = {
					&counterBuffer,
					&sphGridCells,
					&sphParticleCells,
				};
				device->BindUAVs(uav_binning, 0, arraysize(uav_binning), cmd);
				device->DispatchIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
				{
					GPUBarrier barriers[] = {
						GPUBarrier::Buffer(&sphGridCells, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
						GPUBarrier::Buffer(&sphParticleCells, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
				device->EventEnd(cmd);

#endif // SPH_USE_ACCELERATION_GRID

				// 5.) Compute particle density field:
				device->EventBegin("Density Evaluation", cmd);
				device->BindComputeShader(&sphdensityCS, cmd);
				const GPUResource* res_density[] = {
					&aliveList[0], // CURRENT alivelist
					&particleBuffer,
					&sphParticleCells,
					&sphGridCells,
				};
				device->BindResources(res_density, 0, arraysize(res_density), cmd);
				const GPUResource* uav_density[] = {
					&counterBuffer,
					&densityBuffer
				};
				device->BindUAVs(uav_density, 0, arraysize(uav_density), cmd);
				device->DispatchIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
				device->Barrier(GPUBarrier::Memory(), cmd);
				device->EventEnd(cmd);

				// 6.) Compute particle pressure forces:
				device->EventBegin("Force Evaluation", cmd);
				device->BindComputeShader(&sphforceCS, cmd);
				const GPUResource* res_force[] = {
					&aliveList[0], // CURRENT alivelist
					&densityBuffer,
					&sphParticleCells,
					&sphGridCells,
				};
				device->BindResources(res_force, 0, arraysize(res_force), cmd);
				const GPUResource* uav_force[] = {
					&counterBuffer,
					&particleBuffer,
				};
				device->BindUAVs(uav_force, 0, arraysize(uav_force), cmd);
				device->DispatchIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
				device->Barrier(GPUBarrier::Memory(), cmd);
				device->EventEnd(cmd);


				device->EventEnd(cmd);

				wi::profiler::EndRange(range);
			}

			device->EventBegin("Simulate", cmd);

			device->BindResource(&opacityCurveTex, 0, cmd);

			device->BindUAV(&particleBuffer, 0, cmd);
			device->BindUAV(&aliveList[0], 1, cmd);
			device->BindUAV(&aliveList[1], 2, cmd);
			device->BindUAV(&deadList, 3, cmd);
			device->BindUAV(&counterBuffer, 4, cmd);
			device->BindUAV(&indirectBuffers, 5, cmd);
			device->BindUAV(&distanceBuffer, 6, cmd);
			device->BindUAV(&generalBuffer, 7, cmd, vb_pos.subresource_uav);
			device->BindUAV(&generalBuffer, 8, cmd, vb_nor.subresource_uav);
			device->BindUAV(&generalBuffer, 9, cmd, vb_uvs.subresource_uav);
			device->BindUAV(&generalBuffer, 10, cmd, vb_col.subresource_uav);
			device->BindUAV(&culledIndirectionBuffer, 11, cmd);
			device->BindUAV(&culledIndirectionBuffer2, 12, cmd);

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
			device->DispatchIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
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
			wi::gpusortlib::Sort(MAX_PARTICLES, distanceBuffer, counterBuffer, offsetof(ParticleCounters, culledCount), culledIndirectionBuffer, cmd);
		}

		if (!IsPaused() && dt > 0)
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
		const uint32_t oldest_stat_index = wi::graphics::GetDevice()->GetBufferIndex();
		device->CopyBuffer(&statisticsReadbackBuffer[oldest_stat_index], 0, &counterBuffer, 0, sizeof(ParticleCounters), cmd);

		{
			const GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&indirectBuffers, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT),
				GPUBarrier::Buffer(&counterBuffer, ResourceState::COPY_SRC, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&particleBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&aliveList[1], ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&generalBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&culledIndirectionBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
				GPUBarrier::Buffer(&culledIndirectionBuffer2, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE_COMPUTE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->EventEnd(cmd);
	}


	void EmittedParticleSystem::Draw(const MaterialComponent& material, CommandList cmd, const PARTICLESHADERTYPE* shadertype_override) const
	{
		if (IsInactive())
			return;
		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("EmittedParticle", cmd);

		if (wi::renderer::GetWireframeMode() == wi::renderer::WIREFRAME_ONLY)
		{
			device->BindPipelineState(&PSO_wire, cmd);
		}
		else
		{
			const wi::enums::BLENDMODE blendMode = material.GetBlendMode();
			device->BindPipelineState(&PSO[blendMode][shadertype_override == nullptr ? shaderType : *shadertype_override], cmd);

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
			device->DispatchMeshIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
		}
		else
		{
			device->DrawInstancedIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, draw), cmd);
		}

		if (wi::renderer::GetWireframeMode() == wi::renderer::WIREFRAME_OVERLAY)
		{
			device->BindPipelineState(&PSO_wire, cmd);
			if (ALLOW_MESH_SHADER && device->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
			{
				device->DispatchMeshIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
			}
			else
			{
				device->DrawInstancedIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, draw), cmd);
			}
		}

		device->EventEnd(cmd);
	}

	void EmittedParticleSystem::DrawForShadowmap(const MaterialComponent& material, CommandList cmd) const
	{
		if (IsInactive())
			return;
		if (material.GetBlendMode() != BLENDMODE_OPAQUE && material.GetBlendMode() != BLENDMODE_ALPHA)
			return;
		if (!material.IsCastingShadow())
			return;

		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("EmittedParticle Shadow", cmd);

		device->BindPipelineState(&PSO_shadow, cmd);

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
			device->DispatchMeshIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, dispatch), cmd);
		}
		else
		{
			device->DrawInstancedIndirect(&indirectBuffers, offsetof(EmitterIndirectArgs, draw), cmd);
		}

		device->EventEnd(cmd);
	}

	void EmittedParticleSystem::SetOpacityCurveControl(float peakStart, float peakEnd)
	{
		peakStart = saturate(peakStart);
		peakEnd = saturate(peakEnd);

		peakEnd = std::max(peakStart, peakEnd);

		opacityCurveControlPeakStart = peakStart;
		opacityCurveControlPeakEnd = peakEnd;

		uint16_t data[2048];
		int startup_length = int(peakStart * float(arraysize(data) - 1));
		// Ramp up:
		for (int i = 0; i < startup_length; ++i)
		{
			float t = smoothstep(0.0f, 1.0f, float(i) / (startup_length - 1));
			data[i] = uint16_t(t * 65535);
		}
		int keep_length = int((peakEnd - peakStart) * float(arraysize(data) - 1));
		// Keep value:
		for (int i = 0; i < keep_length; ++i)
		{
			data[i + startup_length] = uint16_t(65535);
		}
		// Ramp down:
		for (int i = 0; i < (arraysize(data) - startup_length - keep_length); ++i)
		{
			float t = smoothstep(1.0f, 0.0f, float(i) / (arraysize(data) - startup_length - keep_length - 1));
			data[i + startup_length + keep_length] = uint16_t(t * 65535);
		}
		TextureDesc desc;
		desc.type = TextureDesc::Type::TEXTURE_1D;
		desc.width = arraysize(data);
		desc.format = Format::R16_UNORM;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.swizzle = SwizzleFromString("rrr1");
		SubresourceData initdata;
		initdata.data_ptr = data;
		initdata.row_pitch = sizeof(data);
		GetDevice()->CreateTexture(&desc, &initdata, &opacityCurveTex);
		GetDevice()->SetName(&opacityCurveTex, "EmittedParticleSystem::opacityCurveTex");
	}

	namespace EmittedParticleSystem_Internal
	{
		void LoadShaders()
		{
			wi::renderer::LoadShader(ShaderStage::VS, vertexShader, "emittedparticleVS.cso");
			wi::renderer::LoadShader(ShaderStage::PS, shadowPS, "emittedparticlePS_shadow.cso");

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
			wi::renderer::LoadShader(ShaderStage::CS, sphcellallocationCS, "emittedparticle_sphcellallocationCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, sphbinningCS, "emittedparticle_sphbinningCS.cso");
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
				desc.ps = &shadowPS;
				desc.bs = &blendStates[BLENDMODE_OPAQUE];
				desc.rs = &rasterizerState_shadow;
				desc.dss = &depthStencilState_shadow;

				device->CreatePipelineState(&desc, &PSO_shadow);
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

		if (IsFormatUnorm(wi::renderer::format_depthbuffer_shadowmap))
		{
			rs.depth_bias = -1;
			rs.slope_scaled_depth_bias = -4.0f;
		}
		else
		{
			rs.depth_bias = -10;
			rs.slope_scaled_depth_bias = -3.4f;
		}
		rasterizerState_shadow = rs;


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

		dsd.depth_write_mask = DepthWriteMask::ALL;
		depthStencilState_shadow = dsd;


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

		bd.render_target[0].src_blend = Blend::DEST_COLOR;
		bd.render_target[0].dest_blend = Blend::ZERO;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::DEST_ALPHA;
		bd.render_target[0].dest_blend_alpha = Blend::ZERO;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_MULTIPLY] = bd;

		bd.render_target[0].src_blend = Blend::INV_DEST_COLOR;
		bd.render_target[0].dest_blend = Blend::ZERO;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::DEST_ALPHA;
		bd.render_target[0].dest_blend_alpha = Blend::ZERO;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_INVERSE] = bd;

		bd.render_target[0].blend_enable = false;
		blendStates[BLENDMODE_OPAQUE] = bd;

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { EmittedParticleSystem_Internal::LoadShaders(); });
		EmittedParticleSystem_Internal::LoadShaders();

		wilog("wi::EmittedParticleSystem Initialized (%d ms)", (int)std::round(timer.elapsed()));
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

			if (seri.GetVersion() >= 1)
			{
				archive >> opacityCurveControlPeakStart;
			}
			else
			{
				opacityCurveControlPeakStart = 0;
			}

			if (seri.GetVersion() >= 2)
			{
				archive >> opacityCurveControlPeakEnd;
			}
			else
			{
				opacityCurveControlPeakEnd = opacityCurveControlPeakStart;
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

			if (seri.GetVersion() >= 1)
			{
				archive << opacityCurveControlPeakStart;
			}

			if (seri.GetVersion() >= 2)
			{
				archive << opacityCurveControlPeakEnd;
			}
		}
	}

}
