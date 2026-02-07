#include "wiGPUBVH.h"
#include "wiScene.h"
#include "wiRenderer.h"
#include "shaders/ShaderInterop_BVH.h"
#include "wiProfiler.h"
#include "wiResourceManager.h"
#include "wiGPUSortLib.h"
#include "wiTextureHelper.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiTimer.h"

//#define BVH_VALIDATE // slow but great for debug!

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::ecs;

namespace wi
{

	enum CSTYPES_BVH
	{
		CSTYPE_BVH_PRIMITIVES,
		CSTYPE_BVH_HIERARCHY,
		CSTYPE_BVH_PROPAGATEAABB,
		CSTYPE_BVH_COUNT
	};
	static Shader computeShaders[CSTYPE_BVH_COUNT];

	void GPUBVH::Update(const wi::scene::Scene& scene)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		// Pre-gather scene properties:
		uint totalTriangles = 0;
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			const ObjectComponent& object = scene.objects[i];

			if (object.meshID != INVALID_ENTITY)
			{
				const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

				totalTriangles += (uint)mesh.indices.size() / 3;
			}
		}
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			const wi::HairParticleSystem& hair = scene.hairs[i];

			if (hair.meshID != INVALID_ENTITY)
			{
				totalTriangles += hair.segmentCount * hair.strandCount * 2;
			}
		}
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			const wi::EmittedParticleSystem& emitter = scene.emitters[i];
			totalTriangles += emitter.GetMaxParticleCount() * 2;
		}

		if (totalTriangles > 0 && !primitiveCounterBuffer.IsValid())
		{
			GPUBufferDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.stride = sizeof(uint);
			desc.size = desc.stride;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			desc.usage = Usage::DEFAULT;
			device->CreateBuffer(&desc, nullptr, &primitiveCounterBuffer);
			device->SetName(&primitiveCounterBuffer, "GPUBVH::primitiveCounterBuffer");
		}

		if (totalTriangles == 0)
		{
			primitiveCounterBuffer = {};
		}

		if (totalTriangles > primitiveCapacity)
		{
			primitiveCapacity = std::max(2u, totalTriangles);

			GPUBufferDesc desc;

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.stride = sizeof(BVHNode);
			desc.size = desc.stride * primitiveCapacity * 2;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			desc.usage = Usage::DEFAULT;
			device->CreateBuffer(&desc, nullptr, &bvhNodeBuffer);
			device->SetName(&bvhNodeBuffer, "GPUBVH::BVHNodeBuffer");

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.stride = sizeof(uint);
			desc.size = desc.stride * primitiveCapacity * 2;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			desc.usage = Usage::DEFAULT;
			device->CreateBuffer(&desc, nullptr, &bvhParentBuffer);
			device->SetName(&bvhParentBuffer, "GPUBVH::BVHParentBuffer");

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.stride = sizeof(uint);
			desc.size = desc.stride * (((primitiveCapacity - 1) + 31) / 32); // bitfield for internal nodes
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			desc.usage = Usage::DEFAULT;
			device->CreateBuffer(&desc, nullptr, &bvhFlagBuffer);
			device->SetName(&bvhFlagBuffer, "GPUBVH::BVHFlagBuffer");

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.stride = sizeof(uint);
			desc.size = desc.stride * primitiveCapacity;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			desc.usage = Usage::DEFAULT;
			device->CreateBuffer(&desc, nullptr, &primitiveIDBuffer);
			device->SetName(&primitiveIDBuffer, "GPUBVH::primitiveIDBuffer");

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.stride = sizeof(BVHPrimitive);
			desc.size = desc.stride * primitiveCapacity;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			desc.usage = Usage::DEFAULT;
			device->CreateBuffer(&desc, nullptr, &primitiveBuffer);
			device->SetName(&primitiveBuffer, "GPUBVH::primitiveBuffer");

			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.size = desc.stride * primitiveCapacity;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			desc.usage = Usage::DEFAULT;
			desc.stride = sizeof(float); // morton buffer is float because sorting must be done and gpu sort operates on floats for now!
			device->CreateBuffer(&desc, nullptr, &primitiveMortonBuffer);
			device->SetName(&primitiveMortonBuffer, "GPUBVH::primitiveMortonBuffer");
		}
	}
	void GPUBVH::Build(const Scene& scene, CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		auto range = wi::profiler::BeginRangeGPU("BVH Rebuild", cmd);

		uint32_t primitiveCount = 0;

		device->EventBegin("BVH - Primitive Builder", cmd);
		{
			device->BindComputeShader(&computeShaders[CSTYPE_BVH_PRIMITIVES], cmd);
			const GPUResource* uavs[] = {
				&primitiveIDBuffer,
				&primitiveBuffer,
				&primitiveMortonBuffer,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			for (size_t i = 0; i < scene.objects.GetCount(); ++i)
			{
				const ObjectComponent& object = scene.objects[i];

				if (object.meshID != INVALID_ENTITY)
				{
					const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh.GetLODSubsetRange(object.lod, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];

						BVHPushConstants push;
						push.instanceIndex = (uint)i;
						push.subsetIndex = subsetIndex;
						push.primitiveCount = subset.indexCount / 3;
						push.primitiveOffset = primitiveCount;
						device->PushConstants(&push, sizeof(push), cmd);

						primitiveCount += push.primitiveCount;

						device->Dispatch(
							(push.primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE,
							1,
							1,
							cmd
						);
					}

				}
			}

			for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
			{
				const wi::HairParticleSystem& hair = scene.hairs[i];

				if (hair.meshID != INVALID_ENTITY)
				{
					BVHPushConstants push;
					push.instanceIndex = (uint)(scene.objects.GetCount() + i);
					push.subsetIndex = 0;
					push.primitiveCount = hair.segmentCount * hair.strandCount * 2;
					push.primitiveOffset = primitiveCount;
					device->PushConstants(&push, sizeof(push), cmd);

					primitiveCount += push.primitiveCount;

					device->Dispatch(
						(push.primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE,
						1,
						1,
						cmd
					);
				}
			}

			for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
			{
				const wi::EmittedParticleSystem& emitter = scene.emitters[i];

				if (emitter.GetMaxParticleCount() > 0)
				{
					BVHPushConstants push;
					push.instanceIndex = (uint)(scene.objects.GetCount() + i);
					push.subsetIndex = 0;
					push.primitiveCount = emitter.GetMaxParticleCount() * 2;
					push.primitiveOffset = primitiveCount;
					device->PushConstants(&push, sizeof(push), cmd);

					primitiveCount += push.primitiveCount;

					device->Dispatch(
						(push.primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE,
						1,
						1,
						cmd
					);
				}
			}

			GPUBarrier barriers[] = {
				GPUBarrier::Memory()
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&primitiveCounterBuffer, ResourceState::SHADER_RESOURCE, ResourceState::COPY_DST),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->UpdateBuffer(&primitiveCounterBuffer, &primitiveCount, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&primitiveCounterBuffer, ResourceState::COPY_DST, ResourceState::SHADER_RESOURCE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->EventEnd(cmd);

		device->EventBegin("BVH - Sort Primitive Mortons", cmd);
		wi::gpusortlib::Sort(primitiveCount, primitiveMortonBuffer, primitiveCounterBuffer, 0, primitiveIDBuffer, cmd);
		device->EventEnd(cmd);

		device->EventBegin("BVH - Build Hierarchy", cmd);
		{
			device->BindComputeShader(&computeShaders[CSTYPE_BVH_HIERARCHY], cmd);
			const GPUResource* uavs[] = {
				&bvhNodeBuffer,
				&bvhParentBuffer,
				&bvhFlagBuffer
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			const GPUResource* res[] = {
				&primitiveCounterBuffer,
				&primitiveIDBuffer,
				&primitiveMortonBuffer,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			device->Dispatch((primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory()
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->EventEnd(cmd);

		device->EventBegin("BVH - Propagate AABB", cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory()
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			device->BindComputeShader(&computeShaders[CSTYPE_BVH_PROPAGATEAABB], cmd);
			const GPUResource* uavs[] = {
				&bvhNodeBuffer,
				&bvhFlagBuffer,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			const GPUResource* res[] = {
				&primitiveCounterBuffer,
				&primitiveIDBuffer,
				&primitiveBuffer,
				&bvhParentBuffer,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			device->Dispatch((primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE, 1, 1, cmd);

			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->EventEnd(cmd);

		wi::profiler::EndRange(range); // BVH rebuild

#ifdef BVH_VALIDATE
		{
			GPUBufferDesc readback_desc;
			bool download_success;

			// Download primitive count:
			readback_desc = primitiveCounterBuffer.GetDesc();
			readback_desc.usage = USAGE_STAGING;
			readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
			readback_desc.bind_flags = 0;
			readback_desc.Flags = 0;
			GPUBuffer readback_primitiveCounterBuffer;
			device->CreateBuffer(&readback_desc, nullptr, &readback_primitiveCounterBuffer);
			uint primitiveCount;
			download_success = device->DownloadResource(&primitiveCounterBuffer, &readback_primitiveCounterBuffer, &primitiveCount, cmd);
			assert(download_success);

			if (primitiveCount > 0)
			{
				const uint leafNodeOffset = primitiveCount - 1;

				// Validate node buffer:
				readback_desc = bvhNodeBuffer.GetDesc();
				readback_desc.usage = USAGE_STAGING;
				readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
				readback_desc.bind_flags = 0;
				readback_desc.Flags = 0;
				GPUBuffer readback_nodeBuffer;
				device->CreateBuffer(&readback_desc, nullptr, &readback_nodeBuffer);
				vector<BVHNode> nodes(readback_desc.size / sizeof(BVHNode));
				download_success = device->DownloadResource(&bvhNodeBuffer, &readback_nodeBuffer, nodes.data(), cmd);
				assert(download_success);
				set<uint> visitedLeafs;
				vector<uint> stack;
				stack.push_back(0);
				while (!stack.empty())
				{
					uint nodeIndex = stack.back();
					stack.pop_back();

					if (nodeIndex >= leafNodeOffset)
					{
						// leaf node
						assert(visitedLeafs.count(nodeIndex) == 0); // leaf node was already visited, this must not happen!
						visitedLeafs.insert(nodeIndex);
					}
					else
					{
						// internal node
						BVHNode& node = nodes[nodeIndex];
						stack.push_back(node.LeftChildIndex);
						stack.push_back(node.RightChildIndex);
					}
				}
				for (uint i = 0; i < primitiveCount; ++i)
				{
					uint nodeIndex = leafNodeOffset + i;
					BVHNode& leaf = nodes[nodeIndex];
					assert(leaf.LeftChildIndex == 0 && leaf.RightChildIndex == 0); // a leaf must have no children
					assert(visitedLeafs.count(nodeIndex) > 0); // every leaf node must have been visited in the traversal above
				}

				// Validate flag buffer:
				readback_desc = bvhFlagBuffer.GetDesc();
				readback_desc.usage = USAGE_STAGING;
				readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
				readback_desc.bind_flags = 0;
				readback_desc.Flags = 0;
				GPUBuffer readback_flagBuffer;
				device->CreateBuffer(&readback_desc, nullptr, &readback_flagBuffer);
				vector<uint> flags(readback_desc.size / sizeof(uint));
				download_success = device->DownloadResource(&bvhFlagBuffer, &readback_flagBuffer, flags.data(), cmd);
				assert(download_success);
				for (auto& x : flags)
				{
					if (x > 2)
					{
						assert(0); // flagbuffer anomaly detected: node can't have more than two children (AABB propagation step)!
						break;
					}
				}
			}
		}
#endif // BVH_VALIDATE

	}

	void GPUBVH::Clear()
	{
		primitiveCapacity = 0;
	}

	namespace GPUBVH_Internal
	{
		void LoadShaders()
		{
			wi::renderer::LoadShader(ShaderStage::CS, computeShaders[CSTYPE_BVH_PRIMITIVES], "bvh_primitivesCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, computeShaders[CSTYPE_BVH_HIERARCHY], "bvh_hierarchyCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, computeShaders[CSTYPE_BVH_PROPAGATEAABB], "bvh_propagateaabbCS.cso");
		}
	}

	void GPUBVH::Initialize()
	{
		wi::Timer timer;

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { GPUBVH_Internal::LoadShaders(); });
		GPUBVH_Internal::LoadShaders();

		wilog("wi::GPUBVH Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}
}
