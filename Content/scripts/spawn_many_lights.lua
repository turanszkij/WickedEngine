-- This script will create many lights and move them in one direction.
--	Use this when you already loaded a scene, because the lights will be placed inside the scene bounds
backlog_post("---> START SCRIPT: spawn_many_lights.lua")

scene = GetScene()

local entities = {}
local velocities = {}
local bounds_min = scene.GetBounds().GetMin()
local bounds_max = scene.GetBounds().GetMax()

for i=1,200 do
	local entity = CreateEntity()
	entities[i] = entity
	velocities[i] = math.lerp(0.1, 0.2, math.random())
	local light_transform = scene.Component_CreateTransform(entity)
	light_transform.Translate(Vector(
		math.lerp(bounds_min.GetX(), bounds_max.GetX(), math.random()),
		math.lerp(bounds_min.GetY(), bounds_max.GetY(), math.random()),
		math.lerp(bounds_min.GetZ(), bounds_max.GetZ(), math.random())
	))
	local light_component = scene.Component_CreateLight(entity)
	light_component.SetType(POINT)
	light_component.SetRange(12)
	light_component.SetIntensity(40)
	light_component.SetColor(Vector(1,0.5,0)) -- orange color
	--light_component.SetColor(Vector(math.random(),math.random(),math.random())) -- random color
	--light_component.SetVolumetricsEnabled(true)
	light_component.SetCastShadow(true)
end

runProcess(function()
	local time = 0.0
	while(true) do
		time = time + getDeltaTime()
		bounds_min = scene.GetBounds().GetMin()
		bounds_max = scene.GetBounds().GetMax()

		for i,entity in ipairs(entities) do
			transform = scene.Component_GetTransform(entity)
			if transform ~= nil then
				transform.Translate(Vector(velocities[i]))
				if transform.GetPosition().GetX() > bounds_max.GetX() then
					transform.ClearTransform()
					transform.Translate(Vector(
						bounds_min.GetX(),
						math.lerp(bounds_min.GetY(), bounds_max.GetY(), math.random()),
						math.lerp(bounds_min.GetZ(), bounds_max.GetZ(), math.random())
					))
				end
			end
		end
		
		update()
	end
end)

backlog_post("---> END SCRIPT: spawn_many_lights.lua")
