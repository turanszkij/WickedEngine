-- This script will load a teapot model and animate its emissive color and intensity
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
		t = t + 0.05
		local startcolor = Vector(1,0,1,0)  -- pink, but zero intesity emissive
		local endcolor = Vector(1,0,1,2)  -- 2x intensity pink emissive
		local color = vector.Lerp(startcolor, endcolor, math.sin(t) * 0.5 + 0.5)
		material_component.SetEmissiveColor(color)
		update()
	end
end)

backlog_post("---> END SCRIPT: set_material_color.lua")
