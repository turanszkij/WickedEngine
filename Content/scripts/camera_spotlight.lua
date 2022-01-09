-- This script will add a spot light fixed to camera

backlog_post("---> START SCRIPT: camera_spotlight.lua")

scene = GetScene()

runProcess(function()

	local light_entity = CreateEntity()
	scene.Component_CreateLight(light_entity)

	local light = scene.Component_GetLight(light_entity)
	light.SetType(SPOT)
	light.SetRange(100)
	light.SetEnergy(8)
	light.SetFOV(math.pi / 3.0)
	light.SetColor(Vector(1,0.7,0.8))

	scene.Component_CreateTransform(light_entity)

	while true do

		local camera = GetCamera()
		local campos = camera.GetPosition()
		local camlook = camera.GetLookDirection()
		local camup = camera.GetUpDirection()
		local transform = scene.Component_GetTransform(light_entity)
		if transform == nil then
			backlog_post("light no longer exists, exiting script")
			return
		else
			transform.ClearTransform()
			transform.Rotate(Vector(-math.pi / 2.0, 0, 0)) -- spot light was facing downwards by default, rotate it to face +Z like camera default
			--transform.MatrixTransform(camera.GetInvView())
			transform.MatrixTransform(matrix.Inverse(matrix.LookTo(campos, camlook, camup))) -- This is similar to camera.GetInvView()
		end
		
		update()
	end
end)

backlog_post("---> END SCRIPT: camera_spotlight.lua")
