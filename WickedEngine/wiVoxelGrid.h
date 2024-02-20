#pragma once
#include "CommonInclude.h"
#include "wiVector.h"
#include "wiMath.h"
#include "wiPrimitive.h"
#include "wiGraphicsDevice.h"

namespace wi
{
	struct VoxelGrid
	{
		XMUINT3 resolution = XMUINT3(0, 0, 0);
		XMUINT3 resolution_div4 = XMUINT3(0, 0, 0);
		XMFLOAT3 resolution_rcp = XMFLOAT3(0, 0, 0);
		wi::vector<uint64_t> voxels; // 1 array element stores 4 * 4 * 4 = 64 voxels

		XMFLOAT3 center = XMFLOAT3(0, 0, 0);
		XMFLOAT3 voxelSize = XMFLOAT3(0.25f, 0.25f, 0.25f);
		XMFLOAT3 voxelSize_rcp = XMFLOAT3(1.0f / 0.25f, 1.0f / 0.25f, 1.0f / 0.25f);

		XMFLOAT4 debug_color = XMFLOAT4(0.4f, 1, 0.2f, 0.1f); // color of voxels in debug
		XMFLOAT4 debug_color_extent = XMFLOAT4(1, 1, 0.2f, 1); // color of extent box in debug

		void init(uint32_t dimX, uint32_t dimY, uint32_t dimZ);
		void cleardata();
		void inject_triangle(XMVECTOR A, XMVECTOR B, XMVECTOR C, bool subtract = false);
		XMUINT3 world_to_coord(const XMFLOAT3& worldpos) const;
		XMFLOAT3 coord_to_world(const XMUINT3& coord) const;
		bool check_voxel(XMUINT3 coord) const;
		bool check_voxel(const XMFLOAT3& worldpos) const;
		bool is_coord_valid(const XMUINT3& coord) const;
		void set_voxel(XMUINT3 coord, bool value);
		void set_voxel(const XMFLOAT3& worldpos, bool value);
		size_t get_memory_size() const;
		void set_voxelsize(float size);
		void set_voxelsize(const XMFLOAT3& size);
		wi::primitive::AABB get_aabb() const;
		void from_aabb(const wi::primitive::AABB& aabb);
		void debugdraw(const XMFLOAT4X4& ViewProjection, wi::graphics::CommandList cmd) const;

		struct Neighbors
		{
			XMUINT3 coords[3 * 3 * 3 - 1] = {}; // 3x3x3 grid, except center
			uint32_t count = 0;
		};
		enum class NeighborQueryFlags
		{
			None = 0,
			MustBeValid = 1 << 0, // only get neighbors that are set to 1
			DisableDiagonal = 1 << 1, // get neighbors only in straight directions
		};
		Neighbors get_neighbors(XMUINT3 coord, NeighborQueryFlags flags = NeighborQueryFlags::None) const;
		Neighbors get_neighbors(const XMFLOAT3& worldpos, NeighborQueryFlags flags = NeighborQueryFlags::None) const;

		inline bool IsValid() const { return !voxels.empty(); }

		inline static XMVECTOR __vectorcall world_to_uvw(XMVECTOR P, XMVECTOR center, XMVECTOR resolution_rcp, XMVECTOR voxelSize_rcp)
		{
			XMVECTOR diff = (P - center) * resolution_rcp * voxelSize_rcp;
			XMVECTOR uvw = diff * XMVectorSet(0.5f, -0.5f, 0.5f, 1) + XMVectorReplicate(0.5f);
			return uvw;
		}
		inline static XMVECTOR __vectorcall uvw_to_world(XMVECTOR uvw, XMVECTOR center, XMVECTOR resolution, XMVECTOR voxelSize)
		{
			XMVECTOR P = uvw * 2 - XMVectorReplicate(1);
			P *= XMVectorSet(1, -1, 1, 1);
			P *= voxelSize;
			P *= resolution;
			P += center;
			return P;
		}
	};

}

template<>
struct enable_bitmask_operators<wi::VoxelGrid::NeighborQueryFlags> {
	static const bool enable = true;
};
