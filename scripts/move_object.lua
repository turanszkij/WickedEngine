-- This script will load a teapot model with lights, and move the teapot's lid up and down
killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: move_object.lua")

scene = GetScene()
scene.Clear()
LoadModel("../models/teapot.wiscene")
top_entity = scene.Entity_FindByName("Top") -- query the teapot lid object by name
transform_component = scene.Component_GetTransform(top_entity)
rest_matrix = transform_component.GetMatrix()

runProcess(function()
	local t = 0
	while true do
		t = t + 0.1
		transform_component.ClearTransform()
		transform_component.MatrixTransform(rest_matrix)
		transform_component.Translate(Vector(0, math.sin(t) * 0.5 + 0.5, 0)) -- up and down
		update()
	end
end)

backlog_post("---> END SCRIPT: move_object.lua")
