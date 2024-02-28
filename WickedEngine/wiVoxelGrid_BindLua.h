#pragma once
#include "CommonInclude.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiVoxelGrid.h"

namespace wi::lua
{
	class VoxelGrid_BindLua
	{
	private:
		wi::VoxelGrid owning;
	public:
		wi::VoxelGrid* voxelgrid = nullptr;

		inline static constexpr char className[] = "VoxelGrid";
		static Luna<VoxelGrid_BindLua>::FunctionType methods[];
		static Luna<VoxelGrid_BindLua>::PropertyType properties[];

		VoxelGrid_BindLua() = default;
		VoxelGrid_BindLua(lua_State* L);
		VoxelGrid_BindLua(wi::VoxelGrid& ref) : voxelgrid(&ref) {}
		VoxelGrid_BindLua(wi::VoxelGrid* ref) : voxelgrid(ref) {}

		int Init(lua_State* L);
		int ClearData(lua_State* L);
		int FromAABB(lua_State* L);
		int InjectTriangle(lua_State* L);
		int InjectAABB(lua_State* L);
		int InjectSphere(lua_State* L);
		int InjectCapsule(lua_State* L);
		int WorldToCoord(lua_State* L);
		int CoordToWorld(lua_State* L);
		int CheckVoxel(lua_State* L);
		int SetVoxel(lua_State* L);
		int GetCenter(lua_State* L);
		int SetCenter(lua_State* L);
		int GetVoxelSize(lua_State* L);
		int SetVoxelSize(lua_State* L);
		int GetDebugColor(lua_State* L);
		int SetDebugColor(lua_State* L);
		int GetDebugColorExtent(lua_State* L);
		int SetDebugColorExtent(lua_State* L);
		int GetMemorySize(lua_State* L);
		int Add(lua_State* L);
		int Subtract(lua_State* L);
		int IsVisible(lua_State* L);

		static void Bind();
	};
}
