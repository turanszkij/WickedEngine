-- This script will add a point light fixed to camera

backlog_post("---> START SCRIPT: camera_pointlight.lua")

scene = GetScene()

runProcess(function()

	local light_entity = CreateEntity()
	scene.Component_CreateLight(light_entity)

	local light = scene.Component_GetLight(light_entity)
	light.SetType(POINT)
	light.SetRange(20)
	light.SetEnergy(4)
	light.SetColor(Vector(1,0.9,0.8))

	scene.Component_CreateTransform(light_entity)

	while true do

		local camera = GetCamera()
		local campos = camera.GetPosition()
		local transform = scene.Component_GetTransform(light_entity)
		if transform == nil then
			backlog_post("light no longer exists, exiting script")
			return
		else
			transform.ClearTransform()
			transform.Translate(campos)
		end
		
		update()
	end
end)

backlog_post("---> END SCRIPT: camera_pointlight.lua")
