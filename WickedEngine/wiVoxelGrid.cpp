#include "wiVoxelGrid.h"
#include "shaders/ShaderInterop.h"
#include "wiEventHandler.h"
#include "wiRenderer.h"

#include "Utility/meshoptimizer/meshoptimizer.h"

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
		MAX = XMVectorCeiling(MAX + XMVectorSet(0.0001f, 0.0001f, 0.0001f, 0));

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
						const uint3 macro_coord = uint3(x / 4u, y / 4u, z / 4u);
						const uint3 sub_coord = uint3(x % 4u, y % 4u, z % 4u);
						const uint32_t idx = flatten3D(macro_coord, resolution_div4);
						const uint32_t bit = flatten3D(sub_coord, uint3(4, 4, 4));
						const uint64_t mask = 1ull << bit;
						if (subtract)
						{
							AtomicAnd(data + idx, ~mask);
						}
						else
						{
							AtomicOr(data + idx, mask);
						}
					}
				}
			}
		}
	}
	void VoxelGrid::inject_aabb(const wi::primitive::AABB& aabb, bool subtract)
	{
		const XMVECTOR CENTER = XMLoadFloat3(&center);
		const XMVECTOR RESOLUTION = XMLoadUInt3(&resolution);
		const XMVECTOR RESOLUTION_RCP = XMLoadFloat3(&resolution_rcp);
		const XMVECTOR VOXELSIZE_RCP = XMLoadFloat3(&voxelSize_rcp);

		XMVECTOR _MIN = XMLoadFloat3(&aabb._min);
		XMVECTOR _MAX = XMLoadFloat3(&aabb._max);

		// world -> uvw space:
		_MIN = world_to_uvw(_MIN, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);
		_MAX = world_to_uvw(_MAX, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);

		// pixel space:
		_MIN *= RESOLUTION;
		_MAX *= RESOLUTION;

		// After changing spaces, need to minmax again:
		XMVECTOR MIN = XMVectorMin(_MIN, _MAX);
		XMVECTOR MAX = XMVectorMax(_MIN, _MAX);

		MIN = XMVectorFloor(MIN);
		MAX = XMVectorCeiling(MAX + XMVectorSet(0.0001f, 0.0001f, 0.0001f, 0));

		MIN = XMVectorMax(MIN, XMVectorZero());
		MAX = XMVectorMin(MAX, RESOLUTION);

		XMUINT3 mini, maxi;
		XMStoreUInt3(&mini, MIN);
		XMStoreUInt3(&maxi, MAX);

		wi::primitive::AABB aabb_src;
		XMStoreFloat3(&aabb_src._min, MIN);
		XMStoreFloat3(&aabb_src._max, MAX);

		volatile long long* data = (volatile long long*)voxels.data();
		for (uint32_t x = mini.x; x < maxi.x; ++x)
		{
			for (uint32_t y = mini.y; y < maxi.y; ++y)
			{
				for (uint32_t z = mini.z; z < maxi.z; ++z)
				{
					const uint3 macro_coord = uint3(x / 4u, y / 4u, z / 4u);
					const uint3 sub_coord = uint3(x % 4u, y % 4u, z % 4u);
					const uint32_t idx = flatten3D(macro_coord, resolution_div4);
					const uint32_t bit = flatten3D(sub_coord, uint3(4, 4, 4));
					const uint64_t mask = 1ull << bit;
					if (subtract)
					{
						AtomicAnd(data + idx, ~mask);
					}
					else
					{
						AtomicOr(data + idx, mask);
					}
				}
			}
		}
	}
	void VoxelGrid::inject_sphere(const wi::primitive::Sphere& sphere, bool subtract)
	{
		const XMVECTOR CENTER = XMLoadFloat3(&center);
		const XMVECTOR RESOLUTION = XMLoadUInt3(&resolution);
		const XMVECTOR RESOLUTION_RCP = XMLoadFloat3(&resolution_rcp);
		const XMVECTOR VOXELSIZE_RCP = XMLoadFloat3(&voxelSize_rcp);

		AABB aabb;
		aabb.createFromHalfWidth(sphere.center, XMFLOAT3(sphere.radius, sphere.radius, sphere.radius));

		XMVECTOR _MIN = XMLoadFloat3(&aabb._min);
		XMVECTOR _MAX = XMLoadFloat3(&aabb._max);

		// world -> uvw space:
		_MIN = world_to_uvw(_MIN, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);
		_MAX = world_to_uvw(_MAX, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);

		// pixel space:
		_MIN *= RESOLUTION;
		_MAX *= RESOLUTION;

		// After changing spaces, need to minmax again:
		XMVECTOR MIN = XMVectorMin(_MIN, _MAX);
		XMVECTOR MAX = XMVectorMax(_MIN, _MAX);

		MIN = XMVectorFloor(MIN);
		MAX = XMVectorCeiling(MAX + XMVectorSet(0.0001f, 0.0001f, 0.0001f, 0));

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
					wi::primitive::AABB voxel_aabb;
					XMUINT3 voxel_center_coord = XMUINT3(x, y, z);
					XMFLOAT3 voxel_center_world = coord_to_world(voxel_center_coord);
					voxel_aabb.createFromHalfWidth(voxel_center_world, voxelSize);
					if (voxel_aabb.intersects(sphere))
					{
						const uint3 macro_coord = uint3(x / 4u, y / 4u, z / 4u);
						const uint3 sub_coord = uint3(x % 4u, y % 4u, z % 4u);
						const uint32_t idx = flatten3D(macro_coord, resolution_div4);
						const uint32_t bit = flatten3D(sub_coord, uint3(4, 4, 4));
						const uint64_t mask = 1ull << bit;
						if (subtract)
						{
							AtomicAnd(data + idx, ~mask);
						}
						else
						{
							AtomicOr(data + idx, mask);
						}
					}
				}
			}
		}
	}
	void VoxelGrid::inject_capsule(const wi::primitive::Capsule& capsule, bool subtract)
	{
		const XMVECTOR CENTER = XMLoadFloat3(&center);
		const XMVECTOR RESOLUTION = XMLoadUInt3(&resolution);
		const XMVECTOR RESOLUTION_RCP = XMLoadFloat3(&resolution_rcp);
		const XMVECTOR VOXELSIZE_RCP = XMLoadFloat3(&voxelSize_rcp);

		AABB aabb = capsule.getAABB();

		XMVECTOR _MIN = XMLoadFloat3(&aabb._min);
		XMVECTOR _MAX = XMLoadFloat3(&aabb._max);

		// world -> uvw space:
		_MIN = world_to_uvw(_MIN, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);
		_MAX = world_to_uvw(_MAX, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);

		// pixel space:
		_MIN *= RESOLUTION;
		_MAX *= RESOLUTION;

		// After changing spaces, need to minmax again:
		XMVECTOR MIN = XMVectorMin(_MIN, _MAX);
		XMVECTOR MAX = XMVectorMax(_MIN, _MAX);

		MIN = XMVectorFloor(MIN);
		MAX = XMVectorCeiling(MAX + XMVectorSet(0.0001f, 0.0001f, 0.0001f, 0));

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
					wi::primitive::AABB voxel_aabb;
					XMUINT3 voxel_center_coord = XMUINT3(x, y, z);
					XMFLOAT3 voxel_center_world = coord_to_world(voxel_center_coord);
					voxel_aabb.createFromHalfWidth(voxel_center_world, voxelSize);
					// This capsule-box test can fail if capsule doesn't contain any of the corners or center,
					//	but it intersects with the cube. But for now this simple method is used.
					bool intersects = capsule.intersects(voxel_aabb.getCenter());
					if (!intersects)
					{
						for (int c = 0; c < 8; ++c)
						{
							if (capsule.intersects(voxel_aabb.corner(c)))
							{
								intersects = true;
								break;
							}
						}
					}
					if (intersects)
					{
						const uint3 macro_coord = uint3(x / 4u, y / 4u, z / 4u);
						const uint3 sub_coord = uint3(x % 4u, y % 4u, z % 4u);
						const uint32_t idx = flatten3D(macro_coord, resolution_div4);
						const uint32_t bit = flatten3D(sub_coord, uint3(4, 4, 4));
						const uint64_t mask = 1ull << bit;
						if (subtract)
						{
							AtomicAnd(data + idx, ~mask);
						}
						else
						{
							AtomicOr(data + idx, mask);
						}
					}
				}
			}
		}
	}

	XMUINT3 VoxelGrid::world_to_coord(const XMFLOAT3& worldpos) const
	{
		XMUINT3 coord;
		XMStoreUInt3(&coord, world_to_uvw(XMLoadFloat3(&worldpos), XMLoadFloat3(&center), XMLoadFloat3(&resolution_rcp), XMLoadFloat3(&voxelSize_rcp)) * XMLoadUInt3(&resolution));
		return coord;
	}
	XMINT3 VoxelGrid::world_to_coord_signed(const XMFLOAT3& worldpos) const
	{
		XMFLOAT3 coord;
		XMStoreFloat3(&coord, world_to_uvw(XMLoadFloat3(&worldpos), XMLoadFloat3(&center), XMLoadFloat3(&resolution_rcp), XMLoadFloat3(&voxelSize_rcp)) * XMLoadUInt3(&resolution));
		return XMINT3((int)coord.x, (int)coord.y, (int)coord.z);
	}
	XMFLOAT3 VoxelGrid::coord_to_world(const XMUINT3& coord) const
	{
		XMFLOAT3 worldpos;
		XMStoreFloat3(&worldpos, uvw_to_world((XMLoadUInt3(&coord) + XMVectorReplicate(0.5f)) * XMLoadFloat3(&resolution_rcp), XMLoadFloat3(&center), XMLoadUInt3(&resolution), XMLoadFloat3(&voxelSize)));
		return worldpos;
	}
	XMFLOAT3 VoxelGrid::coord_to_world(const XMINT3& coord) const
	{
		XMFLOAT3 worldpos;
		XMStoreFloat3(&worldpos, uvw_to_world((XMVectorSet((float)coord.x, (float)coord.y, (float)coord.z, 0) + XMVectorReplicate(0.5f)) * XMLoadFloat3(&resolution_rcp), XMLoadFloat3(&center), XMLoadUInt3(&resolution), XMLoadFloat3(&voxelSize)));
		return worldpos;
	}
	bool VoxelGrid::check_voxel(const XMINT3& coord) const
	{
		return check_voxel(XMUINT3((uint32_t)coord.x, (uint32_t)coord.y, (uint32_t)coord.z));
	}
	bool VoxelGrid::check_voxel(const XMUINT3& coord) const
	{
		if (!is_coord_valid(coord))
			return false; // early exit when coord is not valid (outside of resolution)
		const uint3 macro_coord = uint3(coord.x / 4u, coord.y / 4u, coord.z / 4u);
		const uint idx = flatten3D(macro_coord, resolution_div4);
		const uint64_t voxels_4x4_block = voxels[idx];
		if (voxels_4x4_block == 0)
			return false; // early exit when whole block is empty
		uint3 sub_coord;
		sub_coord.x = coord.x % 4u;
		sub_coord.y = coord.y % 4u;
		sub_coord.z = coord.z % 4u;
		const uint bit = flatten3D(sub_coord, uint3(4, 4, 4));
		const uint64_t mask = 1ull << bit;
		return (voxels_4x4_block & mask) != 0ull;
	}
	bool VoxelGrid::check_voxel(const XMFLOAT3& worldpos) const
	{
		return check_voxel(world_to_coord(worldpos));
	}
	bool VoxelGrid::is_coord_valid(const XMINT3& coord) const
	{
		return is_coord_valid(XMUINT3((uint32_t)coord.x, (uint32_t)coord.y, (uint32_t)coord.z));
	}
	bool VoxelGrid::is_coord_valid(const XMUINT3& coord) const
	{
		return coord.x < resolution.x && coord.y < resolution.y && coord.z < resolution.z;
	}
	void VoxelGrid::set_voxel(const XMINT3& coord, bool value)
	{
		set_voxel(XMUINT3((uint32_t)coord.x, (uint32_t)coord.y, (uint32_t)coord.z), value);
	}
	void VoxelGrid::set_voxel(const XMUINT3& coord, bool value)
	{
		if (!is_coord_valid(coord))
			return; // early exit when coord is not valid (outside of resolution)
		const uint3 macro_coord = uint3(coord.x / 4u, coord.y / 4u, coord.z / 4u);
		const uint3 sub_coord = uint3(coord.x % 4u, coord.y % 4u, coord.z % 4u);
		const uint idx = flatten3D(macro_coord, resolution_div4);
		const uint bit = flatten3D(sub_coord, uint3(4, 4, 4));
		const uint64_t mask = 1ull << bit;
		if (value)
		{
			voxels[idx] |= mask;
		}
		else
		{
			voxels[idx] &= ~mask;
		}
	}
	void VoxelGrid::set_voxel(const XMFLOAT3& worldpos, bool value)
	{
		set_voxel(world_to_coord(worldpos), value);
	}
	size_t VoxelGrid::get_memory_size() const
	{
		return voxels.size() * sizeof(uint64_t);
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

	bool VoxelGrid::is_visible(const XMUINT3& start, const XMUINT3& goal) const
	{
		const int dx = int(goal.x) - int(start.x);
		const int dy = int(goal.y) - int(start.y);
		const int dz = int(goal.z) - int(start.z);

		const int step = std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz)));

		const float x_incr = float(dx) / step;
		const float y_incr = float(dy) / step;
		const float z_incr = float(dz) / step;

		float x = float(start.x);
		float y = float(start.y);
		float z = float(start.z);

#ifdef DEBUG_VOXEL_OCCLUSION
		debug_subject_coords.push_back(goal);
#endif // DEBUG_VOXEL_OCCLUSION

		for (int i = 0; i < step; i++)
		{
			XMUINT3 coord = XMUINT3(uint32_t(std::round(x)), uint32_t(std::round(y)), uint32_t(std::round(z)));
			if (coord.x == goal.x && coord.y == goal.y && coord.z == goal.z)
				return true;
			if (check_voxel(coord))
			{
#ifdef DEBUG_VOXEL_OCCLUSION
				debug_occluded_coords.push_back(coord);
#endif // DEBUG_VOXEL_OCCLUSION
				return false;
			}
#ifdef DEBUG_VOXEL_OCCLUSION
			debug_visible_coords.push_back(coord);
#endif // DEBUG_VOXEL_OCCLUSION
			x += x_incr;
			y += y_incr;
			z += z_incr;
		}
		return true;
	}
	bool VoxelGrid::is_visible(const XMFLOAT3& observer, const XMFLOAT3& subject) const
	{
		XMUINT3 start = world_to_coord(observer);
		XMUINT3 goal = world_to_coord(subject);
		return is_visible(start, goal);
	}
	bool VoxelGrid::is_visible(const XMFLOAT3& observer, const AABB& subject) const
	{
		XMUINT3 start = world_to_coord(observer);

		const XMVECTOR CENTER = XMLoadFloat3(&center);
		const XMVECTOR RESOLUTION = XMLoadUInt3(&resolution);
		const XMVECTOR RESOLUTION_RCP = XMLoadFloat3(&resolution_rcp);
		const XMVECTOR VOXELSIZE_RCP = XMLoadFloat3(&voxelSize_rcp);

		XMVECTOR _MIN = XMLoadFloat3(&subject._min);
		XMVECTOR _MAX = XMLoadFloat3(&subject._max);

		// world -> uvw space:
		_MIN = world_to_uvw(_MIN, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);
		_MAX = world_to_uvw(_MAX, CENTER, RESOLUTION_RCP, VOXELSIZE_RCP);

		// pixel space:
		_MIN *= RESOLUTION;
		_MAX *= RESOLUTION;

		// After changing spaces, need to minmax again:
		XMVECTOR MIN = XMVectorMin(_MIN, _MAX);
		XMVECTOR MAX = XMVectorMax(_MIN, _MAX);

		MIN = XMVectorFloor(MIN);
		MAX = XMVectorCeiling(MAX + XMVectorSet(0.0001f, 0.0001f, 0.0001f, 0));

		MIN = XMVectorMax(MIN, XMVectorZero());
		MAX = XMVectorMin(MAX, RESOLUTION);

		XMUINT3 mini, maxi;
		XMStoreUInt3(&mini, MIN);
		XMStoreUInt3(&maxi, MAX);

		wi::primitive::AABB aabb_src;
		XMStoreFloat3(&aabb_src._min, MIN);
		XMStoreFloat3(&aabb_src._max, MAX);

#ifdef DEBUG_VOXEL_OCCLUSION
		// In debug mode we visualize all tests and don't return early
		bool result = false;
		debug_subject_coords.clear();
		debug_occluded_coords.clear();
		debug_visible_coords.clear();
		for (uint32_t x = mini.x; x < maxi.x; ++x)
		{
			for (uint32_t y = mini.y; y < maxi.y; ++y)
			{
				for (uint32_t z = mini.z; z < maxi.z; ++z)
				{
					XMUINT3 goal = XMUINT3(x, y, z);
					if (is_visible(start, goal))
					{
						result = true;
					}
				}
			}
		}
		return result;
#else
		for (uint32_t x = mini.x; x < maxi.x; ++x)
		{
			for (uint32_t y = mini.y; y < maxi.y; ++y)
			{
				for (uint32_t z = mini.z; z < maxi.z; ++z)
				{
					XMUINT3 goal = XMUINT3(x, y, z);
					if (is_visible(start, goal))
					{
						return true;
					}
				}
			}
		}
		return false;
#endif // DEBUG_VOXEL_OCCLUSION
	}

	void VoxelGrid::add(const VoxelGrid& other)
	{
		if (voxels.size() != other.voxels.size())
		{
			assert(0);
			return;
		}
		for (size_t i = 0; i < voxels.size(); ++i)
		{
			voxels[i] |= other.voxels[i];
		}
	}
	void VoxelGrid::subtract(const VoxelGrid& other)
	{
		if (voxels.size() != other.voxels.size())
		{
			assert(0);
			return;
		}
		for (size_t i = 0; i < voxels.size(); ++i)
		{
			voxels[i] &= ~other.voxels[i];
		}
	}

	void VoxelGrid::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> voxels;
			archive >> resolution;
			archive >> voxelSize;
			archive >> center;
			archive >> debug_color;
			archive >> debug_color_extent;

			resolution_div4.x = (resolution.x + 3u) / 4u;
			resolution_div4.y = (resolution.y + 3u) / 4u;
			resolution_div4.z = (resolution.z + 3u) / 4u;
			resolution_rcp.x = 1.0f / resolution.x;
			resolution_rcp.y = 1.0f / resolution.y;
			resolution_rcp.z = 1.0f / resolution.z;
			set_voxelsize(voxelSize);
		}
		else
		{
			archive << _flags;
			archive << voxels;
			archive << resolution;
			archive << voxelSize;
			archive << center;
			archive << debug_color;
			archive << debug_color_extent;
		}
	}

	namespace VoxelGrid_internal
	{
		PipelineState pso;
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
			device->CreatePipelineState(&desc, &pso);
		}
	}
	using namespace VoxelGrid_internal;

	void VoxelGrid::debugdraw(const XMFLOAT4X4& ViewProjection, CommandList cmd) const
	{
		// add box renderer for whole volume:
		XMFLOAT4X4 boxmat;
		XMStoreFloat4x4(&boxmat, get_aabb().getAsBoxMatrix());
		wi::renderer::DrawBox(boxmat, debug_color_extent);

		// Add a cube for every filled voxel below:
		uint32_t numVoxels = 0;
		for (auto& x : voxels)
		{
			if (x != 0)
				numVoxels += (uint32_t)countbits(x);
		}
#ifdef DEBUG_VOXEL_OCCLUSION
		numVoxels += uint32_t(debug_subject_coords.size() + debug_visible_coords.size() + debug_occluded_coords.size());
#endif // DEBUG_VOXEL_OCCLUSION
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
			while (voxel_bits != 0)
			{
				unsigned long bit_index = firstbitlow(voxel_bits);
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
					v.color = debug_color;
				}
				std::memcpy((uint8_t*)mem.data + dst_offset, verts, sizeof(verts));
				dst_offset += sizeof(verts);
			}
		}

#ifdef DEBUG_VOXEL_OCCLUSION
		auto dbg_voxel = [&](const XMUINT3& coord, const XMFLOAT4& color) {
			XMFLOAT3 pos = coord_to_world(coord);
			XMVECTOR P = XMLoadFloat3(&pos);
			Vertex verts[arraysize(cubeVerts)];
			std::memcpy(verts, cubeVerts, sizeof(cubeVerts));
			for (auto& v : verts)
			{
				XMVECTOR C = XMLoadFloat4(&v.position);
				C *= VOXELSIZE;
				C += P;
				C = XMVectorSetW(C, 1);
				XMStoreFloat4(&v.position, C);
				v.color = color;
			}
			std::memcpy((uint8_t*)mem.data + dst_offset, verts, sizeof(verts));
			dst_offset += sizeof(verts);
		};
		for (auto& coord : debug_subject_coords)
		{
			dbg_voxel(coord, XMFLOAT4(1, 1, 1, 0.25f));
		}
		debug_subject_coords.clear();
		for (auto& coord : debug_visible_coords)
		{
			dbg_voxel(coord, XMFLOAT4(1, 1, 0, 0.125f));
		}
		debug_visible_coords.clear();
		for (auto& coord : debug_occluded_coords)
		{
			dbg_voxel(coord, XMFLOAT4(1, 0, 0, 0.5f));
		}
		debug_occluded_coords.clear();
#endif // DEBUG_VOXEL_OCCLUSION

		device->EventBegin("VoxelGrid::debugdraw", cmd);
		device->BindPipelineState(&pso, cmd);

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
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);

		device->Draw(arraysize(cubeVerts) * numVoxels, 0, cmd);

		device->EventEnd(cmd);
	}


	using XYZ = XMFLOAT3;

	// Source: http://www.paulbourke.net/geometry/polygonise/

	/*
	   int edgeTable[256].  It corresponds to the 2^8 possible combinations of
	   of the eight (n) vertices either existing inside or outside (2^n) of the
	   surface.  A vertex is inside of a surface if the value at that vertex is
	   less than that of the surface you are scanning for.  The table index is
	   constructed bitwise with bit 0 corresponding to vertex 0, bit 1 to vert
	   1.. bit 7 to vert 7.  The value in the table tells you which edges of
	   the table are intersected by the surface.  Once again bit 0 corresponds
	   to edge 0 and so on, up to edge 12.
	   Constructing the table simply consisted of having a program run thru
	   the 256 cases and setting the edge bit if the vertices at either end of
	   the edge had different values (one is inside while the other is out).
	   The purpose of the table is to speed up the scanning process.  Only the
	   edges whose bit's are set contain vertices of the surface.
	   Vertex 0 is on the bottom face, back edge, left side.
	   The progression of vertices is clockwise around the bottom face
	   and then clockwise around the top face of the cube.  Edge 0 goes from
	   vertex 0 to vertex 1, Edge 1 is from 2->3 and so on around clockwise to
	   vertex 0 again. Then Edge 4 to 7 make up the top face, 4->5, 5->6, 6->7
	   and 7->4.  Edge 8 thru 11 are the vertical edges from vert 0->4, 1->5,
	   2->6, and 3->7.
		   4--------5     *---4----*
		  /|       /|    /|       /|
		 / |      / |   7 |      5 |
		/  |     /  |  /  8     /  9
	   7--------6   | *----6---*   |
	   |   |    |   | |   |    |   |
	   |   0----|---1 |   *---0|---*
	   |  /     |  /  11 /     10 /
	   | /      | /   | 3      | 1
	   |/       |/    |/       |/
	   3--------2     *---2----*
	*/
	static const int edgeTable[256] = {
		0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
		0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
		0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
		0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
		0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
		0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
		0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
		0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
		0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
		0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
		0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
		0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
		0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
		0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
		0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
		0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
		0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
		0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
		0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
		0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
		0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
		0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
		0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
		0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
		0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
		0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
		0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
		0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
		0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
		0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
		0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
	};
	/*
	   int triTable[256][16] also corresponds to the 256 possible combinations
	   of vertices.
	   The [16] dimension of the table is again the list of edges of the cube
	   which are intersected by the surface.  This time however, the edges are
	   enumerated in the order of the vertices making up the triangle mesh of
	   the surface.  Each edge contains one vertex that is on the surface.
	   Each triple of edges listed in the table contains the vertices of one
	   triangle on the mesh.  The are 16 entries because it has been shown that
	   there are at most 5 triangles in a cube and each "edge triple" list is
	   terminated with the value -1.
	   For example triTable[3] contains
	   {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
	   This corresponds to the case of a cube whose vertex 0 and 1 are inside
	   of the surface and the rest of the verts are outside (00000001 bitwise
	   OR'ed with 00000010 makes 00000011 == 3).  Therefore, this cube is
	   intersected by the surface roughly in the form of a plane which cuts
	   edges 8,9,1 and 3.  This quadrilateral can be constructed from two
	   triangles: one which is made of the intersection vertices found on edges
	   1,8, and 3; the other is formed from the vertices on edges 9,8, and 1.
	   Remember, each intersected edge contains only one surface vertex.  The
	   vertex triples are listed in counter clockwise order for proper facing.
	*/
	static const int triTable[256][16] =
	{
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
		{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
		{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
		{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
		{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
		{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
		{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
		{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
		{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
		{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
		{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
		{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
		{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
		{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
		{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
		{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
		{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
		{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
		{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
		{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
		{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
		{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
		{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
		{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
		{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
		{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
		{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
		{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
		{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
		{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
		{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
		{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
		{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
		{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
		{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
		{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
		{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
		{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
		{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
		{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
		{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
		{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
		{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
		{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
		{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
		{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
		{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
		{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
		{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
		{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
		{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
		{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
		{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
		{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
		{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
		{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
		{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
		{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
		{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
		{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
		{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
		{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
		{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
		{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
		{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
		{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
		{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
		{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
		{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
		{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
		{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
		{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
		{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
		{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
		{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
		{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
		{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
		{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
		{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
		{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
		{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
		{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
		{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
		{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
		{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
		{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
		{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
		{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
		{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
		{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
		{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
		{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
		{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
		{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
		{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
		{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
	};

	typedef struct {
		XYZ p[3];
	} TRIANGLE;

	typedef struct {
		XYZ p[8];
		float val[8];
	} GRIDCELL;

	/*
	   Linearly interpolate the position where an isosurface cuts
	   an edge between two vertices, each with their own scalar value
	*/
	XYZ VertexInterp(float isolevel, XYZ p1, XYZ p2, float valp1, float valp2)
	{
		float mu;
		XYZ p;

		if (std::abs(isolevel - valp1) < 0.00001f)
			return(p1);
		if (std::abs(isolevel - valp2) < 0.00001f)
			return(p2);
		if (std::abs(valp1 - valp2) < 0.00001f)
			return(p1);
		mu = (isolevel - valp1) / (valp2 - valp1);
		p.x = p1.x + mu * (p2.x - p1.x);
		p.y = p1.y + mu * (p2.y - p1.y);
		p.z = p1.z + mu * (p2.z - p1.z);

		return(p);
	}

	/*
	   Given a grid cell and an isolevel, calculate the triangular
	   facets required to represent the isosurface through the cell.
	   Return the number of triangular facets, the array "triangles"
	   will be loaded up with the vertices at most 5 triangular facets.
		0 will be returned if the grid cell is either totally above
	   of totally below the isolevel.
	*/
	int Polygonise(GRIDCELL grid, float isolevel, TRIANGLE* triangles)
	{
		int i, ntriang;
		int cubeindex;
		XYZ vertlist[12];


		/*
		   Determine the index into the edge table which
		   tells us which vertices are inside of the surface
		*/
		cubeindex = 0;
		if (grid.val[0] < isolevel) cubeindex |= 1;
		if (grid.val[1] < isolevel) cubeindex |= 2;
		if (grid.val[2] < isolevel) cubeindex |= 4;
		if (grid.val[3] < isolevel) cubeindex |= 8;
		if (grid.val[4] < isolevel) cubeindex |= 16;
		if (grid.val[5] < isolevel) cubeindex |= 32;
		if (grid.val[6] < isolevel) cubeindex |= 64;
		if (grid.val[7] < isolevel) cubeindex |= 128;

		/* Cube is entirely in/out of the surface */
		if (edgeTable[cubeindex] == 0)
			return(0);

		/* Find the vertices where the surface intersects the cube */
		if (edgeTable[cubeindex] & 1)
			vertlist[0] =
			VertexInterp(isolevel, grid.p[0], grid.p[1], grid.val[0], grid.val[1]);
		if (edgeTable[cubeindex] & 2)
			vertlist[1] =
			VertexInterp(isolevel, grid.p[1], grid.p[2], grid.val[1], grid.val[2]);
		if (edgeTable[cubeindex] & 4)
			vertlist[2] =
			VertexInterp(isolevel, grid.p[2], grid.p[3], grid.val[2], grid.val[3]);
		if (edgeTable[cubeindex] & 8)
			vertlist[3] =
			VertexInterp(isolevel, grid.p[3], grid.p[0], grid.val[3], grid.val[0]);
		if (edgeTable[cubeindex] & 16)
			vertlist[4] =
			VertexInterp(isolevel, grid.p[4], grid.p[5], grid.val[4], grid.val[5]);
		if (edgeTable[cubeindex] & 32)
			vertlist[5] =
			VertexInterp(isolevel, grid.p[5], grid.p[6], grid.val[5], grid.val[6]);
		if (edgeTable[cubeindex] & 64)
			vertlist[6] =
			VertexInterp(isolevel, grid.p[6], grid.p[7], grid.val[6], grid.val[7]);
		if (edgeTable[cubeindex] & 128)
			vertlist[7] =
			VertexInterp(isolevel, grid.p[7], grid.p[4], grid.val[7], grid.val[4]);
		if (edgeTable[cubeindex] & 256)
			vertlist[8] =
			VertexInterp(isolevel, grid.p[0], grid.p[4], grid.val[0], grid.val[4]);
		if (edgeTable[cubeindex] & 512)
			vertlist[9] =
			VertexInterp(isolevel, grid.p[1], grid.p[5], grid.val[1], grid.val[5]);
		if (edgeTable[cubeindex] & 1024)
			vertlist[10] =
			VertexInterp(isolevel, grid.p[2], grid.p[6], grid.val[2], grid.val[6]);
		if (edgeTable[cubeindex] & 2048)
			vertlist[11] =
			VertexInterp(isolevel, grid.p[3], grid.p[7], grid.val[3], grid.val[7]);

		/* Create the triangle */
		ntriang = 0;
		for (i = 0; triTable[cubeindex][i] != -1; i += 3) {
			triangles[ntriang].p[0] = vertlist[triTable[cubeindex][i]];
			triangles[ntriang].p[1] = vertlist[triTable[cubeindex][i + 1]];
			triangles[ntriang].p[2] = vertlist[triTable[cubeindex][i + 2]];
			ntriang++;
		}

		return(ntriang);
	}

	void VoxelGrid::create_mesh(
		wi::vector<uint32_t>& indices,
		wi::vector<XMFLOAT3>& vertices,
		bool simplify
	)
	{
		TRIANGLE triangles[10];
		wi::vector<XMFLOAT3> unindexed_vertices;
		// Note: step over the voxel grid's perimeter as well, without that the polygonize can leave holes on the sides
		for (int x = -1; x < (int)resolution.x; ++x)
		{
			for (int y = -1; y < (int)resolution.y; ++y)
			{
				for (int z = -1; z < (int)resolution.z; ++z)
				{
					int3 coords[8] = {
						int3(x + 0, y + 0, z + 0),
						int3(x + 1, y + 0, z + 0),
						int3(x + 1, y + 1, z + 0),
						int3(x + 0, y + 1, z + 0),
						int3(x + 0, y + 0, z + 1),
						int3(x + 1, y + 0, z + 1),
						int3(x + 1, y + 1, z + 1),
						int3(x + 0, y + 1, z + 1),
					};

					GRIDCELL grid = {};
					for (int c = 0; c < arraysize(coords); ++c)
					{
						grid.p[c] = coord_to_world(coords[c]);
						grid.val[c] = check_voxel(coords[c]) ? -1.0f : 1.0f;
					}

					int num = Polygonise(grid, 0, triangles);
					for (int t = 0; t < num; ++t)
					{
						unindexed_vertices.push_back(triangles[t].p[0]);
						unindexed_vertices.push_back(triangles[t].p[2]);
						unindexed_vertices.push_back(triangles[t].p[1]);
					}

				}
			}
		}
		if (unindexed_vertices.empty())
			return;

		size_t index_count = unindexed_vertices.size();
		size_t unindexed_vertex_count = unindexed_vertices.size();
		std::vector<unsigned int> remap(index_count); // allocate temporary memory for the remap table
		size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, index_count, &unindexed_vertices[0], unindexed_vertex_count, sizeof(XMFLOAT3));

		indices.resize(index_count);
		vertices.resize(vertex_count);

		meshopt_remapIndexBuffer(indices.data(), nullptr, index_count, &remap[0]);
		meshopt_remapVertexBuffer(vertices.data(), &unindexed_vertices[0], unindexed_vertex_count, sizeof(XMFLOAT3), &remap[0]);

		if (simplify)
		{
			static float threshold = 0.2f;
			size_t target_index_count = size_t(index_count * threshold);
			float target_error = 1e-2f;
			unsigned int options = 0; // meshopt_SimplifyX flags, 0 is a safe default

			std::vector<unsigned int> lod(index_count);
			float lod_error = 0.f;
			lod.resize(meshopt_simplify(
				&lod[0],
				indices.data(),
				index_count,
				&vertices[0].x,
				vertex_count,
				sizeof(XMFLOAT3),
				target_index_count,
				target_error,
				options,
				&lod_error)
			);

			index_count = lod.size();
			unindexed_vertex_count = vertices.size();
			size_t vertex_count = meshopt_generateVertexRemap(&remap[0], lod.data(), index_count, &vertices[0], unindexed_vertex_count, sizeof(XMFLOAT3));

			wi::vector<uint32_t> simplified_indices(index_count);
			wi::vector<XMFLOAT3> simplified_vertices(vertex_count);

			meshopt_remapIndexBuffer(simplified_indices.data(), lod.data(), index_count, &remap[0]);
			meshopt_remapVertexBuffer(simplified_vertices.data(), &vertices[0], unindexed_vertex_count, sizeof(XMFLOAT3), &remap[0]);

			std::swap(indices, simplified_indices);
			std::swap(vertices, simplified_vertices);
		}
	}

}
