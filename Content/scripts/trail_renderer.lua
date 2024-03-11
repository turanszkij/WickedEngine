killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: trail_renderer.lua")

local trail = TrailRenderer()
trail.SetWidth(0.2)
trail.SetColor(Vector(10,0.1,0.1,1))
trail.SetBlendMode(BLENDMODE_ADDITIVE)
trail.SetSubdivision(100)

-- Trail begins as red:
trail.AddPoint(Vector(-5,2,-3), 4, Vector(10,0.1,0.1,1))
trail.AddPoint(Vector(5,1,1), 0.5, Vector(10,0.1,0.1,1))
trail.AddPoint(Vector(10,5,4), 1.2, Vector(10,0.1,0.1,1))
trail.AddPoint(Vector(6,8,2), 1, Vector(10,0.1,0.1,1))
trail.AddPoint(Vector(-6,5,0), 1, Vector(10,0.1,0.1,1))
-- Trail turn into green:
trail.AddPoint(Vector(0,2,-5), 1, Vector(0.1,100,0.1,1))
trail.AddPoint(Vector(1,3,5), 1, Vector(0.1,100,0.1,1))
trail.AddPoint(Vector(-3,2,8), 1, Vector(0.1,100,0.1,1))

trail.Cut() -- start a new trail without connecting to previous points

-- Last trail segment is blue:
trail.AddPoint(Vector(-5,0,-2), 1, Vector(0.1,0.1,100,1))
trail.AddPoint(Vector(5,8,5), 1, Vector(0.1,0.1,100,1))

-- First texture is a circle gradient, this makes the overall trail smooth at the edges:
local texture = texturehelper.CreateGradientTexture(
	GradientType.Circular,
	256, 256,
	Vector(0.5, 0.5), Vector(0.5, 0),
	GradientFlags.Inverse,
	"rrrr"
)
trail.SetTexture(texture)

-- Second texture is a linear gradient that will be tiled and animated to achieve stippled look:
local texture2 = texturehelper.CreateGradientTexture(
	GradientType.Linear,
	256, 256,
	Vector(0.5, 0), Vector(0, 0),
	GradientFlags.Inverse | GradientFlags.Smoothstep,
	"rrrr"
)
trail.SetTexture2(texture2)

runProcess(function()
	local scrolling = 0
	while true do
		scrolling = scrolling - getDeltaTime()
		trail.SetTexMulAdd2(Vector(10,1,scrolling,0))
		DrawTrail(trail)
		render() -- this loop will be blocked until render tick
	end
end)

backlog_post("---> END SCRIPT: trail_renderer.lua")
