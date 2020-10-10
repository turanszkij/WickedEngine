#include "wiGPUBVH.h"
#include "wiScene.h"
#include "wiRenderer.h"
#include "ShaderInterop_BVH.h"
#include "wiProfiler.h"
#include "wiResourceManager.h"
#include "wiGPUSortLib.h"
#include "wiTextureHelper.h"
#include "wiBackLog.h"
#include "wiEvent.h"

//#define BVH_VALIDATE // slow but great for debug!
#ifdef BVH_VALIDATE
#include <set>
#endif // BVH_VALIDATE

using namespace std;
using namespace wiGraphics;
using namespace wiScene;
using namespace wiECS;

enum CSTYPES_BVH
{
	CSTYPE_BVH_PRIMITIVES,
	CSTYPE_BVH_HIERARCHY,
	CSTYPE_BVH_PROPAGATEAABB,
	CSTYPE_BVH_COUNT
};
static Shader computeShaders[CSTYPE_BVH_COUNT];
static GPUBuffer constantBuffer;


void wiGPUBVH::UpdateGlobalMaterialResources(const Scene& scene, CommandList cmd)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	using namespace wiRectPacker;

	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		const ObjectComponent& object = scene.objects[i];

		if (object.meshID != INVALID_ENTITY)
		{
			const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

			for (auto& subset : mesh.subsets)
			{
				const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);

				if (material.baseColorMap != nullptr)
				{
					sceneTextures.insert(material.baseColorMap);
				}
				if (material.surfaceMap != nullptr)
				{
					sceneTextures.insert(material.surfaceMap);
				}
				if (material.emissiveMap != nullptr)
				{
					sceneTextures.insert(material.emissiveMap);
				}
				if (material.normalMap != nullptr)
				{
					sceneTextures.insert(material.normalMap);
				}
			}
		}

	}

	bool repackAtlas = false;
	const int atlasWrapBorder = 1;
	for (auto res : sceneTextures)
	{
		if (res == nullptr)
		{
			continue;
		}

		if (storedTextures.find(res) == storedTextures.end())
		{
			// we need to pack this texture into the atlas
			rect_xywh newRect = rect_xywh(0, 0, res->texture->GetDesc().Width + atlasWrapBorder * 2, res->texture->GetDesc().Height + atlasWrapBorder * 2);
			storedTextures[res] = newRect;

			repackAtlas = true;
		}

	}

	if (repackAtlas)
	{
		vector<rect_xywh*> out_rects(storedTextures.size());
		int i = 0;
		for (auto& it : storedTextures)
		{
			out_rects[i] = &it.second;
			i++;
		}

		std::vector<bin> bins;
		if (pack(out_rects.data(), (int)storedTextures.size(), 16384, bins))
		{
			assert(bins.size() == 1 && "The regions won't fit into the texture!");

			TextureDesc desc;
			desc.Width = (uint32_t)bins[0].size.w;
			desc.Height = (uint32_t)bins[0].size.h;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.SampleCount = 1;
			desc.Usage = USAGE_DEFAULT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			device->CreateTexture(&desc, nullptr, &globalMaterialAtlas);
			device->SetName(&globalMaterialAtlas, "globalMaterialAtlas");

			for (auto& it : storedTextures)
			{
				wiRenderer::CopyTexture2D(globalMaterialAtlas, 0, it.second.x + atlasWrapBorder, it.second.y + atlasWrapBorder, *it.first->texture, 0, cmd, wiRenderer::BORDEREXPAND_WRAP);
			}
		}
		else
		{
			wiBackLog::post("Tracing atlas packing failed!");
		}
	}

	materialArray.clear();

	// Pre-gather scene properties:
	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		const ObjectComponent& object = scene.objects[i];

		if (object.meshID != INVALID_ENTITY)
		{
			const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

			for (auto& subset : mesh.subsets)
			{
				const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);
				ShaderMaterial global_material;
				material.WriteShaderMaterial(&global_material);

				// Add extended properties:
				const TextureDesc& desc = globalMaterialAtlas.GetDesc();

				if (material.baseColorMap != nullptr)
				{
					rect_xywh rect = storedTextures[material.baseColorMap];
					// eliminate border expansion:
					rect.x += atlasWrapBorder;
					rect.y += atlasWrapBorder;
					rect.w -= atlasWrapBorder * 2;
					rect.h -= atlasWrapBorder * 2;
					global_material.baseColorAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
						(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
				}

				if (material.surfaceMap != nullptr)
				{
					rect_xywh rect = storedTextures[material.surfaceMap];
					// eliminate border expansion:
					rect.x += atlasWrapBorder;
					rect.y += atlasWrapBorder;
					rect.w -= atlasWrapBorder * 2;
					rect.h -= atlasWrapBorder * 2;
					global_material.surfaceMapAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
						(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
				}

				if (material.emissiveMap != nullptr)
				{
					rect_xywh rect = storedTextures[material.emissiveMap];
					// eliminate border expansion:
					rect.x += atlasWrapBorder;
					rect.y += atlasWrapBorder;
					rect.w -= atlasWrapBorder * 2;
					rect.h -= atlasWrapBorder * 2;
					global_material.emissiveMapAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
						(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
				}

				if (material.normalMap != nullptr)
				{
					rect_xywh rect = storedTextures[material.normalMap];
					// eliminate border expansion:
					rect.x += atlasWrapBorder;
					rect.y += atlasWrapBorder;
					rect.w -= atlasWrapBorder * 2;
					rect.h -= atlasWrapBorder * 2;
					global_material.normalMapAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
						(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
				}

				materialArray.push_back(global_material);
			}
		}
	}

	if (materialArray.empty())
	{
		return;
	}

	if (globalMaterialBuffer.GetDesc().ByteWidth != sizeof(ShaderMaterial) * materialArray.size())
	{
		GPUBufferDesc desc;

		desc.BindFlags = BIND_SHADER_RESOURCE;
		desc.StructureByteStride = sizeof(ShaderMaterial);
		desc.ByteWidth = desc.StructureByteStride * (uint32_t)materialArray.size();
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;

		device->CreateBuffer(&desc, nullptr, &globalMaterialBuffer);
	}
	device->UpdateBuffer(&globalMaterialBuffer, materialArray.data(), cmd, sizeof(ShaderMaterial) * (int)materialArray.size());

}

void wiGPUBVH::Build(const Scene& scene, CommandList cmd)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	if (!constantBuffer.IsValid())
	{
		GPUBufferDesc bd;
		bd.Usage = USAGE_DYNAMIC;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.ByteWidth = sizeof(BVHCB);

		device->CreateBuffer(&bd, nullptr, &constantBuffer);
		device->SetName(&constantBuffer, "BVHGeneratorCB");
	}

	if (!primitiveCounterBuffer.IsValid())
	{
		GPUBufferDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &primitiveCounterBuffer);
		device->SetName(&primitiveCounterBuffer, "primitiveCounterBuffer"); 
	}

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

	if (totalTriangles > primitiveCapacity)
	{
		primitiveCapacity = std::max(2u, totalTriangles);

		GPUBufferDesc desc;

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHNode);
		desc.ByteWidth = desc.StructureByteStride * primitiveCapacity * 2;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &bvhNodeBuffer);
		device->SetName(&bvhNodeBuffer, "BVHNodeBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride * primitiveCapacity * 2;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &bvhParentBuffer);
		device->SetName(&bvhParentBuffer, "BVHParentBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride * (((primitiveCapacity - 1) + 31) / 32); // bitfield for internal nodes
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &bvhFlagBuffer);
		device->SetName(&bvhFlagBuffer, "BVHFlagBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride * primitiveCapacity;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &primitiveIDBuffer);
		device->SetName(&primitiveIDBuffer, "primitiveIDBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHPrimitive);
		desc.ByteWidth = desc.StructureByteStride * primitiveCapacity;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &primitiveBuffer);
		device->SetName(&primitiveBuffer, "primitiveBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHPrimitiveData);
		desc.ByteWidth = desc.StructureByteStride * primitiveCapacity;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &primitiveDataBuffer);
		device->SetName(&primitiveDataBuffer, "primitiveDataBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.ByteWidth = desc.StructureByteStride * primitiveCapacity;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		desc.StructureByteStride = sizeof(float); // morton buffer is float because sorting must be done and gpu sort operates on floats for now!
		device->CreateBuffer(&desc, nullptr, &primitiveMortonBuffer);
		device->SetName(&primitiveMortonBuffer, "primitiveMortonBuffer");
	}


	auto range = wiProfiler::BeginRangeGPU("BVH Rebuild", cmd);

	UpdateGlobalMaterialResources(scene, cmd);

	primitiveCount = 0;
	uint32_t materialCount = 0;

	device->EventBegin("BVH - Primitive Builder", cmd);
	{
		device->BindComputeShader(&computeShaders[CSTYPE_BVH_PRIMITIVES], cmd);
		GPUResource* uavs[] = {
			&primitiveIDBuffer,
			&primitiveBuffer,
			&primitiveDataBuffer,
			&primitiveMortonBuffer,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			const ObjectComponent& object = scene.objects[i];

			if (object.meshID != INVALID_ENTITY)
			{
				const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

				BVHCB cb;
				cb.xBVHWorld = object.transform_index >= 0 ? scene.transforms[object.transform_index].world : IDENTITYMATRIX;
				cb.xBVHInstanceColor = object.color;
				cb.xBVHMaterialOffset = materialCount;
				cb.xBVHMeshTriangleOffset = primitiveCount;
				cb.xBVHMeshTriangleCount = (uint)mesh.indices.size() / 3;
				cb.xBVHMeshVertexPOSStride = sizeof(MeshComponent::Vertex_POS);

				device->UpdateBuffer(&constantBuffer, &cb, cmd);

				primitiveCount += cb.xBVHMeshTriangleCount;

				device->BindConstantBuffer(CS, &constantBuffer, CB_GETBINDSLOT(BVHCB), cmd);

				const GPUResource* res[] = {
					&globalMaterialBuffer,
					&mesh.indexBuffer,
					mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
					&mesh.vertexBuffer_UV0,
					&mesh.vertexBuffer_UV1,
					&mesh.vertexBuffer_COL,
					&mesh.vertexBuffer_SUB,
				};
				device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

				device->Dispatch((cb.xBVHMeshTriangleCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE, 1, 1, cmd);

				materialCount += (uint32_t)mesh.subsets.size();
			}
		}

		GPUBarrier barriers[] = {
			GPUBarrier::Memory()
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}
	device->UpdateBuffer(&primitiveCounterBuffer, &primitiveCount, cmd);
	device->EventEnd(cmd);

	device->EventBegin("BVH - Sort Primitive Mortons", cmd);
	wiGPUSortLib::Sort(primitiveCount, primitiveMortonBuffer, primitiveCounterBuffer, 0, primitiveIDBuffer, cmd);
	device->EventEnd(cmd);

	device->EventBegin("BVH - Build Hierarchy", cmd);
	{
		device->BindComputeShader(&computeShaders[CSTYPE_BVH_HIERARCHY], cmd);
		GPUResource* uavs[] = {
			&bvhNodeBuffer,
			&bvhParentBuffer,
			&bvhFlagBuffer
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		GPUResource* res[] = {
			&primitiveCounterBuffer,
			&primitiveIDBuffer,
			&primitiveMortonBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		device->Dispatch((primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory()
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}
	device->EventEnd(cmd);

	device->EventBegin("BVH - Propagate AABB", cmd);
	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory()
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->BindComputeShader(&computeShaders[CSTYPE_BVH_PROPAGATEAABB], cmd);
		GPUResource* uavs[] = {
			&bvhNodeBuffer,
			&bvhFlagBuffer,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		GPUResource* res[] = {
			&primitiveCounterBuffer,
			&primitiveIDBuffer,
			&primitiveBuffer,
			&bvhParentBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		device->Dispatch((primitiveCount + BVH_BUILDER_GROUPSIZE - 1) / BVH_BUILDER_GROUPSIZE, 1, 1, cmd);

		device->Barrier(barriers, arraysize(barriers), cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}
	device->EventEnd(cmd);

	wiProfiler::EndRange(range); // BVH rebuild

#ifdef BVH_VALIDATE

	GPUBufferDesc readback_desc;
	bool download_success;

	// Download primitive count:
	readback_desc = primitiveCounterBuffer.GetDesc();
	readback_desc.Usage = USAGE_STAGING;
	readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
	readback_desc.BindFlags = 0;
	readback_desc.MiscFlags = 0;
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
		readback_desc.Usage = USAGE_STAGING;
		readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
		readback_desc.BindFlags = 0;
		readback_desc.MiscFlags = 0;
		GPUBuffer readback_nodeBuffer;
		device->CreateBuffer(&readback_desc, nullptr, &readback_nodeBuffer);
		vector<BVHNode> nodes(readback_desc.ByteWidth / sizeof(BVHNode));
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
		readback_desc.Usage = USAGE_STAGING;
		readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
		readback_desc.BindFlags = 0;
		readback_desc.MiscFlags = 0;
		GPUBuffer readback_flagBuffer;
		device->CreateBuffer(&readback_desc, nullptr, &readback_flagBuffer);
		vector<uint> flags(readback_desc.ByteWidth / sizeof(uint));
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

#endif // BVH_VALIDATE

}
void wiGPUBVH::Bind(SHADERSTAGE stage, CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const GPUResource* res[] = {
		&globalMaterialBuffer,
		(globalMaterialAtlas.IsValid() ? &globalMaterialAtlas : wiTextureHelper::getWhite()),
		&primitiveCounterBuffer,
		&primitiveBuffer,
		&primitiveDataBuffer,
		&bvhNodeBuffer,
	};
	device->BindResources(stage, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);
}

void wiGPUBVH::Clear()
{
	primitiveCapacity = 0;
	primitiveCount = 0;
	materialArray.clear();
	storedTextures.clear();
	sceneTextures.clear();
}

namespace wiGPUBVH_Internal
{
	void LoadShaders()
	{
		string SHADERPATH = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(CS, computeShaders[CSTYPE_BVH_PRIMITIVES], "bvh_primitivesCS.cso");
		wiRenderer::LoadShader(CS, computeShaders[CSTYPE_BVH_HIERARCHY], "bvh_hierarchyCS.cso");
		wiRenderer::LoadShader(CS, computeShaders[CSTYPE_BVH_PROPAGATEAABB], "bvh_propagateaabbCS.cso");
	}
}

void wiGPUBVH::Initialize()
{
	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { wiGPUBVH_Internal::LoadShaders(); });
	wiGPUBVH_Internal::LoadShaders();
}
