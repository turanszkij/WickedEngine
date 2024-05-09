-- This script demonstrates spawning multiple objects from one source scene
killProcesses() -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: instantiate.lua")

scene = GetScene()
scene.Clear()

local prefab = Scene()
LoadModel(prefab, script_dir() .. "../models/hologram_test.wiscene")

-- Instantiate first object
scene.Instantiate(prefab)

runProcess(function()
	while true do
		
		-- Instantiate more objects whenever the user presses space
		if input.Press(KEYBOARD_BUTTON_SPACE) then
			-- passing true as second parameter attaches all entities to a common root
			local root_entity = scene.Instantiate(prefab, true)

			local transform_component = scene.Component_GetTransform(root_entity)
			transform_component.Translate(Vector(math.random() * 10 - 5, 0, math.random() * 10 - 5))
		end

		update()
	end
end)

backlog_post("---> END SCRIPT: instantiate.lua")
