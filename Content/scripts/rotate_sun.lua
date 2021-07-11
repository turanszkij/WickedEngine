-- This script will check for directional lights and begin rotating them slowly if there are any
killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: rotate_sun.lua")

scene = GetScene()

runProcess(function()
	while true do
		
		local lights = scene.Component_GetLightArray()
		for i,light in ipairs(lights) do
			if light.GetType() == DIRECTIONAL then
				local entity = scene.Entity_GetLightArray()[i]
				local transform_component = scene.Component_GetTransform(entity)
				transform_component.Rotate(Vector(0.0015 * getDeltaTime() * GetGameSpeed(), 0, 0)) -- rotate around x axis
			end
		end

		update()
	end
end)

backlog_post("---> END SCRIPT: rotate_sun.lua")
