-- This script will play a camera animation chain from "cam0" -to "camN" named camera proxies in the scene
--  To use this, first place four cameras into the scene and name them cam0, cam1, cam2 and cam3, then press F8 to start
--	The animation will repeat infinitely, smoothly

-- If no name is provided, this will return the main camera:
local cam = GetCamera()

-- Camera speed overridable from outer scope too:
scriptableCameraSpeed = 0.4

-- Animation state:
local tt = 0.0
local play = false
local rot = 0

-- Gather camera proxies in the scene from "cam0" to "cam1", "cam2", ... "camN":
local proxies={}
local it = 0
while true do
	local cam = GetCamera("cam" .. it)
	if(cam == nil) then
		break
	end
	it = it + 1
	proxies[it] = cam
end

runProcess(function()
	while true do

		if(input.Press(VK_F8)) then
			-- Reset animation:
			tt = 0.0
			play = not play
			rot = 0
		end
		if(play) then
			-- Play animation:
			--local proxies = { GetCamera("cam0"), GetCamera("cam1"), GetCamera("cam2"), GetCamera("cam3") }
			local count = len(proxies)
			
			-- Place main camera on spline:
			cam.CatmullRom(proxies[(rot - 1) % count + 1], proxies[rot % count + 1], proxies[(rot + 1) % count + 1], proxies[(rot + 2) % count + 1], tt)

			-- Advance animation state:
			tt = tt + scriptableCameraSpeed * getDeltaTime()
			if(tt >= 1.0) then
				tt = 0.0
				rot = rot + 1
			end
		end
		
		-- Wait for update() tick from Engine
		update()
		
	end
end)