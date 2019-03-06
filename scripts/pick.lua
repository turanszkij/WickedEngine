-- This script will load a teapot model and demonstrate picking it with a ray
killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: pick.lua")

scene = GetScene()
scene.Clear()
model_entity = LoadModel("../models/teapot.wiscene")

runProcess(function()
	local t = 0
	while true do
		t = t + 0.02
		
		-- Create a Ray by specifying origin and direction (and also animate the ray origin along sine wave):
		local ray = Ray(Vector(math.sin(t) * 4,1,-10), Vector(0,0,1)) 
		local entity, position, normal, distance = Pick(ray)
		
		if(entity ~= INVALID_ENTITY) then
			-- Draw intersection point as purple X
			DrawPoint(position, 1, Vector(1,0,1,1))
		end
		
		-- Draw ray as yellow line:
		DrawLine(ray.GetOrigin(), ray.GetOrigin():Add(ray.GetDirection():Multiply(1000)), Vector(1,1,0,1))
		
		render()
	end
end)

backlog_post("---> END SCRIPT: pick.lua")
