#include "wiVoxelGrid.h"
#include "shaders/ShaderInterop.h"
#include "wiEventHandler.h"
#include "wiRenderer.h"

#include <intrin.h>

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi
{
	void VoxelGrid::init(uint32_t dimX, uint32_t dimY, uint32_t dimZ)
	{
		resolution.x = std::max(4u, dimX);
		resolution.y = std::max(4u, dimY);
		resolution.z = std::max(4u, dimZ);
		resolution_div4.x = (resolution.x + 3u) / 4u;
		resolution_div4.y = (resolution.y + 3u) / 4u;
		resolution_div4.z = (resolution.z + 3u) / 4u;
		resolution_rcp.x = 1.0f / resolution.x;
		resolution_rcp.y = 1.0f / resolution.y;
		resolution_rcp.z = 1.0f / resolution.z;
		voxels.clear();
		voxels.resize(resolution_div4.x * resolution_div4.y * resolution_div4.z);
	}
	void VoxelGrid::cleardata()
	{
		std::fill(voxels.begin(), voxels.end(), 0ull);
	}

	// 3D array index to flattened 1D array index
	inline uint flatten3D(uint3 coord, uint3 dim)
	{
		return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
	}
	// flattened array index to 3D array index
	inline uint3 unflatten3D(uint idx, uint3 dim)
	{
		const uint z = idx / (dim.x * dim.y);
		idx -= (z * dim.x * dim.y);
		const uint y = idx / dim.x;
		const uint x = idx % dim.x;
		return  uint3(x, y, z);
	}

	void VoxelGrid::inject_triangle(XMVECTOR A, XMVECTOR B, XMVECTOR C, bool subtract)
	{
		const XMVECTOR CENTER = XMLoadFloat3(&center);
		const XMVECTOR RESOLUTION = XMLoadUInt3(&resolution);
		const XMVECTOR RESOLUTION_RCP = XMLoadFloat3(&resolution_rcp);
		const XMVECTOR VOXELSIZE_RCP = XMLoadFloat3(&voxelSize_rcp);

		// world -> uvw space:
		A = world_to_uvw(A, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);
		B = world_to_uvw(B, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);
		C = world_to_uvw(C, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);

		// pixel space:
		A *= RESOLUTION;
		B *= RESOLUTION;
		C *= RESOLUTION;

		// Degenerate triangle check:
		XMVECTOR Normal = XMVector3Cross(XMVectorSubtract(B, A), XMVectorSubtract(C, A));
		if (XMVector3Equal(Normal, XMVectorZero()))
			return;

		XMVECTOR MIN = XMVectorMin(A, XMVectorMin(B, C));
		XMVECTOR MAX = XMVectorMax(A, XMVectorMax(B, C));

		MIN = XMVectorFloor(MIN);
		MAX = XMVectorCeiling(MAX + XMVectorSet(0.5f, 0.5f, 0.5f, 0));

		MIN = XMVectorMax(MIN, XMVectorZero());
		MAX = XMVectorMin(MAX, RESOLUTION);

		XMUINT3 mini, maxi;
		XMStoreUInt3(&mini, MIN);
		XMStoreUInt3(&maxi, MAX);

		volatile long long* data = (volatile long long*)voxels.data();
		for (uint32_t x = mini.x; x < maxi.x; ++x)
		{
			for (uint32_t y = mini.y; y < maxi.y; ++y)
			{
				for (uint32_t z = mini.z; z < maxi.z; ++z)
				{
					const DirectX::BoundingBox voxel_aabb(XMFLOAT3(x + 0.5f, y + 0.5f, z + 0.5f), XMFLOAT3(0.5f, 0.5f, 0.5f));
					if (voxel_aabb.Intersects(A, B, C))
					{
						const uint3 coord = uint3(x / 4u, y / 4u, z / 4u);
						const uint3 sub_coord = uint3(x % 4u, y % 4u, z % 4u);
						const uint32_t idx = flatten3D(coord, resolution_div4);
						const uint32_t bit = flatten3D(sub_coord, uint3(4, 4, 4));
						const uint64_t mask = 1ull << bit;
						if (subtract)
						{
							_InterlockedAnd64(data + idx, ~mask);
						}
						else
						{
							_InterlockedOr64(data + idx, mask);
						}
					}
				}
			}
		}
	}

	bool VoxelGrid::check_voxel(XMUINT3 coord) const
	{
		uint3 sub_coord;
		sub_coord.x = coord.x % 4u;
		sub_coord.y = coord.y % 4u;
		sub_coord.z = coord.z % 4u;
		coord.x /= 4u;
		coord.y /= 4u;
		coord.z /= 4u;
		const uint idx = flatten3D(coord, resolution_div4);
		const uint bit = flatten3D(sub_coord, uint3(4, 4, 4));
		const uint64_t mask = 1ull << bit;
		return (voxels[idx] & mask) != 0ull;
	}

	void VoxelGrid::set_voxelsize(float size)
	{
		set_voxelsize(XMFLOAT3(size, size, size));
	}
	void VoxelGrid::set_voxelsize(const XMFLOAT3& size)
	{
		voxelSize = size;
		voxelSize_rcp.x = 1.0f / voxelSize.x;
		voxelSize_rcp.y = 1.0f / voxelSize.y;
		voxelSize_rcp.z = 1.0f / voxelSize.z;
	}

	wi::primitive::AABB VoxelGrid::get_aabb() const
	{
		AABB aabb;
		aabb.createFromHalfWidth(center, XMFLOAT3(resolution.x * voxelSize.x, resolution.y * voxelSize.y, resolution.z * voxelSize.z));
		return aabb;
	}
	void VoxelGrid::from_aabb(const wi::primitive::AABB& aabb)
	{
		center = aabb.getCenter();
		XMFLOAT3 halfwidth = aabb.getHalfWidth();
		set_voxelsize(XMFLOAT3(halfwidth.x / resolution.x, halfwidth.y / resolution.y, halfwidth.z / resolution.z));
	}

	PipelineState pso_solidpart;
	static void LoadShaders()
	{
		PipelineStateDesc desc;

		desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
		desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
		desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
		desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHREAD);
		desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_FRONT);
		desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_ADDITIVE);
		desc.pt = PrimitiveTopology::TRIANGLELIST;

		GraphicsDevice* device = GetDevice();
		device->CreatePipelineState(&desc, &pso_solidpart);
	}

	void VoxelGrid::debugdraw(const XMFLOAT4X4& ViewProjection, CommandList cmd) const
	{
		// add box renderer for whole volume:
		XMFLOAT4X4 boxmat;
		XMStoreFloat4x4(&boxmat, get_aabb().getAsBoxMatrix());
		wi::renderer::DrawBox(boxmat, XMFLOAT4(1, 1, 0.2f, 1));

		// Add a cube for every filled voxel below:
		uint32_t numVoxels = 0;
		for (auto& x : voxels)
		{
			if (x != 0)
				numVoxels += (uint32_t)__popcnt64(x);
		}
		if (numVoxels == 0)
			return;

		static bool shaders_loaded = false;
		if (!shaders_loaded)
		{
			shaders_loaded = true;
			static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
			LoadShaders();
		}

		struct Vertex
		{
			XMFLOAT4 position;
			XMFLOAT4 color;
		};
		static constexpr Vertex cubeVerts[] = {
			{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
			{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
		};

		GraphicsDevice* device = GetDevice();

		auto mem = device->AllocateGPU(sizeof(cubeVerts) * numVoxels, cmd);

		const XMVECTOR CENTER = XMLoadFloat3(&center);
		const XMVECTOR RESOLUTION = XMLoadUInt3(&resolution);
		const XMVECTOR RESOLUTION_RCP = XMLoadFloat3(&resolution_rcp);
		const XMVECTOR VOXELSIZE = XMLoadFloat3(&voxelSize);
		const XMVECTOR VOXELSIZE_RCP = XMLoadFloat3(&voxelSize_rcp);

		size_t dst_offset = 0;
		for (size_t i = 0; i < voxels.size(); ++i)
		{
			uint64_t voxel_bits = voxels[i];
			if (voxel_bits == 0)
				continue;
			const uint3 coord = unflatten3D(uint(i), resolution_div4);
			unsigned long bit_index;
			while (_BitScanReverse64(&bit_index, voxel_bits))
			{
				voxel_bits ^= 1ull << bit_index; // remove current bit
				const uint3 sub_coord = unflatten3D(bit_index, uint3(4, 4, 4));
				XMVECTOR uvw = XMVectorSet(coord.x * 4 + sub_coord.x + 0.5f, coord.y * 4 + sub_coord.y + 0.5f, coord.z * 4 + sub_coord.z + 0.5f, 1) * RESOLUTION_RCP;
				XMVECTOR P = uvw_to_world(uvw, CENTER, RESOLUTION, VOXELSIZE);
				Vertex verts[arraysize(cubeVerts)];
				std::memcpy(verts, cubeVerts, sizeof(cubeVerts));
				for (auto& v : verts)
				{
					XMVECTOR C = XMLoadFloat4(&v.position);
					C *= VOXELSIZE;
					C += P;
					C = XMVectorSetW(C, 1);
					XMStoreFloat4(&v.position, C);
				}
				std::memcpy((uint8_t*)mem.data + dst_offset, verts, sizeof(verts));
				dst_offset += sizeof(verts);
			}
		}

		device->EventBegin("VoxelGrid::debugdraw", cmd);
		device->BindPipelineState(&pso_solidpart, cmd);

		const GPUBuffer* vbs[] = {
			&mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		MiscCB sb;
		sb.g_xTransform = ViewProjection;
		sb.g_xColor = XMFLOAT4(0.4f, 1, 0.2f, 0.1f);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);

		device->Draw(arraysize(cubeVerts) * numVoxels, 0, cmd);

		device->EventEnd(cmd);
	}

}
