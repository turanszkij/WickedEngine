-- Lua Fighting game sample script

local scene = GetScene()

Character_Havoc = {
	model = INVALID_ENTITY,
	face = 1, -- face direction (X)
	position = Vector(),
	velocity = Vector(),
	force = Vector(),
	input_string = "",
	frame = 0,

	-- List all possible states:
	states = {
		Idle = {
			anim_name = "Idle",
			anim = INVALID_ENTITY,
		},
		Walk_Backward = {
			anim_name = "Back",
			anim = INVALID_ENTITY,
			update = function(self)
				self.force = vector.Add(self.force, Vector(-0.025 * self.face, 0))
			end,
		},
		Walk_Forward = {
			anim_name = "Forward",
			anim = INVALID_ENTITY,
			update = function(self)
				self.force = vector.Add(self.force, Vector(0.025 * self.face, 0))
			end,
		},
		Jump = {
			anim_name = "Jump",
			anim = INVALID_ENTITY,
			looped = false,
			update = function(self)
				if(self.frame == 0) then
					self.force = vector.Add(self.force, Vector(0, 0.6))
				end
			end,
		},
		JumpBack = {
			anim_name = "Jump",
			anim = INVALID_ENTITY,
			looped = false,
			update = function(self)
				if(self.frame == 0) then
					self.force = vector.Add(self.force, Vector(-0.1 * self.face, 0.6))
				end
			end,
		},
		JumpForward = {
			anim_name = "Jump",
			anim = INVALID_ENTITY,
			looped = false,
			update = function(self)
				if(self.frame == 0) then
					self.force = vector.Add(self.force, Vector(0.1 * self.face, 0.6))
				end
			end,
		},
		FallStart = {
			anim_name = "FallStart",
			anim = INVALID_ENTITY,
			looped = false,
		},
		Fall = {
			anim_name = "Fall",
			anim = INVALID_ENTITY,
		},
		FallEnd = {
			anim_name = "FallEnd",
			anim = INVALID_ENTITY,
			looped = false,
		},
		CrouchStart = {
			anim_name = "CrouchStart",
			anim = INVALID_ENTITY,
			looped = false,
		},
		Crouch = {
			anim_name = "Crouch",
			anim = INVALID_ENTITY,
		},
		CrouchEnd = {
			anim_name = "CrouchEnd",
			anim = INVALID_ENTITY,
			looped = false,
		},
	},

	-- Common requirement conditions for state transitions:
	require_input = function(self, value) -- player input notation
		return self.input_string == value
	end,
	require_window = function(self, a, b) -- frame window range
		return self.frame >= a and self.frame <= b
	end,

	-- State machine describes all possible state transitions:
	statemachine = {
		Idle = { 
			{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
			{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
			{ "Jump", condition = function(self) return self:require_input("8") end, },
			{ "JumpBack", condition = function(self) return self:require_input("7") end, },
			{ "JumpForward", condition = function(self) return self:require_input("9") end, },
			{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
		},
		Walk_Backward = { 
			{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
			{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
			{ "JumpBack", condition = function(self) return self:require_input("7") end, },
			{ "Idle", condition = function(self) return self:require_input("5") end, },
		},
		Walk_Forward = { 
			{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
			{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
			{ "JumpForward", condition = function(self) return self:require_input("9") end, },
			{ "Idle", condition = function(self) return self:require_input("5") end, },
		},
		Jump = { 
			{ "FallStart", condition = function(self) return self.velocity.GetY() <= 0 end, },
		},
		JumpForward = { 
			{ "FallStart", condition = function(self) return self.velocity.GetY() <= 0 end, },
		},
		JumpBack = { 
			{ "FallStart", condition = function(self) return self.velocity.GetY() <= 0 end, },
		},
		FallStart = { 
			{ "FallEnd", condition = function(self) return self.position.GetY() <= 0.5 end, },
			{ "Fall", condition = function(self) return self:require_window(3,3) end, },
		},
		Fall = { 
			{ "FallEnd", condition = function(self) return self.position.GetY() <= 0.5 end, },
		},
		FallEnd = { 
			{ "Idle", condition = function(self) return self.position.GetY() <= 0 and self:require_window(3,3) end, },
		},
		CrouchStart = { 
			{ "Idle", condition = function(self) return self:require_input("5") end, },
			{ "Crouch", condition = function(self) return (self:require_input("1") or self:require_input("2") or self:require_input("3")) and self:require_window(3,3) end, },
		},
		Crouch = { 
			{ "CrouchEnd", condition = function(self) return self:require_input("5") end, },
		},
		CrouchEnd = { 
			{ "Idle", condition = function(self) return self:require_window(3,3) end, },
		},
	},

	state = "Idle", -- starting state
	
	-- Ends the current state:
	EndState = function(self)
		scene.Component_GetAnimation(self.states[self.state].anim).Stop()
	end,
	-- Starts a new state:
	StartState = function(self, dst_state)
		scene.Component_GetAnimation(self.states[dst_state].anim).Play()
		self.frame = 0
		self.state = dst_state
	end,
	-- Parse state machine at current state and perform transition if applicable:
	StepStateMachine = function(self)
		local transition_candidates = self.statemachine[self.state]
		if(transition_candidates ~= nil) then
			for i,dst in pairs(transition_candidates) do
				-- check transition requirement conditions:
				local requirements_met = true
				if(dst.condition ~= nil) then
					requirements_met = dst.condition(self)
				end
				if(requirements_met) then
					-- transition to new state when all requirements are met:
					self:EndState()
					self:StartState(dst[1])
					return
				end
			end
		end
	end,
	

	Create = function(self, face, shirt_color)

		-- Load the model into a custom scene:
		--	We use a custom scene because if two models are loaded into the global scene, they will have name collisions
		--	and thus we couldn't properly query entities by name
		local model_scene = Scene()
		self.model = LoadModel(model_scene, "../models/havoc/havoc.wiscene")

		self.face = face

		local shirt_material_entity = model_scene.Entity_FindByName("material_shirt")
		model_scene.Component_GetMaterial(shirt_material_entity).SetBaseColor(shirt_color)
		
		-- Verify states by printing them to backlog:
		backlog_post("\nStates:")
		for i,state in pairs(self.states) do
			state.anim = model_scene.Entity_FindByName(state.anim_name)
			if(state.looped ~= nil) then
				model_scene.Component_GetAnimation(state.anim).SetLooped(state.looped)
			end
			backlog_post(i .. ": " .. state.anim_name .. " (" .. state.anim .. ")")
			for property,value in pairs(state) do
				backlog_post("\t" .. property)
			end
		end
		
		-- Verify state machine transitions by printing them to backlog:
		backlog_post("\nState machine:")
		for src,transition_candidates in pairs(self.statemachine) do
			-- Print source state:
			backlog_post(src .. ": ")

			-- Print destination states:
			for i,dst in pairs(transition_candidates) do
				-- Print destination state name:
				backlog_post("\t" .. i .. ": " .. dst[1])
				
				-- Print destination state requirements:
				if(dst.require ~= nil) then
					for type,values in pairs(dst.require) do
						local values_string = ""
						for i,value in ipairs(values) do
							values_string = values_string .. value .. "; "
						end
						backlog_post("\t\t" .. type .. " = {" .. values_string .. "}")
					end
				end
			end
		end

		-- Move the custom scene into the global scene:
		scene.Merge(model_scene)

		self:StartState(self.state)

	end,
	
	Update = function(self)
		local model_transform = scene.Component_GetTransform(self.model)
		self.position = model_transform.GetPosition()
		
		self.frame = self.frame + 1

		-- read input:
		local left = input.Down(VK_LEFT)
		local right = input.Down(VK_RIGHT)
		local up = input.Down(VK_UP)
		local down = input.Down(VK_DOWN)

		self.input_string = ""
		if(up and left) then
			self.input_string = "7"
		elseif(up and right) then
			self.input_string = "9"
		elseif(up) then
			self.input_string = "8"
		elseif(down and left) then
			self.input_string = "1"
		elseif(down and right) then
			self.input_string = "3"
		elseif(down) then
			self.input_string = "2"
		elseif(left) then
			self.input_string = "4"
		elseif(right) then
			self.input_string = "6"
		else
			self.input_string = "5"
		end

		self:StepStateMachine()
		
		-- Call current state's update method if there is one:
		local current_state = self.states[self.state]
		if(current_state ~= nil) then
			if(current_state.update ~= nil) then
				current_state.update(self)
			end
		end
		
		-- force from gravity:
		self.force = vector.Add(self.force, Vector(0,-0.04,0))

		-- apply force:
		self.velocity = vector.Add(self.velocity, self.force)
		self.force = Vector()

		-- aerial drag:
		self.velocity = vector.Multiply(self.velocity, 0.98)
		
		-- apply velocity:
		model_transform.Translate(self.velocity)
		model_transform.UpdateTransform()
		
		-- check if we are below or on the ground:
		local posY = model_transform.GetPosition().GetY()
		if(posY <= 0 and self.velocity.GetY()<=0) then
			model_transform.Translate(Vector(0,-posY,0)) -- snap to ground
			self.velocity.SetY(0) -- don't fall below ground
			self.velocity = vector.Multiply(self.velocity, 0.8) -- ground drag:
		end
		
	end
}

-- Player Controller
local player1 = Character_Havoc

-- Main loop:
runProcess(function()

	ClearWorld()

	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = main.GetActivePath()
	local path = RenderPath3D_TiledForward()
	main.SetActivePath(path)

	main.SetTargetFrameRate(60)
	main.SetFrameRateLock(true)

	local font = Font("This script is showcasing how to write a simple fighting game.\nControls:\n#####################\narrows: walk\nNumPad: actions\nESCAPE key: quit\nR: reload script");
	font.SetSize(20)
	font.SetPos(Vector(10, GetScreenHeight() - 10))
	font.SetAlign(WIFALIGN_LEFT, WIFALIGN_BOTTOM)
	font.SetColor(0xFFADA3FF)
	font.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(font)

	local info = Font("");
	info.SetSize(20)
	info.SetPos(Vector(GetScreenWidth() / 2, GetScreenHeight() * 0.9))
	info.SetAlign(WIFALIGN_LEFT, WIFALIGN_CENTER)
	info.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(info)
	
	player1:Create(1, Vector(1,1,1,1))
	
	while true do
		
		player1:Update()

		info.SetText("input_string = " .. player1.input_string .."\nstate = " .. player1.state .. "\nframe = " .. player1.frame)
		
		-- Wait for Engine update tick
		update()
		
	
		if(input.Press(VK_ESCAPE)) then
			-- restore previous component
			--	so if you loaded this script from the editor, you can go back to the editor with ESC
			backlog_post("EXIT")
			killProcesses()
			main.SetActivePath(prevPath)
			return
		end
		if(input.Press(string.byte('R'))) then
			-- reload script
			backlog_post("RELOAD")
			killProcesses()
			main.SetActivePath(prevPath)
			dofile("fighting_game.lua")
			return
		end
		
	end
end)



-- Draw
runProcess(function()
	
	while true do

		-- Do some debug draw geometry:
	
		-- If backlog is opened, skip debug draw:
		while(backlog_isactive()) do
			waitSeconds(1)
		end
		
		local model_transform = scene.Component_GetTransform(player1.model)
		
		DrawPoint(model_transform.GetPosition(), 0.1, Vector(1,0,0,1))
		DrawLine(model_transform.GetPosition(),model_transform.GetPosition():Add(player1.velocity), Vector(0,1,0,1))
		DrawLine(model_transform.GetPosition(),model_transform.GetPosition():Add(Vector(player1.face)), Vector(0,0,1,1))
		
		
		-- Wait for the engine to render the scene
		render()
	end
end)

