-- s = Sprite("HelloWorldDemo/HelloWorld.png");
-- s.SetEffects(ImageEffects(100,-100,400,400));
-- f = Font("Hello World Font renderer");
-- f.SetPos(Vector(20,-150));

-- runProcess(function()
-- 	local isserver = false
-- 	local isclient = false
-- 	while true do
-- 		
-- 		if(input.Press(string.byte('S'))) then
-- 			isserver = true
-- 			server = Server()
-- 		end
-- 		if(input.Press(string.byte('C'))) then
-- 			isclient = true
-- 			client = Client()
-- 		end
-- 		
-- 		if(isserver) then
-- 			server.Poll()
-- 		end
-- 		if(isclient) then
-- 			client.Poll()
-- 		end
-- 		
-- 		update()
-- 	end
-- end)

local tt = 0.0
local play = false
local rot = 0

main.SetResolutionDisplay(true);
runProcess(function()
	while true do
		
		if(input.Press(VK_F11)) then
			ReloadShaders("C:\\PROJECTS\\WickedEngine\\WickedEngine\\shaders\\")
		end
		
		if(input.Press(VK_F10)) then
			s = SoundEffect("C:\\PROJECTS\\DXProject\\DXProject\\sound\\1.wav")
			s.Play()
			--PutEnvProbe(GetCamera().GetPosition(),256);
		end
		
		if(input.Press(VK_F4)) then
			main.GetActiveComponent().SetPreferredThreadingCount(2);
		end
		if(input.Press(VK_F5)) then
			main.GetActiveComponent().SetPreferredThreadingCount(3);
		end
		if(input.Press(VK_F6)) then
			main.GetActiveComponent().SetPreferredThreadingCount(4);
		end

		if(input.Press(VK_F7)) then
			-- LoadModel("C:\\PROJECTS\\WickedEngine\\WickedEngine\\Scene\\Sponza\\","sponza");
			runProcess(function()
				local row = 16
				for i = 1, row do
					for j = 1, row do
						LoadModel("C:\\PROJECTS\\BLENDER\\Stormtrooper\\Stormtrooper.wimf", "_"..i..j,matrix.Translation(Vector(i*2-row,0,j*2-row)));
						--waitSeconds(0.05)
					end
				end
			end)
		end

		if(input.Press(VK_F8)) then
			tt = 0.0
			play = true
		end
		if(play) then
			local prox = {}
			prox[0] = GetCamera("cam0")
			prox[1] = GetCamera("cam1")
			prox[2] = GetCamera("cam2")
			prox[3] = GetCamera("cam3")
			local cam = GetCamera()
			
			cam.CatmullRom(prox[(rot + 0) % 4], prox[(rot + 1) % 4], prox[(rot + 2) % 4], prox[(rot + 3) % 4], tt)

			tt = tt + 0.4 * 0.016
			if(tt >= 1.0) then
				tt = 0.0
				rot = rot + 1
			end
		end

		
		update()
	end
end)