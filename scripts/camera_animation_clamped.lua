-- This script will play a camera animation chain from "cam0" -to "camN" named camera proxies in the scene
--  To use this, first place four cameras into the scene and name them cam0, cam1, cam2 and cam3, then press F8 to start
--	The animation will repeat infinitely, but it will cut from last to first proxy at the end

-- Get the main camera:
local cam = GetCamera()

-- This will be the transform that we grab the camera by:
local target = TransformComponent()

-- Get the main scene:
local scene = GetScene()

-- Camera speed overridable from outer scope too:
scriptableCameraSpeed = 0.4

-- Animation state:
local tt = 0.0
local play = false
local rot = 0
ToggleCameraAnimation = function()
	tt = 0.0
	play = not play
	rot = 0
end

-- Gather camera proxy entities in the scene from "cam0" to "cam1", "cam2", ... "camN":
local proxies={}
local it = 0
while true do
	local entity = scene.Entity_FindByName("cam" .. it)
	if(entity == INVALID_ENTITY) then
		break
	end
	it = it + 1
	proxies[it] = entity
end

runProcess(function()
	while true do

		if(input.Press(KEYBOARD_BUTTON_F8)) then
			ToggleCameraAnimation()
		end
		if(play) then
			-- Play animation:
			local count = len(proxies)
			
			-- Place main camera on spline:
			local a = scene.Component_GetTransform(proxies[math.clamp(rot - 1, 0, count - 1) + 1])
			local b = scene.Component_GetTransform(proxies[math.clamp(rot, 0, count - 1) + 1])
			local c = scene.Component_GetTransform(proxies[math.clamp(rot + 1, 0, count - 1) + 1])
			local d = scene.Component_GetTransform(proxies[math.clamp(rot + 2, 0, count - 1) + 1])
			target.CatmullRom(a, b, c, d, tt)
			target.UpdateTransform()
			cam.TransformCamera(target)
			cam.UpdateCamera()

			-- Advance animation state:
			tt = tt + scriptableCameraSpeed * getDeltaTime()
			if(tt >= 1.0) then
				tt = 0.0
				rot = rot + 1
			end
			if(rot >= count - 1) then
				rot = 0
			end

		end
		
		-- Wait for render() tick from Engine
		--	We should wait for update() normally, but Editor tends to update the camera already from update()
		--		and it would overridethe scrips...
		render()
		
	end
end)