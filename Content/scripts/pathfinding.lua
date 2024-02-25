killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: pathfinding.lua")

-- Create a simple voxel grid by inserting some shapes manually:
local voxelgrid = VoxelGrid(64, 64, 64)
local voxelsize = voxelgrid.GetVoxelSize()
voxelsize = vector.Multiply(voxelsize, 0.5) -- reduce the voxelsize by half
voxelgrid.SetVoxelSize(voxelsize)
voxelgrid.InjectTriangle(Vector(-10, 0, -10), Vector(-10, 0, 10), Vector(10, 0, -10))
voxelgrid.InjectTriangle(Vector(-10, 0, 10), Vector(10, 0, 10), Vector(10, 0, -10))
voxelgrid.InjectAABB(AABB(Vector(4, 0, -2), Vector(4.5, 4, 5)))
voxelgrid.InjectAABB(AABB(Vector(4, 0, 0.8), Vector(8, voxelsize.GetY() * 2, 5)))
voxelgrid.InjectAABB(AABB(Vector(4, 0, 3), Vector(8, voxelsize.GetY() * 3.5, 7)))
voxelgrid.InjectAABB(AABB(Vector(4, 0, 6), Vector(8, voxelsize.GetY() * 4.5, 7)))
voxelgrid.InjectSphere(Sphere(Vector(-4.8,1.6,-2.5), 1.6))
voxelgrid.InjectCapsule(Capsule(Vector(4.8,-0.6,-2.5), Vector(2, 1, 1), 0.4))

-- If teapot model can be loaded, then load it and voxelize it too:
local scene = Scene()
local entity = LoadModel(scene, script_dir() .. "../models/teapot.wiscene", matrix.RotationY(math.pi * 0.6))
if entity ~= INVALID_ENTITY then
    for i,entity in ipairs(scene.Entity_GetObjectArray()) do
		scene.VoxelizeObject(i - 1, voxelgrid)
	end
end

-- Create a path query to find paths from start to goal position on voxel grid:
local pathquery = PathQuery()
pathquery.SetDebugDrawWaypointsEnabled(true) -- enable waypoint voxel highlighting in DrawPathQuery()
local start = Vector(-7.63,0,-2.6) -- world space coordinates can be given
local goal = Vector(6.3,voxelsize.GetY() * 4.5, 6.5) -- world space coordinates can be given
pathquery.Process(start, goal, voxelgrid) -- this computes the path

runProcess(function()
	while true do
		DrawVoxelGrid(voxelgrid)
		DrawPathQuery(pathquery)
		render() -- this loop will be blocked until render tick
	end
end)

backlog_post("---> END SCRIPT: pathfinding.lua")
