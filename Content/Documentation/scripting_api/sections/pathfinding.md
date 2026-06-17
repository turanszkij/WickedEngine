# Path finding

[← Back to Scripting API index](../index.md)

## VoxelGrid

```lua
    --- Creates a voxel grid of the given dimensions, used for navigation and
    --- voxel ray queries.
    ---
    ---@param dimX? integer
    ---@param dimY integer
    ---@param dimZ integer
    ---
    ---@return VoxelGrid
    function VoxelGrid(dimX, dimY, dimZ) end

    --- Path finding operations can be made by using a voxel grid and path
    --- queries. The voxel grid can store spatial information about a scene, or
    --- a part of the scene, while the path query manages the path finding
    --- result from a point to a different point within the voxel grid.
    ---
    ---@class VoxelGrid
    local VoxelGrid = {}


    --- Allocates memory for dimX * dimY * dimZ number of voxels and initializes
    --- them to empty.
    ---
    ---@param dimX integer
    ---@param dimY integer
    ---@param dimZ integer
    function VoxelGrid.Init(dimX, dimY, dimZ) end

    --- Initializes all voxels to empty.
    function VoxelGrid.ClearData() end

    --- Places the voxel grid volume to fit to the given AABB. The number of
    --- voxels doesn't change, only the center and voxel size.
    ---
    ---@param aabb AABB
    function VoxelGrid.FromAABB(aabb) end

    --- Voxelizes triangle, and either adds it to the voxels (default), or
    --- removes voxels
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param c Vector
    ---@param subtract? boolean
    function VoxelGrid.InjectTriangle(a, b, c, subtract) end

    --- Voxelizes axis aligned bounding box, and either adds it to the voxels
    --- (default), or removes voxels.
    ---
    ---@param aabb AABB
    ---@param subtract? boolean
    function VoxelGrid.InjectAABB(aabb, subtract) end

    --- Voxelizes sphere, and either adds it to the voxels (default), or removes
    --- voxels.
    ---
    ---@param sphere Sphere
    ---@param subtract? boolean
    function VoxelGrid.InjectSphere(sphere, subtract) end

    --- Voxelizes capsule, and either adds it to the voxels (default), or
    --- removes voxels.
    ---
    ---@param capsule Capsule
    ---@param subtract? boolean
    function VoxelGrid.InjectCapsule(capsule, subtract) end

    --- Converts a position in world space to voxel coordinate.
    ---
    ---@param pos Vector
    ---
    ---@return integer x
    ---@return integer y
    ---@return integer z
    function VoxelGrid.WorldToCoord(pos) end

    --- converts voxel coordinate to world space position
    ---
    ---@param x integer
    ---@param y integer
    ---@param z integer
    ---
    ---@return Vector
    function VoxelGrid.CoordToWorld(x, y, z) end

    --- Returns false if voxel is empty, true if it's valid.
    ---
    ---@param pos Vector
    ---
    ---@return boolean
    function VoxelGrid.CheckVoxel(pos) end

    --- Returns false if voxel is empty, true if it's valid.
    ---
    ---@param x integer
    ---@param y integer
    ---@param z integer
    ---
    ---@return boolean
    function VoxelGrid.CheckVoxel(x, y, z) end

    --- Sets a single voxel to the specified state.
    ---
    ---@param pos Vector
    ---@param value boolean
    function VoxelGrid.SetVoxel(pos, value) end

    --- Sets a single voxel to the specified state.
    ---
    ---@param x integer
    ---@param y integer
    ---@param z integer
    ---@param value boolean
    function VoxelGrid.SetVoxel(x, y, z, value) end

    --- Returns the center of the voxel grid volume.
    ---
    ---@return Vector
    function VoxelGrid.GetCenter() end

    --- Sets the center of the voxel grid volume.
    ---
    ---@param pos Vector
    function VoxelGrid.SetCenter(pos) end

    --- Get the half extent of one voxel in world space.
    ---
    ---@return Vector
    function VoxelGrid.GetVoxelSize() end

    --- Sets the half extent of one voxel in world space.
    ---
    ---@param voxelsize Vector
    function VoxelGrid.SetVoxelSize(voxelsize) end

    --- Sets the half extent of one voxel in world space uniformly in all
    --- dimension.
    ---
    ---@param voxelsize number
    function VoxelGrid.SetVoxelSize(voxelsize) end

    --- Returns color of debug visualization of voxels.
    ---
    ---@return Vector
    function VoxelGrid.GetDebugColor() end

    --- Set the color for debug visualization of voxels.
    ---
    ---@param color Vector
    function VoxelGrid.SetDebugColor(color) end

    --- Returns color of debug visualization of voxel grid extents.
    ---
    ---@return Vector
    function VoxelGrid.GetDebugColorExtent() end

    --- Set the color for debug visualization of voxel grid extents.
    ---
    ---@param color Vector
    function VoxelGrid.SetDebugColorExtent(color) end

    --- Returns the memory consumption of the voxel grid in bytes.
    ---
    ---@return integer
    function VoxelGrid.GetMemorySize() end

    --- Performs line of sight occlusion test from observer to subject voxel
    --- coordinates. Returns false if occlusion was found, true otherwise.
    ---
    ---@param observer_x integer
    ---@param observer_y integer
    ---@param observer_z integer
    ---@param subject_x integer
    ---@param subject_y integer
    ---@param subject_z integer
    ---
    ---@return boolean
    function VoxelGrid.IsVisible(
        observer_x,
        observer_y,
        observer_z,
        subject_x,
        subject_y,
        subject_z
    ) end

    --- Performs line of sight occlusion test from observer to subject world
    --- space points. Returns false if occlusion was found, true otherwise.
    ---
    ---@param observer AABB
    ---@param subject AABB
    ---
    ---@return boolean
    function VoxelGrid.IsVisible(observer, subject) end

    --- Performs line of sight occlusion test from observer world space point to
    --- subject AABB. Returns true if any of the AABB's touched voxels is
    --- visible, false otherwise.
    ---
    ---@param observer AABB
    ---@param subject AABB
    ---
    ---@return boolean
    function VoxelGrid.IsVisible(observer, subject) end

    --- Sets every empty voxel which is enclosed to solid.
    function VoxelGrid.FloodFill() end

    --- Adds the voxels of another grid into this one.
    ---
    ---@param other VoxelGrid
    function VoxelGrid.Add(other) end

    --- Removes the voxels of another grid from this one.
    ---
    ---@param other VoxelGrid
    function VoxelGrid.Subtract(other) end
```

## PathQuery

```lua
    --- Creates a PathQuery for finding paths through a voxel grid.
    ---
    ---@return PathQuery
    function PathQuery() end

    --- Computes and stores a path through a VoxelGrid, for navigation and AI
    --- movement.
    ---
    ---@class PathQuery
    local PathQuery = {}


    --- Computes the path from start to goal on a voxel grid and stores the
    --- result.
    ---
    ---@param start Vector
    ---@param goal Vector
    ---@param voxelgrid VoxelGrid
    function PathQuery.Process(start, goal, voxelgrid) end

    --- Searches for a cover for subject position to hide from observer. The
    --- search will be in a specific direction, within the specified distance
    --- (approximately, within voxel precision).
    ---
    ---@param observer Vector
    ---@param subject Vector
    ---@param direction Vector
    ---@param max_distance number
    ---@param voxelgrid VoxelGrid
    function PathQuery.SearchCover(
        observer,
        subject,
        direction,
        max_distance,
        voxelgrid
    ) end

    --- Returns whether the last call to Process() was successfully able to find
    --- a path.
    ---
    ---@return boolean
    function PathQuery.IsSuccessful() end

    --- Get the next waypoint on the path from the starting location. This
    --- requires that Process() has been called beforehand.
    ---
    ---@return Vector
    function PathQuery.GetNextWaypoint() end

    --- Enable/disable waypoint debug rendering when using DrawPathQuery(). If
    --- enabled, voxel waypoints will be drawn in blue, simplified voxel
    --- waypoints will be drawn in pink.
    ---
    ---@param value boolean
    function PathQuery.SetDebugDrawWaypointsEnabled(value) end

    --- Enable/disable flying behavior. When flying is enabled, then the path
    --- will be on empty voxels (air), otherwise and by default the path will be
    --- on filled voxels (ground).
    ---
    ---@param value boolean
    function PathQuery.SetFlying(value) end

    --- Returns whether flying.
    ---
    ---@return boolean
    function PathQuery.IsFlying() end

    --- Set the navigation width requirement in voxels. This means how many
    --- voxels the query will keep away from obstacles horizontally.
    ---
    ---@param value integer
    function PathQuery.SetAgentWidth(value) end

    --- Returns the agent width.
    ---
    ---@param value integer
    ---
    ---@return integer
    function PathQuery.GetAgentWidth(value) end

    --- Set the navigation height requirement in voxels. This means how many
    --- voxels the query will keep away from obstacles vertically.
    ---
    ---@param value integer
    function PathQuery.SetAgentHeight(value) end

    --- Returns the agent height.
    ---
    ---@param value integer
    ---
    ---@return integer
    function PathQuery.GetAgentHeight(value) end

    --- Returns the number of waypoints that were computed in Process().
    ---
    ---@return integer
    function PathQuery.GetWaypointCount() end

    --- Returns the waypoint at specified index (direction: start -> goal).
    ---
    ---@param index integer
    ---
    ---@return Vector
    function PathQuery.GetWaypoint(index) end

    --- Returns the goal position.
    ---
    ---@return Vector
    function PathQuery.GetGoal() end
```
