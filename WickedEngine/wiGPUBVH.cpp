#include "wiGPUBVH.h"
#include "wiSceneSystem.h"
#include "wiRenderer.h"
#include "ShaderInterop_BVH.h"
#include "wiProfiler.h"
#include "wiResourceManager.h"
#include "wiGPUSortLib.h"
#include "wiTextureHelper.h"
#include "wiBackLog.h"

using namespace std;
using namespace wiGraphics;
using namespace wiSceneSystem;
using namespace wiECS;

enum CSTYPES_BVH
{
	CSTYPE_BVH_RESET,
	CSTYPE_BVH_CLASSIFICATION,
	CSTYPE_BVH_KICKJOBS,
	CSTYPE_BVH_CLUSTERPROCESSOR,
	CSTYPE_BVH_HIERARCHY,
	CSTYPE_BVH_PROPAGATEAABB,
	CSTYPE_BVH_COUNT
};
static const ComputeShader* computeShaders[CSTYPE_BVH_COUNT] = {};
static ComputePSO CPSO[CSTYPE_BVH_COUNT];

static GPUBuffer constantBuffer;


void wiGPUBVH::UpdateGlobalMaterialResources(const Scene& scene, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	using namespace wiRectPacker;

	if (sceneTextures.empty())
	{
		sceneTextures.insert(wiTextureHelper::getWhite());
		sceneTextures.insert(wiTextureHelper::getNormalMapDefault());
	}

	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		const ObjectComponent& object = scene.objects[i];

		if (object.meshID != INVALID_ENTITY)
		{
			const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

			for (auto& subset : mesh.subsets)
			{
				const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);

				sceneTextures.insert(material.GetBaseColorMap());
				sceneTextures.insert(material.GetSurfaceMap());
				sceneTextures.insert(material.GetEmissiveMap());
			}
		}

	}

	bool repackAtlas = false;
	const int atlasWrapBorder = 1;
	for (const Texture2D* tex : sceneTextures)
	{
		if (tex == nullptr)
		{
			continue;
		}

		if (storedTextures.find(tex) == storedTextures.end())
		{
			// we need to pack this texture into the atlas
			rect_xywh newRect = rect_xywh(0, 0, tex->GetDesc().Width + atlasWrapBorder * 2, tex->GetDesc().Height + atlasWrapBorder * 2);
			storedTextures[tex] = newRect;

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
			desc.Width = (UINT)bins[0].size.w;
			desc.Height = (UINT)bins[0].size.h;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = USAGE_DEFAULT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			device->CreateTexture2D(&desc, nullptr, &globalMaterialAtlas);
			device->SetName(&globalMaterialAtlas, "globalMaterialAtlas");

			for (auto& it : storedTextures)
			{
				wiRenderer::CopyTexture2D(&globalMaterialAtlas, 0, it.second.x + atlasWrapBorder, it.second.y + atlasWrapBorder, it.first, 0, threadID, wiRenderer::BORDEREXPAND_WRAP);
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

				TracedRenderingMaterial global_material;

				// Copy base params:
				global_material.baseColor = material.baseColor;
				global_material.emissiveColor = material.emissiveColor;
				global_material.texMulAdd = material.texMulAdd;
				global_material.roughness = material.roughness;
				global_material.reflectance = material.reflectance;
				global_material.metalness = material.metalness;
				global_material.refractionIndex = material.refractionIndex;
				global_material.subsurfaceScattering = material.subsurfaceScattering;
				global_material.normalMapStrength = material.normalMapStrength;
				global_material.normalMapFlip = (material._flags & MaterialComponent::FLIP_NORMALMAP ? -1.0f : 1.0f);
				global_material.parallaxOcclusionMapping = material.parallaxOcclusionMapping;
				global_material.displacementMapping = material.displacementMapping;
				global_material.useVertexColors = material.IsUsingVertexColors() ? 1 : 0;
				global_material.uvset_baseColorMap = material.uvset_baseColorMap;
				global_material.uvset_surfaceMap = material.uvset_surfaceMap;
				global_material.uvset_normalMap = material.uvset_normalMap;
				global_material.uvset_displacementMap = material.uvset_displacementMap;
				global_material.uvset_emissiveMap = material.uvset_emissiveMap;
				global_material.uvset_occlusionMap = material.uvset_occlusionMap;
				global_material.specularGlossinessWorkflow = material.IsUsingSpecularGlossinessWorkflow() ? 1 : 0;
				global_material.occlusion_primary = material.IsOcclusionEnabled_Primary() ? 1 : 0;
				global_material.occlusion_secondary = material.IsOcclusionEnabled_Secondary() ? 1 : 0;

				// Add extended properties:
				const TextureDesc& desc = globalMaterialAtlas.GetDesc();
				rect_xywh rect;


				if (material.GetBaseColorMap() != nullptr)
				{
					rect = storedTextures[material.GetBaseColorMap()];
				}
				else
				{
					rect = storedTextures[wiTextureHelper::getWhite()];
				}
				// eliminate border expansion:
				rect.x += atlasWrapBorder;
				rect.y += atlasWrapBorder;
				rect.w -= atlasWrapBorder * 2;
				rect.h -= atlasWrapBorder * 2;
				global_material.baseColorAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
					(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);



				if (material.GetSurfaceMap() != nullptr)
				{
					rect = storedTextures[material.GetSurfaceMap()];
				}
				else
				{
					rect = storedTextures[wiTextureHelper::getWhite()];
				}
				// eliminate border expansion:
				rect.x += atlasWrapBorder;
				rect.y += atlasWrapBorder;
				rect.w -= atlasWrapBorder * 2;
				rect.h -= atlasWrapBorder * 2;
				global_material.surfaceMapAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
					(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);



				if (material.GetEmissiveMap() != nullptr)
				{
					rect = storedTextures[material.GetEmissiveMap()];
				}
				else
				{

					rect = storedTextures[wiTextureHelper::getWhite()];
				}
				// eliminate border expansion:
				rect.x += atlasWrapBorder;
				rect.y += atlasWrapBorder;
				rect.w -= atlasWrapBorder * 2;
				rect.h -= atlasWrapBorder * 2;
				global_material.emissiveMapAtlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height,
					(float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);

				materialArray.push_back(global_material);
			}
		}
	}

	if (materialArray.empty())
	{
		return;
	}

	if (globalMaterialBuffer.GetDesc().ByteWidth != sizeof(TracedRenderingMaterial) * materialArray.size())
	{
		GPUBufferDesc desc;
		HRESULT hr;

		desc.BindFlags = BIND_SHADER_RESOURCE;
		desc.StructureByteStride = sizeof(TracedRenderingMaterial);
		desc.ByteWidth = desc.StructureByteStride * (UINT)materialArray.size();
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;

		hr = device->CreateBuffer(&desc, nullptr, &globalMaterialBuffer);
		assert(SUCCEEDED(hr));
	}
	device->UpdateBuffer(&globalMaterialBuffer, materialArray.data(), threadID, sizeof(TracedRenderingMaterial) * (int)materialArray.size());

}

//#define BVH_VALIDATE // slow but great for debug!
void wiGPUBVH::Build(const Scene& scene, GRAPHICSTHREAD threadID)
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

	if (totalTriangles > maxTriangleCount)
	{
		maxTriangleCount = totalTriangles;
		maxClusterCount = maxTriangleCount; // todo: cluster / triangle capacity

		GPUBufferDesc desc;
		HRESULT hr;

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHNode);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount * 2;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &bvhNodeBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&bvhNodeBuffer, "BVHNodeBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHAABB);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount * 2;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &bvhAABBBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&bvhAABBBuffer, "BVHAABBBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride * (maxClusterCount - 1); // only for internal nodes
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &bvhFlagBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&bvhFlagBuffer, "BVHFlagBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHMeshTriangle);
		desc.ByteWidth = desc.StructureByteStride * maxTriangleCount;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &triangleBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&triangleBuffer, "BVHTriangleBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &clusterCounterBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&clusterCounterBuffer, "BVHClusterCounterBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &clusterIndexBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&clusterIndexBuffer, "BVHClusterIndexBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &clusterMortonBuffer);
		hr = device->CreateBuffer(&desc, nullptr, &clusterSortedMortonBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&clusterMortonBuffer, "BVHClusterMortonBuffer");
		device->SetName(&clusterSortedMortonBuffer, "BVHSortedClusterMortonBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint2);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &clusterOffsetBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&clusterOffsetBuffer, "BVHClusterOffsetBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(BVHAABB);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &clusterAABBBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&clusterAABBBuffer, "BVHClusterAABBBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(ClusterCone);
		desc.ByteWidth = desc.StructureByteStride * maxClusterCount;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &clusterConeBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&clusterConeBuffer, "BVHClusterConeBuffer");
	}

	static GPUBuffer* indirectBuffer = nullptr; // GPU job kicks
	if (indirectBuffer == nullptr)
	{
		GPUBufferDesc desc;
		HRESULT hr;

		SAFE_DELETE(indirectBuffer);
		indirectBuffer = new GPUBuffer;

		desc.BindFlags = BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(IndirectDispatchArgs) * 2;
		desc.ByteWidth = desc.StructureByteStride;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_DRAWINDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, indirectBuffer);
		assert(SUCCEEDED(hr));
	}


	wiProfiler::BeginRange("BVH Rebuild", wiProfiler::DOMAIN_GPU, threadID);

	UpdateGlobalMaterialResources(scene, threadID);

	device->EventBegin("BVH - Reset", threadID);
	{
		device->BindComputePSO(&CPSO[CSTYPE_BVH_RESET], threadID);

		GPUResource* uavs[] = {
			&clusterCounterBuffer,
			&bvhNodeBuffer,
			&bvhAABBBuffer,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		device->Dispatch(1, 1, 1, threadID);
	}
	device->EventEnd(threadID);


	uint32_t triangleCount = 0;
	uint32_t materialCount = 0;

	device->EventBegin("BVH - Classification", threadID);
	{
		device->BindComputePSO(&CPSO[CSTYPE_BVH_CLASSIFICATION], threadID);
		GPUResource* uavs[] = {
			&triangleBuffer,
			&clusterCounterBuffer,
			&clusterIndexBuffer,
			&clusterMortonBuffer,
			&clusterOffsetBuffer,
			&clusterAABBBuffer,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			const ObjectComponent& object = scene.objects[i];

			if (object.meshID != INVALID_ENTITY)
			{
				const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

				BVHCB cb;
				cb.xTraceBVHWorld = object.transform_index >= 0 ? scene.transforms[object.transform_index].world : IDENTITYMATRIX;
				cb.xTraceBVHInstanceColor = object.color;
				cb.xTraceBVHMaterialOffset = materialCount;
				cb.xTraceBVHMeshTriangleOffset = triangleCount;
				cb.xTraceBVHMeshTriangleCount = (uint)mesh.indices.size() / 3;
				cb.xTraceBVHMeshVertexPOSStride = sizeof(MeshComponent::Vertex_POS);

				device->UpdateBuffer(&constantBuffer, &cb, threadID);

				triangleCount += cb.xTraceBVHMeshTriangleCount;

				device->BindConstantBuffer(CS, &constantBuffer, CB_GETBINDSLOT(BVHCB), threadID);

				GPUResource* res[] = {
					&globalMaterialBuffer,
					mesh.indexBuffer.get(),
					mesh.vertexBuffer_POS.get(),
					mesh.vertexBuffer_UV0.get(),
					mesh.vertexBuffer_UV1.get(),
					mesh.vertexBuffer_COL.get(),
				};
				device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);

				device->Dispatch((UINT)ceilf((float)cb.xTraceBVHMeshTriangleCount / (float)BVH_CLASSIFICATION_GROUPSIZE), 1, 1, threadID);

				for (auto& subset : mesh.subsets)
				{
					materialCount++;
				}
			}
		}

		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
	}
	device->EventEnd(threadID);


	device->EventBegin("BVH - Sort Cluster Mortons", threadID);
	wiGPUSortLib::Sort(maxClusterCount, clusterMortonBuffer, clusterCounterBuffer, 0, clusterIndexBuffer, threadID);
	device->EventEnd(threadID);

	device->EventBegin("BVH - Kick Jobs", threadID);
	{
		device->BindComputePSO(&CPSO[CSTYPE_BVH_KICKJOBS], threadID);
		GPUResource* uavs[] = {
			indirectBuffer,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		GPUResource* res[] = {
			&clusterCounterBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);

		device->Dispatch(1, 1, 1, threadID);

		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
	}
	device->EventEnd(threadID);

	device->EventBegin("BVH - Cluster Processor", threadID);
	{
		device->BindComputePSO(&CPSO[CSTYPE_BVH_CLUSTERPROCESSOR], threadID);
		GPUResource* uavs[] = {
			&clusterSortedMortonBuffer,
			&clusterConeBuffer,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		GPUResource* res[] = {
			&clusterCounterBuffer,
			&clusterIndexBuffer,
			&clusterMortonBuffer,
			&clusterOffsetBuffer,
			&clusterAABBBuffer,
			&triangleBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);

		device->DispatchIndirect(indirectBuffer, ARGUMENTBUFFER_OFFSET_CLUSTERPROCESSOR, threadID);


		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
	}
	device->EventEnd(threadID);

	device->EventBegin("BVH - Build Hierarchy", threadID);
	{
		device->BindComputePSO(&CPSO[CSTYPE_BVH_HIERARCHY], threadID);
		GPUResource* uavs[] = {
			&bvhNodeBuffer,
			&bvhFlagBuffer,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		GPUResource* res[] = {
			&clusterCounterBuffer,
			&clusterSortedMortonBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);

		device->DispatchIndirect(indirectBuffer, ARGUMENTBUFFER_OFFSET_HIERARCHY, threadID);


		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
	}
	device->EventEnd(threadID);

	device->EventBegin("BVH - Propagate AABB", threadID);
	{
		device->BindComputePSO(&CPSO[CSTYPE_BVH_PROPAGATEAABB], threadID);
		GPUResource* uavs[] = {
			&bvhAABBBuffer,
			&bvhFlagBuffer,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		GPUResource* res[] = {
			&clusterCounterBuffer,
			&clusterIndexBuffer,
			&clusterAABBBuffer,
			&bvhNodeBuffer,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);

		device->DispatchIndirect(indirectBuffer, ARGUMENTBUFFER_OFFSET_CLUSTERPROCESSOR, threadID);


		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
	}
	device->EventEnd(threadID);

#ifdef BVH_VALIDATE

	GPUBufferDesc readback_desc;
	bool download_success;

	// Download cluster count:
	readback_desc = clusterCounterBuffer.GetDesc();
	readback_desc.Usage = USAGE_STAGING;
	readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
	readback_desc.BindFlags = 0;
	readback_desc.MiscFlags = 0;
	GPUBuffer readback_clusterCounterBuffer;
	device->CreateBuffer(&readback_desc, nullptr, &readback_clusterCounterBuffer);
	uint clusterCount;
	download_success = device->DownloadResource(&clusterCounterBuffer, &readback_clusterCounterBuffer, &clusterCount, threadID);
	assert(download_success);

	if (clusterCount > 0)
	{
		const uint leafNodeOffset = clusterCount - 1;

		// Validate node buffer:
		readback_desc = bvhNodeBuffer.GetDesc();
		readback_desc.Usage = USAGE_STAGING;
		readback_desc.CPUAccessFlags = CPU_ACCESS_READ;
		readback_desc.BindFlags = 0;
		readback_desc.MiscFlags = 0;
		GPUBuffer readback_nodeBuffer;
		device->CreateBuffer(&readback_desc, nullptr, &readback_nodeBuffer);
		vector<BVHNode> nodes(readback_desc.ByteWidth / sizeof(BVHNode));
		download_success = device->DownloadResource(&bvhNodeBuffer, &readback_nodeBuffer, nodes.data(), threadID);
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
		for (uint i = 0; i < clusterCount; ++i)
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
		download_success = device->DownloadResource(&bvhFlagBuffer, &readback_flagBuffer, flags.data(), threadID);
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


	wiProfiler::EndRange(threadID); // BVH rebuild
}
void wiGPUBVH::Bind(SHADERSTAGE stage, GRAPHICSTHREAD threadID) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const GPUResource* res[] = {
		&globalMaterialBuffer,
		(globalMaterialAtlas.IsValid() ? &globalMaterialAtlas : wiTextureHelper::getWhite()),
		&triangleBuffer,
		&clusterCounterBuffer,
		&clusterIndexBuffer,
		&clusterOffsetBuffer,
		&clusterConeBuffer,
		&bvhNodeBuffer,
		&bvhAABBBuffer,
	};
	device->BindResources(stage, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);
}


void wiGPUBVH::LoadShaders()
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	string SHADERPATH = wiRenderer::GetShaderPath();

	computeShaders[CSTYPE_BVH_RESET] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "bvh_resetCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_BVH_CLASSIFICATION] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "bvh_classificationCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_BVH_KICKJOBS] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "bvh_kickjobsCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_BVH_CLUSTERPROCESSOR] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "bvh_clusterprocessorCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_BVH_HIERARCHY] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "bvh_hierarchyCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_BVH_PROPAGATEAABB] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "bvh_propagateaabbCS.cso", wiResourceManager::COMPUTESHADER));


	for (int i = 0; i < CSTYPE_BVH_COUNT; ++i)
	{
		ComputePSODesc desc;
		desc.cs = computeShaders[i];
		device->CreateComputePSO(&desc, &CPSO[i]);
	}
}
