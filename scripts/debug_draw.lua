-- This script will draw some debug primitives in the world such as line, point, box
killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: debug_draw.lua")

runProcess(function()
	while true do
		DrawLine(Vector(-10,4,0), Vector(10,1,1), Vector(1,0,1,1))
		DrawPoint(Vector(0,4,0), 3, Vector(1,1,0,1))
		
		local S = matrix.Scale(Vector(1,2,3))
		local R = matrix.Rotation(Vector(0.2, 0.6))
		local T = matrix.Translation(Vector(0,2,3))
		local M = S:Multiply(R):Multiply(T)
		DrawBox(M, Vector(0,1,1,1))
		
		render()
	end
end)

backlog_post("---> END SCRIPT: debug_draw.lua")
