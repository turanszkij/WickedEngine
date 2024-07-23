#include "wiScene.h"
#include "wiTextureHelper.h"
#include "wiResourceManager.h"
#include "wiPhysics.h"
#include "wiRenderer.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiUnorderedMap.h"
#include "wiLua.h"
#include "wiAllocator.h"
#include "wiProfiler.h"

#include "shaders/ShaderInterop_SurfelGI.h"
#include "shaders/ShaderInterop_DDGI.h"

using namespace wi::ecs;
using namespace wi::enums;
using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::scene
{
	const uint32_t small_subtask_groupsize = 64u;

	void Scene::Update(float dt)
	{
		this->dt = dt;
		time += dt;

		wi::jobsystem::context ctx;

		// Script system runs first, because it could create new entities and components
		//	So GPU persistent resources need to be created accordingly for them too:
		RunScriptUpdateSystem(ctx);

		ScanAnimationDependencies();
		ScanSpringDependencies();

		// Terrains updates kick off:
		if (dt > 0)
		{
			// Because this also spawns render tasks, this must not be during dt == 0 (eg. background loading)
			for (size_t i = 0; i < terrains.GetCount(); ++i)
			{
				wi::terrain::Terrain& terrain = terrains[i];
				terrain.terrainEntity = terrains.GetEntity(i);
				terrain.scene = this;
				terrain.Generation_Update(camera);
			}
		}

		GraphicsDevice* device = wi::graphics::GetDevice();

		instanceArraySize = objects.GetCount() + hairs.GetCount() + emitters.GetCount();
		if (impostors.GetCount() > 0)
		{
			impostorInstanceOffset = uint32_t(instanceArraySize);
			instanceArraySize += 1;
		}
		if (weathers.GetCount() > 0 && weathers[0].rain_amount > 0)
		{
			rainInstanceOffset = uint32_t(instanceArraySize);
			instanceArraySize += 1;
		}
		if (instanceUploadBuffer[0].desc.size < (instanceArraySize * sizeof(ShaderMeshInstance)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMeshInstance);
			desc.size = desc.stride * instanceArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			if (!device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				// Non-UMA: separate Default usage buffer
				device->CreateBuffer(&desc, nullptr, &instanceBuffer);
				device->SetName(&instanceBuffer, "Scene::instanceBuffer");

				// Upload buffer shouldn't be used by shaders with Non-UMA:
				desc.bind_flags = BindFlag::NONE;
				desc.misc_flags = ResourceMiscFlag::NONE;
			}

			desc.usage = Usage::UPLOAD;
			for (int i = 0; i < arraysize(instanceUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &instanceUploadBuffer[i]);
				device->SetName(&instanceUploadBuffer[i], "Scene::instanceUploadBuffer");
			}
		}
		instanceArrayMapped = (ShaderMeshInstance*)instanceUploadBuffer[device->GetBufferIndex()].mapped_data;

		materialArraySize = materials.GetCount();
		if (impostors.GetCount() > 0)
		{
			impostorMaterialOffset = uint32_t(materialArraySize);
			materialArraySize += 1;
		}
		if (weathers.GetCount() > 0 && weathers[0].rain_amount > 0)
		{
			rainMaterialOffset = uint32_t(materialArraySize);
			materialArraySize += 1;
		}
		if (materialUploadBuffer[0].desc.size < (materialArraySize * sizeof(ShaderMaterial)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMaterial);
			desc.size = desc.stride * materialArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			if (!device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				// Non-UMA: separate Default usage buffer
				device->CreateBuffer(&desc, nullptr, &materialBuffer);
				device->SetName(&materialBuffer, "Scene::materialBuffer");

				// Upload buffer shouldn't be used by shaders with Non-UMA:
				desc.bind_flags = BindFlag::NONE;
				desc.misc_flags = ResourceMiscFlag::NONE;
			}

			desc.usage = Usage::UPLOAD;
			for (int i = 0; i < arraysize(materialUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &materialUploadBuffer[i]);
				device->SetName(&materialUploadBuffer[i], "Scene::materialUploadBuffer");
			}
		}
		materialArrayMapped = (ShaderMaterial*)materialUploadBuffer[device->GetBufferIndex()].mapped_data;

		if (textureStreamingFeedbackBuffer.desc.size < materialArraySize * sizeof(uint32_t))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(uint32_t);
			desc.size = desc.stride * materialArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R32_UINT;
			device->CreateBuffer(&desc, nullptr, &textureStreamingFeedbackBuffer);
			device->SetName(&textureStreamingFeedbackBuffer, "Scene::textureStreamingFeedbackBuffer");

			// Readback buffer shouldn't be used by shaders:
			desc.usage = Usage::READBACK;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(materialUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &textureStreamingFeedbackBuffer_readback[i]);
				device->SetName(&textureStreamingFeedbackBuffer_readback[i], "Scene::textureStreamingFeedbackBuffer_readback");
			}
		}
		textureStreamingFeedbackMapped = (const uint32_t*)textureStreamingFeedbackBuffer_readback[device->GetBufferIndex()].mapped_data;

		// Occlusion culling read:
		if(wi::renderer::GetOcclusionCullingEnabled() && !wi::renderer::GetFreezeCullingCameraEnabled())
		{
			uint32_t minQueryCount = uint32_t(objects.GetCount() + lights.GetCount() + 1); // +1: ocean (don't know for sure if it exists yet before weather update)
			if (queryHeap.desc.query_count < minQueryCount)
			{
				GPUQueryHeapDesc desc;
				desc.type = GpuQueryType::OCCLUSION_BINARY;
				desc.query_count = minQueryCount * 2; // *2 to grow fast
				bool success = device->CreateQueryHeap(&desc, &queryHeap);
				assert(success);

				GPUBufferDesc bd;
				bd.usage = Usage::READBACK;
				bd.size = desc.query_count * sizeof(uint64_t);

				for (int i = 0; i < arraysize(queryResultBuffer); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &queryResultBuffer[i]);
					assert(success);
					device->SetName(&queryResultBuffer[i], "Scene::queryResultBuffer");
				}

				if (device->CheckCapability(GraphicsDeviceCapability::PREDICATION))
				{
					bd.usage = Usage::DEFAULT;
					bd.misc_flags |= ResourceMiscFlag::PREDICATION;
					success = device->CreateBuffer(&bd, nullptr, &queryPredicationBuffer);
					assert(success);
					device->SetName(&queryPredicationBuffer, "Scene::queryPredicationBuffer");
				}
			}

			// Advance to next query result buffer to use (this will be the oldest one that was written)
			queryheap_idx = device->GetBufferIndex();

			// Clear query allocation state:
			queryAllocator.store(0);
		}

		if (dt > 0)
		{
			// Scan objects to check if lightmap rendering is requested:
			lightmap_request_allocator.store(0);
			lightmap_requests.reserve(objects.GetCount());
			wi::jobsystem::Dispatch(ctx, (uint32_t)objects.GetCount(), small_subtask_groupsize, [this](wi::jobsystem::JobArgs args) {
				ObjectComponent& object = objects[args.jobIndex];
				if (object.IsLightmapRenderRequested())
				{
					uint32_t request_index = lightmap_request_allocator.fetch_add(1);
					*(lightmap_requests.data() + request_index) = args.jobIndex;
				}
			});

			// Scan mesh subset counts and skinning data sizes to allocate GPU geometry data:
			geometryAllocator.store(0u);
			skinningAllocator.store(0u);
			wi::jobsystem::Dispatch(ctx, (uint32_t)meshes.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {
				MeshComponent& mesh = meshes[args.jobIndex];
				mesh.geometryOffset = geometryAllocator.fetch_add((uint32_t)mesh.subsets.size());
				skinningAllocator.fetch_add(uint32_t(mesh.morph_targets.size() * sizeof(MorphTargetGPU)));
			});
			wi::jobsystem::Dispatch(ctx, (uint32_t)armatures.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {
				ArmatureComponent& armature = armatures[args.jobIndex];
				skinningAllocator.fetch_add(uint32_t(armature.boneCollection.size() * sizeof(ShaderTransform)));
			});

			wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
				// Must not keep inactive instances, so init them for safety:
				ShaderMeshInstance inst;
				inst.init();
				for (uint32_t i = 0; i < instanceArraySize; ++i)
				{
					std::memcpy(instanceArrayMapped + i, &inst, sizeof(inst));
				}
			});
		}

		RunAnimationUpdateSystem(ctx);

		wi::physics::RunPhysicsUpdateSystem(ctx, *this, dt);

		RunTransformUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunHierarchyUpdateSystem(ctx);

		// Lightmap requests are determined at this point, so we know if we need TLAS or not:
		if (lightmap_request_allocator.load() > 0)
		{
			SetAccelerationStructureUpdateRequested(true);
		}

		// This must be after lightmap requests were determined:
		TLAS_instancesMapped = nullptr;
		if (IsAccelerationStructureUpdateRequested() && device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			GPUBufferDesc desc;
			desc.stride = (uint32_t)device->GetTopLevelAccelerationStructureInstanceSize();
			desc.size = desc.stride * instanceArraySize * 2; // *2 to grow fast
			desc.usage = Usage::UPLOAD;
			if (TLAS_instancesUpload->desc.size < desc.size)
			{
				for (int i = 0; i < arraysize(TLAS_instancesUpload); ++i)
				{
					device->CreateBuffer(&desc, nullptr, &TLAS_instancesUpload[i]);
					device->SetName(&TLAS_instancesUpload[i], "Scene::TLAS_instancesUpload");
				}
			}
			TLAS_instancesMapped = TLAS_instancesUpload[device->GetBufferIndex()].mapped_data;

			wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
				// Must not keep inactive TLAS instances, so zero them out for safety:
				std::memset(TLAS_instancesMapped, 0, TLAS_instancesUpload->desc.size);
				});
		}

		// GPU subset count allocation is ready at this point:
		geometryArraySize = geometryAllocator.load();
		geometryArraySize += hairs.GetCount();
		geometryArraySize += emitters.GetCount();
		if (impostors.GetCount() > 0)
		{
			impostorGeometryOffset = uint32_t(geometryArraySize);
			geometryArraySize += 1;
		}
		if (weathers.GetCount() > 0 && weathers[0].rain_amount > 0)
		{
			rainGeometryOffset = uint32_t(geometryArraySize);
			geometryArraySize += 1;
		}
		if (geometryUploadBuffer[0].desc.size < (geometryArraySize * sizeof(ShaderGeometry)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderGeometry);
			desc.size = desc.stride * geometryArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			if (!device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				// Non-UMA: separate Default usage buffer
				device->CreateBuffer(&desc, nullptr, &geometryBuffer);
				device->SetName(&geometryBuffer, "Scene::geometryBuffer");

				// Upload buffer shouldn't be used by shaders with Non-UMA:
				desc.bind_flags = BindFlag::NONE;
				desc.misc_flags = ResourceMiscFlag::NONE;
			}

			desc.usage = Usage::UPLOAD;
			for (int i = 0; i < arraysize(geometryUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &geometryUploadBuffer[i]);
				device->SetName(&geometryUploadBuffer[i], "Scene::geometryUploadBuffer");
			}
		}
		geometryArrayMapped = (ShaderGeometry*)geometryUploadBuffer[device->GetBufferIndex()].mapped_data;

		// Skinning data size is ready at this point:
		skinningDataSize = skinningAllocator.load();
		skinningAllocator.store(0);
		if (skinningUploadBuffer[0].desc.size < skinningDataSize)
		{
			GPUBufferDesc desc;
			desc.size = skinningDataSize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			if (!device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				// Non-UMA: separate Default usage buffer
				device->CreateBuffer(&desc, nullptr, &skinningBuffer);
				device->SetName(&skinningBuffer, "Scene::skinningBuffer");

				// Upload buffer shouldn't be used by shaders with Non-UMA:
				desc.bind_flags = BindFlag::NONE;
				desc.misc_flags = ResourceMiscFlag::NONE;
			}

			desc.usage = Usage::UPLOAD;
			for (int i = 0; i < arraysize(skinningUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &skinningUploadBuffer[i]);
				device->SetName(&skinningUploadBuffer[i], "Scene::skinningUploadBuffer");
			}
		}
		skinningDataMapped = skinningUploadBuffer[device->GetBufferIndex()].mapped_data;

		RunExpressionUpdateSystem(ctx);

		RunMeshUpdateSystem(ctx);

		RunMaterialUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunProceduralAnimationUpdateSystem(ctx);

		RunArmatureUpdateSystem(ctx);

		RunWeatherUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunObjectUpdateSystem(ctx);

		RunCameraUpdateSystem(ctx);

		RunDecalUpdateSystem(ctx);

		RunProbeUpdateSystem(ctx);

		RunForceUpdateSystem(ctx);

		RunLightUpdateSystem(ctx);

		RunParticleUpdateSystem(ctx);

		RunSoundUpdateSystem(ctx);

		RunVideoUpdateSystem(ctx);

		RunImpostorUpdateSystem(ctx);

		RunSpriteUpdateSystem(ctx);

		RunFontUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		// Merge parallel bounds computation (depends on object update system):
		bounds = AABB();
		for (auto& group_bound : parallel_bounds)
		{
			bounds = AABB::Merge(bounds, group_bound);
		}

		// Meshlet buffer:
		uint32_t meshletCount = meshletAllocator.load();
		if(meshletBuffer.desc.size < meshletCount * sizeof(ShaderMeshlet))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMeshlet);
			desc.size = desc.stride * meshletCount * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			bool success = device->CreateBuffer(&desc, nullptr, &meshletBuffer);
			assert(success);
			device->SetName(&meshletBuffer, "meshletBuffer");
		}

		if (IsAccelerationStructureUpdateRequested())
		{
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				// Recreate top level acceleration structure if the object count changed:
				if (TLAS.desc.top_level.count < instanceArraySize)
				{
					RaytracingAccelerationStructureDesc desc;
					desc.flags = RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;
					desc.type = RaytracingAccelerationStructureDesc::Type::TOPLEVEL;
					desc.top_level.count = (uint32_t)instanceArraySize * 2; // *2 to grow fast
					GPUBufferDesc bufdesc;
					bufdesc.misc_flags |= ResourceMiscFlag::RAY_TRACING;
					bufdesc.stride = (uint32_t)device->GetTopLevelAccelerationStructureInstanceSize();
					bufdesc.size = bufdesc.stride * desc.top_level.count;
					bool success = device->CreateBuffer(&bufdesc, nullptr, &desc.top_level.instance_buffer);
					assert(success);
					device->SetName(&desc.top_level.instance_buffer, "Scene::TLAS.instanceBuffer");
					success = device->CreateRaytracingAccelerationStructure(&desc, &TLAS);
					assert(success);
					device->SetName(&TLAS, "Scene::TLAS");
				}
			}
			else
			{
				// Software GPU BVH:
				BVH.Update(*this);
			}
		}

		// Update water ripples:
		for (size_t i = 0; i < waterRipples.size(); ++i)
		{
			auto& ripple = waterRipples[i];
			ripple.Update(dt * 60);

			// Remove inactive ripples:
			if (ripple.params.opacity <= 0 + FLT_EPSILON || ripple.params.fade >= 1 - FLT_EPSILON)
			{
				ripple = waterRipples.back();
				waterRipples.pop_back();
				i--;
			}
		}

		if (wi::renderer::GetSurfelGIEnabled())
		{
			if (!surfelgi.surfelBuffer.IsValid())
			{
				surfelgi.cleared = false;

				GPUBufferDesc buf;
				buf.stride = sizeof(Surfel);
				buf.size = buf.stride * SURFEL_CAPACITY;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				buf.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				device->CreateBuffer(&buf, nullptr, &surfelgi.surfelBuffer);
				device->SetName(&surfelgi.surfelBuffer, "surfelgi.surfelBuffer");

				buf.stride = sizeof(SurfelData);
				buf.size = buf.stride * SURFEL_CAPACITY;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &surfelgi.dataBuffer);
				device->SetName(&surfelgi.dataBuffer, "surfelgi.dataBuffer");

				buf.stride = sizeof(SurfelVarianceDataPacked);
				buf.size = buf.stride * SURFEL_CAPACITY * SURFEL_MOMENT_RESOLUTION * SURFEL_MOMENT_RESOLUTION;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &surfelgi.varianceBuffer);
				device->SetName(&surfelgi.varianceBuffer, "surfelgi.varianceBuffer");

				buf.stride = sizeof(uint);
				buf.size = buf.stride * SURFEL_CAPACITY;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &surfelgi.aliveBuffer[0]);
				device->SetName(&surfelgi.aliveBuffer[0], "surfelgi.aliveBuffer[0]");
				device->CreateBuffer(&buf, nullptr, &surfelgi.aliveBuffer[1]);
				device->SetName(&surfelgi.aliveBuffer[1], "surfelgi.aliveBuffer[1]");

				auto fill_dead_indices = [&](void* dest) {
					uint32_t* dead_indices = (uint32_t*)dest;
					for (uint32_t i = 0; i < SURFEL_CAPACITY; ++i)
					{
						uint32_t ind = uint32_t(SURFEL_CAPACITY - 1 - i);
						std::memcpy(dead_indices + i, &ind, sizeof(ind));
					}
				};
				device->CreateBuffer2(&buf, fill_dead_indices, &surfelgi.deadBuffer);
				device->SetName(&surfelgi.deadBuffer, "surfelgi.deadBuffer");

				buf.stride = sizeof(uint);
				buf.size = SURFEL_STATS_SIZE;
				buf.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				uint stats_data[] = { 0,0,SURFEL_CAPACITY,0,0,0 };
				device->CreateBuffer(&buf, &stats_data, &surfelgi.statsBuffer);
				device->SetName(&surfelgi.statsBuffer, "surfelgi.statsBuffer");

				buf.stride = sizeof(uint);
				buf.size = SURFEL_INDIRECT_SIZE;
				buf.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
				uint indirect_data[] = { 0,0,0, 0,0,0, 0,0,0 };
				device->CreateBuffer(&buf, &indirect_data, &surfelgi.indirectBuffer);
				device->SetName(&surfelgi.indirectBuffer, "surfelgi.indirectBuffer");

				buf.stride = sizeof(SurfelGridCell);
				buf.size = buf.stride * SURFEL_TABLE_SIZE;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &surfelgi.gridBuffer);
				device->SetName(&surfelgi.gridBuffer, "surfelgi.gridBuffer");

				buf.stride = sizeof(uint);
				buf.size = buf.stride * SURFEL_CAPACITY * 27; // each surfel can be in 3x3x3=27 cells
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &surfelgi.cellBuffer);
				device->SetName(&surfelgi.cellBuffer, "surfelgi.cellBuffer");

				buf.stride = sizeof(SurfelRayDataPacked);
				buf.size = buf.stride * SURFEL_RAY_BUDGET;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &surfelgi.rayBuffer);
				device->SetName(&surfelgi.rayBuffer, "surfelgi.rayBuffer");

				TextureDesc tex;
				tex.width = SURFEL_MOMENT_ATLAS_TEXELS;
				tex.height = SURFEL_MOMENT_ATLAS_TEXELS;
				tex.format = Format::R16G16_FLOAT;
				tex.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				tex.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
				device->CreateTexture(&tex, nullptr, &surfelgi.momentsTexture);
				device->SetName(&surfelgi.momentsTexture, "surfelgi.momentsTexture");

				tex.bind_flags = BindFlag::SHADER_RESOURCE;
				tex.misc_flags = ResourceMiscFlag::SPARSE;
				tex.format = Format::BC6H_UF16;
				tex.width = SURFEL_MOMENT_ATLAS_TEXELS;
				tex.height = SURFEL_MOMENT_ATLAS_TEXELS;
				tex.width = std::max(256u, tex.width);		// force non-packed mip behaviour
				tex.height = std::max(256u, tex.height);	// force non-packed mip behaviour
				device->CreateTexture(&tex, nullptr, &surfelgi.irradianceTexture);
				device->SetName(&surfelgi.irradianceTexture, "surfelgi.irradianceTexture");

				tex.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				tex.misc_flags = ResourceMiscFlag::SPARSE;
				tex.width = SURFEL_MOMENT_ATLAS_TEXELS / 4;
				tex.height = SURFEL_MOMENT_ATLAS_TEXELS / 4;
				tex.format = Format::R32G32B32A32_UINT;
				tex.layout = ResourceState::UNORDERED_ACCESS;
				device->CreateTexture(&tex, nullptr, &surfelgi.irradianceTexture_rw);
				device->SetName(&surfelgi.irradianceTexture_rw, "surfelgi.irradianceTexture_rw");

				buf = {};
				buf.alignment = surfelgi.irradianceTexture.sparse_page_size;
				buf.size = surfelgi.irradianceTexture.sparse_properties->total_tile_count * buf.alignment * 2;
				buf.misc_flags = ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS;
				device->CreateBuffer(&buf, nullptr, &surfelgi.sparse_tile_pool);

				SparseUpdateCommand commands[2];
				commands[0].sparse_resource = &surfelgi.irradianceTexture;
				commands[0].tile_pool = &surfelgi.sparse_tile_pool;
				commands[0].num_resource_regions = 1;
				uint32_t tile_count = surfelgi.irradianceTexture_rw.sparse_properties->total_tile_count;
				uint32_t tile_offset[2] = { 0, tile_count };
				SparseRegionSize region;
				region.width = (tex.width + surfelgi.irradianceTexture_rw.sparse_properties->tile_width - 1) / surfelgi.irradianceTexture_rw.sparse_properties->tile_width;
				region.height = (tex.height + surfelgi.irradianceTexture_rw.sparse_properties->tile_height - 1) / surfelgi.irradianceTexture_rw.sparse_properties->tile_height;
				SparseResourceCoordinate coordinate;
				coordinate.x = 0;
				coordinate.y = 0;
				TileRangeFlags flags = TileRangeFlags::None;
				commands[0].sizes = &region;
				commands[0].coordinates = &coordinate;
				commands[0].range_flags = &flags;
				commands[0].range_tile_counts = &tile_count;
				commands[0].range_start_offsets = &tile_offset[0];
				commands[1] = commands[0];
				commands[1].sparse_resource = &surfelgi.irradianceTexture_rw;
				device->SparseUpdate(QUEUE_GRAPHICS, commands, arraysize(commands));
			}
			std::swap(surfelgi.aliveBuffer[0], surfelgi.aliveBuffer[1]);
		}
		else
		{
			surfelgi = {};
		}

		if (wi::renderer::GetDDGIEnabled())
		{
			ddgi.frame_index++;
			if (!ddgi.color_texture_rw.IsValid()) // Check the _rw texture here because that is invalid with serialized DDGI data, and we can detect if dynamic resources need recreation when serialized is loaded
			{
				ddgi.frame_index = 0;

				const uint32_t probe_count = ddgi.grid_dimensions.x * ddgi.grid_dimensions.y * ddgi.grid_dimensions.z;

				GPUBufferDesc buf;
				buf.stride = sizeof(DDGIRayDataPacked);
				buf.size = buf.stride * probe_count * DDGI_MAX_RAYCOUNT;
				buf.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &ddgi.ray_buffer);
				device->SetName(&ddgi.ray_buffer, "ddgi.ray_buffer");

				buf.stride = sizeof(DDGIVarianceDataPacked);
				buf.size = buf.stride * probe_count * DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &ddgi.variance_buffer);
				device->SetName(&ddgi.variance_buffer, "ddgi.variance_buffer");

				buf.stride = sizeof(uint8_t);
				buf.size = buf.stride * probe_count;
				buf.misc_flags = ResourceMiscFlag::NONE;
				buf.format = Format::R8_UINT;
				device->CreateBuffer(&buf, nullptr, &ddgi.raycount_buffer);
				device->SetName(&ddgi.raycount_buffer, "ddgi.raycount_buffer");

				buf.stride = sizeof(uint32_t);
				buf.size = buf.stride * (probe_count * DDGI_MAX_RAYCOUNT + 4); // +4: counter/indirect dispatch args
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				buf.format = Format::UNKNOWN;
				device->CreateBuffer(&buf, nullptr, &ddgi.rayallocation_buffer);
				device->SetName(&ddgi.rayallocation_buffer, "ddgi.rayallocation_buffer");

				TextureDesc tex;
				tex.width = DDGI_COLOR_TEXELS * ddgi.grid_dimensions.x * ddgi.grid_dimensions.y;
				tex.height = DDGI_COLOR_TEXELS * ddgi.grid_dimensions.z;
				tex.format = Format::BC6H_UF16;
				tex.misc_flags = ResourceMiscFlag::SPARSE; // sparse aliasing to write BC6H_UF16 as uint
				tex.width = std::max(256u, tex.width);		// force non-packed mip behaviour
				tex.height = std::max(256u, tex.height);	// force non-packed mip behaviour
				tex.bind_flags = BindFlag::SHADER_RESOURCE;
				tex.layout = ResourceState::SHADER_RESOURCE;
				device->CreateTexture(&tex, nullptr, &ddgi.color_texture);
				device->SetName(&ddgi.color_texture, "ddgi.color_texture");

				tex.format = Format::R32G32B32A32_UINT; // packed BC6H_UF16
				tex.width /= 4;
				tex.height /= 4;
				tex.bind_flags = BindFlag::UNORDERED_ACCESS;
				tex.layout = ResourceState::UNORDERED_ACCESS;
				device->CreateTexture(&tex, nullptr, &ddgi.color_texture_rw);
				device->SetName(&ddgi.color_texture_rw, "ddgi.color_texture_rw");

				buf = {};
				buf.alignment = ddgi.color_texture_rw.sparse_page_size;
				buf.size = ddgi.color_texture_rw.sparse_properties->total_tile_count * buf.alignment * 2;
				buf.misc_flags = ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS;
				device->CreateBuffer(&buf, nullptr, &ddgi.sparse_tile_pool);

				SparseUpdateCommand commands[2];
				commands[0].sparse_resource = &ddgi.color_texture;
				commands[0].tile_pool = &ddgi.sparse_tile_pool;
				commands[0].num_resource_regions = 1;
				uint32_t tile_count = ddgi.color_texture_rw.sparse_properties->total_tile_count;
				uint32_t tile_offset[2] = { 0, tile_count };
				SparseRegionSize region;
				region.width = (tex.width + ddgi.color_texture_rw.sparse_properties->tile_width - 1) / ddgi.color_texture_rw.sparse_properties->tile_width;
				region.height = (tex.height + ddgi.color_texture_rw.sparse_properties->tile_height - 1) / ddgi.color_texture_rw.sparse_properties->tile_height;
				SparseResourceCoordinate coordinate;
				coordinate.x = 0;
				coordinate.y = 0;
				TileRangeFlags flags = TileRangeFlags::None;
				commands[0].sizes = &region;
				commands[0].coordinates = &coordinate;
				commands[0].range_flags = &flags;
				commands[0].range_tile_counts = &tile_count;
				commands[0].range_start_offsets = &tile_offset[0];
				commands[1] = commands[0];
				commands[1].sparse_resource = &ddgi.color_texture_rw;
				device->SparseUpdate(QUEUE_GRAPHICS, commands, arraysize(commands));

				tex.width = DDGI_DEPTH_TEXELS * ddgi.grid_dimensions.x * ddgi.grid_dimensions.y;
				tex.height = DDGI_DEPTH_TEXELS * ddgi.grid_dimensions.z;
				tex.format = Format::R16G16_FLOAT;
				tex.misc_flags = {};
				tex.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				tex.layout = ResourceState::SHADER_RESOURCE;
				device->CreateTexture(&tex, nullptr, &ddgi.depth_texture);
				device->SetName(&ddgi.depth_texture, "ddgi.depth_texture");

				tex.type = TextureDesc::Type::TEXTURE_3D;
				tex.width = ddgi.grid_dimensions.x;
				tex.height = ddgi.grid_dimensions.z;
				tex.depth = ddgi.grid_dimensions.y;
				tex.format = Format::R10G10B10A2_UNORM;
				tex.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				tex.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
				device->CreateTexture(&tex, nullptr, &ddgi.offset_texture);
				device->SetName(&ddgi.offset_texture, "ddgi.offset_texture");
			}
			ddgi.grid_min = bounds.getMin();
			ddgi.grid_min.x -= 1;
			ddgi.grid_min.y -= 1;
			ddgi.grid_min.z -= 1;
			ddgi.grid_max = bounds.getMax();
			ddgi.grid_max.x += 1;
			ddgi.grid_max.y += 1;
			ddgi.grid_max.z += 1;
		}
		else if (ddgi.color_texture_rw.IsValid()) // if color_texture_rw is valid, it means DDGI was not from serialization, so it will be deleted when DDGI is disabled
		{
			ddgi = {};
		}

		if (wi::renderer::GetVXGIEnabled())
		{
			if(!vxgi.radiance.IsValid())
			{
				TextureDesc desc;
				desc.type = TextureDesc::Type::TEXTURE_3D;
				desc.width = vxgi.res * (6 + DIFFUSE_CONE_COUNT);
				desc.height = vxgi.res * VXGI_CLIPMAP_COUNT;
				desc.depth = vxgi.res;
				desc.mip_levels = 1;
				desc.format = Format::R16G16B16A16_FLOAT;
				desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				desc.usage = Usage::DEFAULT;

				device->CreateTexture(&desc, nullptr, &vxgi.radiance);
				device->SetName(&vxgi.radiance, "vxgi.radiance");

				device->CreateTexture(&desc, nullptr, &vxgi.prev_radiance);
				device->SetName(&vxgi.prev_radiance, "vxgi.prev_radiance");

				vxgi.pre_clear = true;
			}
			if (!vxgi.render_atomic.IsValid())
			{
				TextureDesc desc;
				desc.type = TextureDesc::Type::TEXTURE_3D;
				desc.width = vxgi.res * 6;
				desc.height = vxgi.res;
				desc.depth = vxgi.res * VOXELIZATION_CHANNEL_COUNT;
				desc.mip_levels = 1;
				desc.usage = Usage::DEFAULT;
				desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				desc.format = Format::R32_UINT;
				device->CreateTexture(&desc, nullptr, &vxgi.render_atomic);
				device->SetName(&vxgi.render_atomic, "vxgi.render_atomic");
			}
			if (!vxgi.sdf.IsValid())
			{
				TextureDesc desc;
				desc.type = TextureDesc::Type::TEXTURE_3D;
				desc.width = vxgi.res;
				desc.height = vxgi.res * VXGI_CLIPMAP_COUNT;
				desc.depth = vxgi.res;
				desc.mip_levels = 1;
				desc.usage = Usage::DEFAULT;
				desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				desc.format = Format::R16_FLOAT;
				device->CreateTexture(&desc, nullptr, &vxgi.sdf);
				device->SetName(&vxgi.sdf, "vxgi.sdf");
				device->CreateTexture(&desc, nullptr, &vxgi.sdf_temp);
				device->SetName(&vxgi.sdf_temp, "vxgi.sdf_temp");
			}
			vxgi.clipmap_to_update = (vxgi.clipmap_to_update + 1) % VXGI_CLIPMAP_COUNT;
		}

		if (impostors.GetCount() > 0 && objects.GetCount() > 0)
		{
			impostor_ib_format = GetIndexBufferFormatRaw((uint32_t)objects.GetCount() * 4);

			if (allocated_impostor_capacity < objects.GetCount())
			{
				allocated_impostor_capacity = uint32_t(objects.GetCount() * 2); // *2 to grow fast

				GPUBufferDesc desc;
				desc.usage = Usage::DEFAULT;
				desc.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::TYPED_FORMAT_CASTING | ResourceMiscFlag::INDIRECT_ARGS | ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS;

				const uint64_t alignment =
					device->GetMinOffsetAlignment(&desc) *
					sizeof(IndirectDrawArgsIndexedInstanced) * // additional alignment
					sizeof(MeshComponent::Vertex_POS32) // additional alignment
					;

				desc.size =
					AlignTo(sizeof(IndirectDrawArgsIndexedInstanced), alignment) +	// indirect args
					AlignTo(allocated_impostor_capacity * sizeof(uint) * 6, alignment) +	// indices (must overestimate here for 32-bit indices, because we create 16 bit and 32 bit descriptors)
					AlignTo(allocated_impostor_capacity * sizeof(MeshComponent::Vertex_POS32) * 4, alignment) +	// vertices
					AlignTo(allocated_impostor_capacity * sizeof(MeshComponent::Vertex_NOR) * 4, alignment) +	// vertices
					AlignTo(allocated_impostor_capacity * sizeof(uint2), alignment)		// impostordata
				;
				device->CreateBuffer(&desc, nullptr, &impostorBuffer);
				device->SetName(&impostorBuffer, "impostorBuffer");

				uint64_t buffer_offset = 0ull;

				const uint32_t indirect_stride = sizeof(IndirectDrawArgsIndexedInstanced);
				buffer_offset = AlignTo(buffer_offset, sizeof(IndirectDrawArgsIndexedInstanced)); // additional structured buffer alignment
				buffer_offset = AlignTo(buffer_offset, alignment);
				impostor_indirect.offset = buffer_offset;
				impostor_indirect.size = sizeof(IndirectDrawArgsIndexedInstanced);
				impostor_indirect.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_indirect.offset, impostor_indirect.size, nullptr, &indirect_stride);
				buffer_offset += impostor_indirect.size;

				buffer_offset = AlignTo(buffer_offset, alignment);
				Format format32 = Format::R32_UINT;
				Format format16 = Format::R16_UINT;
				impostor_ib32.offset = buffer_offset;
				impostor_ib32.size = allocated_impostor_capacity * sizeof(uint32_t) * 6;
				impostor_ib16.offset = buffer_offset;
				impostor_ib16.size = allocated_impostor_capacity * sizeof(uint16_t) * 6;
				impostor_ib32.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_ib32.offset, impostor_ib32.size, &format32);
				impostor_ib32.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_ib32.offset, impostor_ib32.size, &format32);
				impostor_ib32.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_ib32.subresource_srv);
				impostor_ib32.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_ib32.subresource_uav);
				buffer_offset += impostor_ib32.size;

				impostor_ib16.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_ib16.offset, impostor_ib16.size, &format16);
				impostor_ib16.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_ib16.offset, impostor_ib16.size, &format16);
				impostor_ib16.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_ib16.subresource_srv);
				impostor_ib16.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_ib16.subresource_uav);

				buffer_offset = AlignTo(buffer_offset, alignment);
				impostor_vb_pos.offset = buffer_offset;
				impostor_vb_pos.size = allocated_impostor_capacity * sizeof(MeshComponent::Vertex_POS32) * 4;
				impostor_vb_pos.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_vb_pos.offset, impostor_vb_pos.size, &MeshComponent::Vertex_POS32::FORMAT);
				impostor_vb_pos.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_vb_pos.offset, impostor_vb_pos.size); // can't have RGB32F format for UAV!
				impostor_vb_pos.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_vb_pos.subresource_srv);
				impostor_vb_pos.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_vb_pos.subresource_uav);
				buffer_offset += impostor_vb_pos.size;

				buffer_offset = AlignTo(buffer_offset, alignment);
				impostor_vb_nor.offset = buffer_offset;
				impostor_vb_nor.size = allocated_impostor_capacity * sizeof(MeshComponent::Vertex_NOR) * 4;
				impostor_vb_nor.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_vb_nor.offset, impostor_vb_nor.size, &MeshComponent::Vertex_NOR::FORMAT);
				impostor_vb_nor.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_vb_nor.offset, impostor_vb_nor.size, &MeshComponent::Vertex_NOR::FORMAT);
				impostor_vb_nor.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_vb_nor.subresource_srv);
				impostor_vb_nor.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_vb_nor.subresource_uav);
				buffer_offset += impostor_vb_nor.size;

				buffer_offset = AlignTo(buffer_offset, alignment);
				impostor_data.offset = buffer_offset;
				impostor_data.size = allocated_impostor_capacity * sizeof(uint2);
				impostor_data.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_data.offset, impostor_data.size);
				impostor_data.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_data.offset, impostor_data.size);
				impostor_data.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_data.subresource_srv);
				impostor_data.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_data.subresource_uav);
				buffer_offset += impostor_data.size;

			}
		}

		// VXGI volume update:
		//	Note: this is using camera that the scene is associated with
		{
			VXGI::ClipMap& clipmap = vxgi.clipmaps[vxgi.clipmap_to_update];
			clipmap.voxelsize = vxgi.clipmaps[0].voxelsize * (1u << vxgi.clipmap_to_update);
			const float texelSize = clipmap.voxelsize * 2;
			XMFLOAT3 center = XMFLOAT3(std::floor(camera.Eye.x / texelSize) * texelSize, std::floor(camera.Eye.y / texelSize) * texelSize, std::floor(camera.Eye.z / texelSize) * texelSize);
			clipmap.offsetfromPrevFrame.x = int((clipmap.center.x - center.x) / texelSize);
			clipmap.offsetfromPrevFrame.y = -int((clipmap.center.y - center.y) / texelSize);
			clipmap.offsetfromPrevFrame.z = int((clipmap.center.z - center.z) / texelSize);
			clipmap.center = center;
			XMFLOAT3 extents = XMFLOAT3(vxgi.res * clipmap.voxelsize, vxgi.res * clipmap.voxelsize, vxgi.res * clipmap.voxelsize);
			if (extents.x != clipmap.extents.x || extents.y != clipmap.extents.y || extents.z != clipmap.extents.z)
			{
				vxgi.pre_clear = true;
			}
			clipmap.extents = extents;
		}

		{
			for (size_t voxelgridIndex = 0; voxelgridIndex < voxel_grids.GetCount(); ++voxelgridIndex)
			{
				wi::VoxelGrid& voxelgrid = voxel_grids[voxelgridIndex];
				Entity entity = voxel_grids.GetEntity(voxelgridIndex);

				const TransformComponent* transform = transforms.GetComponent(entity);
				if (transform != nullptr)
				{
					voxelgrid.center = transform->GetPosition();
					voxelgrid.set_voxelsize(transform->GetScale());
				}
			}
		}

		// Shader scene resources:
		if (device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
		{
			shaderscene.instancebuffer = device->GetDescriptorIndex(&instanceUploadBuffer[device->GetBufferIndex()], SubresourceType::SRV);
			shaderscene.geometrybuffer = device->GetDescriptorIndex(&geometryUploadBuffer[device->GetBufferIndex()], SubresourceType::SRV);
			shaderscene.materialbuffer = device->GetDescriptorIndex(&materialUploadBuffer[device->GetBufferIndex()], SubresourceType::SRV);
		}
		else
		{
			shaderscene.instancebuffer = device->GetDescriptorIndex(&instanceBuffer, SubresourceType::SRV);
			shaderscene.geometrybuffer = device->GetDescriptorIndex(&geometryBuffer, SubresourceType::SRV);
			shaderscene.materialbuffer = device->GetDescriptorIndex(&materialBuffer, SubresourceType::SRV);
		}
		shaderscene.meshletbuffer = device->GetDescriptorIndex(&meshletBuffer, SubresourceType::SRV);
		shaderscene.texturestreamingbuffer = device->GetDescriptorIndex(&textureStreamingFeedbackBuffer, SubresourceType::UAV);
		if (weather.skyMap.IsValid())
		{
			shaderscene.globalenvmap = device->GetDescriptorIndex(&weather.skyMap.GetTexture(), SubresourceType::SRV, weather.skyMap.GetTextureSRGBSubresource());
		}
		else
		{
			shaderscene.globalenvmap = -1;
		}

		if (probes.GetCount() > 0 && probes[0].texture.IsValid())
		{
			shaderscene.globalprobe = device->GetDescriptorIndex(&probes[0].texture, SubresourceType::SRV);
		}
		else if (global_dynamic_probe.texture.IsValid())
		{
			shaderscene.globalprobe = device->GetDescriptorIndex(&global_dynamic_probe.texture, SubresourceType::SRV);
		}
		else
		{
			shaderscene.globalprobe = -1;
		}

		shaderscene.impostorInstanceOffset = impostorInstanceOffset;
		shaderscene.TLAS = device->GetDescriptorIndex(&TLAS, SubresourceType::SRV);
		shaderscene.BVH_counter = device->GetDescriptorIndex(&BVH.primitiveCounterBuffer, SubresourceType::SRV);
		shaderscene.BVH_nodes = device->GetDescriptorIndex(&BVH.bvhNodeBuffer, SubresourceType::SRV);
		shaderscene.BVH_primitives = device->GetDescriptorIndex(&BVH.primitiveBuffer, SubresourceType::SRV);

		shaderscene.aabb_min = bounds.getMin();
		shaderscene.aabb_max = bounds.getMax();
		shaderscene.aabb_extents.x = abs(shaderscene.aabb_max.x - shaderscene.aabb_min.x);
		shaderscene.aabb_extents.y = abs(shaderscene.aabb_max.y - shaderscene.aabb_min.y);
		shaderscene.aabb_extents.z = abs(shaderscene.aabb_max.z - shaderscene.aabb_min.z);
		shaderscene.aabb_extents_rcp.x = 1.0f / shaderscene.aabb_extents.x;
		shaderscene.aabb_extents_rcp.y = 1.0f / shaderscene.aabb_extents.y;
		shaderscene.aabb_extents_rcp.z = 1.0f / shaderscene.aabb_extents.z;

		shaderscene.weather.sun_color = weather.sunColor;
		shaderscene.weather.sun_direction = weather.sunDirection;
		shaderscene.weather.most_important_light_index = weather.most_important_light_index;
		shaderscene.weather.ambient = weather.ambient;
		shaderscene.weather.sky_rotation_sin = std::sin(weather.sky_rotation);
		shaderscene.weather.sky_rotation_cos = std::cos(weather.sky_rotation);
		shaderscene.weather.fog.start = weather.fogStart;
		shaderscene.weather.fog.density = weather.fogDensity;
		shaderscene.weather.fog.height_start = weather.fogHeightStart;
		shaderscene.weather.fog.height_end = weather.fogHeightEnd;
		shaderscene.weather.horizon = weather.horizon;
		shaderscene.weather.zenith = weather.zenith;
		shaderscene.weather.sky_exposure = weather.skyExposure;
		shaderscene.weather.wind.speed = weather.windSpeed;
		shaderscene.weather.wind.randomness = weather.windRandomness;
		shaderscene.weather.wind.wavesize = weather.windWaveSize;
		shaderscene.weather.wind.direction = weather.windDirection;
		shaderscene.weather.atmosphere = weather.atmosphereParameters;
		shaderscene.weather.volumetric_clouds = weather.volumetricCloudParameters;
		shaderscene.weather.ocean.water_color = weather.oceanParameters.waterColor;
		shaderscene.weather.ocean.extinction_color = XMFLOAT4(1 - weather.oceanParameters.extinctionColor.x, 1 - weather.oceanParameters.extinctionColor.y, 1 - weather.oceanParameters.extinctionColor.z, 1);
		shaderscene.weather.ocean.water_height = weather.oceanParameters.waterHeight;
		shaderscene.weather.ocean.patch_size_rcp = 1.0f / weather.oceanParameters.patch_length;
		shaderscene.weather.ocean.texture_displacementmap = device->GetDescriptorIndex(ocean.getDisplacementMap(), SubresourceType::SRV);
		shaderscene.weather.ocean.texture_gradientmap = device->GetDescriptorIndex(ocean.getGradientMap(), SubresourceType::SRV);
		shaderscene.weather.stars = weather.stars;
		XMStoreFloat4x4(&shaderscene.weather.stars_rotation, XMMatrixRotationQuaternion(XMLoadFloat4(&weather.stars_rotation_quaternion)));
		shaderscene.weather.rain_amount = weather.rain_amount;
		shaderscene.weather.rain_length = weather.rain_length;
		shaderscene.weather.rain_speed = weather.rain_speed;
		shaderscene.weather.rain_scale = weather.rain_scale;
		shaderscene.weather.rain_splash_scale = weather.rain_splash_scale;
		shaderscene.weather.rain_color = weather.rain_color;

		shaderscene.ddgi.grid_dimensions = ddgi.grid_dimensions;
		shaderscene.ddgi.probe_count = ddgi.grid_dimensions.x * ddgi.grid_dimensions.y * ddgi.grid_dimensions.z;
		shaderscene.ddgi.color_texture_resolution = uint2(ddgi.color_texture.desc.width, ddgi.color_texture.desc.height);
		shaderscene.ddgi.color_texture_resolution_rcp = float2(1.0f / shaderscene.ddgi.color_texture_resolution.x, 1.0f / shaderscene.ddgi.color_texture_resolution.y);
		shaderscene.ddgi.depth_texture_resolution = uint2(ddgi.depth_texture.desc.width, ddgi.depth_texture.desc.height);
		shaderscene.ddgi.depth_texture_resolution_rcp = float2(1.0f / shaderscene.ddgi.depth_texture_resolution.x, 1.0f / shaderscene.ddgi.depth_texture_resolution.y);
		shaderscene.ddgi.color_texture = device->GetDescriptorIndex(&ddgi.color_texture, SubresourceType::SRV);
		shaderscene.ddgi.depth_texture = device->GetDescriptorIndex(&ddgi.depth_texture, SubresourceType::SRV);
		shaderscene.ddgi.offset_texture = device->GetDescriptorIndex(&ddgi.offset_texture, SubresourceType::SRV);
		shaderscene.ddgi.grid_min = ddgi.grid_min;
		shaderscene.ddgi.grid_extents.x = abs(ddgi.grid_max.x - ddgi.grid_min.x);
		shaderscene.ddgi.grid_extents.y = abs(ddgi.grid_max.y - ddgi.grid_min.y);
		shaderscene.ddgi.grid_extents.z = abs(ddgi.grid_max.z - ddgi.grid_min.z);
		shaderscene.ddgi.grid_extents_rcp.x = 1.0f / shaderscene.ddgi.grid_extents.x;
		shaderscene.ddgi.grid_extents_rcp.y = 1.0f / shaderscene.ddgi.grid_extents.y;
		shaderscene.ddgi.grid_extents_rcp.z = 1.0f / shaderscene.ddgi.grid_extents.z;
		shaderscene.ddgi.smooth_backface = ddgi.smooth_backface;
		shaderscene.ddgi.cell_size.x = shaderscene.ddgi.grid_extents.x / (ddgi.grid_dimensions.x - 1);
		shaderscene.ddgi.cell_size.y = shaderscene.ddgi.grid_extents.y / (ddgi.grid_dimensions.y - 1);
		shaderscene.ddgi.cell_size.z = shaderscene.ddgi.grid_extents.z / (ddgi.grid_dimensions.z - 1);
		shaderscene.ddgi.cell_size_rcp.x = 1.0f / shaderscene.ddgi.cell_size.x;
		shaderscene.ddgi.cell_size_rcp.y = 1.0f / shaderscene.ddgi.cell_size.y;
		shaderscene.ddgi.cell_size_rcp.z = 1.0f / shaderscene.ddgi.cell_size.z;
		shaderscene.ddgi.max_distance = std::max(shaderscene.ddgi.cell_size.x, std::max(shaderscene.ddgi.cell_size.y, shaderscene.ddgi.cell_size.z)) * 1.5f;

		shaderscene.terrain.init();
		if (terrains.GetCount() > 0)
		{
			shaderscene.terrain = terrains[0].GetShaderTerrain();
		}

		shaderscene.voxelgrid.init();
		if (voxel_grids.GetCount() > 0)
		{
			VoxelGrid& voxelgrid = voxel_grids[0];
			const uint64_t required_size = voxelgrid.voxels.size() * sizeof(uint64_t);
			if (voxelgrid_gpu.desc.size < required_size)
			{
				GPUBufferDesc desc;
				desc.size = required_size;
				desc.bind_flags = BindFlag::SHADER_RESOURCE;
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				device->CreateBuffer(&desc, nullptr, &voxelgrid_gpu);
				device->SetName(&voxelgrid_gpu, "voxelgrid_gpu");
			}
			shaderscene.voxelgrid.buffer = device->GetDescriptorIndex(&voxelgrid_gpu, SubresourceType::SRV);
			shaderscene.voxelgrid.resolution = voxelgrid.resolution;
			shaderscene.voxelgrid.resolution_div4 = voxelgrid.resolution_div4;
			shaderscene.voxelgrid.resolution_rcp = voxelgrid.resolution_rcp;
			shaderscene.voxelgrid.center = voxelgrid.center;
			shaderscene.voxelgrid.voxelSize = voxelgrid.voxelSize;
			shaderscene.voxelgrid.voxelSize_rcp = voxelgrid.voxelSize_rcp;
		}
	}
	void Scene::Clear()
	{
		for(auto& entry : componentLibrary.entries)
		{
			entry.second.component_manager->Clear();
		}

		TLAS = RaytracingAccelerationStructure();
		BVH.Clear();
		waterRipples.clear();

		surfelgi = {};
		ddgi = {};
		ocean = {};

		aabb_objects.clear();
		aabb_lights.clear();
		aabb_decals.clear();
		aabb_probes.clear();

		matrix_objects.clear();
		matrix_objects_prev.clear();

		collider_count_cpu = 0;
		collider_count_gpu = 0;
	}
	void Scene::Merge(Scene& other)
	{
		for (auto& entry : componentLibrary.entries)
		{
			entry.second.component_manager->Merge(*other.componentLibrary.entries[entry.first].component_manager);
		}

		bounds = AABB::Merge(bounds, other.bounds);

		if (!ddgi.color_texture.IsValid() && other.ddgi.color_texture.IsValid())
		{
			ddgi = std::move(other.ddgi);
		}

		aabb_objects.insert(aabb_objects.end(), other.aabb_objects.begin(), other.aabb_objects.end());
		aabb_lights.insert(aabb_lights.end(), other.aabb_lights.begin(), other.aabb_lights.end());
		aabb_decals.insert(aabb_decals.end(), other.aabb_decals.begin(), other.aabb_decals.end());
		aabb_probes.insert(aabb_probes.end(), other.aabb_probes.begin(), other.aabb_probes.end());

		matrix_objects.insert(matrix_objects.end(), other.matrix_objects.begin(), other.matrix_objects.end());
		matrix_objects_prev.insert(matrix_objects_prev.end(), other.matrix_objects_prev.begin(), other.matrix_objects_prev.end());

		// Recount colliders:
		collider_allocator_cpu.store(0u);
		collider_allocator_gpu.store(0u);
		collider_deinterleaved_data.reserve(
			sizeof(wi::primitive::AABB) * colliders.GetCount() +
			sizeof(ColliderComponent) * colliders.GetCount() +
			sizeof(ColliderComponent) * colliders.GetCount()
		);
		aabb_colliders_cpu = (wi::primitive::AABB*)collider_deinterleaved_data.data();
		colliders_cpu = (ColliderComponent*)(aabb_colliders_cpu + colliders.GetCount());
		colliders_gpu = colliders_cpu + colliders.GetCount();

		for (size_t i = 0; i < colliders.GetCount(); ++i)
		{
			ColliderComponent& collider = colliders[i];
			Entity entity = colliders.GetEntity(i);
			const TransformComponent* transform = transforms.GetComponent(entity);
			if (transform == nullptr)
				return;

			XMFLOAT3 scale = transform->GetScale();
			collider.sphere.radius = collider.radius * std::max(scale.x, std::max(scale.y, scale.z));
			collider.capsule.radius = collider.sphere.radius;

			XMMATRIX W = XMLoadFloat4x4(&transform->world);
			XMVECTOR offset = XMLoadFloat3(&collider.offset);
			XMVECTOR tail = XMLoadFloat3(&collider.tail);
			offset = XMVector3Transform(offset, W);
			tail = XMVector3Transform(tail, W);

			XMStoreFloat3(&collider.sphere.center, offset);
			XMVECTOR N = XMVector3Normalize(offset - tail);
			offset += N * collider.capsule.radius;
			tail -= N * collider.capsule.radius;
			XMStoreFloat3(&collider.capsule.base, offset);
			XMStoreFloat3(&collider.capsule.tip, tail);

			AABB aabb;

			switch (collider.shape)
			{
			default:
			case ColliderComponent::Shape::Sphere:
				aabb.createFromHalfWidth(collider.sphere.center, XMFLOAT3(collider.sphere.radius, collider.sphere.radius, collider.sphere.radius));
				break;
			case ColliderComponent::Shape::Capsule:
				aabb = collider.capsule.getAABB();
				break;
			case ColliderComponent::Shape::Plane:
			{
				collider.plane.origin = collider.sphere.center;
				XMVECTOR N = XMVectorSet(0, 1, 0, 0);
				N = XMVector3Normalize(XMVector3TransformNormal(N, W));
				XMStoreFloat3(&collider.plane.normal, N);

				aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));

				XMMATRIX PLANE = XMMatrixScaling(collider.radius, 1, collider.radius);
				PLANE = PLANE * XMMatrixTranslationFromVector(XMLoadFloat3(&collider.offset));
				PLANE = PLANE * W;
				aabb = aabb.transform(PLANE);

				PLANE = XMMatrixInverse(nullptr, PLANE);
				XMStoreFloat4x4(&collider.plane.projection, PLANE);
			}
			break;
			}

			if (collider.IsCPUEnabled())
			{
				uint32_t index = collider_allocator_cpu.fetch_add(1u);
				colliders_cpu[index] = collider;
				aabb_colliders_cpu[index] = aabb;
			}
			if (collider.IsGPUEnabled())
			{
				uint32_t index = collider_allocator_gpu.fetch_add(1u);
				colliders_gpu[index] = collider;
			}
		}
		collider_count_cpu = collider_allocator_cpu.load();
		collider_count_gpu = collider_allocator_gpu.load();
		collider_bvh.Build(aabb_colliders_cpu, collider_count_cpu);
	}
	Entity Scene::Instantiate(Scene& prefab, bool attached)
	{
		wi::Timer timer;

		// Duplicate prefab into tmp scene
		//	Note: we directly use componentLibrary.Serialize instead of serializing whole scene
		//	Because prefab scene's resources are already in memory, and we don't need to handle them
		//	Also other generic scene serialization can be skipped
		Scene tmp;
		wi::Archive archive;
		EntitySerializer seri;

		archive.SetReadModeAndResetPos(false);
		prefab.componentLibrary.Serialize(archive, seri);

		archive.SetReadModeAndResetPos(true);
		tmp.componentLibrary.Serialize(archive, seri);

		Entity rootEntity = INVALID_ENTITY;

		if (attached)
		{
			// Create root entity
			rootEntity = CreateEntity();
			tmp.transforms.Create(rootEntity);
			tmp.layers.Create(rootEntity).layerMask = ~0;

			// Parent all unparented transforms to new root entity
			for (size_t i = 0; i < tmp.transforms.GetCount(); ++i)
			{
				Entity entity = tmp.transforms.GetEntity(i);
				if (entity != rootEntity && !tmp.hierarchy.Contains(entity))
				{
					tmp.Component_Attach(entity, rootEntity);
				}
			}
		}

		wi::jobsystem::Wait(seri.ctx); // wait for completion of component serializations background threads here

		Merge(tmp);

		char text[64] = {};
		snprintf(text, arraysize(text), "Scene::Instantiate took %.2f ms", timer.elapsed_milliseconds());
		wi::backlog::post(text);

		return rootEntity;
	}
	void Scene::FindAllEntities(wi::unordered_set<wi::ecs::Entity>& entities) const
	{
		for (auto& entry : componentLibrary.entries)
		{
			entities.insert(entry.second.component_manager->GetEntityArray().begin(), entry.second.component_manager->GetEntityArray().end());
		}
	}

	void Scene::Entity_Remove(Entity entity, bool recursive, bool keep_sorted)
	{
		if (recursive)
		{
			wi::vector<Entity> entities_to_remove;
			for (size_t i = 0; i < hierarchy.GetCount(); ++i)
			{
				const HierarchyComponent& hier = hierarchy[i];
				if (hier.parentID == entity)
				{
					Entity child = hierarchy.GetEntity(i);
					entities_to_remove.push_back(child);
				}
			}
			for (auto& child : entities_to_remove)
			{
				Entity_Remove(child);
			}
		}

		for (auto& entry : componentLibrary.entries)
		{
			if (keep_sorted)
			{
				entry.second.component_manager->Remove_KeepSorted(entity);
			}
			else
			{
				entry.second.component_manager->Remove(entity);
			}
		}
	}
	Entity Scene::Entity_FindByName(const std::string& name, Entity ancestor)
	{
		for (size_t i = 0; i < names.GetCount(); ++i)
		{
			if (names[i] == name)
			{
				Entity entity = names.GetEntity(i);
				if (ancestor != INVALID_ENTITY && !Entity_IsDescendant(entity, ancestor))
					continue;
				return entity;
			}
		}
		return INVALID_ENTITY;
	}
	Entity Scene::Entity_Duplicate(Entity entity)
	{
		wi::Archive archive;
		EntitySerializer seri;

		// First write the root entity to staging area:
		archive.SetReadModeAndResetPos(false);
		Entity_Serialize(archive, seri, entity, EntitySerializeFlags::RECURSIVE);

		// Then deserialize root:
		archive.SetReadModeAndResetPos(true);
		Entity root = Entity_Serialize(archive, seri, INVALID_ENTITY, EntitySerializeFlags::RECURSIVE | EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

		return root;
	}
	bool Scene::Entity_IsDescendant(wi::ecs::Entity entity, wi::ecs::Entity ancestor) const
	{
		const HierarchyComponent* hier = hierarchy.GetComponent(entity);
		while (hier != nullptr)
		{
			if (hier->parentID == ancestor)
				return true;
			hier = hierarchy.GetComponent(hier->parentID);
		}
		return false;
	}
	Entity Scene::Entity_CreateTransform(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		transforms.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateMaterial(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		materials.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateObject(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		objects.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateMesh(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		meshes.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateLight(
		const std::string& name,
		const XMFLOAT3& position,
		const XMFLOAT3& color,
		float intensity,
		float range,
		LightComponent::LightType type,
		float outerConeAngle,
		float innerConeAngle)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		LightComponent& light = lights.Create(entity);
		light.intensity = intensity;
		light.range = range;
		light.color = color;
		light.SetType(type);
		light.outerConeAngle = outerConeAngle;
		light.innerConeAngle = innerConeAngle;

		return entity;
	}
	Entity Scene::Entity_CreateForce(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		ForceFieldComponent& force = forces.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateEnvironmentProbe(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		probes.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateDecal(
		const std::string& name,
		const std::string& textureName,
		const std::string& normalMapName
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		decals.Create(entity);

		MaterialComponent& material = materials.Create(entity);
		material.textures[MaterialComponent::BASECOLORMAP].name = textureName;
		material.textures[MaterialComponent::NORMALMAP].name = normalMapName;
		material.CreateRenderData();

		return entity;
	}
	Entity Scene::Entity_CreateCamera(
		const std::string& name,
		float width, float height, float nearPlane, float farPlane, float fov
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		CameraComponent& camera = cameras.Create(entity);
		camera.CreatePerspective(width, height, nearPlane, farPlane, fov);

		return entity;
	}
	Entity Scene::Entity_CreateEmitter(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		emitters.Create(entity).count = 10;

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		materials.Create(entity).userBlendMode = BLENDMODE_ALPHA;

		return entity;
	}
	Entity Scene::Entity_CreateHair(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		hairs.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		materials.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateSound(
		const std::string& name,
		const std::string& filename,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		if (!filename.empty())
		{
			SoundComponent& sound = sounds.Create(entity);
			sound.filename = filename;
			sound.soundResource = wi::resourcemanager::Load(filename);
			wi::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);
		}

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		return entity;
	}
	Entity Scene::Entity_CreateVideo(
		const std::string& name,
		const std::string& filename
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		if (!filename.empty())
		{
			VideoComponent& video = videos.Create(entity);
			video.filename = filename;
			video.videoResource = wi::resourcemanager::Load(filename);
			wi::video::CreateVideoInstance(&video.videoResource.GetVideo(), &video.videoinstance);
		}

		return entity;
	}
	Entity Scene::Entity_CreateCube(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		if (!name.empty())
		{
			names.Create(entity) = name;
		}

		layers.Create(entity);

		transforms.Create(entity);

		ObjectComponent& object = objects.Create(entity);

		MeshComponent& mesh = meshes.Create(entity);

		// object references the mesh entity (there can be multiple objects referencing one mesh):
		object.meshID = entity;

		mesh.vertex_positions = {
			// -Z
			XMFLOAT3(-1,1,	-1),
			XMFLOAT3(-1,-1,	-1),
			XMFLOAT3(1,-1,	-1),
			XMFLOAT3(1,1,	-1),

			// +Z
			XMFLOAT3(-1,1,	1),
			XMFLOAT3(-1,-1,	1),
			XMFLOAT3(1,-1,	1),
			XMFLOAT3(1,1,	1),

			// -X
			XMFLOAT3(-1, -1,1),
			XMFLOAT3(-1, -1,-1),
			XMFLOAT3(-1, 1,-1),
			XMFLOAT3(-1, 1,1),

			// +X
			XMFLOAT3(1, -1,1),
			XMFLOAT3(1, -1,-1),
			XMFLOAT3(1, 1,-1),
			XMFLOAT3(1, 1,1),

			// -Y
			XMFLOAT3(-1, -1,1),
			XMFLOAT3(-1, -1,-1),
			XMFLOAT3(1, -1,-1),
			XMFLOAT3(1, -1,1),

			// +Y
			XMFLOAT3(-1, 1,1),
			XMFLOAT3(-1, 1,-1),
			XMFLOAT3(1, 1,-1),
			XMFLOAT3(1, 1,1),
		};

		mesh.vertex_normals = {
			XMFLOAT3(0,0,-1),
			XMFLOAT3(0,0,-1),
			XMFLOAT3(0,0,-1),
			XMFLOAT3(0,0,-1),

			XMFLOAT3(0,0,1),
			XMFLOAT3(0,0,1),
			XMFLOAT3(0,0,1),
			XMFLOAT3(0,0,1),

			XMFLOAT3(-1,0,0),
			XMFLOAT3(-1,0,0),
			XMFLOAT3(-1,0,0),
			XMFLOAT3(-1,0,0),

			XMFLOAT3(1,0,0),
			XMFLOAT3(1,0,0),
			XMFLOAT3(1,0,0),
			XMFLOAT3(1,0,0),

			XMFLOAT3(0,-1,0),
			XMFLOAT3(0,-1,0),
			XMFLOAT3(0,-1,0),
			XMFLOAT3(0,-1,0),

			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
		};

		mesh.vertex_uvset_0 = {
			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),
		};

		mesh.indices = {
			0,			1,			2,			0,			2,			3,
			0 + 4,		2 + 4,		1 + 4,		0 + 4,		3 + 4,		2 + 4,		// swapped winding
			0 + 4 * 2,	1 + 4 * 2,	2 + 4 * 2,	0 + 4 * 2,	2 + 4 * 2,	3 + 4 * 2,
			0 + 4 * 3,	2 + 4 * 3,	1 + 4 * 3,	0 + 4 * 3,	3 + 4 * 3,	2 + 4 * 3,	// swapped winding
			0 + 4 * 4,	2 + 4 * 4,	1 + 4 * 4,	0 + 4 * 4,	3 + 4 * 4,	2 + 4 * 4,	// swapped winding
			0 + 4 * 5,	1 + 4 * 5,	2 + 4 * 5,	0 + 4 * 5,	2 + 4 * 5,	3 + 4 * 5,
		};

		// Subset maps a part of the mesh to a material:
		MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
		subset.indexCount = uint32_t(mesh.indices.size());
		materials.Create(entity);
		subset.materialID = entity; // the material component is created on the same entity as the mesh component, though it is not required as it could also use a different material entity

		// vertex buffer GPU data will be packed and uploaded here:
		mesh.CreateRenderData();

		return entity;
	}
	Entity Scene::Entity_CreatePlane(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		if (!name.empty())
		{
			names.Create(entity) = name;
		}

		layers.Create(entity);

		transforms.Create(entity);

		ObjectComponent& object = objects.Create(entity);

		MeshComponent& mesh = meshes.Create(entity);

		// object references the mesh entity (there can be multiple objects referencing one mesh):
		object.meshID = entity;

		mesh.vertex_positions = {
			// +Y
			XMFLOAT3(-1, 0,1),
			XMFLOAT3(-1, 0,-1),
			XMFLOAT3(1, 0,-1),
			XMFLOAT3(1, 0,1),
		};

		mesh.vertex_normals = {
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
		};

		mesh.vertex_uvset_0 = {
			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),
		};

		mesh.indices = {
			0, 1, 2, 0, 2, 3,
		};

		// Subset maps a part of the mesh to a material:
		MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
		subset.indexCount = uint32_t(mesh.indices.size());
		materials.Create(entity);
		subset.materialID = entity; // the material component is created on the same entity as the mesh component, though it is not required as it could also use a different material entity

		// vertex buffer GPU data will be packed and uploaded here:
		mesh.CreateRenderData();

		return entity;
	}
	Entity Scene::Entity_CreateSphere(
		const std::string& name,
		float radius,
		uint32_t latitudeBands,
		uint32_t longitudeBands
	)
	{
		Entity entity = CreateEntity();

		if (!name.empty())
		{
			names.Create(entity) = name;
		}

		layers.Create(entity);

		transforms.Create(entity);

		ObjectComponent& object = objects.Create(entity);

		MeshComponent& mesh = meshes.Create(entity);

		// object references the mesh entity (there can be multiple objects referencing one mesh):
		object.meshID = entity;

		for (uint32_t latNumber = 0; latNumber <= latitudeBands; latNumber++)
		{
			float theta = float(latNumber) * XM_PI / float(latitudeBands);
			float sinTheta = sin(theta);
			float cosTheta = cos(theta);

			for (uint32_t longNumber = 0; longNumber <= longitudeBands; longNumber++)
			{
				float phi = float(longNumber) * 2 * XM_PI / float(longitudeBands);
				float sinPhi = sin(phi);
				float cosPhi = cos(phi);

				XMFLOAT3& position = mesh.vertex_positions.emplace_back();
				XMFLOAT3& normal = mesh.vertex_normals.emplace_back();
				XMFLOAT2& uv = mesh.vertex_uvset_0.emplace_back();

				normal.x = cosPhi * sinTheta;   // x
				normal.y = cosTheta;            // y
				normal.z = sinPhi * sinTheta;   // z
				uv.x = float(longNumber) / float(longitudeBands); // u
				uv.y = float(latNumber) / float(latitudeBands);   // v
				position.x = radius * normal.x;
				position.y = radius * normal.y;
				position.z = radius * normal.z;
			}
		}

		for (uint32_t latNumber = 0; latNumber < latitudeBands; latNumber++)
		{
			for (uint32_t longNumber = 0; longNumber < longitudeBands; longNumber++)
			{
				uint32_t first = (latNumber * (longitudeBands + 1)) + longNumber;
				uint32_t second = first + longitudeBands + 1;

				mesh.indices.push_back(first);
				mesh.indices.push_back(second);
				mesh.indices.push_back(first + 1);

				mesh.indices.push_back(second);
				mesh.indices.push_back(second + 1);
				mesh.indices.push_back(first + 1);
			}
		}

		// Subset maps a part of the mesh to a material:
		MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
		subset.indexCount = uint32_t(mesh.indices.size());
		materials.Create(entity);
		subset.materialID = entity; // the material component is created on the same entity as the mesh component, though it is not required as it could also use a different material entity

		// vertex buffer GPU data will be packed and uploaded here:
		mesh.CreateRenderData();

		return entity;
	}

	void Scene::Component_Attach(Entity entity, Entity parent, bool child_already_in_local_space)
	{
		assert(entity != parent);

		if (hierarchy.Contains(entity))
		{
			Component_Detach(entity);
		}

		HierarchyComponent& parentcomponent = hierarchy.Create(entity);
		parentcomponent.parentID = parent;

		TransformComponent* transform_parent = transforms.GetComponent(parent);
		TransformComponent* transform_child = transforms.GetComponent(entity);
		if (transform_parent != nullptr && transform_child != nullptr)
		{
			if (!child_already_in_local_space)
			{
				XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world));
				transform_child->MatrixTransform(B);
				transform_child->UpdateTransform();
			}
			transform_child->UpdateTransform_Parented(*transform_parent);
		}
	}
	void Scene::Component_Detach(Entity entity)
	{
		const HierarchyComponent* parent = hierarchy.GetComponent(entity);

		if (parent != nullptr)
		{
			TransformComponent* transform = transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				transform->ApplyTransform();
			}

			LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				layer->propagationMask = ~0;
			}

			hierarchy.Remove(entity);
		}
	}
	void Scene::Component_DetachChildren(Entity parent)
	{
		for (size_t i = 0; i < hierarchy.GetCount(); )
		{
			if (hierarchy[i].parentID == parent)
			{
				Entity entity = hierarchy.GetEntity(i);
				Component_Detach(entity);
			}
			else
			{
				++i;
			}
		}
	}

	void Scene::RunAnimationUpdateSystem(wi::jobsystem::context& ctx)
	{
		auto range = wi::profiler::BeginRangeCPU("Animations");

		wi::jobsystem::Wait(animation_dependency_scan_workload);

		wi::jobsystem::Dispatch(ctx, (uint32_t)animation_queue_count, 1, [&](wi::jobsystem::JobArgs args) {

			AnimationQueue& animation_queue = animation_queues[args.jobIndex];
			for (size_t animation_index = 0; animation_index < animation_queue.animations.size(); ++animation_index)
			{
				AnimationComponent& animation = *animation_queue.animations[animation_index];
				animation.last_update_time = animation.timer;

				for (const AnimationComponent::AnimationChannel& channel : animation.channels)
				{
					assert(channel.samplerIndex < (int)animation.samplers.size());
					const AnimationComponent::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
					const Scene* data_scene = sampler.scene == nullptr ? this : (const Scene*)sampler.scene;
					const AnimationDataComponent* animationdata = data_scene->animation_datas.GetComponent(sampler.data);
					if (animationdata == nullptr)
						continue;
					if (animationdata->keyframe_times.empty())
						continue;

					const AnimationComponent::AnimationChannel::PathDataType path_data_type = channel.GetPathDataType();

					float timeFirst = std::numeric_limits<float>::max();
					float timeLast = std::numeric_limits<float>::min();
					int keyLeft = 0;	float timeLeft = std::numeric_limits<float>::min();
					int keyRight = 0;	float timeRight = std::numeric_limits<float>::max();

					// search for usable keyframes:
					for (int k = 0; k < (int)animationdata->keyframe_times.size(); ++k)
					{
						const float time = animationdata->keyframe_times[k];
						if (time < timeFirst)
						{
							timeFirst = time;
						}
						if (time > timeLast)
						{
							timeLast = time;
						}
						if (time <= animation.timer && time > timeLeft)
						{
							timeLeft = time;
							keyLeft = k;
						}
						if (time >= animation.timer && time < timeRight)
						{
							timeRight = time;
							keyRight = k;
						}
					}
					if (path_data_type != AnimationComponent::AnimationChannel::PathDataType::Event)
					{
						if (animation.timer < timeFirst)
						{
							// animation beginning haven't been reached, force first keyframe:
							timeLeft = timeFirst;
							timeRight = timeFirst;
							keyLeft = 0;
							keyRight = 0;
						}
					}
					else
					{
						timeLeft = std::max(timeLeft, timeFirst);
						timeRight = std::max(timeRight, timeLast);
					}

					const float left = animationdata->keyframe_times[keyLeft];
					const float right = animationdata->keyframe_times[keyRight];

					union Interpolator
					{
						XMFLOAT4 f4;
						XMFLOAT3 f3;
						XMFLOAT2 f2;
						float f;
					} interpolator = {};

					TransformComponent* target_transform = nullptr;
					MeshComponent* target_mesh = nullptr;
					LightComponent* target_light = nullptr;
					SoundComponent* target_sound = nullptr;
					EmittedParticleSystem* target_emitter = nullptr;
					CameraComponent* target_camera = nullptr;
					ScriptComponent* target_script = nullptr;
					MaterialComponent* target_material = nullptr;

					if (
						channel.path == AnimationComponent::AnimationChannel::Path::TRANSLATION ||
						channel.path == AnimationComponent::AnimationChannel::Path::ROTATION ||
						channel.path == AnimationComponent::AnimationChannel::Path::SCALE
						)
					{
						target_transform = transforms.GetComponent(channel.target);
						if (target_transform == nullptr)
							continue;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::TRANSLATION:
							interpolator.f3 = target_transform->translation_local;
							break;
						case AnimationComponent::AnimationChannel::Path::ROTATION:
							interpolator.f4 = target_transform->rotation_local;
							break;
						case AnimationComponent::AnimationChannel::Path::SCALE:
							interpolator.f3 = target_transform->scale_local;
							break;
						default:
							break;
						}
					}
					else if (channel.path == AnimationComponent::AnimationChannel::Path::WEIGHTS)
					{
						target_mesh = meshes.GetComponent(channel.target);
						if (target_mesh == nullptr)
						{
							// Also try going through object's mesh reference:
							ObjectComponent* object = objects.GetComponent(channel.target);
							if (object == nullptr)
								continue;
							target_mesh = meshes.GetComponent(object->meshID);
						}
						if (target_mesh == nullptr)
							continue;
						animation.morph_weights_temp.resize(target_mesh->morph_targets.size());
					}
					else if (
						channel.path >= AnimationComponent::AnimationChannel::Path::LIGHT_COLOR &&
						channel.path < AnimationComponent::AnimationChannel::Path::_LIGHT_RANGE_END
						)
					{
						target_light = lights.GetComponent(channel.target);
						if (target_light == nullptr)
							continue;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
							interpolator.f3 = target_light->color;
							break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
							interpolator.f = target_light->intensity;
							break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
							interpolator.f = target_light->range;
							break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
							interpolator.f = target_light->innerConeAngle;
							break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
							interpolator.f = target_light->outerConeAngle;
							break;
						default:
							break;
						}
					}
					else if (
						channel.path >= AnimationComponent::AnimationChannel::Path::SOUND_PLAY &&
						channel.path < AnimationComponent::AnimationChannel::Path::_SOUND_RANGE_END
						)
					{
						target_sound = sounds.GetComponent(channel.target);
						if (target_sound == nullptr)
							continue;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
							interpolator.f = target_sound->volume;
							break;
						default:
							break;
						}
					}
					else if (
						channel.path >= AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT &&
						channel.path < AnimationComponent::AnimationChannel::Path::_EMITTER_RANGE_END
						)
					{
						target_emitter = emitters.GetComponent(channel.target);
						if (target_emitter == nullptr)
							continue;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
							interpolator.f = target_emitter->count;
							break;
						default:
							break;
						}
					}
					else if (
						channel.path >= AnimationComponent::AnimationChannel::Path::CAMERA_FOV &&
						channel.path < AnimationComponent::AnimationChannel::Path::_CAMERA_RANGE_END
						)
					{
						target_camera = cameras.GetComponent(channel.target);
						if (target_camera == nullptr)
							continue;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
							interpolator.f = target_camera->fov;
							break;
						case AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
							interpolator.f = target_camera->focal_length;
							break;
						case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
							interpolator.f = target_camera->aperture_size;
							break;
						case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
							interpolator.f2 = target_camera->aperture_shape;
							break;
						default:
							break;
						}
					}
					else if (
						channel.path >= AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY &&
						channel.path < AnimationComponent::AnimationChannel::Path::_SCRIPT_RANGE_END
						)
					{
						target_script = scripts.GetComponent(channel.target);
						if (target_script == nullptr)
							continue;
					}
					else if (
						channel.path >= AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR &&
						channel.path < AnimationComponent::AnimationChannel::Path::_MATERIAL_RANGE_END
						)
					{
						target_material = materials.GetComponent(channel.target);
						if (target_material == nullptr)
							continue;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
							interpolator.f4 = target_material->baseColor;
							break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
							interpolator.f4 = target_material->emissiveColor;
							break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
							interpolator.f = target_material->roughness;
							break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
							interpolator.f = target_material->metalness;
							break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
							interpolator.f = target_material->reflectance;
							break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
							interpolator.f4 = target_material->texMulAdd;
							break;
						default:
							break;
						}
					}
					else
					{
						assert(0);
						continue;
					}

					if (path_data_type == AnimationComponent::AnimationChannel::PathDataType::Event)
					{
						// No path data, only event trigger:
						if (keyLeft == channel.next_event && animation.timer >= timeLeft)
						{
							channel.next_event++;
							switch (channel.path)
							{
							case AnimationComponent::AnimationChannel::Path::SOUND_PLAY:
								target_sound->Play();
								break;
							case AnimationComponent::AnimationChannel::Path::SOUND_STOP:
								target_sound->Stop();
								break;
							case AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY:
								target_script->Play();
								break;
							case AnimationComponent::AnimationChannel::Path::SCRIPT_STOP:
								target_script->Stop();
								break;
							default:
								break;
							}
						}
					}
					else
					{
						// Path data interpolation:
						switch (sampler.mode)
						{
						default:
						case AnimationComponent::AnimationSampler::Mode::STEP:
						{
							// Nearest neighbor method:
							const int key = wi::math::InverseLerp(timeLeft, timeRight, animation.timer) > 0.5f ? keyRight : keyLeft;
							switch (path_data_type)
							{
							default:
							case AnimationComponent::AnimationChannel::PathDataType::Float:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size());
								interpolator.f = animationdata->keyframe_data[key];
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float2:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 2);
								interpolator.f2 = ((const XMFLOAT2*)animationdata->keyframe_data.data())[key];
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float3:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
								interpolator.f3 = ((const XMFLOAT3*)animationdata->keyframe_data.data())[key];
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float4:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4);
								interpolator.f4 = ((const XMFLOAT4*)animationdata->keyframe_data.data())[key];
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Weights:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size());
								for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
								{
									animation.morph_weights_temp[j] = animationdata->keyframe_data[key * animation.morph_weights_temp.size() + j];
								}
							}
							break;
							}
						}
						break;
						case AnimationComponent::AnimationSampler::Mode::LINEAR:
						{
							// Linear interpolation method:
							float t;
							if (keyLeft == keyRight)
							{
								t = 0;
							}
							else
							{
								t = (animation.timer - left) / (right - left);
							}
							t = saturate(t);

							switch (path_data_type)
							{
							default:
							case AnimationComponent::AnimationChannel::PathDataType::Float:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size());
								float vLeft = animationdata->keyframe_data[keyLeft];
								float vRight = animationdata->keyframe_data[keyRight];
								float vAnim = wi::math::Lerp(vLeft, vRight, t);
								interpolator.f = vAnim;
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float2:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 2);
								const XMFLOAT2* data = (const XMFLOAT2*)animationdata->keyframe_data.data();
								XMVECTOR vLeft = XMLoadFloat2(&data[keyLeft]);
								XMVECTOR vRight = XMLoadFloat2(&data[keyRight]);
								XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
								XMStoreFloat2(&interpolator.f2, vAnim);
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float3:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
								const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
								XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft]);
								XMVECTOR vRight = XMLoadFloat3(&data[keyRight]);
								XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
								XMStoreFloat3(&interpolator.f3, vAnim);
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float4:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4);
								const XMFLOAT4* data = (const XMFLOAT4*)animationdata->keyframe_data.data();
								XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft]);
								XMVECTOR vRight = XMLoadFloat4(&data[keyRight]);
								XMVECTOR vAnim;
								if (channel.path == AnimationComponent::AnimationChannel::Path::ROTATION)
								{
									vAnim = XMQuaternionSlerp(vLeft, vRight, t);
									vAnim = XMQuaternionNormalize(vAnim);
								}
								else
								{
									vAnim = XMVectorLerp(vLeft, vRight, t);
								}
								XMStoreFloat4(&interpolator.f4, vAnim);
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Weights:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size());
								for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
								{
									float vLeft = animationdata->keyframe_data[keyLeft * animation.morph_weights_temp.size() + j];
									float vRight = animationdata->keyframe_data[keyRight * animation.morph_weights_temp.size() + j];
									float vAnim = wi::math::Lerp(vLeft, vRight, t);
									animation.morph_weights_temp[j] = vAnim;
								}
							}
							break;
							}
						}
						break;
						case AnimationComponent::AnimationSampler::Mode::CUBICSPLINE:
						{
							// Cubic Spline interpolation method:
							float t;
							if (keyLeft == keyRight)
							{
								t = 0;
							}
							else
							{
								t = (animation.timer - left) / (right - left);
							}
							t = saturate(t);

							const float t2 = t * t;
							const float t3 = t2 * t;

							switch (path_data_type)
							{
							default:
							case AnimationComponent::AnimationChannel::PathDataType::Float:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size());
								float vLeft = animationdata->keyframe_data[keyLeft * 3 + 1];
								float vLeftTanOut = animationdata->keyframe_data[keyLeft * 3 + 2];
								float vRightTanIn = animationdata->keyframe_data[keyRight * 3 + 0];
								float vRight = animationdata->keyframe_data[keyRight * 3 + 1];
								float vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
								interpolator.f = vAnim;
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float2:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 2 * 3);
								const XMFLOAT2* data = (const XMFLOAT2*)animationdata->keyframe_data.data();
								XMVECTOR vLeft = XMLoadFloat2(&data[keyLeft * 3 + 1]);
								XMVECTOR vLeftTanOut = dt * XMLoadFloat2(&data[keyLeft * 3 + 2]);
								XMVECTOR vRightTanIn = dt * XMLoadFloat2(&data[keyRight * 3 + 0]);
								XMVECTOR vRight = XMLoadFloat2(&data[keyRight * 3 + 1]);
								XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
								XMStoreFloat2(&interpolator.f2, vAnim);
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float3:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3 * 3);
								const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
								XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft * 3 + 1]);
								XMVECTOR vLeftTanOut = dt * XMLoadFloat3(&data[keyLeft * 3 + 2]);
								XMVECTOR vRightTanIn = dt * XMLoadFloat3(&data[keyRight * 3 + 0]);
								XMVECTOR vRight = XMLoadFloat3(&data[keyRight * 3 + 1]);
								XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
								XMStoreFloat3(&interpolator.f3, vAnim);
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Float4:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4 * 3);
								const XMFLOAT4* data = (const XMFLOAT4*)animationdata->keyframe_data.data();
								XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft * 3 + 1]);
								XMVECTOR vLeftTanOut = dt * XMLoadFloat4(&data[keyLeft * 3 + 2]);
								XMVECTOR vRightTanIn = dt * XMLoadFloat4(&data[keyRight * 3 + 0]);
								XMVECTOR vRight = XMLoadFloat4(&data[keyRight * 3 + 1]);
								XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
								if (channel.path == AnimationComponent::AnimationChannel::Path::ROTATION)
								{
									vAnim = XMQuaternionNormalize(vAnim);
								}
								XMStoreFloat4(&interpolator.f4, vAnim);
							}
							break;
							case AnimationComponent::AnimationChannel::PathDataType::Weights:
							{
								assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size() * 3);
								for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
								{
									float vLeft = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 1];
									float vLeftTanOut = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 2];
									float vRightTanIn = animationdata->keyframe_data[(keyRight * animation.morph_weights_temp.size() + j) * 3 + 0];
									float vRight = animationdata->keyframe_data[(keyRight * animation.morph_weights_temp.size() + j) * 3 + 1];
									float vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
									animation.morph_weights_temp[j] = vAnim;
								}
							}
							break;
							}
						}
						break;
						}
					}

					// The interpolated raw values will be blended on top of component values:
					const float t = animation.amount;

					// CheckIf this channel is the root motion bone or not.
					const bool isRootBone = (animation.IsRootMotion() && animation.rootMotionBone != wi::ecs::INVALID_ENTITY && (target_transform == transforms.GetComponent(animation.rootMotionBone)));

					if (target_transform != nullptr)
					{
						target_transform->SetDirty();

						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::TRANSLATION:
						{
							const XMVECTOR aT = XMLoadFloat3(&target_transform->translation_local);
							XMVECTOR bT = XMLoadFloat3(&interpolator.f3);
							if (channel.retargetIndex >= 0 && channel.retargetIndex < (int)animation.retargets.size())
							{
								// Retargeting transfer from source to destination:
								const AnimationComponent::RetargetSourceData& retarget = animation.retargets[channel.retargetIndex];
								TransformComponent* source_transform = data_scene->transforms.GetComponent(retarget.source);
								if (source_transform != nullptr)
								{
									XMMATRIX dstRelativeMatrix = XMLoadFloat4x4(&retarget.dstRelativeMatrix);
									XMMATRIX srcRelativeParentMatrix = XMLoadFloat4x4(&retarget.srcRelativeParentMatrix);
									XMVECTOR S, R; // matrix decompose destinations
									TransformComponent transform = *source_transform;
									XMStoreFloat3(&transform.translation_local, bT);
									XMMATRIX localMatrix = dstRelativeMatrix * transform.GetLocalMatrix() * srcRelativeParentMatrix;
									XMMatrixDecompose(&S, &R, &bT, localMatrix);
								}
							}
							const XMVECTOR T = XMVectorLerp(aT, bT, t);
							if (!isRootBone)
							{
								// Not root motion bone.
								XMStoreFloat3(&target_transform->translation_local, T);
							}
							else
							{
								if (XMVector4Equal(animation.rootPrevTranslation, animation.INVALID_VECTOR) || animation.end < animation.prevLocTimer)
								{
									// If root motion bone.
									animation.rootPrevTranslation = T;
								}

								XMVECTOR rotation_quat = animation.rootPrevRotation;

								if (XMVector4Equal(animation.rootPrevRotation, animation.INVALID_VECTOR) || animation.end < animation.prevRotTimer)
								{
									// If root motion bone.
									rotation_quat = XMLoadFloat4(&target_transform->rotation_local);
								}

								const XMVECTOR root_trans = XMVectorSubtract(T, animation.rootPrevTranslation);
								XMVECTOR inverseQuaternion = XMQuaternionInverse(rotation_quat);
								XMVECTOR rotatedDirectionVector = XMVector3Rotate(root_trans, inverseQuaternion);

								XMMATRIX mat = XMLoadFloat4x4(&target_transform->world);
								rotatedDirectionVector = XMVector4Transform(rotatedDirectionVector, mat);

								// Store root motion offset
								XMStoreFloat3(&animation.rootTranslationOffset, rotatedDirectionVector);
								// If root motion bone.
								animation.rootPrevTranslation = T;
								animation.prevLocTimer = animation.timer;
							}
						}
						break;
						case AnimationComponent::AnimationChannel::Path::ROTATION:
						{
							const XMVECTOR aR = XMLoadFloat4(&target_transform->rotation_local);
							XMVECTOR bR = XMLoadFloat4(&interpolator.f4);
							if (channel.retargetIndex >= 0 && channel.retargetIndex < (int)animation.retargets.size())
							{
								// Retargeting transfer from source to destination:
								const AnimationComponent::RetargetSourceData& retarget = animation.retargets[channel.retargetIndex];
								TransformComponent* source_transform = data_scene->transforms.GetComponent(retarget.source);
								if (source_transform != nullptr)
								{
									XMMATRIX dstRelativeMatrix = XMLoadFloat4x4(&retarget.dstRelativeMatrix);
									XMMATRIX srcRelativeParentMatrix = XMLoadFloat4x4(&retarget.srcRelativeParentMatrix);
									XMVECTOR S, T; // matrix decompose destinations
									TransformComponent transform = *source_transform;
									XMStoreFloat4(&transform.rotation_local, bR);
									XMMATRIX localMatrix = dstRelativeMatrix * transform.GetLocalMatrix() * srcRelativeParentMatrix;
									XMMatrixDecompose(&S, &bR, &T, localMatrix);
								}
							}
							const XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
							if (!isRootBone)
							{
								// Not root motion bone.
								XMStoreFloat4(&target_transform->rotation_local, R);
							}
							else
							{
								if (XMVector4Equal(animation.rootPrevRotation, animation.INVALID_VECTOR) || animation.end < animation.prevRotTimer)
								{
									// If root motion bone.
									animation.rootPrevRotation = R;
								}

								// Assuming q1 and q2 are the two quaternions you want to subtract
								// // Let's say you want to find the relative rotation from q1 to q2
								XMMATRIX mat1 = XMMatrixRotationQuaternion(animation.rootPrevRotation);
								XMMATRIX mat2 = XMMatrixRotationQuaternion(R);
								// Compute the relative rotation matrix by multiplying the inverse of the first rotation
								// by the second rotation
								XMMATRIX relativeRotationMatrix = XMMatrixMultiply(XMMatrixTranspose(mat1), mat2);
								// Extract the quaternion representing the relative rotation
								XMVECTOR relativeRotationQuaternion = XMQuaternionRotationMatrix(relativeRotationMatrix);

								// Store root motion offset
								XMStoreFloat4(&animation.rootRotationOffset, relativeRotationQuaternion);
								// Swap Y and Z Axis for Unknown reason
								const float Y = animation.rootRotationOffset.y;
								animation.rootRotationOffset.y = animation.rootRotationOffset.z;
								animation.rootRotationOffset.z = Y;

								// If root motion bone.
								animation.rootPrevRotation = R;
								animation.prevRotTimer = animation.timer;
							}

						}
						break;
						case AnimationComponent::AnimationChannel::Path::SCALE:
						{
							const XMVECTOR aS = XMLoadFloat3(&target_transform->scale_local);
							XMVECTOR bS = XMLoadFloat3(&interpolator.f3);
							if (channel.retargetIndex >= 0 && channel.retargetIndex < (int)animation.retargets.size())
							{
								// Retargeting transfer from source to destination:
								const AnimationComponent::RetargetSourceData& retarget = animation.retargets[channel.retargetIndex];
								TransformComponent* source_transform = data_scene->transforms.GetComponent(retarget.source);
								if (source_transform != nullptr)
								{
									XMMATRIX dstRelativeMatrix = XMLoadFloat4x4(&retarget.dstRelativeMatrix);
									XMMATRIX srcRelativeParentMatrix = XMLoadFloat4x4(&retarget.srcRelativeParentMatrix);
									XMVECTOR R, T; // matrix decompose destinations
									TransformComponent transform = *source_transform;
									XMStoreFloat3(&transform.scale_local, bS);
									XMMATRIX localMatrix = dstRelativeMatrix * transform.GetLocalMatrix() * srcRelativeParentMatrix;
									XMMatrixDecompose(&bS, &R, &T, localMatrix);
								}
							}
							const XMVECTOR S = XMVectorLerp(aS, bS, t);
							XMStoreFloat3(&target_transform->scale_local, S);
						}
						break;
						default:
							break;
						}
					}

					if (target_mesh != nullptr)
					{
						for (size_t j = 0; j < target_mesh->morph_targets.size(); ++j)
						{
							target_mesh->morph_targets[j].weight = wi::math::Lerp(target_mesh->morph_targets[j].weight, animation.morph_weights_temp[j], t);
						}
					}

					if (target_light != nullptr)
					{
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
						{
							target_light->color = wi::math::Lerp(target_light->color, interpolator.f3, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
						{
							target_light->intensity = wi::math::Lerp(target_light->intensity, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
						{
							target_light->range = wi::math::Lerp(target_light->range, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
						{
							target_light->innerConeAngle = wi::math::Lerp(target_light->innerConeAngle, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
						{
							target_light->outerConeAngle = wi::math::Lerp(target_light->outerConeAngle, interpolator.f, t);
						}
						break;
						default:
							break;
						}
					}

					if (target_sound != nullptr)
					{
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
						{
							target_sound->volume = wi::math::Lerp(target_sound->volume, interpolator.f, t);
						}
						break;
						default:
							break;
						}
					}

					if (target_emitter != nullptr)
					{
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
						{
							target_emitter->count = wi::math::Lerp(target_emitter->count, interpolator.f, t);
						}
						break;
						default:
							break;
						}
					}

					if (target_camera != nullptr)
					{
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
						{
							target_camera->fov = wi::math::Lerp(target_camera->fov, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
						{
							target_camera->focal_length = wi::math::Lerp(target_camera->focal_length, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
						{
							target_camera->aperture_size = wi::math::Lerp(target_camera->aperture_size, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
						{
							target_camera->aperture_shape = wi::math::Lerp(target_camera->aperture_shape, interpolator.f2, t);
						}
						break;
						default:
							break;
						}
					}

					if (target_material != nullptr)
					{
						target_material->SetDirty();

						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
						{
							target_material->baseColor = wi::math::Lerp(target_material->baseColor, interpolator.f4, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
						{
							target_material->emissiveColor = wi::math::Lerp(target_material->emissiveColor, interpolator.f4, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
						{
							target_material->roughness = wi::math::Lerp(target_material->roughness, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
						{
							target_material->metalness = wi::math::Lerp(target_material->metalness, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
						{
							target_material->reflectance = wi::math::Lerp(target_material->reflectance, interpolator.f, t);
						}
						break;
						case AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
						{
							target_material->texMulAdd = wi::math::Lerp(target_material->texMulAdd, interpolator.f4, t);
						}
						break;
						default:
							break;
						}
					}

				}

				if (animation.timer > animation.end && animation.speed > 0)
				{
					if (animation.IsLooped())
					{
						animation.timer = animation.start;
						for (auto& channel : animation.channels)
						{
							channel.next_event = 0;
						}
					}
					else if (animation.IsPingPong())
					{
						animation.timer = animation.end;
						animation.speed = -animation.speed;
					}
					else
					{
						animation.timer = animation.end;
						animation.Pause();
					}
				}
				else if (animation.timer < animation.start && animation.speed < 0)
				{
					if (animation.IsLooped())
					{
						animation.timer = animation.end;
						for (auto& channel : animation.channels)
						{
							channel.next_event = 0;
						}
					}
					else if (animation.IsPingPong())
					{
						animation.timer = animation.start;
						animation.speed = -animation.speed;
					}
					else
					{
						animation.timer = animation.start;
						animation.Pause();
					}
				}

				if (animation.IsPlaying())
				{
					animation.timer += dt * animation.speed;
				}
			}
		});

		wi::jobsystem::Wait(ctx);

		wi::profiler::EndRange(range);
	}
	void Scene::RunTransformUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)transforms.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			TransformComponent& transform = transforms[args.jobIndex];
			transform.UpdateTransform();
		});
	}
	void Scene::RunHierarchyUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)hierarchy.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			HierarchyComponent& hier = hierarchy[args.jobIndex];
			Entity entity = hierarchy.GetEntity(args.jobIndex);

			TransformComponent* transform_child = transforms.GetComponent(entity);
			XMMATRIX worldmatrix;
			if (transform_child != nullptr)
			{
				worldmatrix = transform_child->GetLocalMatrix();
			}

			LayerComponent* layer_child = layers.GetComponent(entity);
			if (layer_child != nullptr)
			{
				layer_child->propagationMask = ~0u; // clear propagation mask to full
			}

			if (transform_child == nullptr && layer_child == nullptr)
				return;

			Entity parentID = hier.parentID;
			while (parentID != INVALID_ENTITY)
			{
				TransformComponent* transform_parent = transforms.GetComponent(parentID);
				if (transform_child != nullptr && transform_parent != nullptr)
				{
					worldmatrix *= transform_parent->GetLocalMatrix();
				}

				LayerComponent* layer_parent = layers.GetComponent(parentID);
				if (layer_child != nullptr && layer_parent != nullptr)
				{
					layer_child->propagationMask &= layer_parent->layerMask;
				}

				const HierarchyComponent* hier_recursive = hierarchy.GetComponent(parentID);
				if (hier_recursive != nullptr)
				{
					parentID = hier_recursive->parentID;
				}
				else
				{
					parentID = INVALID_ENTITY;
				}
			}

			if (transform_child != nullptr)
			{
				XMStoreFloat4x4(&transform_child->world, worldmatrix);
			}

		});
	}
	void Scene::RunExpressionUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < expressions.GetCount(); ++i)
		{
			Entity entity = expressions.GetEntity(i);
			ExpressionComponent& expression_mastering = expressions[i];

			// Procedural blink:
			expression_mastering.blink_timer += expression_mastering.blink_frequency * dt;
			if (expression_mastering.blink_timer >= 1)
			{
				int blink = expression_mastering.presets[(int)ExpressionComponent::Preset::Blink];
				if (blink >= 0 && blink < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[blink];
					expression_mastering.blink_count = std::max(1, expression_mastering.blink_count);
					float one_blink_length = expression_mastering.blink_length * expression_mastering.blink_frequency;
					float all_blink_length = one_blink_length * (float)expression_mastering.blink_count;
					float blink_index = std::floor(wi::math::Lerp(0, (float)expression_mastering.blink_count, (expression_mastering.blink_timer - 1) / all_blink_length));
					float blink_trim = 1 + one_blink_length * blink_index;
					float blink_state = wi::math::InverseLerp(0, one_blink_length, expression_mastering.blink_timer - blink_trim);
					if (blink_state < 0.5f)
					{
						// closing
						expression.weight = wi::math::Lerp(0, 1, saturate(blink_state * 2));
					}
					else
					{
						// opening
						expression.weight = wi::math::Lerp(1, 0, saturate((blink_state - 0.5f) * 2));
					}
					if (expression_mastering.blink_timer >= 1 + all_blink_length)
					{
						expression.weight = 0;
						expression_mastering.blink_timer = -wi::random::GetRandom(0.0f, 1.0f);
					}
					expression.SetDirty();
				}
			}

			// Procedural look:
			if (expression_mastering.look_timer == 0)
			{
				// Roll new random look direction for next look away event:
				float vertical = wi::random::GetRandom(-1.0f, 1.0f);
				float horizontal = wi::random::GetRandom(-1.0f, 1.0f);
				expression_mastering.look_weights[0] = saturate(vertical);
				expression_mastering.look_weights[1] = saturate(-vertical);
				expression_mastering.look_weights[2] = saturate(horizontal);
				expression_mastering.look_weights[3] = saturate(-horizontal);
			}
			expression_mastering.look_timer += expression_mastering.look_frequency * dt;
			if (expression_mastering.look_timer >= 1)
			{
				int looks[] = {
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookDown],
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookUp],
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookLeft],
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookRight],
				};
				for (int idx = 0; idx<arraysize(looks); ++idx)
				{
					int look = looks[idx];
					const float weight = expression_mastering.look_weights[idx];
					if (look >= 0 && look < expression_mastering.expressions.size())
					{
						ExpressionComponent::Expression& expression = expression_mastering.expressions[look];
						float look_state = wi::math::InverseLerp(0, expression_mastering.look_length * expression_mastering.look_frequency, expression_mastering.look_timer - 1);
						if (look_state < 0.25f)
						{
							expression.weight = wi::math::Lerp(0, weight, saturate(look_state * 4));
						}
						else
						{
							expression.weight = wi::math::Lerp(weight, 0, saturate((look_state - 0.75f) * 4));
						}
						expression.SetDirty();
					}
				}
				if (expression_mastering.look_timer >= 1 + expression_mastering.look_length * expression_mastering.look_frequency)
				{
					expression_mastering.look_timer = -wi::random::GetRandom(0.0f, 1.0f);
				}
			}

			// Talking animation based on sound:
			const SoundComponent* sound = sounds.GetComponent(entity);
			const bool voice_playing = sound != nullptr && sound->soundResource.IsValid() && sound->IsPlaying();
			if(voice_playing || expression_mastering.IsForceTalkingEnabled())
			{
				ExpressionComponent::Preset unused_phonemes[4];
				int next = 0;
				for (int phoneme = (int)ExpressionComponent::Preset::Aa; phoneme <= (int)ExpressionComponent::Preset::Oh; phoneme++)
				{
					if (phoneme != (int)expression_mastering.talking_phoneme) // don't allow to select the current phoneme next
					{
						unused_phonemes[next++] = (ExpressionComponent::Preset)phoneme;
						int mouth = expression_mastering.presets[(int)phoneme];
						ExpressionComponent::Expression& expression = expression_mastering.expressions[mouth];
						expression.weight = wi::math::Lerp(expression.weight, 0, 0.4f); // fade out unused
						expression.SetDirty();
					}
				}
				int mouth = expression_mastering.presets[(int)expression_mastering.talking_phoneme];
				ExpressionComponent::Expression& expression = expression_mastering.expressions[mouth];

				if (voice_playing)
				{
					// Take voice sample from audio:
					wi::audio::SampleInfo info = wi::audio::GetSampleInfo(&sound->soundResource.GetSound());
					uint32_t sample_frequency = info.sample_rate * info.channel_count;
					uint64_t current_sample = wi::audio::GetTotalSamplesPlayed(&sound->soundinstance);
					if (sound->IsLooped())
					{
						float total_time = float(current_sample) / float(info.sample_rate);
						if (total_time > sound->soundinstance.loop_begin)
						{
							float loop_length = sound->soundinstance.loop_length > 0 ? sound->soundinstance.loop_length : (float(info.sample_count) / float(sample_frequency));
							float loop_time = std::fmod(total_time - sound->soundinstance.loop_begin, loop_length);
							current_sample = uint64_t(loop_time * info.sample_rate);
						}
					}
					current_sample *= info.channel_count;
					current_sample = std::min(current_sample, info.sample_count);

					float voice = 0;
					const int sample_count = 64;
					for (int sam = 0; sam < sample_count; ++sam)
					{
						voice = std::max(voice, std::abs((float)info.samples[std::min(current_sample + sam, info.sample_count)] / 32768.0f));
					}
					const float strength = 0.4f;
					if (voice > 0.1f)
					{
						expression.weight = wi::math::Lerp(expression.weight, 1, strength);
					}
					else
					{
						expression.weight = wi::math::Lerp(expression.weight, 0, strength);
					}
				}
				else
				{
					float wave = std::sin(time * 30) * 0.5f + 0.5f;
					expression.weight = wave;
				}

				float prev_slope = expression_mastering.talking_weight_prev - expression_mastering.talking_weight_prev_prev;
				float curr_slope = expression.weight - expression_mastering.talking_weight_prev;
				expression_mastering.talking_weight_prev_prev = expression_mastering.talking_weight_prev;
				expression_mastering.talking_weight_prev = expression.weight;
				if (prev_slope < 0 && curr_slope > 0)
				{
					// New phoneme when voice slope valley is detected:
					expression_mastering.talking_phoneme = unused_phonemes[wi::random::GetRandom(0, (int)arraysize(unused_phonemes) - 1)];
				}

				expression.SetDirty();
			}
			else if (expression_mastering._flags & ExpressionComponent::TALKING_ENDED)
			{
				// When talking ended, smoothly blend out all phoneme expressions:
				bool talking_active = false;
				int phonemes[] = {
					expression_mastering.presets[(int)ExpressionComponent::Preset::Aa],
					expression_mastering.presets[(int)ExpressionComponent::Preset::Ih],
					expression_mastering.presets[(int)ExpressionComponent::Preset::Ou],
					expression_mastering.presets[(int)ExpressionComponent::Preset::Ee],
					expression_mastering.presets[(int)ExpressionComponent::Preset::Oh],
				};
				for (auto& phoneme : phonemes)
				{
					if (phoneme < 0)
						continue;
					auto& expression = expression_mastering.expressions[phoneme];
					expression.weight = wi::math::Lerp(expression.weight, 0, 0.4f);
					expression.SetDirty();
					if (expression.weight > 0)
						talking_active = true;
				}
				if (!talking_active)
				{
					expression_mastering._flags &= ~ExpressionComponent::TALKING_ENDED;
				}
			}

			float overrideMouthBlend = 0;
			float overrideBlinkBlend = 0;
			float overrideLookBlend = 0;

			// Pass 1: reset targets that will be modified by expressions:
			//	Also accumulate override weights
			for(ExpressionComponent::Expression& expression : expression_mastering.expressions)
			{
				if (expression.weight > 0)
				{
					const float blend = expression.IsBinary() ? 1 : expression.weight;
					if (expression.override_mouth == ExpressionComponent::Override::Block)
					{
						overrideMouthBlend += 1;
					}
					if (expression.override_mouth == ExpressionComponent::Override::Blend)
					{
						overrideMouthBlend += blend;
					}
					if (expression.override_blink == ExpressionComponent::Override::Block)
					{
						overrideBlinkBlend += 1;
					}
					if (expression.override_blink == ExpressionComponent::Override::Blend)
					{
						overrideBlinkBlend += blend;
					}
					if (expression.override_look == ExpressionComponent::Override::Block)
					{
						overrideLookBlend += 1;
					}
					if (expression.override_look == ExpressionComponent::Override::Blend)
					{
						overrideLookBlend += blend;
					}
				}

				if (!expression.IsDirty())
					continue;

				for (const ExpressionComponent::Expression::MorphTargetBinding& morph_target_binding : expression.morph_target_bindings)
				{
					MeshComponent* mesh = meshes.GetComponent(morph_target_binding.meshID);
					if (mesh != nullptr && (int)mesh->morph_targets.size() > morph_target_binding.index)
					{
						MeshComponent::MorphTarget& morph_target = mesh->morph_targets[morph_target_binding.index];
						if (morph_target.weight > 0)
						{
							morph_target.weight = 0;
						}
					}
				}
			}

			// Override weights are factored in:
			const int mouths[] = {
				expression_mastering.presets[(int)ExpressionComponent::Preset::Aa],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Ih],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Ou],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Ee],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Oh],
			};
			for (int mouth : mouths)
			{
				if (mouth >= 0 && mouth < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[mouth];
					expression.weight *= 1 - saturate(overrideMouthBlend);
				}
			}
			const int blinks[] = {
				expression_mastering.presets[(int)ExpressionComponent::Preset::Blink],
				expression_mastering.presets[(int)ExpressionComponent::Preset::BlinkLeft],
				expression_mastering.presets[(int)ExpressionComponent::Preset::BlinkRight],
			};
			for (int blink : blinks)
			{
				if (blink >= 0 && blink < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[blink];
					expression.weight *= 1 - saturate(overrideBlinkBlend);
				}
			}
			const int looks[] = {
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookUp],
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookDown],
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookLeft],
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookRight],
			};
			for (int look : looks)
			{
				if (look >= 0 && look < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[look];
					expression.weight *= 1 - saturate(overrideLookBlend);
				}
			}

			// Pass 2: apply expressions:
			for (ExpressionComponent::Expression& expression : expression_mastering.expressions)
			{
				if (!expression.IsDirty())
					continue;

				expression.SetDirty(false);
				const float blend = expression.IsBinary() ? (expression.weight > 0 ? 1 : 0) : expression.weight;

				for (const ExpressionComponent::Expression::MorphTargetBinding& morph_target_binding : expression.morph_target_bindings)
				{
					MeshComponent* mesh = meshes.GetComponent(morph_target_binding.meshID);
					if (mesh != nullptr && (int)mesh->morph_targets.size() > morph_target_binding.index)
					{
						MeshComponent::MorphTarget& morph_target = mesh->morph_targets[morph_target_binding.index];
						morph_target.weight = wi::math::Lerp(morph_target.weight, morph_target_binding.weight, blend);
					}
				}
			}
		}
	}
	void Scene::RunProceduralAnimationUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (dt <= 0)
			return;

		auto range = wi::profiler::BeginRangeCPU("Procedural Animations");

		if (inverse_kinematics.GetCount() > 0 || humanoids.GetCount() > 0)
		{
			transforms_temp = transforms.GetComponentArray(); // make copy
		}

		bool recompute_hierarchy = false;
		for (size_t i = 0; i < inverse_kinematics.GetCount(); ++i)
		{
			const InverseKinematicsComponent& ik = inverse_kinematics[i];
			if (ik.IsDisabled())
			{
				continue;
			}
			Entity entity = inverse_kinematics.GetEntity(i);
			size_t transform_index = transforms.GetIndex(entity);
			size_t target_index = transforms.GetIndex(ik.target);
			const HierarchyComponent* hier = hierarchy.GetComponent(entity);
			if (transform_index == ~0ull || target_index == ~0ull || hier == nullptr)
			{
				continue;
			}
			TransformComponent& transform = transforms_temp[transform_index];
			TransformComponent& target = transforms_temp[target_index];

			const XMVECTOR target_pos = target.GetPositionV();
			for (uint32_t iteration = 0; iteration < ik.iteration_count; ++iteration)
			{
				TransformComponent* stack[32] = {};
				Entity parent_entity = hier->parentID;
				TransformComponent* child_transform = &transform;
				for (uint32_t chain = 0; chain < std::min(ik.chain_length, (uint32_t)arraysize(stack)); ++chain)
				{
					recompute_hierarchy = true; // any IK will trigger a full transform hierarchy recompute step at the end(**)

					// stack stores all traversed chain links so far:
					stack[chain] = child_transform;

					// Compute required parent rotation that moves ik transform closer to target transform:
					size_t parent_index = transforms.GetIndex(parent_entity);
					if (parent_index == ~0ull)
						continue;
					TransformComponent& parent_transform = transforms_temp[parent_index];
					const XMVECTOR parent_pos = parent_transform.GetPositionV();
					const XMVECTOR dir_parent_to_ik = XMVector3Normalize(transform.GetPositionV() - parent_pos);
					const XMVECTOR dir_parent_to_target = XMVector3Normalize(target_pos - parent_pos);

					// Check if this transform is part of a humanoid and need some constraining:
					bool constrain = false;
					XMFLOAT3 constraint_min = XMFLOAT3(0, 0, 0);
					XMFLOAT3 constraint_max = XMFLOAT3(0, 0, 0);
					for (size_t humanoid_idx = 0; (humanoid_idx < humanoids.GetCount()) && !constrain; ++humanoid_idx)
					{
						const HumanoidComponent& humanoid = humanoids[humanoid_idx];
						Entity humanoidEntity = humanoids.GetEntity(humanoid_idx);
						const float facing = GetHumanoidDefaultFacing(humanoid, humanoidEntity);
						int bone_type_idx = 0;
						for (auto& bone : humanoid.bones)
						{
							if (bone == parent_entity)
							{
								switch ((HumanoidComponent::HumanoidBone)bone_type_idx)
								{
								default:
									break;
								case HumanoidComponent::HumanoidBone::LeftUpperLeg:
								case HumanoidComponent::HumanoidBone::RightUpperLeg:
									constrain = true;
									constraint_min = XMFLOAT3(XM_PI * 0.6f, XM_PI * 0.1f, XM_PI * 0.1f);
									constraint_max = XMFLOAT3(XM_PI * 0.1f, XM_PI * 0.1f, XM_PI * 0.1f);
									break;
								case HumanoidComponent::HumanoidBone::LeftLowerLeg:
								case HumanoidComponent::HumanoidBone::RightLowerLeg:
									constrain = true;
									constraint_min = XMFLOAT3(0, 0, 0);
									constraint_max = XMFLOAT3(XM_PI * 0.8f, 0, 0);
									break;
								}
							}
							if (constrain)
							{
								// Constraint swapping fixes for flipped model orientations:
								if (facing < 0)
								{
									// Note: this is a fix for VRM 1.0 and Mixamo model
									std::swap(constraint_min, constraint_max);
								}
								const TransformComponent* bone_transform = transforms.GetComponent(bone);
								if (bone_transform != nullptr)
								{
									if (bone_transform->GetForward().z < 0)
									{
										// Note: this is a fix for FBX Mixamo models
										std::swap(constraint_min, constraint_max);
									}
								}
								break;
							}
							bone_type_idx++;
						}
					}

					XMVECTOR Q;
					if (constrain)
					{
						// Apply constrained rotation:
						Q = XMQuaternionIdentity();
						XMMATRIX W = XMLoadFloat4x4(&parent_transform.world);
						const float iteration_count_rcp = 1.0f / (float)ik.iteration_count;
						for (int axis_idx = 0; axis_idx < 3; ++axis_idx)
						{
							XMFLOAT3 axis_floats = XMFLOAT3(0, 0, 0);
							((float*)&axis_floats)[axis_idx] = 1;
							XMVECTOR axis = XMLoadFloat3(&axis_floats);
							const float axis_min = ((float*)&constraint_min)[axis_idx] * iteration_count_rcp;
							const float axis_max = ((float*)&constraint_max)[axis_idx] * iteration_count_rcp;
							axis = XMVector3Normalize(XMVector3TransformNormal(axis, W));
							const XMVECTOR projA = XMVector3Normalize(dir_parent_to_ik - axis * XMVector3Dot(axis, dir_parent_to_ik));
							const XMVECTOR projB = XMVector3Normalize(dir_parent_to_target - axis * XMVector3Dot(axis, dir_parent_to_target));
							float angle = XMVectorGetX(XMVector3AngleBetweenNormals(projA, projB));
							if (XMVectorGetX(XMVector3Dot(XMVector3Cross(projA, projB), axis)) < 0)
							{
								angle = XM_2PI - std::min(angle, axis_min);
							}
							else
							{
								angle = std::min(angle, axis_max);
							}
							const XMVECTOR Q1 = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));
							W = XMMatrixRotationQuaternion(Q1) * W;
							Q = XMQuaternionMultiply(Q1, Q);
						}
						Q = XMQuaternionNormalize(Q);
					}
					else
					{
						// Simple shortest rotation without constraint:
						const XMVECTOR axis = XMVector3Normalize(XMVector3Cross(dir_parent_to_ik, dir_parent_to_target));
						const float angle = XMScalarACos(XMVectorGetX(XMVector3Dot(dir_parent_to_ik, dir_parent_to_target)));
						Q = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));
					}

					// parent to world space:
					parent_transform.ApplyTransform();
					// rotate parent:
					parent_transform.Rotate(Q);
					parent_transform.UpdateTransform();
					// parent back to local space (if parent has parent):
					const HierarchyComponent* hier_parent = hierarchy.GetComponent(parent_entity);
					if (hier_parent != nullptr)
					{
						Entity parent_of_parent_entity = hier_parent->parentID;
						size_t parent_of_parent_index = transforms.GetIndex(parent_of_parent_entity);
						if (parent_of_parent_index != ~0ull)
						{
							const TransformComponent* transform_parent_of_parent = &transforms_temp[parent_of_parent_index];
							XMMATRIX parent_of_parent_inverse = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent_of_parent->world));
							parent_transform.MatrixTransform(parent_of_parent_inverse);
							// Do not call UpdateTransform() here, to keep parent world matrix in world space!
						}
					}

					// update chain from parent to children:
					const TransformComponent* recurse_parent = &parent_transform;
					for (int recurse_chain = (int)chain; recurse_chain >= 0; --recurse_chain)
					{
						stack[recurse_chain]->UpdateTransform_Parented(*recurse_parent);
						recurse_parent = stack[recurse_chain];
					}

					if (hier_parent == nullptr)
					{
						// chain root reached, exit
						break;
					}

					// move up in the chain by one:
					child_transform = &parent_transform;
					parent_entity = hier_parent->parentID;
					assert(chain < (uint32_t)arraysize(stack) - 1); // if this is encountered, just extend stack array size

				}
			}
		}

		for (size_t i = 0; i < humanoids.GetCount(); ++i)
		{
			Entity humanoidEntity = humanoids.GetEntity(i);
			HumanoidComponent& humanoid = humanoids[i];

			// The head is always taken as reference frame transform even for the eyes:
			//	Note: taking eye reference frame transform for the eyes was causing issue with VRM 1.0 because eyes were rotated differently than head
			const Entity headBone = humanoid.bones[size_t(HumanoidComponent::HumanoidBone::Head)];
			if (headBone == INVALID_ENTITY)
				continue;
			const size_t headBoneIndex = transforms.GetIndex(headBone);
			if (headBoneIndex == ~0ull)
				continue;
			const TransformComponent& head_transform = transforms_temp[headBoneIndex];

			const XMVECTOR UP = XMVectorSet(0, 1, 0, 0);
			const XMVECTOR SIDE = XMVectorSet(1, 0, 0, 0);
			const XMVECTOR FORWARD = XMVectorSet(0, 0, GetHumanoidDefaultFacing(humanoid, humanoidEntity), 0);

			struct LookAtSource
			{
				HumanoidComponent::HumanoidBone type;
				XMFLOAT2* rotation_max;
				float* rotation_speed;
				XMFLOAT4* lookAtDeltaRotationState;
			};
			LookAtSource sources[] = {
				{ HumanoidComponent::HumanoidBone::Head, &humanoid.head_rotation_max, &humanoid.head_rotation_speed, &humanoid.lookAtDeltaRotationState_Head },
				{ HumanoidComponent::HumanoidBone::LeftEye, &humanoid.eye_rotation_max, &humanoid.eye_rotation_speed, &humanoid.lookAtDeltaRotationState_LeftEye },
				{ HumanoidComponent::HumanoidBone::RightEye, &humanoid.eye_rotation_max, &humanoid.eye_rotation_speed, &humanoid.lookAtDeltaRotationState_RightEye },
			};

			for (auto& source : sources)
			{
				const Entity bone = humanoid.bones[size_t(source.type)];
				if (bone == INVALID_ENTITY)
					continue;
				const size_t boneIndex = transforms.GetIndex(bone);
				if (boneIndex == ~0ull)
					continue;

				if (boneIndex < transforms_temp.size())
				{
					recompute_hierarchy = true;
					TransformComponent& transform = transforms_temp[boneIndex];
					XMVECTOR Q = XMQuaternionIdentity();

					if (humanoid.IsLookAtEnabled())
					{
						const HierarchyComponent* hier = hierarchy.GetComponent(bone);
						size_t parent_index = hier == nullptr ? ~0ull : transforms.GetIndex(hier->parentID);
						if (parent_index != ~0ull)
						{
							const TransformComponent& parent_transform = transforms_temp[parent_index];
							transform.UpdateTransform_Parented(parent_transform);
						}

						const XMVECTOR P = transform.GetPositionV();
						const XMMATRIX HeadW = XMLoadFloat4x4(&head_transform.world); // take it inside iteration loop!
						const XMMATRIX HeadInverseW = XMMatrixInverse(nullptr, HeadW); // take it inside iteration loop!
						const XMVECTOR TARGET = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&humanoid.lookAt) - P, HeadInverseW));
						const XMVECTOR TARGET_HORIZONTAL = XMVector3Normalize(XMVectorSetY(TARGET, 0));
						const XMVECTOR TARGET_VERTICAL = XMVector3Normalize(XMVectorSetX(TARGET, 0) + FORWARD);

						const float angle_horizontal = wi::math::GetAngle(FORWARD, TARGET_HORIZONTAL, UP, source.rotation_max->x);
						const float angle_vertical = wi::math::GetAngle(FORWARD, TARGET_VERTICAL, SIDE, source.rotation_max->y);

						Q = XMQuaternionNormalize(XMQuaternionRotationRollPitchYaw(angle_vertical, angle_horizontal, 0));

#if 0
						wi::renderer::RenderableLine line;
						line.color_start = XMFLOAT4(0, 0, 1, 1);
						line.color_end = XMFLOAT4(0, 1, 0, 1);
						XMVECTOR E = P + FORWARD;
						XMStoreFloat3(&line.start, P);
						XMStoreFloat3(&line.end, E);
						wi::renderer::DrawLine(line);

						line.color_end = XMFLOAT4(1, 0, 0, 1);
						E = P + TARGET;
						XMStoreFloat3(&line.end, E);
						wi::renderer::DrawLine(line);

						line.color_start = line.color_end = XMFLOAT4(1, 0, 1, 1);
						E = P + UP;
						XMStoreFloat3(&line.end, E);
						wi::renderer::DrawLine(line);

						line.color_start = line.color_end = XMFLOAT4(1, 1, 0, 1);
						E = P + SIDE;
						XMStoreFloat3(&line.end, E);
						wi::renderer::DrawLine(line);

						std::string text = "angle_horizontal = " + std::to_string(angle_horizontal);
						text += "\nangle_vertical = " + std::to_string(angle_vertical);
						wi::renderer::DebugTextParams textparams;
						textparams.flags |= wi::renderer::DebugTextParams::CAMERA_FACING;
						textparams.flags |= wi::renderer::DebugTextParams::CAMERA_SCALING;
						textparams.position = humanoid.lookAt;
						textparams.scaling = 0.8f;
						wi::renderer::DrawDebugText(text.c_str(), textparams);
#endif
					}

					Q = XMQuaternionSlerp(XMLoadFloat4(source.lookAtDeltaRotationState), Q, *source.rotation_speed);
					Q = XMQuaternionNormalize(Q);
					XMStoreFloat4(source.lookAtDeltaRotationState, Q);

					// Local space and world space updated separately:
					transform.Rotate(Q); // local space for having hierarchy recompute at the end
					XMMATRIX W = XMLoadFloat4x4(&transform.world);
					W = XMMatrixRotationQuaternion(Q) * W;
					XMStoreFloat4x4(&transform.world, W); // world space to have immediate feedback from parent to child (head -> eyes)

				}
			}
		}

		if (recompute_hierarchy)
		{
			wi::jobsystem::Dispatch(ctx, (uint32_t)hierarchy.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

				HierarchyComponent& hier = hierarchy[args.jobIndex];
				Entity entity = hierarchy.GetEntity(args.jobIndex);
				size_t child_index = transforms.GetIndex(entity);
				if (child_index == ~0ull)
					return;

				TransformComponent& transform_child = transforms_temp[child_index];
				XMMATRIX worldmatrix = transform_child.GetLocalMatrix();

				Entity parentID = hier.parentID;
				while (parentID != INVALID_ENTITY)
				{
					size_t parent_index = transforms.GetIndex(parentID);
					if (parent_index == ~0ull)
						break;
					TransformComponent& transform_parent = transforms_temp[parent_index];
					worldmatrix *= transform_parent.GetLocalMatrix();

					const HierarchyComponent* hier_recursive = hierarchy.GetComponent(parentID);
					if (hier_recursive != nullptr)
					{
						parentID = hier_recursive->parentID;
					}
					else
					{
						parentID = INVALID_ENTITY;
					}
				}

				// Now the real (not temp) transform world matrix is updated:
				XMStoreFloat4x4(&transforms[child_index].world, worldmatrix);

				});

			wi::jobsystem::Wait(ctx);
		}

		// Colliders:
		collider_allocator_cpu.store(0u);
		collider_allocator_gpu.store(0u);
		collider_deinterleaved_data.reserve(
			sizeof(wi::primitive::AABB) * colliders.GetCount() +
			sizeof(ColliderComponent) * colliders.GetCount() +
			sizeof(ColliderComponent) * colliders.GetCount()
		);
		aabb_colliders_cpu = (wi::primitive::AABB*)collider_deinterleaved_data.data();
		colliders_cpu = (ColliderComponent*)(aabb_colliders_cpu + colliders.GetCount());
		colliders_gpu = colliders_cpu + colliders.GetCount();

		wi::jobsystem::Dispatch(ctx, (uint32_t)colliders.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			ColliderComponent& collider = colliders[args.jobIndex];
			Entity entity = colliders.GetEntity(args.jobIndex);
			const TransformComponent* transform = transforms.GetComponent(entity);
			if (transform == nullptr)
				return;

			XMFLOAT3 scale = transform->GetScale();
			collider.sphere.radius = collider.radius * std::max(scale.x, std::max(scale.y, scale.z));
			collider.capsule.radius = collider.sphere.radius;

			XMMATRIX W = XMLoadFloat4x4(&transform->world);
			XMVECTOR offset = XMLoadFloat3(&collider.offset);
			XMVECTOR tail = XMLoadFloat3(&collider.tail);
			offset = XMVector3Transform(offset, W);
			tail = XMVector3Transform(tail, W);

			XMStoreFloat3(&collider.sphere.center, offset);
			XMVECTOR N = XMVector3Normalize(offset - tail);
			offset += N * collider.capsule.radius;
			tail -= N * collider.capsule.radius;
			XMStoreFloat3(&collider.capsule.base, offset);
			XMStoreFloat3(&collider.capsule.tip, tail);

			AABB aabb;

			switch (collider.shape)
			{
			default:
			case ColliderComponent::Shape::Sphere:
				aabb.createFromHalfWidth(collider.sphere.center, XMFLOAT3(collider.sphere.radius, collider.sphere.radius, collider.sphere.radius));
				break;
			case ColliderComponent::Shape::Capsule:
				aabb = collider.capsule.getAABB();
				break;
			case ColliderComponent::Shape::Plane:
			{
				collider.plane.origin = collider.sphere.center;
				XMVECTOR N = XMVectorSet(0, 1, 0, 0);
				N = XMVector3Normalize(XMVector3TransformNormal(N, W));
				XMStoreFloat3(&collider.plane.normal, N);

				aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));

				XMMATRIX PLANE = XMMatrixScaling(collider.radius, 1, collider.radius);
				PLANE = PLANE * XMMatrixTranslationFromVector(XMLoadFloat3(&collider.offset));
				PLANE = PLANE * W;
				aabb = aabb.transform(PLANE);

				PLANE = XMMatrixInverse(nullptr, PLANE);
				XMStoreFloat4x4(&collider.plane.projection, PLANE);
			}
			break;
			}

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				collider.layerMask = layer->GetLayerMask();
			}

			if (collider.IsCPUEnabled())
			{
				uint32_t index = collider_allocator_cpu.fetch_add(1u);
				colliders_cpu[index] = collider;
				aabb_colliders_cpu[index] = aabb;
			}
			if (collider.IsGPUEnabled())
			{
				uint32_t index = collider_allocator_gpu.fetch_add(1u);
				colliders_gpu[index] = collider;
			}

			});

		wi::jobsystem::Wait(ctx);
		collider_count_cpu = collider_allocator_cpu.load();
		collider_count_gpu = collider_allocator_gpu.load();
		collider_bvh.Build(aabb_colliders_cpu, collider_count_cpu);

		// Springs:
		wi::jobsystem::Wait(spring_dependency_scan_workload);
		wi::jobsystem::Dispatch(ctx, (uint32_t)spring_queues.size(), 1, [this](wi::jobsystem::JobArgs args){
			UpdateSpringsTopDownRecursive(nullptr, *spring_queues[args.jobIndex]);
		});
		wi::jobsystem::Wait(ctx);

		wi::profiler::EndRange(range);
	}
	void Scene::RunArmatureUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)armatures.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			ArmatureComponent& armature = armatures[args.jobIndex];
			Entity entity = armatures.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;
			const TransformComponent& transform = *transforms.GetComponent(entity);

			// The transform world matrices are in world space, but skinning needs them in armature-local space, 
			//	so that the skin is reusable for instanced meshes.
			//	We remove the armature's world matrix from the bone world matrix to obtain the bone local transform
			//	These local bone matrices will only be used for skinning, the actual transform components for the bones
			//	remain unchanged.
			//
			//	This is useful for an other thing too:
			//	If a whole transform tree is transformed by some parent (even gltf import does that to convert from RH to LH space)
			//	then the inverseBindMatrices are not reflected in that because they are not contained in the hierarchy system. 
			//	But this will correct them too.
			XMMATRIX R = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform.world));

			armature.gpuBoneOffset = skinningAllocator.fetch_add(uint32_t(armature.boneCollection.size() * sizeof(ShaderTransform)));
			ShaderTransform* gpu_dst = (ShaderTransform*)((uint8_t*)skinningDataMapped + armature.gpuBoneOffset);

			if (armature.boneData.size() != armature.boneCollection.size())
			{
				armature.boneData.resize(armature.boneCollection.size());
			}

			XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

			uint32_t boneIndex = 0;
			for (Entity boneEntity : armature.boneCollection)
			{
				const TransformComponent* bone = transforms.GetComponent(boneEntity);
				if (bone == nullptr)
					continue;

				XMMATRIX B = XMLoadFloat4x4(&armature.inverseBindMatrices[boneIndex]);
				XMMATRIX W = XMLoadFloat4x4(&bone->world);
				XMMATRIX M = B * W * R;

				XMFLOAT4X4 mat;
				XMStoreFloat4x4(&mat, M);

				ShaderTransform& shadertransform = armature.boneData[boneIndex];
				shadertransform.Create(mat);
				if (skinningDataMapped != nullptr)
				{
					std::memcpy(gpu_dst + boneIndex, &shadertransform, sizeof(shadertransform));
				}

				const float bone_radius = 1;
				XMFLOAT3 bonepos = bone->GetPosition();
				AABB boneAABB;
				boneAABB.createFromHalfWidth(bonepos, XMFLOAT3(bone_radius, bone_radius, bone_radius));
				_min = wi::math::Min(_min, boneAABB._min);
				_max = wi::math::Max(_max, boneAABB._max);

				boneIndex++;
			}

			armature.aabb = AABB(_min, _max);
		});
		wi::jobsystem::Dispatch(ctx, (uint32_t)softbodies.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {
			SoftBodyPhysicsComponent& softbody = softbodies[args.jobIndex];
			const uint32_t dataSize = uint32_t(softbody.boneData.size() * sizeof(ShaderTransform));
			softbody.gpuBoneOffset = skinningAllocator.fetch_add(dataSize);
			ShaderTransform* gpu_dst = (ShaderTransform*)((uint8_t*)skinningDataMapped + softbody.gpuBoneOffset);
			std::memcpy(gpu_dst, softbody.boneData.data(), dataSize);
		});
	}
	void Scene::RunMeshUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)meshes.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			Entity entity = meshes.GetEntity(args.jobIndex);
			MeshComponent& mesh = meshes[args.jobIndex];

			if (mesh.so_pos.IsValid() && mesh.so_pre.IsValid())
			{
				std::swap(mesh.so_pos, mesh.so_pre);
			}

			mesh._flags &= ~MeshComponent::TLAS_FORCE_DOUBLE_SIDED;

			mesh.active_morph_count = 0;
			if (skinningDataMapped != nullptr && !mesh.morph_targets.empty())
			{
				mesh.morphGPUOffset = skinningAllocator.fetch_add(uint32_t(mesh.morph_targets.size() * sizeof(MorphTargetGPU)));
				MorphTargetGPU* gpu_dst = (MorphTargetGPU*)((uint8_t*)skinningDataMapped + mesh.morphGPUOffset);
				for (const MeshComponent::MorphTarget& morph : mesh.morph_targets)
				{
					if (morph.weight > 0)
					{
						MorphTargetGPU morph_target_gpu = {};
						morph_target_gpu.weight = morph.weight;
						morph_target_gpu.offset_pos = (uint)morph.offset_pos;
						morph_target_gpu.offset_nor = (uint)morph.offset_nor;
						morph_target_gpu.offset_tan = ~0u;
						std::memcpy(gpu_dst + mesh.active_morph_count, &morph_target_gpu, sizeof(morph_target_gpu));
						mesh.active_morph_count++;
					}
				}
			}

			if (geometryArrayMapped != nullptr)
			{
				ShaderGeometry geometry;
				geometry.init();
				geometry.ib = mesh.ib.descriptor_srv;
				if (mesh.so_pos.IsValid())
				{
					geometry.vb_pos_wind = mesh.so_pos.descriptor_srv;
				}
				else
				{
					geometry.vb_pos_wind = mesh.vb_pos_wind.descriptor_srv;
				}
				if (mesh.so_nor.IsValid())
				{
					geometry.vb_nor = mesh.so_nor.descriptor_srv;
				}
				else
				{
					geometry.vb_nor = mesh.vb_nor.descriptor_srv;
				}
				if (mesh.so_tan.IsValid())
				{
					geometry.vb_tan = mesh.so_tan.descriptor_srv;
				}
				else
				{
					geometry.vb_tan = mesh.vb_tan.descriptor_srv;
				}
				geometry.vb_col = mesh.vb_col.descriptor_srv;
				geometry.vb_uvs = mesh.vb_uvs.descriptor_srv;
				geometry.vb_atl = mesh.vb_atl.descriptor_srv;
				geometry.vb_pre = mesh.so_pre.descriptor_srv;
				geometry.aabb_min = mesh.aabb._min;
				geometry.aabb_max = mesh.aabb._max;
				geometry.tessellation_factor = mesh.tessellationFactor;
				geometry.uv_range_min = mesh.uv_range_min;
				geometry.uv_range_max = mesh.uv_range_max;

				const ImpostorComponent* impostor = impostors.GetComponent(entity);
				if (impostor != nullptr && impostor->textureIndex >= 0)
				{
					geometry.impostorSliceOffset = impostor->textureIndex * impostorCaptureAngles * 3;
				}

				if (mesh.IsDoubleSided())
				{
					geometry.flags |= SHADERMESH_FLAG_DOUBLE_SIDED;
				}

				mesh.meshletCount = 0;

				uint32_t subsetIndex = 0;
				for (auto& subset : mesh.subsets)
				{
					const MaterialComponent* material = materials.GetComponent(subset.materialID);
					if (material != nullptr)
					{
						subset.materialIndex = (uint32_t)materials.GetIndex(subset.materialID);
					}
					else
					{
						subset.materialIndex = 0;
					}

					geometry.indexOffset = subset.indexOffset;
					geometry.indexCount = subset.indexCount;
					geometry.materialIndex = subset.materialIndex;
					geometry.meshletOffset = mesh.meshletCount;
					geometry.meshletCount = triangle_count_to_meshlet_count(subset.indexCount / 3u);
					mesh.meshletCount += geometry.meshletCount;
					std::memcpy(geometryArrayMapped + mesh.geometryOffset + subsetIndex, &geometry, sizeof(geometry));
					subsetIndex++;
				}
			}

			if (TLAS_instancesMapped != nullptr) // check TLAS, to know if we need to care about BLAS
			{
				if (mesh.BLASes.empty() || !mesh.BLASes[0].IsValid())
				{
					mesh.CreateRaytracingRenderData();
				}

				const uint32_t lod_count = mesh.GetLODCount();
				assert(uint32_t(mesh.BLASes.size()) == lod_count);
				for (uint32_t lod = 0; lod < lod_count; ++lod)
				{
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh.GetLODSubsetRange(lod, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
						if (materials.GetCount() <= subset.materialIndex)
							continue;
						const MaterialComponent& material = materials[subset.materialIndex];

						const uint32_t geometry_index = subsetIndex - first_subset;
						auto& geometry = mesh.BLASes[lod].desc.bottom_level.geometries[geometry_index];
						uint32_t flags = geometry.flags;
						if (material.IsAlphaTestEnabled() || (material.GetFilterMask() & FILTER_TRANSPARENT) || !material.IsCastingShadow())
						{
							geometry.flags &= ~RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE;
						}
						else
						{
							geometry.flags = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE;
						}
						if (flags != geometry.flags || mesh.active_morph_count > 0)
						{
							mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
						}
						if (mesh.streamoutBuffer.IsValid())
						{
							mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
							geometry.triangles.vertex_buffer = mesh.streamoutBuffer;
							geometry.triangles.vertex_byte_offset = mesh.so_pos.offset;
						}
						if (material.IsDoubleSided())
						{
							mesh._flags |= MeshComponent::TLAS_FORCE_DOUBLE_SIDED;
						}
					}
				}
			}

		});
	}
	void Scene::RunMaterialUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)materials.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			MaterialComponent& material = materials[args.jobIndex];
			Entity entity = materials.GetEntity(args.jobIndex);
			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				material.layerMask = layer->layerMask;
			}

			material.texAnimElapsedTime += dt * material.texAnimFrameRate;
			if (material.texAnimElapsedTime >= 1.0f)
			{
				material.texMulAdd.z = fmodf(material.texMulAdd.z + material.texAnimDirection.x, 1);
				material.texMulAdd.w = fmodf(material.texMulAdd.w + material.texAnimDirection.y, 1);
				material.texAnimElapsedTime = 0.0f;

				material.SetDirty();
			}

			material.engineStencilRef = STENCILREF_DEFAULT;
			if (material.IsCustomShader())
			{
				if (material.IsOutlineEnabled())
				{
					material.engineStencilRef = STENCILREF_CUSTOMSHADER_OUTLINE;
				}
				else
				{
					material.engineStencilRef = STENCILREF_CUSTOMSHADER;
				}
			}
			else if (material.IsOutlineEnabled())
			{
				material.engineStencilRef = STENCILREF_OUTLINE;
			}

			if (material.IsDirty())
			{
				material.SetDirty(false);
			}

			material.WriteShaderMaterial(materialArrayMapped + args.jobIndex);

			VideoComponent* video = videos.GetComponent(entity);
			if (video != nullptr)
			{
				// Video attachment will overwrite texture slots on shader side:
				int descriptor = GetDevice()->GetDescriptorIndex(&video->videoinstance.output.texture, SubresourceType::SRV, video->videoinstance.output.subresource_srgb);
				material.WriteShaderTextureSlot(materialArrayMapped + args.jobIndex, BASECOLORMAP, descriptor);
				material.WriteShaderTextureSlot(materialArrayMapped + args.jobIndex, EMISSIVEMAP, descriptor);
			}

			if (textureStreamingFeedbackMapped != nullptr)
			{
				const uint32_t request_packed = textureStreamingFeedbackMapped[args.jobIndex];
				if (request_packed != 0)
				{
					const uint32_t request_uvset0 = request_packed & 0xFFFF;
					const uint32_t request_uvset1 = (request_packed >> 16u) & 0xFFFF;
					for (auto& slot : material.textures)
					{
						if (slot.resource.IsValid())
						{
							slot.resource.StreamingRequestResolution(slot.uvset == 0 ? request_uvset0 : request_uvset1);
						}
					}
				}
			}

		});
	}
	void Scene::RunImpostorUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (dt == 0)
			return;

		if (impostors.GetCount() > 0 && !impostorArray.IsValid())
		{
			GraphicsDevice* device = wi::graphics::GetDevice();

			TextureDesc desc;
			desc.width = impostorTextureDim;
			desc.height = impostorTextureDim;

			desc.sample_count = 8;
			desc.bind_flags = BindFlag::DEPTH_STENCIL;
			desc.format = Format::D16_UNORM;
			desc.layout = ResourceState::DEPTHSTENCIL;
			desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;
			device->CreateTexture(&desc, nullptr, &impostorDepthStencil);
			device->SetName(&impostorDepthStencil, "impostorDepthStencil");

			desc.bind_flags = BindFlag::RENDER_TARGET;
			desc.layout = ResourceState::RENDERTARGET;
			desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;

			desc.format = Format::R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &impostorRenderTarget_Albedo_MSAA);
			device->SetName(&impostorRenderTarget_Albedo_MSAA, "impostorRenderTarget_Albedo_MSAA");
			desc.format = Format::R11G11B10_FLOAT;
			device->CreateTexture(&desc, nullptr, &impostorRenderTarget_Normal_MSAA);
			device->SetName(&impostorRenderTarget_Normal_MSAA, "impostorRenderTarget_Normal_MSAA");
			desc.format = Format::R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &impostorRenderTarget_Surface_MSAA);
			device->SetName(&impostorRenderTarget_Surface_MSAA, "impostorRenderTarget_Surface_MSAA");

			desc.sample_count = 1;
			desc.misc_flags = ResourceMiscFlag::NONE;
			desc.layout = ResourceState::SHADER_RESOURCE;

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET; // Note: RenderTarget required for MSAA resolve dest [PS5]
			desc.format = Format::R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &impostorRenderTarget_Albedo);
			device->SetName(&impostorRenderTarget_Albedo, "impostorRenderTarget_Albedo");
			desc.format = Format::R11G11B10_FLOAT;
			device->CreateTexture(&desc, nullptr, &impostorRenderTarget_Normal);
			device->SetName(&impostorRenderTarget_Normal, "impostorRenderTarget_Normal");
			desc.format = Format::R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &impostorRenderTarget_Surface);
			device->SetName(&impostorRenderTarget_Surface, "impostorRenderTarget_Surface");

			desc.format = Format::BC3_UNORM;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.layout = ResourceState::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			desc.array_size = maxImpostorCount * impostorCaptureAngles * 3;
			device->CreateTexture(&desc, nullptr, &impostorArray);
			device->SetName(&impostorArray, "impostorArray");

			std::string info;
			info += "Created impostor array with " + std::to_string(maxImpostorCount) + " max impostors";
			info += "\n\tResolution (width * height * angles * properties * capacity) = " + std::to_string(impostorTextureDim) + " * " + std::to_string(impostorTextureDim) + " * " + std::to_string(impostorCaptureAngles) + " * 3 * " + std::to_string(maxImpostorCount);
			info += "\n\tRender Sample count = " + std::to_string(impostorRenderTarget_Albedo_MSAA.desc.sample_count);
			info += "\n\tRender Format Albedo = ";
			info += GetFormatString(impostorRenderTarget_Albedo.desc.format);
			info += "\n\tRender Format Normal = ";
			info += GetFormatString(impostorRenderTarget_Normal.desc.format);
			info += "\n\tRender Format Surface = ";
			info += GetFormatString(impostorRenderTarget_Surface.desc.format);
			info += "\n\tDepth Format = ";
			info += GetFormatString(impostorDepthStencil.desc.format);
			info += "\n\tSampled Format = ";
			info += GetFormatString(impostorArray.desc.format);
			size_t total_size = 0;
			total_size += ComputeTextureMemorySizeInBytes(impostorArray.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorDepthStencil.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorRenderTarget_Albedo.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorRenderTarget_Surface.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorRenderTarget_Normal.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorRenderTarget_Albedo_MSAA.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorRenderTarget_Surface_MSAA.desc);
			total_size += ComputeTextureMemorySizeInBytes(impostorRenderTarget_Normal_MSAA.desc);
			info += "\n\tMemory = " + wi::helper::GetMemorySizeText(total_size) + "\n";
			wi::backlog::post(info);
		}

		// reconstruct impostor array status:
		bool impostorTaken[maxImpostorCount] = {};
		for (size_t i = 0; i < impostors.GetCount(); ++i)
		{
			ImpostorComponent& impostor = impostors[i];
			if (impostor.textureIndex >= 0 && impostor.textureIndex < maxImpostorCount)
			{
				impostorTaken[impostor.textureIndex] = true;
			}
			else
			{
				impostor.textureIndex = -1;
			}
		}

		for (size_t i = 0; i < impostors.GetCount(); ++i)
		{
			ImpostorComponent& impostor = impostors[i];

			if (impostor.IsDirty())
			{
				impostor.SetDirty(false);
				impostor.render_dirty = true;
			}

			if (impostor.render_dirty && impostor.textureIndex < 0)
			{
				// need to take a free impostor texture slot:
				for (int i = 0; i < arraysize(impostorTaken); ++i)
				{
					if (impostorTaken[i] == false)
					{
						impostorTaken[i] = true;
						impostor.textureIndex = i;
						break;
					}
				}
			}
		}

		if (impostors.GetCount() > 0)
		{
			ShaderMaterial material;
			material.init();
			material.shaderType = ~0u;
			std::memcpy(materialArrayMapped + impostorMaterialOffset, &material, sizeof(material));

			ShaderGeometry geometry;
			geometry.init();
			geometry.meshletCount = triangle_count_to_meshlet_count(uint32_t(objects.GetCount()) * 2);
			geometry.meshletOffset = 0; // local meshlet offset
			geometry.ib = impostor_ib_format == Format::R32_UINT ? impostor_ib32.descriptor_srv : impostor_ib16.descriptor_srv;
			geometry.vb_pos_wind = impostor_vb_pos.descriptor_srv;
			geometry.vb_nor = impostor_vb_nor.descriptor_srv;
			geometry.materialIndex = impostorMaterialOffset;
			std::memcpy(geometryArrayMapped + impostorGeometryOffset, &geometry, sizeof(geometry));

			ShaderMeshInstance inst;
			inst.init();
			inst.geometryOffset = impostorGeometryOffset;
			inst.geometryCount = 1;
			inst.baseGeometryOffset = inst.geometryOffset;
			inst.baseGeometryCount = inst.geometryCount;
			inst.meshletOffset = meshletAllocator.fetch_add(geometry.meshletCount); // global meshlet offset
			std::memcpy(instanceArrayMapped + impostorInstanceOffset, &inst, sizeof(inst));
		}
	}
	void Scene::RunObjectUpdateSystem(wi::jobsystem::context& ctx)
	{
		aabb_objects.resize(objects.GetCount());
		matrix_objects.resize(objects.GetCount());
		matrix_objects_prev.resize(objects.GetCount());
		occlusion_results_objects.resize(objects.GetCount());

		meshletAllocator.store(0u);

		parallel_bounds.clear();
		parallel_bounds.resize((size_t)wi::jobsystem::DispatchGroupCount((uint32_t)objects.GetCount(), small_subtask_groupsize));
		
		wi::jobsystem::Dispatch(ctx, (uint32_t)objects.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			Entity entity = objects.GetEntity(args.jobIndex);
			ObjectComponent& object = objects[args.jobIndex];
			AABB& aabb = aabb_objects[args.jobIndex];
			GraphicsDevice* device = wi::graphics::GetDevice();

			// Update occlusion culling status:
			OcclusionResult& occlusion_result = occlusion_results_objects[args.jobIndex];
			if (!wi::renderer::GetFreezeCullingCameraEnabled())
			{
				occlusion_result.occlusionHistory <<= 1u; // advance history by 1 frame
				int query_id = occlusion_result.occlusionQueries[queryheap_idx];
				if (queryResultBuffer[queryheap_idx].mapped_data != nullptr && query_id >= 0)
				{
					uint64_t visible = ((uint64_t*)queryResultBuffer[queryheap_idx].mapped_data)[query_id];
					if (visible)
					{
						occlusion_result.occlusionHistory |= 1; // visible
					}
				}
				else
				{
					occlusion_result.occlusionHistory |= 1; // visible
				}
			}
			occlusion_result.occlusionQueries[queryheap_idx] = -1; // invalidate query

			const LayerComponent* layer = layers.GetComponent(entity);
			uint32_t layerMask;
			if (layer == nullptr)
			{
				layerMask = ~0;
			}
			else
			{
				layerMask = layer->GetLayerMask();
			}

			aabb = AABB();
			object.filterMaskDynamic = 0;
			object.sort_bits = {};
			object.SetDynamic(false);
			object.SetRequestPlanarReflection(false);
			object.fadeDistance = object.draw_distance;

			if (object.meshID != INVALID_ENTITY && meshes.Contains(object.meshID) && transforms.Contains(entity))
			{
				// These will only be valid for a single frame:
				object.mesh_index = (uint32_t)meshes.GetIndex(object.meshID);
				const MeshComponent& mesh = meshes[object.mesh_index];

				if (object.IsWetmapEnabled() && !object.wetmap.IsValid())
				{
					GPUBufferDesc desc;
					desc.size = mesh.vertex_positions.size() * sizeof(uint16_t);
					desc.format = Format::R16_UNORM;
					desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
					device->CreateBuffer(&desc, nullptr, &object.wetmap);
					device->SetName(&object.wetmap, "wetmap");
					object.wetmap_cleared = false;
				}
				else if(!object.IsWetmapEnabled() && object.wetmap.IsValid())
				{
					object.wetmap = {};
				}

				const TransformComponent& transform = *transforms.GetComponent(entity);

				XMMATRIX W = XMLoadFloat4x4(&transform.world);
				aabb = mesh.aabb.transform(W);

				if (mesh.IsSkinned() || mesh.IsDynamic())
				{
					object.SetDynamic(true);
					const ArmatureComponent* armature = armatures.GetComponent(mesh.armatureID);
					if (armature != nullptr)
					{
						aabb = AABB::Merge(aabb, armature->aabb);
					}
				}

				ImpostorComponent* impostor = impostors.GetComponent(object.meshID);
				if (impostor != nullptr)
				{
					object.fadeDistance = std::min(object.fadeDistance, impostor->swapInDistance);
				}

				SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
				if (softbody != nullptr)
				{
					if (wi::physics::IsEnabled())
					{
						// this will be registered as soft body in the next physics update
						softbody->_flags |= SoftBodyPhysicsComponent::SAFE_TO_REGISTER;

						// soft body manipulated with the object matrix
						softbody->worldMatrix = transform.world;
					}

					if (softbody->physicsobject != nullptr)
					{
						// simulation aabb will be used for soft bodies
						aabb = softbody->aabb;

						// soft bodies have no transform, their vertices are simulated in world space
						W = XMMatrixIdentity();
					}
				}

				object.center = aabb.getCenter();
				object.radius = aabb.getRadius();

				// LOD select:
				if (mesh.subsets_per_lod > 0)
				{
					const float distsq = wi::math::DistanceSquared(camera.Eye, object.center);
					const float radius = object.radius;
					const float radiussq = radius * radius;
					if (distsq < radiussq)
					{
						object.lod = 0;
					}
					else
					{
						const float dist = std::sqrt(distsq);
						const float dist_to_sphere = dist - radius;
						object.lod = uint32_t(dist_to_sphere * object.lod_distance_multiplier);
						object.lod = std::min(object.lod, mesh.GetLODCount() - 1);
					}
				}

				union SortBits
				{
					struct
					{
						uint32_t shadertype : MaterialComponent::SHADERTYPE_COUNT;
						uint32_t blendmode : wi::enums::BLENDMODE_COUNT;
						uint32_t doublesided : 1;	// bool
						uint32_t tessellation : 1;	// bool
						uint32_t alphatest : 1;		// bool
						uint32_t customshader : 8;
						uint32_t sort_priority : 4;
					} bits;
					uint32_t value;
				} sort_bits;
				static_assert(sizeof(SortBits) == sizeof(uint32_t));

				sort_bits.bits.tessellation = mesh.GetTessellationFactor() > 0;
				sort_bits.bits.doublesided = mesh.IsDoubleSided();
				sort_bits.bits.sort_priority = object.sort_priority;

				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh.GetLODSubsetRange(object.lod, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
					const MaterialComponent* material = materials.GetComponent(subset.materialID);

					if (material != nullptr)
					{
						object.filterMask |= material->GetFilterMask();

						if (material->HasPlanarReflection())
						{
							object.SetRequestPlanarReflection(true);
						}

						sort_bits.bits.shadertype |= 1 << material->shaderType;
						sort_bits.bits.blendmode |= 1 << material->GetBlendMode();
						sort_bits.bits.doublesided |= material->IsDoubleSided();
						sort_bits.bits.alphatest |= material->IsAlphaTestEnabled();

						int customshader = material->GetCustomShaderID();
						if (customshader >= 0)
						{
							sort_bits.bits.customshader |= 1 << customshader;
						}
					}
				}

				object.sort_bits = sort_bits.value;

				//// Correction matrix for mesh normals with non-uniform object scaling:
				//XMMATRIX worldMatrixInverseTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, W));
				//XMFLOAT4X4 transformIT;
				//XMStoreFloat4x4(&transformIT, worldMatrixInverseTranspose);

				// Create GPU instance data:
				ShaderMeshInstance inst;
				inst.init();
				XMFLOAT4X4 worldMatrixPrev = matrix_objects[args.jobIndex];
				matrix_objects_prev[args.jobIndex] = worldMatrixPrev;
				XMStoreFloat4x4(matrix_objects.data() + args.jobIndex, W);
				XMFLOAT4X4 worldMatrix = matrix_objects[args.jobIndex];

				if (IsFormatUnorm(mesh.position_format) && !mesh.so_pos.IsValid())
				{
					// The UNORM correction is only done for the GPU data!
					XMMATRIX R = mesh.aabb.getUnormRemapMatrix();
					XMStoreFloat4x4(&worldMatrix, R * W);
					XMStoreFloat4x4(&worldMatrixPrev, R * XMLoadFloat4x4(&worldMatrixPrev));
				}
				inst.transform.Create(worldMatrix);
				inst.transformPrev.Create(worldMatrixPrev);

				// Get the quaternion from W because that reflects changes by other components (eg. softbody)
				XMVECTOR S, R, T;
				XMMatrixDecompose(&S, &R, &T, W);
				XMStoreFloat4(&inst.quaternion, R);

				if (object.lightmap.IsValid())
				{
					inst.lightmap = device->GetDescriptorIndex(&object.lightmap, SubresourceType::SRV);
				}
				inst.uid = entity;
				inst.layerMask = layerMask;
				inst.color = wi::math::CompressColor(object.color);
				inst.emissive = wi::math::pack_half3(XMFLOAT3(object.emissiveColor.x * object.emissiveColor.w, object.emissiveColor.y * object.emissiveColor.w, object.emissiveColor.z * object.emissiveColor.w));
				inst.baseGeometryOffset = mesh.geometryOffset;
				inst.baseGeometryCount = (uint)mesh.subsets.size();
				inst.geometryOffset = inst.baseGeometryOffset + first_subset;
				inst.geometryCount = last_subset - first_subset;
				inst.meshletOffset = meshletAllocator.fetch_add(mesh.meshletCount);
				inst.fadeDistance = object.fadeDistance;
				inst.center = object.center;
				inst.radius = object.radius;
				inst.vb_ao = object.vb_ao_srv;
				inst.vb_wetmap = device->GetDescriptorIndex(&object.wetmap, SubresourceType::SRV);
				inst.alphaTest = 1 - object.alphaRef;
				inst.SetUserStencilRef(object.userStencilRef);

				std::memcpy(instanceArrayMapped + args.jobIndex, &inst, sizeof(inst)); // memcpy whole structure into mapped pointer to avoid read from uncached memory

				if (TLAS_instancesMapped != nullptr)
				{
					// TLAS instance data:
					RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
					for (int i = 0; i < arraysize(instance.transform); ++i)
					{
						for (int j = 0; j < arraysize(instance.transform[i]); ++j)
						{
							instance.transform[i][j] = worldMatrix.m[j][i];
						}
					}
					instance.instance_id = args.jobIndex;
					instance.instance_mask = layerMask == 0 ? 0 : 0xFF;
					if (!object.IsRenderable() || !mesh.IsRenderable())
					{
						instance.instance_mask = 0;
					}
					if (!object.IsCastingShadow())
					{
						instance.instance_mask &= ~wi::renderer::raytracing_inclusion_mask_shadow;
					}
					if (object.IsNotVisibleInReflections())
					{
						instance.instance_mask &= ~wi::renderer::raytracing_inclusion_mask_reflection;
					}
					instance.bottom_level = &mesh.BLASes[object.lod];
					instance.instance_contribution_to_hit_group_index = 0;
					instance.flags = 0;

					if (mesh.IsDoubleSided() || mesh._flags & MeshComponent::TLAS_FORCE_DOUBLE_SIDED)
					{
						instance.flags |= RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;
					}

					if (XMVectorGetX(XMMatrixDeterminant(W)) > 0)
					{
						// There is a mismatch between object space winding and BLAS winding:
						//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_raytracing_instance_flags
						instance.flags |= RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
					}

					void* dest = (void*)((size_t)TLAS_instancesMapped + (size_t)args.jobIndex * device->GetTopLevelAccelerationStructureInstanceSize());
					device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
				}

				// lightmap things:
				if (object.IsLightmapRenderRequested() && dt > 0)
				{
					if (!object.lightmap.IsValid())
					{
						object.lightmapWidth = wi::math::GetNextPowerOfTwo(object.lightmapWidth + 1) / 2;
						object.lightmapHeight = wi::math::GetNextPowerOfTwo(object.lightmapHeight + 1) / 2;

						TextureDesc desc;
						desc.width = object.lightmapWidth;
						desc.height = object.lightmapHeight;
						desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
						// Note: we need the full precision format to achieve correct accumulative blending! 
						//	But the final lightmap will be compressed into an optimal format when the rendering is finished
						desc.format = Format::R32G32B32A32_FLOAT;

						device->CreateTexture(&desc, nullptr, &object.lightmap);
						device->SetName(&object.lightmap, "lightmap_renderable");

						object.lightmapIterationCount = 0; // reset accumulation
					}
				}

				if (!object.lightmapTextureData.empty() && !object.lightmap.IsValid())
				{
					// Create a GPU-side per object lightmap if there is none yet, but the data exists already:
					const size_t lightmap_size = object.lightmapTextureData.size();
					if (lightmap_size == object.lightmapWidth * object.lightmapHeight * sizeof(XMFLOAT4))
					{
						object.lightmap.desc.format = Format::R32G32B32A32_FLOAT;
					}
					else if (lightmap_size == object.lightmapWidth * object.lightmapHeight * sizeof(PackedVector::XMFLOAT3PK))
					{
						object.lightmap.desc.format = Format::R11G11B10_FLOAT;
					}
					else if (lightmap_size == (object.lightmapWidth / GetFormatBlockSize(Format::BC6H_UF16)) * (object.lightmapHeight / GetFormatBlockSize(Format::BC6H_UF16)) * GetFormatStride(Format::BC6H_UF16))
					{
						object.lightmap.desc.format = Format::BC6H_UF16;
					}
					else
					{
						assert(0); // unknown data format
					}
					wi::texturehelper::CreateTexture(object.lightmap, object.lightmapTextureData.data(), object.lightmapWidth, object.lightmapHeight, object.lightmap.desc.format);
					device->SetName(&object.lightmap, "lightmap");
				}

				aabb.layerMask = layerMask;

				// parallel bounds computation using shared memory:
				AABB* shared_bounds = (AABB*)args.sharedmemory;
				if (args.isFirstJobInGroup)
				{
					*shared_bounds = aabb_objects[args.jobIndex];
				}
				else
				{
					*shared_bounds = AABB::Merge(*shared_bounds, aabb_objects[args.jobIndex]);
				}
				if (args.isLastJobInGroup)
				{
					parallel_bounds[args.groupID] = *shared_bounds;
				}
			}

		}, sizeof(AABB));
	}
	void Scene::RunCameraUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)cameras.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			CameraComponent& camera = cameras[args.jobIndex];
			Entity entity = cameras.GetEntity(args.jobIndex);
			const TransformComponent* transform = transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				camera.TransformCamera(*transform);
			}
			camera.UpdateCamera();
		});
	}
	void Scene::RunDecalUpdateSystem(wi::jobsystem::context& ctx)
	{
		aabb_decals.resize(decals.GetCount());

		for (size_t i = 0; i < decals.GetCount(); ++i)
		{
			DecalComponent& decal = decals[i];
			Entity entity = decals.GetEntity(i);
			if (!transforms.Contains(entity))
				continue;
			const TransformComponent& transform = *transforms.GetComponent(entity);
			decal.world = transform.world;

			XMMATRIX W = XMLoadFloat4x4(&decal.world);
			XMVECTOR front = XMVectorSet(0, 0, -1, 0);
			front = XMVector3TransformNormal(front, W);
			front = XMVector3Normalize(front);
			XMStoreFloat3(&decal.front, front);

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);
			XMStoreFloat3(&decal.position, T);
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, S);
			decal.range = std::max(scale.x, std::max(scale.y, scale.z)) * 2;

			AABB& aabb = aabb_decals[i];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.transform(transform.world);

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer == nullptr)
			{
				aabb.layerMask = ~0;
			}
			else
			{
				aabb.layerMask = layer->GetLayerMask();
			}

			const MaterialComponent& material = *materials.GetComponent(entity);
			decal.color = material.baseColor;
			decal.emissive = material.GetEmissiveStrength();
			decal.texture = material.textures[MaterialComponent::BASECOLORMAP].resource;
			decal.normal = material.textures[MaterialComponent::NORMALMAP].resource;
			decal.surfacemap = material.textures[MaterialComponent::SURFACEMAP].resource;
			decal.displacementmap = material.textures[MaterialComponent::DISPLACEMENTMAP].resource;
			decal.normal_strength = material.normalMapStrength;
			decal.displacement_strength = material.parallaxOcclusionMapping;
			decal.texMulAdd = material.texMulAdd;
		}
	}
	void Scene::RunProbeUpdateSystem(wi::jobsystem::context& ctx)
	{
		aabb_probes.resize(probes.GetCount());

		if (dt == 0)
			return;

		for (size_t probeIndex = 0; probeIndex < probes.GetCount(); ++probeIndex)
		{
			EnvironmentProbeComponent& probe = probes[probeIndex];
			Entity entity = probes.GetEntity(probeIndex);
			if (!transforms.Contains(entity))
				continue;
			const TransformComponent& transform = *transforms.GetComponent(entity);

			probe.position = transform.GetPosition();

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMStoreFloat4x4(&probe.inverseMatrix, XMMatrixInverse(nullptr, W));

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, S);
			probe.range = std::max(scale.x, std::max(scale.y, scale.z)) * 2;

			AABB& aabb = aabb_probes[probeIndex];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.transform(transform.world);

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer == nullptr)
			{
				aabb.layerMask = ~0;
			}
			else
			{
				aabb.layerMask = layer->GetLayerMask();
			}

			if (probe.IsDirty() || probe.IsRealTime())
			{
				probe.SetDirty(false);
				probe.render_dirty = true;
			}

			probe.CreateRenderData();
		}

		if (probes.GetCount() == 0)
		{
			global_dynamic_probe.SetRealTime(true);
			global_dynamic_probe.resolution = 64;
			global_dynamic_probe.CreateRenderData();
		}
	}
	void Scene::RunForceUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)forces.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			ForceFieldComponent& force = forces[args.jobIndex];
			Entity entity = forces.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;
			const TransformComponent& transform = *transforms.GetComponent(entity);

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);

			XMStoreFloat3(&force.position, T);
			XMStoreFloat3(&force.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, -1, 0, 0), W)));

		});
	}
	void Scene::RunLightUpdateSystem(wi::jobsystem::context& ctx)
	{
		aabb_lights.resize(lights.GetCount());

		wi::jobsystem::Dispatch(ctx, (uint32_t)lights.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			LightComponent& light = lights[args.jobIndex];
			Entity entity = lights.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;
			const TransformComponent& transform = *transforms.GetComponent(entity);
			AABB& aabb = aabb_lights[args.jobIndex];

			light.occlusionquery = -1;

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer == nullptr)
			{
				aabb.layerMask = ~0;
			}
			else
			{
				aabb.layerMask = layer->GetLayerMask();
			}

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);

			XMStoreFloat3(&light.position, T);
			XMStoreFloat4(&light.rotation, R);
			XMStoreFloat3(&light.scale, S);
			XMStoreFloat3(&light.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), W)));

			switch (light.type)
			{
			default:
			case LightComponent::DIRECTIONAL:
				XMStoreFloat3(&light.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), W)));
				aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
				locker.lock();
				if (args.jobIndex < weather.most_important_light_index)
				{
					weather.most_important_light_index = args.jobIndex;
					weather.sunColor = light.color;
					weather.sunColor.x *= light.intensity;
					weather.sunColor.y *= light.intensity;
					weather.sunColor.z *= light.intensity;
					weather.sunDirection = light.direction;
					weather.stars_rotation_quaternion = light.rotation;
				}
				locker.unlock();
				break;
			case LightComponent::SPOT:
				XMStoreFloat3(&light.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), W)));
				aabb.createFromHalfWidth(light.position, XMFLOAT3(light.GetRange(), light.GetRange(), light.GetRange()));
				break;
			case LightComponent::POINT:
				XMStoreFloat3(&light.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), W)));
				aabb.createFromHalfWidth(light.position, XMFLOAT3(light.GetRange(), light.GetRange(), light.GetRange()));
				break;
			}

		});
	}
	void Scene::RunParticleUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)hairs.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			HairParticleSystem& hair = hairs[args.jobIndex];
			Entity entity = hairs.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;

			if (hair.IsDirty())
			{
				hair.SetDirty(false);
			}

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				hair.layerMask = layer->GetLayerMask();
			}

			if (hair.meshID != INVALID_ENTITY)
			{
				const MeshComponent* mesh = meshes.GetComponent(hair.meshID);

				if (mesh != nullptr)
				{
					const TransformComponent& transform = *transforms.GetComponent(entity);

					hair.UpdateCPU(transform, *mesh, dt);
				}
			}

			GraphicsDevice* device = wi::graphics::GetDevice();

			uint32_t indexCount = hair.GetParticleCount() * 6;
			uint32_t triangleCount = indexCount / 3u;
			uint32_t meshletCount = triangle_count_to_meshlet_count(triangleCount);
			uint32_t meshletOffset = meshletAllocator.fetch_add(meshletCount);

			ShaderGeometry geometry;
			geometry.init();
			geometry.indexOffset = 0;
			geometry.indexCount = indexCount;
			geometry.materialIndex = (uint)materials.GetIndex(entity);
			geometry.ib = device->GetDescriptorIndex(&hair.primitiveBuffer, SubresourceType::SRV);
			geometry.vb_pos_wind = hair.vb_pos[0].descriptor_srv;
			geometry.vb_nor = hair.vb_nor.descriptor_srv;
			geometry.vb_pre = hair.vb_pos[1].descriptor_srv;
			geometry.vb_uvs = hair.vb_uvs.descriptor_srv;
			geometry.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_HAIRPARTICLE;
			geometry.meshletOffset = 0;
			geometry.meshletCount = meshletCount;
			geometry.aabb_min = hair.aabb._min;
			geometry.aabb_max = hair.aabb._max;

			size_t geometryAllocation = geometryAllocator.fetch_add(1);
			std::memcpy(geometryArrayMapped + geometryAllocation, &geometry, sizeof(geometry));

			ShaderMeshInstance inst;
			inst.init();
			inst.uid = entity;
			inst.layerMask = hair.layerMask;
			inst.emissive = wi::math::pack_half3(XMFLOAT3(1, 1, 1));
			inst.color = wi::math::CompressColor(XMFLOAT4(1, 1, 1, 1));
			inst.center = hair.aabb.getCenter();
			inst.radius = hair.aabb.getRadius();
			inst.geometryOffset = (uint)geometryAllocation;
			inst.geometryCount = 1;
			inst.baseGeometryOffset = inst.geometryOffset;
			inst.baseGeometryCount = inst.geometryCount;
			inst.meshletOffset = meshletOffset;
			inst.vb_wetmap = hair.wetmap.descriptor_srv;

			XMFLOAT4X4 remapMatrix;
			XMStoreFloat4x4(&remapMatrix, hair.aabb.getUnormRemapMatrix());
			inst.transform.Create(remapMatrix);
			inst.transformPrev = inst.transform;

			const size_t instanceIndex = objects.GetCount() + args.jobIndex;
			std::memcpy(instanceArrayMapped + instanceIndex, &inst, sizeof(inst));

			if (TLAS_instancesMapped != nullptr)
			{
				if (!hair.BLAS.IsValid())
				{
					hair.CreateRaytracingRenderData();
				}
				if (hair.BLAS.IsValid())
				{
					// TLAS instance data:
					RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
					for (int i = 0; i < arraysize(instance.transform); ++i)
					{
						for (int j = 0; j < arraysize(instance.transform[i]); ++j)
						{
							instance.transform[i][j] = remapMatrix.m[j][i];
						}
					}
					instance.instance_id = (uint32_t)instanceIndex;
					instance.instance_mask = hair.layerMask == 0 ? 0 : 0xFF;
					instance.bottom_level = &hair.BLAS;
					instance.instance_contribution_to_hit_group_index = 0;
					instance.flags = RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;

					void* dest = (void*)((size_t)TLAS_instancesMapped + instanceIndex * device->GetTopLevelAccelerationStructureInstanceSize());
					device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
				}
			}

		});

		wi::jobsystem::Dispatch(ctx, (uint32_t)emitters.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			EmittedParticleSystem& emitter = emitters[args.jobIndex];
			Entity entity = emitters.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;

			MaterialComponent* material = materials.GetComponent(entity);
			if (material != nullptr)
			{
				if (!material->IsUsingVertexColors())
				{
					material->SetUseVertexColors(true);
				}
				if (emitter.shaderType == EmittedParticleSystem::PARTICLESHADERTYPE::SOFT_LIGHTING)
				{
					material->shaderType = MaterialComponent::SHADERTYPE_PBR;
				}
				else
				{
					material->shaderType = MaterialComponent::SHADERTYPE_UNLIT;
				}
			}

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				emitter.layerMask = layer->GetLayerMask();
			}

			const TransformComponent& transform = *transforms.GetComponent(entity);
			emitter.UpdateCPU(transform, dt);

			GraphicsDevice* device = wi::graphics::GetDevice();

			ShaderGeometry geometry;
			geometry.init();
			geometry.indexOffset = 0;
			geometry.indexCount = emitter.GetMaxParticleCount() * 6;
			geometry.materialIndex = (uint)materials.GetIndex(entity);
			geometry.ib = device->GetDescriptorIndex(&emitter.primitiveBuffer, SubresourceType::SRV);
			geometry.vb_pos_wind = emitter.vb_pos.descriptor_srv;
			geometry.vb_nor = emitter.vb_nor.descriptor_srv;
			geometry.vb_uvs = emitter.vb_uvs.descriptor_srv;
			geometry.vb_col = emitter.vb_col.descriptor_srv;
			geometry.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_EMITTEDPARTICLE;

			size_t geometryAllocation = geometryAllocator.fetch_add(1);
			std::memcpy(geometryArrayMapped + geometryAllocation, &geometry, sizeof(geometry));

			ShaderMeshInstance inst;
			inst.init();
			inst.uid = entity;
			inst.layerMask = emitter.layerMask;
			inst.emissive = wi::math::pack_half3(XMFLOAT3(1, 1, 1));
			inst.color = wi::math::CompressColor(XMFLOAT4(1, 1, 1, 1));
			inst.geometryOffset = (uint)geometryAllocation;
			inst.geometryCount = 1;
			inst.baseGeometryOffset = inst.geometryOffset;
			inst.baseGeometryCount = inst.geometryCount;

			const size_t instanceIndex = objects.GetCount() + hairs.GetCount() + args.jobIndex;
			std::memcpy(instanceArrayMapped + instanceIndex, &inst, sizeof(inst));

			if (TLAS_instancesMapped != nullptr)
			{
				if (!emitter.BLAS.IsValid())
				{
					emitter.CreateRaytracingRenderData();
				}

				// TLAS instance data:
				RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
				for (int i = 0; i < arraysize(instance.transform); ++i)
				{
					for (int j = 0; j < arraysize(instance.transform[i]); ++j)
					{
						instance.transform[i][j] = wi::math::IDENTITY_MATRIX.m[j][i];
					}
				}
				instance.instance_id = (uint32_t)instanceIndex;
				instance.instance_mask = emitter.layerMask == 0 ? 0 : 0xFF;
				instance.bottom_level = &emitter.BLAS;
				instance.instance_contribution_to_hit_group_index = 0;
				instance.flags = RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;

				void* dest = (void*)((size_t)TLAS_instancesMapped + instanceIndex * device->GetTopLevelAccelerationStructureInstanceSize());
				device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
			}

		});
	}
	void Scene::RunWeatherUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (weathers.GetCount() > 0)
		{
			weather = weathers[0];
			weather.most_important_light_index = ~0;

			if (weather.IsOceanEnabled() && !ocean.IsValid())
			{
				OceanRegenerate();
			}
			if (!weather.IsOceanEnabled())
			{
				ocean = {};
			}

			// Ocean occlusion status:
			if (!wi::renderer::GetFreezeCullingCameraEnabled() && weather.IsOceanEnabled())
			{
				ocean.occlusionHistory <<= 1u; // advance history by 1 frame
				int query_id = ocean.occlusionQueries[queryheap_idx];
				if (queryResultBuffer[queryheap_idx].mapped_data != nullptr && query_id >= 0)
				{
					uint64_t visible = ((uint64_t*)queryResultBuffer[queryheap_idx].mapped_data)[query_id];
					if (visible)
					{
						ocean.occlusionHistory |= 1; // visible
					}
				}
				else
				{
					ocean.occlusionHistory |= 1; // visible
				}
			}
			ocean.occlusionQueries[queryheap_idx] = -1; // invalidate query
		}

		if (ocean.IsValid())
		{
			ocean.params = weather.oceanParameters;
		}

		if (weather.rain_amount > 0)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			rainEmitter._flags |= wi::EmittedParticleSystem::FLAG_USE_RAIN_BLOCKER;
			rainEmitter.shaderType = wi::EmittedParticleSystem::PARTICLESHADERTYPE::SOFT_LIGHTING;
			rainEmitter.SetCollidersDisabled(true);
			rainEmitter.SetVolumeEnabled(true);
			constexpr uint32_t target_max_particle_count = 1000000;
			if (rainEmitter.GetMaxParticleCount() != target_max_particle_count)
			{
				rainEmitter.SetMaxParticleCount(target_max_particle_count);
			}
			rainEmitter.count = wi::math::Lerp(0, (float)target_max_particle_count, weather.rain_amount);
			rainEmitter.life = 1;
			rainEmitter.size = weather.rain_scale;
			rainEmitter.random_factor = weather.windRandomness;
			rainEmitter.random_life = 1;
			rainEmitter.motionBlurAmount = weather.rain_length;
			rainEmitter.velocity = XMFLOAT3(
				weather.windDirection.x * weather.windSpeed,
				-weather.rain_speed,
				weather.windDirection.z * weather.windSpeed
			);
			rainMaterial.SetUseVertexColors(true);
			rainMaterial.shaderType = MaterialComponent::SHADERTYPE_PBR;
			rainMaterial.subsurfaceScattering = XMFLOAT4(1, 1, 1, 2);
			rainMaterial.userBlendMode = BLENDMODE_ALPHA;
			rainMaterial.baseColor = weather.rain_color;
			if (!rainMaterial.textures[MaterialComponent::BASECOLORMAP].resource.IsValid())
			{
				Texture gradientTex = wi::texturehelper::CreateGradientTexture(
					wi::texturehelper::GradientType::Circular,
					64, 64,
					XMFLOAT2(0.5f, 0.5f), XMFLOAT2(0.5f, 0),
					wi::texturehelper::GradientFlags::Smoothstep | wi::texturehelper::GradientFlags::Inverse
				);
				Texture gradientTexBC;
				TextureDesc desc = gradientTex.GetDesc();
				desc.format = Format::BC4_UNORM;
				desc.swizzle = { wi::graphics::ComponentSwizzle::ONE,wi::graphics::ComponentSwizzle::ONE,wi::graphics::ComponentSwizzle::ONE,wi::graphics::ComponentSwizzle::R };
				bool success = device->CreateTexture(&desc, nullptr, &gradientTexBC);
				assert(success);
				wi::renderer::AddDeferredBlockCompression(gradientTex, gradientTexBC);
				rainMaterial.textures[MaterialComponent::BASECOLORMAP].resource.SetTexture(gradientTexBC);
			}
			if (!rainMaterial.textures[MaterialComponent::NORMALMAP].resource.IsValid())
			{
				Texture gradientTex = wi::texturehelper::CreateLensDistortionNormalMap(
					32, 32
				);
				Texture gradientTexBC;
				TextureDesc desc = gradientTex.GetDesc();
				desc.format = Format::BC5_UNORM;
				desc.swizzle = { wi::graphics::ComponentSwizzle::R,wi::graphics::ComponentSwizzle::G,wi::graphics::ComponentSwizzle::ONE,wi::graphics::ComponentSwizzle::ONE };
				bool success = device->CreateTexture(&desc, nullptr, &gradientTexBC);
				assert(success);
				wi::renderer::AddDeferredBlockCompression(gradientTex, gradientTexBC);
				rainMaterial.textures[MaterialComponent::NORMALMAP].resource.SetTexture(gradientTexBC);
			}
			//rainMaterial.shadingRate = ShadingRate::RATE_4X4;
			TransformComponent transform;
			transform.scale_local = XMFLOAT3(30, 30, 30);
			transform.translation_local.x = camera.Eye.x + camera.At.x * 10;
			transform.translation_local.y = camera.Eye.y + camera.At.y * 10 + transform.scale_local.y * 0.5f;
			transform.translation_local.z = camera.Eye.z + camera.At.z * 10;
			transform.UpdateTransform();
			rainEmitter.UpdateCPU(transform, dt);
			rain_blocker_dummy_light.cascade_distances[0] = transform.scale_local.x;

			ShaderMaterial material;
			material.init();
			rainMaterial.WriteShaderMaterial(&material);
			std::memcpy(materialArrayMapped + rainMaterialOffset, &material, sizeof(material));

			ShaderGeometry geometry;
			geometry.init();
			geometry.indexOffset = 0;
			geometry.indexCount = rainEmitter.GetMaxParticleCount() * 6;
			geometry.materialIndex = rainMaterialOffset;
			geometry.ib = device->GetDescriptorIndex(&rainEmitter.primitiveBuffer, SubresourceType::SRV);
			geometry.vb_pos_wind = rainEmitter.vb_pos.descriptor_srv;
			geometry.vb_nor = rainEmitter.vb_nor.descriptor_srv;
			geometry.vb_uvs = rainEmitter.vb_uvs.descriptor_srv;
			geometry.vb_col = rainEmitter.vb_col.descriptor_srv;
			geometry.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_EMITTEDPARTICLE;

			std::memcpy(geometryArrayMapped + rainGeometryOffset, &geometry, sizeof(geometry));

			ShaderMeshInstance inst;
			inst.init();
			inst.uid = 0;
			inst.layerMask = ~0u;
			inst.emissive = wi::math::pack_half3(XMFLOAT3(1, 1, 1));
			inst.color = wi::math::CompressColor(XMFLOAT4(1, 1, 1, 1));
			inst.geometryOffset = (uint)rainGeometryOffset;
			inst.geometryCount = 1;
			inst.baseGeometryOffset = inst.geometryOffset;
			inst.baseGeometryCount = inst.geometryCount;

			const size_t instanceIndex = rainInstanceOffset;
			std::memcpy(instanceArrayMapped + instanceIndex, &inst, sizeof(inst));

			if (TLAS_instancesMapped != nullptr)
			{
				if (!rainEmitter.BLAS.IsValid())
				{
					rainEmitter.CreateRaytracingRenderData();
				}

				// TLAS instance data:
				RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
				for (int i = 0; i < arraysize(instance.transform); ++i)
				{
					for (int j = 0; j < arraysize(instance.transform[i]); ++j)
					{
						instance.transform[i][j] = wi::math::IDENTITY_MATRIX.m[j][i];
					}
				}
				instance.instance_id = (uint32_t)instanceIndex;
				instance.instance_mask = rainEmitter.layerMask == 0 ? 0 : 0xFF;
				instance.bottom_level = &rainEmitter.BLAS;
				instance.instance_contribution_to_hit_group_index = 0;
				instance.flags = RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;

				void* dest = (void*)((size_t)TLAS_instancesMapped + instanceIndex * device->GetTopLevelAccelerationStructureInstanceSize());
				device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
			}
		}
		else
		{
			rainMaterial = {};
			rainEmitter = {};
		}

		wetmap_fadeout_time -= dt;
		if (weather.IsOceanEnabled() || weather.rain_amount > 0)
		{
			wetmap_fadeout_time = 60; // allow 60 sec for remaining wetmaps to fade out
		}
	}
	void Scene::RunSoundUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::audio::SoundInstance3D instance3D;
		instance3D.listenerPos = camera.Eye;
		instance3D.listenerUp = camera.Up;
		instance3D.listenerFront = camera.At;

		for (size_t i = 0; i < sounds.GetCount(); ++i)
		{
			SoundComponent& sound = sounds[i];

			if (!sound.soundinstance.IsValid() && sound.soundResource.IsValid())
			{
				sound.soundinstance.SetLooped(sound.IsLooped());
				wi::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);
			}

			if (!sound.IsDisable3D())
			{
				Entity entity = sounds.GetEntity(i);
				const TransformComponent* transform = transforms.GetComponent(entity);
				if (transform != nullptr)
				{
					instance3D.emitterPos = transform->GetPosition();
					instance3D.emitterFront = transform->GetForward();
					instance3D.emitterUp = transform->GetUp();
					wi::audio::Update3D(&sound.soundinstance, instance3D);
				}
			}
			if (sound.IsPlaying())
			{
				wi::audio::Play(&sound.soundinstance);
			}
			else
			{
				wi::audio::Stop(&sound.soundinstance);
			}
			wi::audio::SetVolume(sound.volume, &sound.soundinstance);
		}
	}
	void Scene::RunVideoUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < videos.GetCount(); ++i)
		{
			VideoComponent& video = videos[i];

			if (video.IsPlaying())
			{
				video.videoinstance.flags |= wi::video::VideoInstance::Flags::Playing;
			}
			else
			{
				video.videoinstance.flags &= ~wi::video::VideoInstance::Flags::Playing;
			}

			if (video.IsLooped())
			{
				video.videoinstance.flags |= wi::video::VideoInstance::Flags::Looped;
			}
			else
			{
				video.videoinstance.flags &= ~wi::video::VideoInstance::Flags::Looped;
			}

			video.videoinstance.flags |= wi::video::VideoInstance::Flags::Mipmapped;

		}
	}
	void Scene::RunScriptUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (dt == 0)
			return; // not allowed to be run when dt == 0 as it could be on separate thread!
		auto range = wi::profiler::BeginRangeCPU("Script Components");
		for (size_t i = 0; i < scripts.GetCount(); ++i)
		{
			ScriptComponent& script = scripts[i];
			Entity entity = scripts.GetEntity(i);

			if (script.IsPlaying())
			{
				if (script.resource.IsValid() && (script.script.empty() || script.script_hash != script.resource.GetScriptHash()))
				{
					script.script.clear();
					script.script_hash = script.resource.GetScriptHash();
					std::string str = script.resource.GetScript();
					wi::lua::AttachScriptParameters(str, script.filename, wi::lua::GeneratePID(), "local function GetEntity() return " + std::to_string(entity) + "; end;", "");
					wi::lua::CompileText(str, script.script);
				}
				if (!script.script.empty())
				{
					wi::lua::RunBinaryData(script.script.data(), script.script.size(), script.filename.c_str());
				}

				if (script.IsPlayingOnlyOnce())
				{
					script.Stop();
				}
			}
		}
		wi::profiler::EndRange(range);
	}
	void Scene::RunSpriteUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)sprites.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {
			Sprite& sprite = sprites[args.jobIndex];
			if (sprite.params.isExtractNormalMapEnabled())
			{
				sprite.params.image_subresource = -1;
			}
			else if (sprite.textureResource.IsValid())
			{
				sprite.params.image_subresource = sprite.textureResource.GetTextureSRGBSubresource();
			}
			if (sprite.maskResource.IsValid())
			{
				sprite.params.mask_subresource = sprite.maskResource.GetTextureSRGBSubresource();
			}
			sprite.Update(dt);
		});
	}
	void Scene::RunFontUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)fonts.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {
			SpriteFont& font = fonts[args.jobIndex];
			Entity entity = fonts.GetEntity(args.jobIndex);
			const SoundComponent* sound = sounds.GetComponent(entity);
			if (sound != nullptr && sound->soundResource.IsValid())
			{
				font.anim.typewriter.sound = sound->soundResource.GetSound();
				font.anim.typewriter.soundinstance = sound->soundinstance;
			}
			else
			{
				font.anim.typewriter.sound = {};
				font.anim.typewriter.soundinstance = {};
			}
			font.Update(dt);
		});
	}

	Scene::RayIntersectionResult Scene::Intersects(const Ray& ray, uint32_t filterMask, uint32_t layerMask, uint32_t lod) const
	{
		RayIntersectionResult result;

		const XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
		const XMVECTOR rayDirection = XMVector3Normalize(XMLoadFloat3(&ray.direction));

		if ((filterMask & FILTER_COLLIDER) && collider_bvh.IsValid())
		{
			collider_bvh.Intersects(ray, 0, [&](uint32_t collider_index) {
				if (colliders.GetCount() <= collider_index)
					return;
				const ColliderComponent& collider = colliders_cpu[collider_index];

				if ((collider.layerMask & layerMask) == 0)
					return;

				float dist = 0;
				XMFLOAT3 direction = {};
				bool intersects = false;

				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
					intersects = ray.intersects(collider.sphere, dist, direction);
					break;
				case ColliderComponent::Shape::Capsule:
					intersects = ray.intersects(collider.capsule, dist, direction);
					break;
				case ColliderComponent::Shape::Plane:
					intersects = ray.intersects(collider.plane, dist, direction);
					break;
				}

				if (intersects)
				{
					if (dist < result.distance)
					{
						result.distance = dist;
						result.bary = {};
						result.entity = colliders.GetEntity(collider_index);
						result.normal = direction;
						result.uv = {};
						result.velocity = {};
						XMStoreFloat3(&result.position, rayOrigin + rayDirection * dist);
						result.subsetIndex = -1;
						result.vertexID0 = 0;
						result.vertexID1 = 0;
						result.vertexID2 = 0;
					}
				}
			});
		}

		if (filterMask & FILTER_OBJECT_ALL)
		{
			const size_t objectCount = std::min(objects.GetCount(), aabb_objects.size());
			for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
			{
				const AABB& aabb = aabb_objects[objectIndex];
				if (!ray.intersects(aabb) || (layerMask & aabb.layerMask) == 0)
					continue;

				const ObjectComponent& object = objects[objectIndex];
				if (object.meshID == INVALID_ENTITY)
					continue;
				if ((filterMask & object.GetFilterMask()) == 0)
					continue;

				const MeshComponent* mesh = meshes.GetComponent(object.meshID);
				if (mesh == nullptr)
					continue;

				const Entity entity = objects.GetEntity(objectIndex);
				const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
				const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
				const XMMATRIX objectMatPrev = XMLoadFloat4x4(&matrix_objects_prev[objectIndex]);
				const XMMATRIX objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);
				const XMVECTOR rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
				const XMVECTOR rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));
				const ArmatureComponent* armature = mesh->IsSkinned() ? armatures.GetComponent(mesh->armatureID) : nullptr;

				auto intersect_triangle = [&](uint32_t subsetIndex, uint32_t indexOffset, uint32_t triangleIndex)
				{
					const uint32_t i0 = mesh->indices[indexOffset + triangleIndex * 3 + 0];
					const uint32_t i1 = mesh->indices[indexOffset + triangleIndex * 3 + 1];
					const uint32_t i2 = mesh->indices[indexOffset + triangleIndex * 3 + 2];

					XMVECTOR p0;
					XMVECTOR p1;
					XMVECTOR p2;
					if (softbody != nullptr && !softbody->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *softbody, i0);
						p1 = SkinVertex(*mesh, *softbody, i1);
						p2 = SkinVertex(*mesh, *softbody, i2);
					}
					else if (armature != nullptr && !armature->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *armature, i0);
						p1 = SkinVertex(*mesh, *armature, i1);
						p2 = SkinVertex(*mesh, *armature, i2);
					}
					else
					{
						p0 = XMLoadFloat3(&mesh->vertex_positions[i0]);
						p1 = XMLoadFloat3(&mesh->vertex_positions[i1]);
						p2 = XMLoadFloat3(&mesh->vertex_positions[i2]);
					}

					float distance;
					XMFLOAT2 bary;
					if (wi::math::RayTriangleIntersects(rayOrigin_local, rayDirection_local, p0, p1, p2, distance, bary))
					{
						const XMVECTOR pos_local = XMVectorAdd(rayOrigin_local, rayDirection_local * distance);
						const XMVECTOR pos = XMVector3Transform(pos_local, objectMat);
						distance = wi::math::Distance(pos, rayOrigin);

						// Note: we do the TMin, Tmax check here, in world space! We use the RayTriangleIntersects in local space, so we don't use those in there
						if (distance < result.distance && distance >= ray.TMin && distance <= ray.TMax)
						{
							XMVECTOR nor;
							if (softbody != nullptr || mesh->vertex_normals.empty()) // Note: for soft body we compute it instead of loading the simulated normals
							{
								nor = XMVector3Cross(p2 - p1, p1 - p0);
							}
							else
							{
								nor = XMVectorBaryCentric(
									XMLoadFloat3(&mesh->vertex_normals[i0]),
									XMLoadFloat3(&mesh->vertex_normals[i1]),
									XMLoadFloat3(&mesh->vertex_normals[i2]),
									bary.x,
									bary.y
								);
							}
							nor = XMVector3Normalize(XMVector3TransformNormal(nor, objectMat));
							const XMVECTOR vel = pos - XMVector3Transform(pos_local, objectMatPrev);

							result.uv = {};
							if (!mesh->vertex_uvset_0.empty())
							{
								XMVECTOR uv = XMVectorBaryCentric(
									XMLoadFloat2(&mesh->vertex_uvset_0[i0]),
									XMLoadFloat2(&mesh->vertex_uvset_0[i1]),
									XMLoadFloat2(&mesh->vertex_uvset_0[i2]),
									bary.x,
									bary.y
								);
								result.uv.x = XMVectorGetX(uv);
								result.uv.y = XMVectorGetY(uv);
							}
							if (!mesh->vertex_uvset_1.empty())
							{
								XMVECTOR uv = XMVectorBaryCentric(
									XMLoadFloat2(&mesh->vertex_uvset_1[i0]),
									XMLoadFloat2(&mesh->vertex_uvset_1[i1]),
									XMLoadFloat2(&mesh->vertex_uvset_1[i2]),
									bary.x,
									bary.y
								);
								result.uv.z = XMVectorGetX(uv);
								result.uv.w = XMVectorGetY(uv);
							}

							result.entity = entity;
							XMStoreFloat3(&result.position, pos);
							XMStoreFloat3(&result.normal, nor);
							XMStoreFloat3(&result.velocity, vel);
							result.distance = distance;
							result.subsetIndex = (int)subsetIndex;
							result.vertexID0 = (int)i0;
							result.vertexID1 = (int)i1;
							result.vertexID2 = (int)i2;
							result.bary = bary;
						}
					}
				};

				if (mesh->bvh.IsValid())
				{
					Ray ray_local = Ray(rayOrigin_local, rayDirection_local);

					mesh->bvh.Intersects(ray_local, 0, [&](uint32_t index) {
						const AABB& leaf = mesh->bvh_leaf_aabbs[index];
						const uint32_t triangleIndex = leaf.layerMask;
						const uint32_t subsetIndex = leaf.userdata;
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							return;
						const uint32_t indexOffset = subset.indexOffset;
						intersect_triangle(subsetIndex, indexOffset, triangleIndex);
					});
				}
				else
				{
					// Brute-force intersection test:
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(lod, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							continue;
						const uint32_t indexOffset = subset.indexOffset;
						const uint32_t triangleCount = subset.indexCount / 3;

						for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
						{
							intersect_triangle(subsetIndex, indexOffset, triangleIndex);
						}
					}
				}

			}
		}

		result.orientation = ray.GetPlacementOrientation(result.position, result.normal);

		return result;
	}
	bool Scene::IntersectsFirst(const wi::primitive::Ray& ray, uint32_t filterMask, uint32_t layerMask, uint32_t lod) const
	{
		bool result = false;

		const XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
		const XMVECTOR rayDirection = XMVector3Normalize(XMLoadFloat3(&ray.direction));

		if ((filterMask & FILTER_COLLIDER) && collider_bvh.IsValid())
		{
			collider_bvh.IntersectsFirst(ray, [&](uint32_t collider_index) {
				if (colliders.GetCount() <= collider_index)
					return false;
				const ColliderComponent& collider = colliders_cpu[collider_index];

				if ((collider.layerMask & layerMask) == 0)
					return false;

				float dist = 0;
				XMFLOAT3 direction = {};
				bool intersects = false;

				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
					intersects = ray.intersects(collider.sphere, dist, direction);
					break;
				case ColliderComponent::Shape::Capsule:
					intersects = ray.intersects(collider.capsule, dist, direction);
					break;
				case ColliderComponent::Shape::Plane:
					intersects = ray.intersects(collider.plane, dist, direction);
					break;
				}

				if (intersects)
				{
					result = true;
					return true;
				}
				return false;
			});
			if (result)
				return result;
		}

		if (filterMask & FILTER_OBJECT_ALL)
		{
			const size_t objectCount = std::min(objects.GetCount(), aabb_objects.size());
			for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
			{
				const AABB& aabb = aabb_objects[objectIndex];
				if (!ray.intersects(aabb) || (layerMask & aabb.layerMask) == 0)
					continue;

				const ObjectComponent& object = objects[objectIndex];
				if (object.meshID == INVALID_ENTITY)
					continue;
				if ((filterMask & object.GetFilterMask()) == 0)
					continue;

				const MeshComponent* mesh = meshes.GetComponent(object.meshID);
				if (mesh == nullptr)
					continue;

				const Entity entity = objects.GetEntity(objectIndex);
				const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
				const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
				const XMMATRIX objectMatPrev = XMLoadFloat4x4(&matrix_objects_prev[objectIndex]);
				const XMMATRIX objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);
				const XMVECTOR rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
				const XMVECTOR rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));
				const ArmatureComponent* armature = mesh->IsSkinned() ? armatures.GetComponent(mesh->armatureID) : nullptr;

				auto intersect_triangle = [&](uint32_t subsetIndex, uint32_t indexOffset, uint32_t triangleIndex)
				{
					const uint32_t i0 = mesh->indices[indexOffset + triangleIndex * 3 + 0];
					const uint32_t i1 = mesh->indices[indexOffset + triangleIndex * 3 + 1];
					const uint32_t i2 = mesh->indices[indexOffset + triangleIndex * 3 + 2];

					XMVECTOR p0;
					XMVECTOR p1;
					XMVECTOR p2;
					if (softbody != nullptr && !softbody->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *softbody, i0);
						p1 = SkinVertex(*mesh, *softbody, i1);
						p2 = SkinVertex(*mesh, *softbody, i2);
					}
					else if (armature != nullptr && !armature->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *armature, i0);
						p1 = SkinVertex(*mesh, *armature, i1);
						p2 = SkinVertex(*mesh, *armature, i2);
					}
					else
					{
						p0 = XMLoadFloat3(&mesh->vertex_positions[i0]);
						p1 = XMLoadFloat3(&mesh->vertex_positions[i1]);
						p2 = XMLoadFloat3(&mesh->vertex_positions[i2]);
					}

					float distance;
					XMFLOAT2 bary;
					if (wi::math::RayTriangleIntersects(rayOrigin_local, rayDirection_local, p0, p1, p2, distance, bary))
					{
						const XMVECTOR pos_local = XMVectorAdd(rayOrigin_local, rayDirection_local * distance);
						const XMVECTOR pos = XMVector3Transform(pos_local, objectMat);
						distance = wi::math::Distance(pos, rayOrigin);

						// Note: we do the TMin, Tmax check here, in world space! We use the RayTriangleIntersects in local space, so we don't use those in there
						if (distance >= ray.TMin && distance <= ray.TMax)
						{
							result = true;
							return true;
						}
					}
					return false;
				};

				if (mesh->bvh.IsValid())
				{
					Ray ray_local = Ray(rayOrigin_local, rayDirection_local);

					mesh->bvh.IntersectsFirst(ray_local, [&](uint32_t index) {
						const AABB& leaf = mesh->bvh_leaf_aabbs[index];
						const uint32_t triangleIndex = leaf.layerMask;
						const uint32_t subsetIndex = leaf.userdata;
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							return false;
						const uint32_t indexOffset = subset.indexOffset;
						return intersect_triangle(subsetIndex, indexOffset, triangleIndex);
					});
				}
				else
				{
					// Brute-force intersection test:
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(lod, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							continue;
						const uint32_t indexOffset = subset.indexOffset;
						const uint32_t triangleCount = subset.indexCount / 3;

						for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
						{
							if (intersect_triangle(subsetIndex, indexOffset, triangleIndex))
							{
								result = true;
								return true;
							}
						}
					}
				}

			}
		}
		return result;
	}
	Scene::SphereIntersectionResult Scene::Intersects(const Sphere& sphere, uint32_t filterMask, uint32_t layerMask, uint32_t lod) const
	{
		SphereIntersectionResult result;

		const XMVECTOR Center = XMLoadFloat3(&sphere.center);
		const XMVECTOR Radius = XMVectorReplicate(sphere.radius);
		const XMVECTOR RadiusSq = XMVectorMultiply(Radius, Radius);

		if ((filterMask & FILTER_COLLIDER) && collider_bvh.IsValid())
		{
			collider_bvh.Intersects(sphere, 0, [&](uint32_t collider_index) {
				if (colliders.GetCount() <= collider_index)
					return;
				const ColliderComponent& collider = colliders_cpu[collider_index];

				if ((collider.layerMask & layerMask) == 0)
					return;

				float dist = 0;
				XMFLOAT3 direction = {};
				XMFLOAT3 position = {};
				bool intersects = false;

				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
					intersects = sphere.intersects(collider.sphere, dist, direction);
					XMStoreFloat3(&position, XMLoadFloat3(&collider.sphere.center) + XMLoadFloat3(&direction) * dist);
					break;
				case ColliderComponent::Shape::Capsule:
					intersects = sphere.intersects(collider.capsule, dist, direction);
					break;
				case ColliderComponent::Shape::Plane:
					intersects = sphere.intersects(collider.plane, dist, direction);
					break;
				}

				if (intersects)
				{
					if (dist > result.depth)
					{
						result.depth = dist;
						result.entity = colliders.GetEntity(collider_index);
						result.normal = direction;
						result.position = position;
						result.velocity = {};
					}
				}
			});
		}

		if (filterMask & FILTER_OBJECT_ALL)
		{
			const size_t objectCount = std::min(objects.GetCount(), aabb_objects.size());
			for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
			{
				const AABB& aabb = aabb_objects[objectIndex];
				if (!sphere.intersects(aabb) || (layerMask & aabb.layerMask) == 0)
					continue;

				const ObjectComponent& object = objects[objectIndex];
				if (object.meshID == INVALID_ENTITY)
					continue;
				if ((filterMask & object.GetFilterMask()) == 0)
					continue;

				const MeshComponent* mesh = meshes.GetComponent(object.meshID);
				if (mesh == nullptr)
					continue;

				const Entity entity = objects.GetEntity(objectIndex);
				const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
				const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
				const XMMATRIX objectMatPrev = XMLoadFloat4x4(&matrix_objects_prev[objectIndex]);
				const XMMATRIX objectMatInverse = XMMatrixInverse(nullptr, objectMat);
				const ArmatureComponent* armature = mesh->IsSkinned() ? armatures.GetComponent(mesh->armatureID) : nullptr;

				auto intersect_triangle = [&](uint32_t subsetIndex, uint32_t indexOffset, uint32_t triangleIndex)
				{
					const uint32_t i0 = mesh->indices[indexOffset + triangleIndex * 3 + 0];
					const uint32_t i1 = mesh->indices[indexOffset + triangleIndex * 3 + 1];
					const uint32_t i2 = mesh->indices[indexOffset + triangleIndex * 3 + 2];

					XMVECTOR p0;
					XMVECTOR p1;
					XMVECTOR p2;
					if (softbody != nullptr && !softbody->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *softbody, i0);
						p1 = SkinVertex(*mesh, *softbody, i1);
						p2 = SkinVertex(*mesh, *softbody, i2);
					}
					else if (armature != nullptr && !armature->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *armature, i0);
						p1 = SkinVertex(*mesh, *armature, i1);
						p2 = SkinVertex(*mesh, *armature, i2);
						p0 = XMVector3Transform(p0, objectMat);
						p1 = XMVector3Transform(p1, objectMat);
						p2 = XMVector3Transform(p2, objectMat);
					}
					else
					{
						p0 = XMLoadFloat3(&mesh->vertex_positions[i0]);
						p1 = XMLoadFloat3(&mesh->vertex_positions[i1]);
						p2 = XMLoadFloat3(&mesh->vertex_positions[i2]);
						p0 = XMVector3Transform(p0, objectMat);
						p1 = XMVector3Transform(p1, objectMat);
						p2 = XMVector3Transform(p2, objectMat);
					}

					XMFLOAT3 min, max;
					XMStoreFloat3(&min, XMVectorMin(p0, XMVectorMin(p1, p2)));
					XMStoreFloat3(&max, XMVectorMax(p0, XMVectorMax(p1, p2)));
					AABB aabb_triangle(min, max);
					if (sphere.intersects(aabb_triangle) == AABB::OUTSIDE)
						return;

					// Compute the plane of the triangle (has to be normalized).
					XMVECTOR N = XMVector3Normalize(XMVector3Cross(p1 - p0, p2 - p0));

					// Assert that the triangle is not degenerate.
					assert(!XMVector3Equal(N, XMVectorZero()));

					// Find the nearest feature on the triangle to the sphere.
					XMVECTOR Dist = XMVector3Dot(XMVectorSubtract(Center, p0), N);

					if (!mesh->IsDoubleSided() && XMVectorGetX(Dist) > 0)
						return; // pass through back faces

					// If the center of the sphere is farther from the plane of the triangle than
					// the radius of the sphere, then there cannot be an intersection.
					XMVECTOR NoIntersection = XMVectorLess(Dist, XMVectorNegate(Radius));
					NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Dist, Radius));

					// Project the center of the sphere onto the plane of the triangle.
					XMVECTOR Point0 = XMVectorNegativeMultiplySubtract(N, Dist, Center);

					// Is it inside all the edges? If so we intersect because the distance 
					// to the plane is less than the radius.
					//XMVECTOR Intersection = DirectX::Internal::PointOnPlaneInsideTriangle(Point0, p0, p1, p2);

					// Compute the cross products of the vector from the base of each edge to 
					// the point with each edge vector.
					XMVECTOR C0 = XMVector3Cross(XMVectorSubtract(Point0, p0), XMVectorSubtract(p1, p0));
					XMVECTOR C1 = XMVector3Cross(XMVectorSubtract(Point0, p1), XMVectorSubtract(p2, p1));
					XMVECTOR C2 = XMVector3Cross(XMVectorSubtract(Point0, p2), XMVectorSubtract(p0, p2));

					// If the cross product points in the same direction as the normal the the
					// point is inside the edge (it is zero if is on the edge).
					XMVECTOR Zero = XMVectorZero();
					XMVECTOR Inside0 = XMVectorLessOrEqual(XMVector3Dot(C0, N), Zero);
					XMVECTOR Inside1 = XMVectorLessOrEqual(XMVector3Dot(C1, N), Zero);
					XMVECTOR Inside2 = XMVectorLessOrEqual(XMVector3Dot(C2, N), Zero);

					// If the point inside all of the edges it is inside.
					XMVECTOR Intersection = XMVectorAndInt(XMVectorAndInt(Inside0, Inside1), Inside2);

					bool inside = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

					// Find the nearest point on each edge.

					// Edge 0,1
					XMVECTOR Point1 = DirectX::Internal::PointOnLineSegmentNearestPoint(p0, p1, Center);

					// If the distance to the center of the sphere to the point is less than 
					// the radius of the sphere then it must intersect.
					Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point1)), RadiusSq));

					// Edge 1,2
					XMVECTOR Point2 = DirectX::Internal::PointOnLineSegmentNearestPoint(p1, p2, Center);

					// If the distance to the center of the sphere to the point is less than 
					// the radius of the sphere then it must intersect.
					Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point2)), RadiusSq));

					// Edge 2,0
					XMVECTOR Point3 = DirectX::Internal::PointOnLineSegmentNearestPoint(p2, p0, Center);

					// If the distance to the center of the sphere to the point is less than 
					// the radius of the sphere then it must intersect.
					Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point3)), RadiusSq));

					bool intersects = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

					if (intersects)
					{
						XMVECTOR bestPoint = Point0;
						if (!inside)
						{
							// If the sphere center's projection on the triangle plane is not within the triangle,
							//	determine the closest point on triangle to the sphere center
							float bestDist = XMVectorGetX(XMVector3LengthSq(Point1 - Center));
							bestPoint = Point1;

							float d = XMVectorGetX(XMVector3LengthSq(Point2 - Center));
							if (d < bestDist)
							{
								bestDist = d;
								bestPoint = Point2;
							}
							d = XMVectorGetX(XMVector3LengthSq(Point3 - Center));
							if (d < bestDist)
							{
								bestDist = d;
								bestPoint = Point3;
							}
						}
						XMVECTOR intersectionVec = Center - bestPoint;
						XMVECTOR intersectionVecLen = XMVector3Length(intersectionVec);

						float depth = sphere.radius - XMVectorGetX(intersectionVecLen);
						if (depth > result.depth)
						{
							result.entity = entity;
							result.depth = depth;
							XMStoreFloat3(&result.position, bestPoint);
							XMStoreFloat3(&result.normal, intersectionVec / intersectionVecLen);

							XMVECTOR vel = bestPoint - XMVector3Transform(XMVector3Transform(bestPoint, objectMatInverse), objectMatPrev);
							XMStoreFloat3(&result.velocity, vel);

							result.subsetIndex = (int)subsetIndex;
						}
					}
				};

				if (mesh->bvh.IsValid())
				{
					XMFLOAT3 center_local;
					float radius_local;
					XMStoreFloat3(&center_local, XMVector3Transform(XMLoadFloat3(&sphere.center), objectMatInverse));
					XMStoreFloat(&radius_local, XMVector3Length(XMVector3TransformNormal(XMLoadFloat(&sphere.radius), objectMatInverse)));
					Sphere sphere_local = Sphere(center_local, radius_local);

					mesh->bvh.Intersects(sphere_local, 0, [&](uint32_t index) {
						const AABB& leaf = mesh->bvh_leaf_aabbs[index];
						const uint32_t triangleIndex = leaf.layerMask;
						const uint32_t subsetIndex = leaf.userdata;
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							return;
						const uint32_t indexOffset = subset.indexOffset;
						intersect_triangle(subsetIndex, indexOffset, triangleIndex);
						});
				}
				else
				{
					// Brute-force intersection test:
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(lod, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							continue;
						const uint32_t indexOffset = subset.indexOffset;
						const uint32_t triangleCount = subset.indexCount / 3;

						for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
						{
							intersect_triangle(subsetIndex, indexOffset, triangleIndex);
						}
					}
				}

			}
		}

		result.orientation = sphere.GetPlacementOrientation(result.position, result.normal);

		return result;
	}
	Scene::CapsuleIntersectionResult Scene::Intersects(const Capsule& capsule, uint32_t filterMask, uint32_t layerMask, uint32_t lod) const
	{
		CapsuleIntersectionResult result;

		const XMVECTOR Base = XMLoadFloat3(&capsule.base);
		const XMVECTOR Tip = XMLoadFloat3(&capsule.tip);
		const XMVECTOR Radius = XMVectorReplicate(capsule.radius);
		const XMVECTOR Axis = XMVector3Normalize(Tip - Base);
		const XMVECTOR LineEndOffset = Axis * Radius;
		const XMVECTOR A = Base + LineEndOffset;
		const XMVECTOR B = Tip - LineEndOffset;
		const XMVECTOR RadiusSq = XMVectorMultiply(Radius, Radius);
		const AABB capsule_aabb = capsule.getAABB();

		if ((filterMask & FILTER_COLLIDER) && collider_bvh.IsValid())
		{
			collider_bvh.Intersects(capsule_aabb, 0, [&](uint32_t collider_index) {
				if (colliders.GetCount() <= collider_index)
					return;
				const ColliderComponent& collider = colliders_cpu[collider_index];

				if ((collider.layerMask & layerMask) == 0)
					return;

				float dist = 0;
				XMFLOAT3 direction = {};
				XMFLOAT3 position = {};
				bool intersects = false;

				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
					intersects = capsule.intersects(collider.sphere, dist, direction);
					XMStoreFloat3(&position, XMLoadFloat3(&collider.sphere.center) + XMLoadFloat3(&direction) * dist);
					break;
				case ColliderComponent::Shape::Capsule:
					intersects = capsule.intersects(collider.capsule, position, direction, dist);
					break;
				case ColliderComponent::Shape::Plane:
					intersects = capsule.intersects(collider.plane, dist, direction);
					break;
				}

				if (intersects)
				{
					if (dist > result.depth)
					{
						result.depth = dist;
						result.entity = colliders.GetEntity(collider_index);
						result.normal = direction;
						result.position = position;
						result.velocity = {};
					}
				}
			});
		}

		if (filterMask & FILTER_OBJECT_ALL)
		{
			const size_t objectCount = std::min(objects.GetCount(), aabb_objects.size());
			for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
			{
				const AABB& aabb = aabb_objects[objectIndex];
				if (capsule_aabb.intersects(aabb) == AABB::INTERSECTION_TYPE::OUTSIDE || (layerMask & aabb.layerMask) == 0)
					continue;

				const ObjectComponent& object = objects[objectIndex];

				if (object.meshID == INVALID_ENTITY)
					continue;
				if ((filterMask & object.GetFilterMask()) == 0)
					continue;

				const MeshComponent* mesh = meshes.GetComponent(object.meshID);
				if (mesh == nullptr)
					continue;

				const Entity entity = objects.GetEntity(objectIndex);
				const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
				const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
				const XMMATRIX objectMatPrev = XMLoadFloat4x4(&matrix_objects_prev[objectIndex]);
				const ArmatureComponent* armature = mesh->IsSkinned() ? armatures.GetComponent(mesh->armatureID) : nullptr;
				const XMMATRIX objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);
				
				auto intersect_triangle = [&](uint32_t subsetIndex, uint32_t indexOffset, uint32_t triangleIndex)
				{
					const uint32_t i0 = mesh->indices[indexOffset + triangleIndex * 3 + 0];
					const uint32_t i1 = mesh->indices[indexOffset + triangleIndex * 3 + 1];
					const uint32_t i2 = mesh->indices[indexOffset + triangleIndex * 3 + 2];

					XMVECTOR p0;
					XMVECTOR p1;
					XMVECTOR p2;
					if (softbody != nullptr && !softbody->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *softbody, i0);
						p1 = SkinVertex(*mesh, *softbody, i1);
						p2 = SkinVertex(*mesh, *softbody, i2);
					}
					else if (armature != nullptr && !armature->boneData.empty())
					{
						p0 = SkinVertex(*mesh, *armature, i0);
						p1 = SkinVertex(*mesh, *armature, i1);
						p2 = SkinVertex(*mesh, *armature, i2);
						p0 = XMVector3Transform(p0, objectMat);
						p1 = XMVector3Transform(p1, objectMat);
						p2 = XMVector3Transform(p2, objectMat);
					}
					else
					{
						p0 = XMLoadFloat3(&mesh->vertex_positions[i0]);
						p1 = XMLoadFloat3(&mesh->vertex_positions[i1]);
						p2 = XMLoadFloat3(&mesh->vertex_positions[i2]);
						p0 = XMVector3Transform(p0, objectMat);
						p1 = XMVector3Transform(p1, objectMat);
						p2 = XMVector3Transform(p2, objectMat);
					}

					XMFLOAT3 min, max;
					XMStoreFloat3(&min, XMVectorMin(p0, XMVectorMin(p1, p2)));
					XMStoreFloat3(&max, XMVectorMax(p0, XMVectorMax(p1, p2)));
					AABB aabb_triangle(min, max);
					if (capsule_aabb.intersects(aabb_triangle) == AABB::OUTSIDE)
						return;

					// Compute the plane of the triangle (has to be normalized).
					XMVECTOR N = XMVector3Normalize(XMVector3Cross(p1 - p0, p2 - p0));

					XMVECTOR ReferencePoint;
					XMVECTOR d = XMVector3Normalize(B - A);
					if (std::abs(XMVectorGetX(XMVector3Dot(N, d))) < std::numeric_limits<float>::epsilon())
					{
						// Capsule line cannot be intersected with triangle plane (they are parallel)
						//	In this case, just take a point from triangle
						ReferencePoint = p0;
					}
					else
					{
						// Intersect capsule line with triangle plane:
						XMVECTOR t = XMVector3Dot(N, (Base - p0) / XMVectorAbs(XMVector3Dot(N, d)));
						XMVECTOR LinePlaneIntersection = Base + d * t;

						// Compute the cross products of the vector from the base of each edge to 
						// the point with each edge vector.
						XMVECTOR C0 = XMVector3Cross(XMVectorSubtract(LinePlaneIntersection, p0), XMVectorSubtract(p1, p0));
						XMVECTOR C1 = XMVector3Cross(XMVectorSubtract(LinePlaneIntersection, p1), XMVectorSubtract(p2, p1));
						XMVECTOR C2 = XMVector3Cross(XMVectorSubtract(LinePlaneIntersection, p2), XMVectorSubtract(p0, p2));

						// If the cross product points in the same direction as the normal the the
						// point is inside the edge (it is zero if is on the edge).
						XMVECTOR Zero = XMVectorZero();
						XMVECTOR Inside0 = XMVectorLessOrEqual(XMVector3Dot(C0, N), Zero);
						XMVECTOR Inside1 = XMVectorLessOrEqual(XMVector3Dot(C1, N), Zero);
						XMVECTOR Inside2 = XMVectorLessOrEqual(XMVector3Dot(C2, N), Zero);

						// If the point inside all of the edges it is inside.
						XMVECTOR Intersection = XMVectorAndInt(XMVectorAndInt(Inside0, Inside1), Inside2);

						bool inside = XMVectorGetIntX(Intersection) != 0;

						if (inside)
						{
							ReferencePoint = LinePlaneIntersection;
						}
						else
						{
							// Find the nearest point on each edge.

							// Edge 0,1
							XMVECTOR Point1 = wi::math::ClosestPointOnLineSegment(p0, p1, LinePlaneIntersection);

							// Edge 1,2
							XMVECTOR Point2 = wi::math::ClosestPointOnLineSegment(p1, p2, LinePlaneIntersection);

							// Edge 2,0
							XMVECTOR Point3 = wi::math::ClosestPointOnLineSegment(p2, p0, LinePlaneIntersection);

							ReferencePoint = Point1;
							float bestDist = XMVectorGetX(XMVector3LengthSq(Point1 - LinePlaneIntersection));
							float d = abs(XMVectorGetX(XMVector3LengthSq(Point2 - LinePlaneIntersection)));
							if (d < bestDist)
							{
								bestDist = d;
								ReferencePoint = Point2;
							}
							d = abs(XMVectorGetX(XMVector3LengthSq(Point3 - LinePlaneIntersection)));
							if (d < bestDist)
							{
								bestDist = d;
								ReferencePoint = Point3;
							}
						}


					}

					// Place a sphere on closest point on line segment to intersection:
					XMVECTOR Center = wi::math::ClosestPointOnLineSegment(A, B, ReferencePoint);

					// Assert that the triangle is not degenerate.
					assert(!XMVector3Equal(N, XMVectorZero()));

					// Find the nearest feature on the triangle to the sphere.
					XMVECTOR Dist = XMVector3Dot(XMVectorSubtract(Center, p0), N);

					bool onBackside = XMVectorGetX(Dist) > 0;
					if (!mesh->IsDoubleSided() && onBackside)
						return; // pass through back faces

					// If the center of the sphere is farther from the plane of the triangle than
					// the radius of the sphere, then there cannot be an intersection.
					XMVECTOR NoIntersection = XMVectorLess(Dist, XMVectorNegate(Radius));
					NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Dist, Radius));

					// Project the center of the sphere onto the plane of the triangle.
					XMVECTOR Point0 = XMVectorNegativeMultiplySubtract(N, Dist, Center);

					// Is it inside all the edges? If so we intersect because the distance 
					// to the plane is less than the radius.
					//XMVECTOR Intersection = DirectX::Internal::PointOnPlaneInsideTriangle(Point0, p0, p1, p2);

					// Compute the cross products of the vector from the base of each edge to 
					// the point with each edge vector.
					XMVECTOR C0 = XMVector3Cross(XMVectorSubtract(Point0, p0), XMVectorSubtract(p1, p0));
					XMVECTOR C1 = XMVector3Cross(XMVectorSubtract(Point0, p1), XMVectorSubtract(p2, p1));
					XMVECTOR C2 = XMVector3Cross(XMVectorSubtract(Point0, p2), XMVectorSubtract(p0, p2));

					// If the cross product points in the same direction as the normal the the
					// point is inside the edge (it is zero if is on the edge).
					XMVECTOR Zero = XMVectorZero();
					XMVECTOR Inside0 = XMVectorLessOrEqual(XMVector3Dot(C0, N), Zero);
					XMVECTOR Inside1 = XMVectorLessOrEqual(XMVector3Dot(C1, N), Zero);
					XMVECTOR Inside2 = XMVectorLessOrEqual(XMVector3Dot(C2, N), Zero);

					// If the point inside all of the edges it is inside.
					XMVECTOR Intersection = XMVectorAndInt(XMVectorAndInt(Inside0, Inside1), Inside2);

					bool inside = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

					// Find the nearest point on each edge.

					// Edge 0,1
					XMVECTOR Point1 = wi::math::ClosestPointOnLineSegment(p0, p1, Center);

					// If the distance to the center of the sphere to the point is less than 
					// the radius of the sphere then it must intersect.
					Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point1)), RadiusSq));

					// Edge 1,2
					XMVECTOR Point2 = wi::math::ClosestPointOnLineSegment(p1, p2, Center);

					// If the distance to the center of the sphere to the point is less than 
					// the radius of the sphere then it must intersect.
					Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point2)), RadiusSq));

					// Edge 2,0
					XMVECTOR Point3 = wi::math::ClosestPointOnLineSegment(p2, p0, Center);

					// If the distance to the center of the sphere to the point is less than 
					// the radius of the sphere then it must intersect.
					Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point3)), RadiusSq));

					bool intersects = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

					if (intersects)
					{
						XMVECTOR bestPoint = Point0;
						if (!inside)
						{
							// If the sphere center's projection on the triangle plane is not within the triangle,
							//	determine the closest point on triangle to the sphere center
							float bestDist = XMVectorGetX(XMVector3LengthSq(Point1 - Center));
							bestPoint = Point1;

							float d = XMVectorGetX(XMVector3LengthSq(Point2 - Center));
							if (d < bestDist)
							{
								bestDist = d;
								bestPoint = Point2;
							}
							d = XMVectorGetX(XMVector3LengthSq(Point3 - Center));
							if (d < bestDist)
							{
								bestDist = d;
								bestPoint = Point3;
							}
						}
						XMVECTOR intersectionVec = Center - bestPoint;
						XMVECTOR intersectionVecLen = XMVector3Length(intersectionVec);

						float lenX = XMVectorGetX(intersectionVecLen);
						float depth = capsule.radius - lenX;
						if (depth > result.depth)
						{
							result.entity = entity;
							XMStoreFloat3(&result.position, bestPoint);
							if (lenX > std::numeric_limits<float>::epsilon())
							{
								result.depth = depth;
								XMStoreFloat3(&result.normal, intersectionVec / intersectionVecLen);
							} 
							else 
							{
								// The line segment that makes the spine of the capsule has 
								// intersected the triangle plane, so interSectionVec ~= Zero, 
								// and depth ~= capsule.radius.  Use the triangle normal.
								XMVECTOR CandNorm; 
								if (onBackside)
								{
									CandNorm = N;
								} else 
								{
									CandNorm = XMVectorNegate(N);
								}
								XMStoreFloat3(&result.normal, CandNorm);

								// If the capsule has penetrated enough to intersect the spine, the
								// depth is calculated from closest point on the spine, not from the
								// actual endpoint, so the real depth may be greater, depending on the
								// orientation of the capsule relative to the triangle normal.
								// For simplicity, we assume the penetrating endpoint is the one closest
								// to Center, and we project the distance from Center to the closest endpoint
								// onto the normal.
								XMVECTOR A_C = XMVector3LengthSq(Center - A);
								XMVECTOR B_C = XMVector3LengthSq(Center - B);
								XMVECTOR CDiff;
								if (XMVector3Less(A_C, B_C)) 
								{
									CDiff = XMVectorSubtract(A, Center);
								}
								else 
								{
									CDiff = XMVectorSubtract(B, Center);
								}
								XMVECTOR CDiffOnN = XMVectorMultiply(XMVector3Dot(CDiff, N), CDiff);
								result.depth = depth + XMVectorGetX(XMVector3Length(CDiffOnN));
							}

							XMVECTOR vel = bestPoint - XMVector3Transform(XMVector3Transform(bestPoint, objectMat_Inverse), objectMatPrev);
							XMStoreFloat3(&result.velocity, vel);

							result.subsetIndex = (int)subsetIndex;
						}
					}
				};

				if (mesh->bvh.IsValid())
				{
					XMFLOAT3 base_local;
					XMFLOAT3 tip_local;
					float radius_local;
					XMStoreFloat3(&base_local, XMVector3Transform(XMLoadFloat3(&capsule.base), objectMat_Inverse));
					XMStoreFloat3(&tip_local, XMVector3Transform(XMLoadFloat3(&capsule.tip), objectMat_Inverse));
					XMStoreFloat(&radius_local, XMVector3Length(XMVector3TransformNormal(XMLoadFloat(&capsule.radius), objectMat_Inverse)));
					AABB capsule_local_aabb = Capsule(base_local, tip_local, radius_local).getAABB();

					mesh->bvh.Intersects(capsule_local_aabb, 0, [&](uint32_t index){
						const AABB& leaf = mesh->bvh_leaf_aabbs[index];
						const uint32_t triangleIndex = leaf.layerMask;
						const uint32_t subsetIndex = leaf.userdata;
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							return;
						const uint32_t indexOffset = subset.indexOffset;
						intersect_triangle(subsetIndex, indexOffset, triangleIndex);
					});
				}
				else
				{
					// Brute-force intersection test:
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(lod, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							continue;
						const uint32_t indexOffset = subset.indexOffset;
						const uint32_t triangleCount = subset.indexCount / 3;

						for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
						{
							intersect_triangle(subsetIndex, indexOffset, triangleIndex);
						}
					}
				}

			}
		}

		result.orientation = capsule.GetPlacementOrientation(result.position, result.normal);

		return result;
	}

	void Scene::VoxelizeObject(size_t objectIndex, wi::VoxelGrid& grid, bool subtract, uint32_t lod)
	{
		if (objectIndex >= objects.GetCount() || objectIndex >= aabb_objects.size())
			return;
		if (aabb_objects[objectIndex].intersects(grid.get_aabb()) == wi::primitive::AABB::OUTSIDE)
			return;
		const ObjectComponent& object = objects[objectIndex];
		const MeshComponent* mesh = meshes.GetComponent(object.meshID);
		if (mesh == nullptr)
			return;
		const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
		const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
		const ArmatureComponent* armature = mesh->IsSkinned() ? armatures.GetComponent(mesh->armatureID) : nullptr;

		uint32_t first_subset = 0;
		uint32_t last_subset = 0;
		mesh->GetLODSubsetRange(lod, first_subset, last_subset);
		for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
		{
			const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
			if (subset.indexCount == 0)
				continue;
			const uint32_t indexOffset = subset.indexOffset;
			const uint32_t triangleCount = subset.indexCount / 3;

			for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
			{
				const uint32_t i0 = mesh->indices[indexOffset + triangleIndex * 3 + 0];
				const uint32_t i1 = mesh->indices[indexOffset + triangleIndex * 3 + 1];
				const uint32_t i2 = mesh->indices[indexOffset + triangleIndex * 3 + 2];

				XMVECTOR p0;
				XMVECTOR p1;
				XMVECTOR p2;
				if (softbody != nullptr && !softbody->boneData.empty())
				{
					p0 = SkinVertex(*mesh, *softbody, i0);
					p1 = SkinVertex(*mesh, *softbody, i1);
					p2 = SkinVertex(*mesh, *softbody, i2);
				}
				else if (armature != nullptr && !armature->boneData.empty())
				{
					p0 = SkinVertex(*mesh, *armature, i0);
					p1 = SkinVertex(*mesh, *armature, i1);
					p2 = SkinVertex(*mesh, *armature, i2);
					p0 = XMVector3Transform(p0, objectMat);
					p1 = XMVector3Transform(p1, objectMat);
					p2 = XMVector3Transform(p2, objectMat);
				}
				else
				{
					p0 = XMLoadFloat3(&mesh->vertex_positions[i0]);
					p1 = XMLoadFloat3(&mesh->vertex_positions[i1]);
					p2 = XMLoadFloat3(&mesh->vertex_positions[i2]);
					p0 = XMVector3Transform(p0, objectMat);
					p1 = XMVector3Transform(p1, objectMat);
					p2 = XMVector3Transform(p2, objectMat);
				}

				grid.inject_triangle(p0, p1, p2, subtract);
			}
		}
	}
	
	void Scene::VoxelizeScene(wi::VoxelGrid& voxelgrid, bool subtract, uint32_t filterMask, uint32_t layerMask, uint32_t lod)
	{
		wi::jobsystem::context ctx;
		if ((filterMask & FILTER_COLLIDER))
		{
			for (size_t i = 0; i < collider_count_cpu; ++i)
			{
				const ColliderComponent& collider = colliders_cpu[i];

				if ((collider.layerMask & layerMask) == 0)
					continue;

				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
				{
					Sphere sphere = collider.sphere;
					// TODO: fix heap allocating lambda capture!
					wi::jobsystem::Execute(ctx, [&voxelgrid, subtract, sphere](wi::jobsystem::JobArgs args) {
						voxelgrid.inject_sphere(sphere, subtract);
						});
				}
				break;
				case ColliderComponent::Shape::Capsule:
				{
					Capsule capsule = collider.capsule;
					// TODO: fix heap allocating lambda capture!
					wi::jobsystem::Execute(ctx, [&voxelgrid, subtract, capsule](wi::jobsystem::JobArgs args) {
						voxelgrid.inject_capsule(capsule, subtract);
						});
				}
				break;
				case ColliderComponent::Shape::Plane:
				{
					XMMATRIX planeMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&collider.plane.projection));
					XMVECTOR P0 = XMVector3Transform(XMVectorSet(-1, 0, -1, 1), planeMatrix);
					XMVECTOR P1 = XMVector3Transform(XMVectorSet(1, 0, -1, 1), planeMatrix);
					XMVECTOR P2 = XMVector3Transform(XMVectorSet(1, 0, 1, 1), planeMatrix);
					XMVECTOR P3 = XMVector3Transform(XMVectorSet(-1, 0, 1, 1), planeMatrix);
					// TODO: fix heap allocating lambda capture!
					wi::jobsystem::Execute(ctx, [&voxelgrid, subtract, P0, P1, P2, P3](wi::jobsystem::JobArgs args) {
						voxelgrid.inject_triangle(P0, P1, P2, subtract);
						voxelgrid.inject_triangle(P0, P2, P3, subtract);
						});
				}
				break;
				}
			}
		}
		if (filterMask & FILTER_OBJECT_ALL)
		{
			for (size_t i = 0; i < objects.GetCount(); ++i)
			{
				const ObjectComponent& object = objects[i];
				if ((filterMask & object.GetFilterMask()) == 0)
					continue;
				const AABB& aabb = aabb_objects[i];
				if ((layerMask & aabb.layerMask) == 0)
					continue;
				// TODO: fix heap allocating lambda capture!
				wi::jobsystem::Execute(ctx, [this, &voxelgrid, subtract, lod, i](wi::jobsystem::JobArgs args) {
					VoxelizeObject(i, voxelgrid, subtract, lod);
					});
			}
		}
		wi::jobsystem::Wait(ctx);
	}

	XMFLOAT3 Scene::GetPositionOnSurface(wi::ecs::Entity objectEntity, int vertexID0, int vertexID1, int vertexID2, const XMFLOAT2& bary) const
	{
		const ObjectComponent* object = objects.GetComponent(objectEntity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			return XMFLOAT3(0, 0, 0);
		const MeshComponent* mesh = meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			return XMFLOAT3(0, 0, 0);

		const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object->meshID);
		const ArmatureComponent* armature = mesh->IsSkinned() ? armatures.GetComponent(mesh->armatureID) : nullptr;

		XMVECTOR P;
		XMVECTOR p0;
		XMVECTOR p1;
		XMVECTOR p2;
		if (softbody != nullptr && !softbody->boneData.empty())
		{
			p0 = SkinVertex(*mesh, *softbody, vertexID0);
			p1 = SkinVertex(*mesh, *softbody, vertexID1);
			p2 = SkinVertex(*mesh, *softbody, vertexID2);
			P = XMVectorBaryCentric(p0, p1, p2, bary.x, bary.y);
		}
		else if (armature != nullptr && !armature->boneData.empty())
		{
			p0 = SkinVertex(*mesh, *armature, vertexID0);
			p1 = SkinVertex(*mesh, *armature, vertexID1);
			p2 = SkinVertex(*mesh, *armature, vertexID2);

			P = XMVectorBaryCentric(p0, p1, p2, bary.x, bary.y);
			const size_t objectIndex = objects.GetIndex(objectEntity);
			const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
			P = XMVector3Transform(P, objectMat);
		}
		else
		{
			p0 = XMLoadFloat3(&mesh->vertex_positions[vertexID0]);
			p1 = XMLoadFloat3(&mesh->vertex_positions[vertexID1]);
			p2 = XMLoadFloat3(&mesh->vertex_positions[vertexID2]);

			P = XMVectorBaryCentric(p0, p1, p2, bary.x, bary.y);
			const size_t objectIndex = objects.GetIndex(objectEntity);
			const XMMATRIX objectMat = XMLoadFloat4x4(&matrix_objects[objectIndex]);
			P = XMVector3Transform(P, objectMat);
		}

		XMFLOAT3 result;
		XMStoreFloat3(&result, P);
		return result;
	}

	void Scene::ResetPose(wi::ecs::Entity entity)
	{
		// All child armatures will be also calling ResetPose, in case you give a parent entity of them, for convenience:
		for (size_t i = 0; i < armatures.GetCount(); ++i)
		{
			Entity armatureEntity = armatures.GetEntity(i);
			if (Entity_IsDescendant(armatureEntity, entity))
			{
				ResetPose(armatureEntity);
			}
		}

		const ArmatureComponent* armature = armatures.GetComponent(entity);
		if (armature == nullptr)
			return;

		XMMATRIX W = XMMatrixIdentity();
		const TransformComponent* armature_transform = transforms.GetComponent(entity);
		if (armature_transform != nullptr)
		{
			W = XMLoadFloat4x4(&armature_transform->world);
		}

		for (size_t i = 0; i < armature->boneCollection.size(); ++i)
		{
			Entity bone = armature->boneCollection[i];
			TransformComponent* transform = transforms.GetComponent(bone);
			if (transform != nullptr)
			{
				transform->ClearTransform();
				transform->MatrixTransform(XMMatrixInverse(nullptr, XMLoadFloat4x4(&armature->inverseBindMatrices[i])) * W);
				transform->UpdateTransform();
				const HierarchyComponent* hier = hierarchy.GetComponent(bone);
				if (hier != nullptr && hier->parentID != INVALID_ENTITY)
				{
					Component_Attach(bone, hier->parentID, false);
				}
			}
		}
	}

	XMFLOAT3 Scene::GetOceanPosAt(const XMFLOAT3& worldPosition) const
	{
		if (!ocean.IsValid())
			return worldPosition;
		return ocean.GetDisplacedPosition(worldPosition);
	}

	bool Scene::IsWetmapProcessingRequired() const
	{
		return wetmap_fadeout_time > 0;
	}

	void Scene::PutWaterRipple(const std::string& image, const XMFLOAT3& pos)
	{
		wi::Sprite img(image);
		img.params.enableExtractNormalMap();
		img.params.blendFlag = BLENDMODE_ADDITIVE;
		img.anim.fad = 0.01f;
		img.anim.scaleX = 0.1f;
		img.anim.scaleY = 0.1f;
		img.params.pos = pos;
		img.params.rotation = (wi::random::GetRandom(0, 1000) * 0.001f) * 2 * 3.1415f;
		img.params.siz = XMFLOAT2(1, 1);
		img.params.quality = wi::image::QUALITY_ANISOTROPIC;
		img.params.pivot = XMFLOAT2(0.5f, 0.5f);
		waterRipples.push_back(img);
	}
	void Scene::PutWaterRipple(const XMFLOAT3& pos)
	{
		wi::Sprite img;
		img.textureResource.SetTexture(*wi::texturehelper::getWaterRipple());
		img.params.enableExtractNormalMap();
		img.params.blendFlag = BLENDMODE_ADDITIVE;
		img.anim.fad = 0.01f;
		img.anim.scaleX = 0.1f;
		img.anim.scaleY = 0.1f;
		img.params.pos = pos;
		img.params.rotation = (wi::random::GetRandom(0, 1000) * 0.001f) * 2 * 3.1415f;
		img.params.siz = XMFLOAT2(1, 1);
		img.params.quality = wi::image::QUALITY_ANISOTROPIC;
		img.params.pivot = XMFLOAT2(0.5f, 0.5f);
		waterRipples.push_back(img);
	}

	XMVECTOR SkinVertex(const MeshComponent& mesh, const wi::vector<ShaderTransform>& boneData, uint32_t index, XMVECTOR* N)
	{
		XMVECTOR P = XMLoadFloat3(&mesh.vertex_positions[index]);

		const uint32_t influence_div4 = mesh.GetBoneInfluenceDiv4();

		XMVECTOR skinnedP = XMVectorZero();
		XMVECTOR skinnedN = XMVectorZero();
		for (uint32_t influence = 0; influence < influence_div4; ++influence)
		{
			const XMUINT4& ind = influence == 0 ? mesh.vertex_boneindices[index] : mesh.vertex_boneindices2[index];
			const XMFLOAT4& wei = influence == 0 ? mesh.vertex_boneweights[index]: mesh.vertex_boneweights2[index];

			const XMFLOAT4X4 mat[] = {
				boneData[ind.x].GetMatrix(),
				boneData[ind.y].GetMatrix(),
				boneData[ind.z].GetMatrix(),
				boneData[ind.w].GetMatrix(),
			};
			const XMMATRIX M[] = {
				XMMatrixTranspose(XMLoadFloat4x4(&mat[0])),
				XMMatrixTranspose(XMLoadFloat4x4(&mat[1])),
				XMMatrixTranspose(XMLoadFloat4x4(&mat[2])),
				XMMatrixTranspose(XMLoadFloat4x4(&mat[3])),
			};

			skinnedP += XMVector3Transform(P, M[0]) * wei.x;
			skinnedP += XMVector3Transform(P, M[1]) * wei.y;
			skinnedP += XMVector3Transform(P, M[2]) * wei.z;
			skinnedP += XMVector3Transform(P, M[3]) * wei.w;

			if (N != nullptr)
			{
				*N = XMLoadFloat3(&mesh.vertex_normals[index]);
				skinnedN += XMVector3TransformNormal(*N, M[0]) * wei.x;
				skinnedN += XMVector3TransformNormal(*N, M[1]) * wei.y;
				skinnedN += XMVector3TransformNormal(*N, M[2]) * wei.z;
				skinnedN += XMVector3TransformNormal(*N, M[3]) * wei.w;
			}
		}

		P = skinnedP;

		if (N != nullptr)
		{
			*N = XMVector3Normalize(skinnedN);
		}

		return P;
	}
	XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N)
	{
		return SkinVertex(mesh, armature.boneData, index, N);
	}
	XMVECTOR SkinVertex(const MeshComponent& mesh, const SoftBodyPhysicsComponent& softbody, uint32_t index, XMVECTOR* N)
	{
		return SkinVertex(mesh, softbody.boneData, index, N);
	}




	Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix, bool attached)
	{
		Entity rootEntity = INVALID_ENTITY;
		if (attached)
		{
			rootEntity = CreateEntity();
		}
		LoadModel2(fileName, transformMatrix, rootEntity);
		return rootEntity;
	}

	Entity LoadModel(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix, bool attached)
	{
		Entity rootEntity = INVALID_ENTITY;
		if (attached)
		{
			rootEntity = CreateEntity();
		}
		LoadModel2(scene, fileName, transformMatrix, rootEntity);
		return rootEntity;
	}

	void LoadModel2(const std::string& fileName, const XMMATRIX& transformMatrix, Entity rootEntity)
	{
		Scene scene;
		LoadModel(scene, fileName, transformMatrix, rootEntity);
		GetScene().Merge(scene);
	}

	void LoadModel2(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix, Entity rootEntity)
	{
		wi::Archive archive(fileName, true);
		if (!archive.IsOpen())
			return;

		// Serialize it from file:
		scene.Serialize(archive);

		// First, create new root:
		bool attached = true;
		if (rootEntity == INVALID_ENTITY)
		{
			rootEntity = CreateEntity();
			attached = false;
		}
		scene.transforms.Create(rootEntity);
		scene.layers.Create(rootEntity).layerMask = ~0;

		{
			// Apply the optional transformation matrix to the new scene:

			// Parent all unparented transforms to new root entity
			for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
			{
				Entity entity = scene.transforms.GetEntity(i);
				if (entity != rootEntity && !scene.hierarchy.Contains(entity))
				{
					scene.Component_Attach(entity, rootEntity);
				}
			}

			// The root component is transformed, scene is updated:
			TransformComponent* root_transform = scene.transforms.GetComponent(rootEntity);
			root_transform->MatrixTransform(transformMatrix);

			scene.Update(0);
		}

		if (!attached)
		{
			// In this case, we don't care about the root anymore, so delete it. This will simplify overall hierarchy
			scene.Component_DetachChildren(rootEntity);
			scene.Entity_Remove(rootEntity);
		}
	}

	PickResult Pick(const wi::primitive::Ray& ray, uint32_t filterMask, uint32_t layerMask, const Scene& scene, uint32_t lod)
	{
		return scene.Intersects(ray, filterMask, layerMask, lod);
	}
	SceneIntersectSphereResult SceneIntersectSphere(const wi::primitive::Sphere& sphere, uint32_t filterMask, uint32_t layerMask, const Scene& scene, uint32_t lod)
	{
		return scene.Intersects(sphere, filterMask, layerMask, lod);
	}
	SceneIntersectCapsuleResult SceneIntersectCapsule(const wi::primitive::Capsule& capsule, uint32_t filterMask, uint32_t layerMask, const Scene& scene, uint32_t lod)
	{
		return scene.Intersects(capsule, filterMask, layerMask, lod);
	}


	XMMATRIX Scene::ComputeParentMatrixRecursive(Entity entity) const
	{
		XMMATRIX parentMatrix = XMMatrixIdentity();

		HierarchyComponent* hier = hierarchy.GetComponent(entity);
		if (hier != nullptr)
		{
			Entity parentID = hier->parentID;
			while (parentID != INVALID_ENTITY)
			{
				TransformComponent* transform_parent = transforms.GetComponent(parentID);
				if (transform_parent == nullptr)
					break;

				parentMatrix *= transform_parent->GetLocalMatrix();

				const HierarchyComponent* hier_recursive = hierarchy.GetComponent(parentID);
				if (hier_recursive != nullptr)
				{
					parentID = hier_recursive->parentID;
				}
				else
				{
					parentID = INVALID_ENTITY;
				}
			}
		}
		return parentMatrix;
	}

	Entity Scene::RetargetAnimation(Entity dst, Entity src, bool bake_data, const Scene* src_scene)
	{
		if (src_scene == nullptr)
			src_scene = this;

		const AnimationComponent* animation_source = src_scene->animations.GetComponent(src);
		if (animation_source == nullptr)
			return INVALID_ENTITY;
		const HumanoidComponent* humanoid_dest = humanoids.GetComponent(dst);
		if (humanoid_dest == nullptr)
			return INVALID_ENTITY;

		bool retarget_valid = false;
		Scene retarget_scene;
		Entity retarget_entity = CreateEntity();
		AnimationComponent& animation = retarget_scene.animations.Create(retarget_entity);
		animation = *animation_source;
		animation.channels.clear();
		animation.samplers.clear();
		animation.retargets.clear();

		for (auto& channel : animation_source->channels)
		{
			bool found = false;
			for (size_t i = 0; (i < src_scene->humanoids.GetCount()) && !found; ++i)
			{
				const HumanoidComponent& humanoid_source = src_scene->humanoids[i];
				for (size_t humanoidBoneIndex = 0; humanoidBoneIndex < arraysize(humanoid_source.bones); ++humanoidBoneIndex)
				{
					Entity bone_source = humanoid_source.bones[humanoidBoneIndex];
					if (bone_source == channel.target)
					{
						Entity bone_dest = humanoid_dest->bones[humanoidBoneIndex];

						TransformComponent* transform_source = src_scene->transforms.GetComponent(bone_source);
						TransformComponent* transform_dest = transforms.GetComponent(bone_dest);
						if (transform_source != nullptr && transform_dest != nullptr)
						{
							retarget_valid = true;
							found = true;

							auto& retarget_channel = animation.channels.emplace_back();
							retarget_channel = channel;
							retarget_channel.target = bone_dest;
							retarget_channel.samplerIndex = (int)animation.samplers.size();

							auto& sampler = animation_source->samplers[channel.samplerIndex];

							auto& retarget_sampler = animation.samplers.emplace_back();
							retarget_sampler = sampler;
							retarget_sampler.backwards_compatibility_data = {};
							retarget_sampler.scene = src_scene == this ? nullptr : src_scene;

							XMMATRIX srcParentMatrix = src_scene->ComputeParentMatrixRecursive(bone_source);
							XMMATRIX srcMatrix = transform_source->GetLocalMatrix() * srcParentMatrix;
							XMMATRIX inverseSrcMatrix = XMMatrixInverse(nullptr, srcMatrix);

							XMMATRIX dstParentMatrix = ComputeParentMatrixRecursive(bone_dest);
							XMMATRIX dstMatrix = transform_dest->GetLocalMatrix() * dstParentMatrix;
							XMMATRIX inverseDstParentMatrix = XMMatrixInverse(nullptr, dstParentMatrix);

							XMMATRIX dstRelativeMatrix = dstMatrix * inverseSrcMatrix;
							XMMATRIX srcRelativeParentMatrix = srcParentMatrix * inverseDstParentMatrix;

							if (bake_data)
							{
								// Create new animation data and bake the retargeted result into it:
								Entity retarget_data_entity = CreateEntity();
								auto& retarget_animation_data = retarget_scene.animation_datas.Create(retarget_data_entity);
								retarget_sampler.data = retarget_data_entity;
								retarget_scene.Component_Attach(retarget_data_entity, retarget_entity);

								auto& animation_data = animation_datas.Contains(sampler.data) ? *animation_datas.GetComponent(sampler.data) : sampler.backwards_compatibility_data;
								retarget_animation_data = animation_data;

								XMVECTOR S, R, T; // matrix decompose destinations

								switch (channel.path)
								{
								case AnimationComponent::AnimationChannel::Path::SCALE:
									for (size_t offset = 0; offset < retarget_animation_data.keyframe_data.size(); offset += 3)
									{
										XMFLOAT3* data = (XMFLOAT3*)&retarget_animation_data.keyframe_data[offset];
										TransformComponent transform = *transform_source;
										transform.scale_local = *data;
										XMMATRIX localMatrix = dstRelativeMatrix * transform.GetLocalMatrix() * srcRelativeParentMatrix;
										XMMatrixDecompose(&S, &R, &T, localMatrix);
										XMStoreFloat3(data, S);
									}
									break;
								case AnimationComponent::AnimationChannel::Path::ROTATION:
									for (size_t offset = 0; offset < retarget_animation_data.keyframe_data.size(); offset += 4)
									{
										XMFLOAT4* data = (XMFLOAT4*)&retarget_animation_data.keyframe_data[offset];
										TransformComponent transform = *transform_source;
										transform.rotation_local = *data;
										XMMATRIX localMatrix = dstRelativeMatrix * transform.GetLocalMatrix() * srcRelativeParentMatrix;
										XMMatrixDecompose(&S, &R, &T, localMatrix);
										XMStoreFloat4(data, R);
									}
									break;
								case AnimationComponent::AnimationChannel::Path::TRANSLATION:
									for (size_t offset = 0; offset < retarget_animation_data.keyframe_data.size(); offset += 3)
									{
										XMFLOAT3* data = (XMFLOAT3*)&retarget_animation_data.keyframe_data[offset];
										TransformComponent transform = *transform_source;
										transform.translation_local = *data;
										XMMATRIX localMatrix = dstRelativeMatrix * transform.GetLocalMatrix() * srcRelativeParentMatrix;
										XMMatrixDecompose(&S, &R, &T, localMatrix);
										XMStoreFloat3(data, T);
									}
									break;
								default:
									break;
								}
							}
							else
							{
								// Don't bake retarget data, but inform the animation channel of original source data:
								retarget_channel.retargetIndex = (int)animation.retargets.size();
								AnimationComponent::RetargetSourceData& retarget = animation.retargets.emplace_back();
								retarget.source = bone_source;
								XMStoreFloat4x4(&retarget.dstRelativeMatrix, dstRelativeMatrix);
								XMStoreFloat4x4(&retarget.srcRelativeParentMatrix, srcRelativeParentMatrix);
							}
						}

						break;
					}
				}
			}
		}
		if (retarget_valid)
		{
			retarget_scene.Component_Attach(retarget_entity, dst);
			Merge(retarget_scene);
			return retarget_entity;
		}
		return INVALID_ENTITY;
	}

	XMMATRIX Scene::GetRestPose(wi::ecs::Entity entity) const
	{
		if (entity != INVALID_ENTITY)
		{
			for (size_t i = 0; i < armatures.GetCount(); ++i)
			{
				const ArmatureComponent& armature = armatures[i];
				int boneIndex = -1;
				for (auto& x : armature.boneCollection)
				{
					boneIndex++;
					if (x == entity)
					{
						XMMATRIX inverseBindMatrix = XMLoadFloat4x4(armature.inverseBindMatrices.data() + boneIndex);
						XMMATRIX bindMatrix = XMMatrixInverse(nullptr, inverseBindMatrix);
						return bindMatrix;
					}
				}
			}

			const TransformComponent* transform = transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				return XMLoadFloat4x4(&transform->world);
			}
		}
		return XMMatrixIdentity();
	}

	float Scene::GetHumanoidDefaultFacing(const HumanoidComponent& humanoid, Entity humanoidEntity) const
	{
		Entity left_shoulder = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperArm];
		Entity right_shoulder = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperArm];
		XMVECTOR left_shoulder_pos = GetRestPose(left_shoulder).r[3];
		XMVECTOR right_shoulder_pos = GetRestPose(right_shoulder).r[3];
		const TransformComponent* transform = transforms.GetComponent(humanoidEntity);
		if (transform != nullptr)
		{
			XMVECTOR S = transform->GetScaleV();
			left_shoulder_pos *= S;
			right_shoulder_pos *= S;
		}
		if (XMVectorGetX(right_shoulder_pos) < XMVectorGetX(left_shoulder_pos))
		{
			return -1;
		}
		return 1;
	}

	void Scene::ScanAnimationDependencies()
	{
		if (animations.GetCount() == 0)
		{
			animation_queue_count = 0;
			return;
		}

		animation_queues.reserve(animations.GetCount());
		animation_queue_count = 0;

		wi::jobsystem::Execute(animation_dependency_scan_workload, [&](wi::jobsystem::JobArgs args) {
			auto range = wi::profiler::BeginRangeCPU("Animation Dependencies");
			for (size_t i = 0; i < animations.GetCount(); ++i)
			{
				AnimationComponent& animationA = animations[i];
				if (!animationA.IsPlaying() && animationA.last_update_time == animationA.timer)
				{
					continue;
				}
				bool dependency = false;
				for (size_t queue_index = 0; queue_index < animation_queue_count; ++queue_index)
				{
					AnimationQueue& queue = animation_queues[queue_index];
					for (auto& channelA : animationA.channels)
					{
						if (dependency)
						{
							// If dependency has been found, record all other entities in this animation too:
							queue.entities.insert(channelA.target);
						}
						else if (queue.entities.find(channelA.target) != queue.entities.end())
						{
							// If two animations target the same entity, they have a dependency and need to be executed in order:
							dependency = true;
							queue.animations.push_back(&animationA);
						}
					}
					if (dependency) break;
				}
				if (!dependency)
				{
					// No dependency, it can be executed on a separate queue (thread)
					if (animation_queues.size() <= animation_queue_count)
					{
						animation_queues.resize(animation_queue_count + 1);
					}
					AnimationQueue& queue = animation_queues[animation_queue_count];
					queue.animations.clear();
					queue.animations.push_back(&animationA);
					queue.entities.clear();
					for (auto& channelA : animationA.channels)
					{
						queue.entities.insert(channelA.target);
					}
					animation_queue_count++;
				}
			}
			wi::profiler::EndRange(range);
		});

		// We don't wait for this job here, it will be waited just before animation update
	}

	void Scene::ScanSpringDependencies()
	{
		wi::jobsystem::Execute(spring_dependency_scan_workload, [this](wi::jobsystem::JobArgs args){
			auto range = wi::profiler::BeginRangeCPU("Spring Dependencies");
			spring_queues.clear();
			// First, reset all spring temp state:
			for (size_t i = 0; i < springs.GetCount(); ++i)
			{
				SpringComponent& spring = springs[i];
				spring.children.clear();
				spring.entity = INVALID_ENTITY;
				spring.transform = nullptr;
				spring.parent_transform = nullptr;
			}
			// Then determine dependencies and set temp values:
			for (size_t i = 0; i < springs.GetCount(); ++i)
			{
				SpringComponent& spring = springs[i];
				if (spring.IsDisabled())
					continue;
				Entity entity = springs.GetEntity(i);
				TransformComponent* transform = transforms.GetComponent(entity);
				if (transform == nullptr)
					continue;
				spring.entity = entity;
				spring.transform = transform;
				const HierarchyComponent* hier = hierarchy.GetComponent(entity);
				if (hier == nullptr)
				{
					// This is a root spring
					spring_queues.push_back(&spring);
				}
				else
				{
					spring.parent_transform = transforms.GetComponent(hier->parentID);
					SpringComponent* parent = springs.GetComponent(hier->parentID);
					if (parent == nullptr)
					{
						// This is a root spring
						spring_queues.push_back(&spring);
					}
					else
					{
						// This has a parent
						parent->children.push_back(&spring);
					}
				}
			}
			wi::profiler::EndRange(range);
		});

		// We don't wait for this job here, it will be waited just before spring update
	}
	void Scene::UpdateSpringsTopDownRecursive(SpringComponent* parent_spring, SpringComponent& spring)
	{
		Entity entity = spring.entity;
		TransformComponent& transform = *spring.transform;

		if (spring.IsResetting())
		{
			spring.Reset(false);

			// Note: the spring resetting works on the rest pose, not the current pose!

			XMMATRIX parentWorldMatrix = XMMatrixIdentity();
			{
				const HierarchyComponent* hier = hierarchy.GetComponent(entity);
				if (hier != nullptr)
				{
					parentWorldMatrix = GetRestPose(hier->parentID);
				}
			}
			XMMATRIX parentWorldMatrixInverse = XMMatrixInverse(nullptr, parentWorldMatrix);

			XMVECTOR position_root = GetRestPose(entity).r[3];
			XMVECTOR tail = position_root + XMVectorSet(0, 1, 0, 0);
			// Search for child to find the rest pose tail position:
			bool child_found = false;
			for (size_t j = 0; j < hierarchy.GetCount(); ++j)
			{
				const HierarchyComponent& hier = hierarchy[j];
				Entity child = hierarchy.GetEntity(j);
				if (hier.parentID == entity && transforms.Contains(child))
				{
					tail = GetRestPose(child).r[3];
					child_found = true;
					break;
				}
			}
			if (!child_found)
			{
				// No child, try to guess tail position compared to parent:
				const XMVECTOR parent_pos = parentWorldMatrix.r[3];
				const XMVECTOR ab = position_root - parent_pos;
				tail = position_root + ab;
			}

			XMVECTOR axis = tail - position_root;
			axis = XMVector3TransformNormal(axis, parentWorldMatrixInverse);
			XMStoreFloat3(&spring.boneAxis, axis);
			XMStoreFloat3(&spring.currentTail, tail);
			spring.prevTail = spring.currentTail;
		}

		XMMATRIX parentWorldMatrix = XMMatrixIdentity();
		if (spring.parent_transform != nullptr)
		{
			transform.UpdateTransform_Parented(*spring.parent_transform);
			parentWorldMatrix = XMLoadFloat4x4(&spring.parent_transform->world);
		}

		XMVECTOR position_root = transform.GetPositionV();

		// fixup spring locations by snapping position to parent's tail:
		//	(This is done after resetting code intentionally)
		if (parent_spring != nullptr)
		{
			position_root = XMLoadFloat3(&parent_spring->currentTail);
		}

		XMVECTOR boneAxis = XMLoadFloat3(&spring.boneAxis);
		boneAxis = XMVector3TransformNormal(boneAxis, parentWorldMatrix);

		const float boneLength = XMVectorGetX(XMVector3Length(boneAxis));
		boneAxis /= boneLength;
		const float dragForce = spring.dragForce;
		const float stiffnessForce = spring.stiffnessForce;
		const XMVECTOR gravityDir = XMLoadFloat3(&spring.gravityDir);
		const float gravityPower = spring.gravityPower;

		const XMVECTOR tail_current = XMLoadFloat3(&spring.currentTail);
		const XMVECTOR tail_prev = XMLoadFloat3(&spring.prevTail);

		XMVECTOR inertia = (tail_current - tail_prev) * (1 - dragForce);
		XMVECTOR stiffness = boneAxis * stiffnessForce;
		XMVECTOR external = XMVectorZero();

		if (spring.windForce > 0)
		{
			const XMVECTOR windDir = XMLoadFloat3(&weather.windDirection);
			external += std::sin(time * weather.windSpeed + XMVectorGetX(XMVector3Dot(tail_current, windDir))) * windDir * spring.windForce;
		}
		if (spring.IsGravityEnabled())
		{
			external += gravityDir * gravityPower;
		}

		XMVECTOR tail_next = tail_current + inertia + dt * (stiffness + external);
		XMVECTOR to_tail = XMVector3Normalize(tail_next - position_root);

		// Limit offset to keep distance from parent:
		tail_next = position_root + to_tail * boneLength;

#if 1
		// Collider checks:
		//	apply scaling to radius:
		XMFLOAT3 scale = transform.GetScale();
		const float hitRadius = spring.hitRadius * std::max(scale.x, std::max(scale.y, scale.z));
		wi::primitive::Sphere tail_sphere;
		XMStoreFloat3(&tail_sphere.center, tail_next);
		tail_sphere.radius = hitRadius;

		if (colliders_cpu != nullptr && collider_bvh.IsValid())
		{
			collider_bvh.Intersects(tail_sphere, 0, [&](uint32_t collider_index) {
				if (colliders.GetCount() <= collider_index)
					return;
				const ColliderComponent& collider = colliders_cpu[collider_index];

				float dist = 0;
				XMFLOAT3 direction = {};
				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
					tail_sphere.intersects(collider.sphere, dist, direction);
					break;
				case ColliderComponent::Shape::Capsule:
					tail_sphere.intersects(collider.capsule, dist, direction);
					break;
				case ColliderComponent::Shape::Plane:
					tail_sphere.intersects(collider.plane, dist, direction);
					break;
				}

				if (dist < 0)
				{
					tail_next = tail_next - XMLoadFloat3(&direction) * dist;
					to_tail = XMVector3Normalize(tail_next - position_root);

					// Limit offset to keep distance from parent:
					tail_next = position_root + to_tail * boneLength;

					XMStoreFloat3(&tail_sphere.center, tail_next);
					tail_sphere.radius = hitRadius;
				}
				});
		}
#endif

		XMStoreFloat3(&spring.prevTail, tail_current);
		XMStoreFloat3(&spring.currentTail, tail_next);

		// Rotate to face tail position:
		const XMVECTOR axis = XMVector3Normalize(XMVector3Cross(boneAxis, to_tail));
		const float angle = XMScalarACos(XMVectorGetX(XMVector3Dot(boneAxis, to_tail)));
		const XMVECTOR Q = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));

		// Modify world matrix:
		XMMATRIX M = XMLoadFloat4x4(&transform.world);
		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, M);

		T = position_root;
		R = XMQuaternionMultiply(R, Q);
		R = XMQuaternionNormalize(R);

		M = XMMatrixScalingFromVector(S) * XMMatrixRotationQuaternion(R) * XMMatrixTranslationFromVector(T);

		XMStoreFloat4x4(&transform.world, M);

#if 0
		// Debug axis:
		static wi::SpinLock dbglocker;
		wi::renderer::RenderableLine line;
		line.color_start = line.color_end = XMFLOAT4(1, 1, 0, 1);
		XMStoreFloat3(&line.start, position_root);
		line.end = spring.currentTail;
		dbglocker.lock();
		wi::renderer::DrawLine(line);
		dbglocker.unlock();
#endif

		for (SpringComponent* child : spring.children)
		{
			UpdateSpringsTopDownRecursive(&spring, *child);
		}
	}

}
