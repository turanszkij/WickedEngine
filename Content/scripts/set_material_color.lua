-- This script will load a teapot model and animate its base color between red and green
killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: set_material_color.lua")

scene = GetScene()
scene.Clear()
LoadModel("../models/teapot.wiscene")
material_entity = scene.Entity_FindByName("teapot_material") -- query the teapot's material by name
material_component = scene.Component_GetMaterial(material_entity)

runProcess(function()
	local t = 0
	while true do
		t = t + 0.1
		local red = Vector(1,0,0,1)
		local green = Vector(0,1,0,1)
		local color = vector.Lerp(red, green, math.sin(t) * 0.5 + 0.5)
		material_component.SetBaseColor(color)
		update()
	end
end)

backlog_post("---> END SCRIPT: set_material_color.lua")
