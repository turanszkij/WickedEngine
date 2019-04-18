-- Lua Fighting game sample script
--
-- README:
-- The script is programmable using common fighting game "numpad notations" (read this if you are unfamiliar: http://www.dustloop.com/wiki/index.php/Notation )
-- There are four action buttons: A, B, C, D
--	So for example a forward motion combined with action D would look like this in code: "6D" 
--	A D action without motion (neutral D) would be: "5D"
--	A quarter circle forward + A would be "236A"
--	"Shoryuken" + A command would be: "623A"
--	For a full circle motion, the input would be: "23698741"
--		But because that full circle motion is difficult to execute properly, we can make it easier by accpeting similar inputs, like:
--			"2684" or "2369874"...

local scene = GetScene()

-- The character "class" is a wrapper function that returns a local internal table called "self"
local function Character(face, shirt_color)
	local self = {
		model = INVALID_ENTITY,
		face = 1, -- face direction (X)
		request_face = 1,
		position = Vector(),
		velocity = Vector(),
		force = Vector(),
		frame = 0,
		input_buffer = {},

		-- Common requirement conditions for state transitions:
		require_input_window = function(self, inputString, window) -- player input notation with some tolerance to input execution window (in frames) (help: see readme on top of this file)
			-- reduce remaining input with non-expired commands:
			local remaining_input = inputString
			for i,element in ipairs(self.input_buffer) do
				if(element.age <= window and element.command == string.sub(remaining_input, 0, string.len(element.command))) then
					remaining_input = string.sub(remaining_input, string.len(element.command) + 1)
					if(string.len(remaining_input) == 0) then
						return true
					end
				end
			end
			return false -- match failure
		end,
		require_input = function(self, inputString) -- player input notation (immediate) (help: see readme on top of this file)
			return self:require_input_window(inputString, 0)
		end,
		require_frame = function(self, frame) -- specific frame
			return self.frame == frame
		end,
		require_window = function(self, frameStart,  frameEnd) -- frame window range
			return self.frame >= frameStart and self.frame <= frameEnd
		end,
		require_animationfinish = function(self) -- animation is finished
			return scene.Component_GetAnimation(self.states[self.state].anim).IsEnded()
		end,
		
		-- Common motion helpers:
		require_motion_qcf = function(self, button)
			local window = 20
			return 
				self:require_input_window("236" .. button, window) or
				self:require_input_window("2365" .. button, window) or
				self:require_input_window("26" .. button, window) or
				self:require_input_window("265" .. button, window)
		end,
		require_motion_shoryuken = function(self, button)
			local window = 20
			return 
				self:require_input_window("623" .. button, window) or
				self:require_input_window("6235" .. button, window) or
				self:require_input_window("626" .. button, window) or
				self:require_input_window("6265" .. button, window)
		end,

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
			Dash_Backward = {
				anim_name = "BDash",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self:require_window(0,6)) then
						self.force = vector.Add(self.force, Vector(-0.08 * self.face, 0))
					end
				end,
			},
			RunStart = {
				anim_name = "RunStart",
				anim = INVALID_ENTITY,
			},
			Run = {
				anim_name = "Run",
				anim = INVALID_ENTITY,
				update = function(self)
					self.force = vector.Add(self.force, Vector(0.06 * self.face, 0))
				end,
			},
			RunEnd = {
				anim_name = "RunEnd",
				anim = INVALID_ENTITY,
			},
			Jump = {
				anim_name = "Jump",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self.frame == 0) then
						self.force = vector.Add(self.force, Vector(0, 0.8))
					end
				end,
			},
			JumpBack = {
				anim_name = "Jump",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self.frame == 0) then
						self.force = vector.Add(self.force, Vector(-0.2 * self.face, 0.8))
					end
				end,
			},
			JumpForward = {
				anim_name = "Jump",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self.frame == 0) then
						self.force = vector.Add(self.force, Vector(0.2 * self.face, 0.8))
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
			Turn = {
				anim_name = "Turn",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self.frame == 0) then
						self.face = self.request_face
					end
				end,
			},
			
			LightPunch = {
				anim_name = "LightPunch",
				anim = INVALID_ENTITY,
				looped = false,
			},
			ForwardLightPunch = {
				anim_name = "FLightPunch",
				anim = INVALID_ENTITY,
				looped = false,
			},
			HeavyPunch = {
				anim_name = "HeavyPunch",
				anim = INVALID_ENTITY,
				looped = false,
			},
			LowPunch = {
				anim_name = "LowPunch",
				anim = INVALID_ENTITY,
				looped = false,
			},
			LightKick = {
				anim_name = "LightKick",
				anim = INVALID_ENTITY,
				looped = false,
			},
			HeavyKick = {
				anim_name = "HeavyKick",
				anim = INVALID_ENTITY,
				looped = false,
			},
			LowKick = {
				anim_name = "LowKick",
				anim = INVALID_ENTITY,
				looped = false,
			},
			Uppercut = {
				anim_name = "Uppercut",
				anim = INVALID_ENTITY,
				looped = false,
			},
			SpearJaunt = {
				anim_name = "SpearJaunt",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self:require_frame(16)) then
						self.force = vector.Add(self.force, Vector(1.3 * self.face))
					end
				end,
			},
			Shoryuken = {
				anim_name = "Shoryuken",
				anim = INVALID_ENTITY,
				looped = false,
				update = function(self)
					if(self:require_frame(0)) then
						self.force = vector.Add(self.force, Vector(0.3 * self.face, 0.9))
					end
				end,
			},
		},

		-- State machine describes all possible state transitions:
		--	StateFrom = {
		--		{ "StateTo1", condition = function(self) return [requirements that should be met] end },
		--		{ "StateTo2", condition = function(self) return [requirements that should be met] end },
		--	}
		statemachine = {
			Idle = { 
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("D") end, },
				{ "SpearJaunt", condition = function(self) return self:require_motion_qcf("D") end, },
				{ "Turn", condition = function(self) return self.request_face ~= self.face and self:require_input("5") end, },
				{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
				{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
				{ "Jump", condition = function(self) return self:require_input("8") end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
			},
			Walk_Backward = { 
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("D") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
				{ "Dash_Backward", condition = function(self) return self:require_input_window("454",10) end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "Idle", condition = function(self) return self:require_input("5") end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("6C") end, },
			},
			Walk_Forward = { 
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("D") end, },
				{ "SpearJaunt", condition = function(self) return self:require_motion_qcf("D") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
				{ "RunStart", condition = function(self) return self:require_input_window("656", 10) end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "Idle", condition = function(self) return self:require_input("5") end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("6C") end, },
			},
			Dash_Backward = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			RunStart = { 
				{ "Run", condition = function(self) return self:require_animationfinish() end, },
			},
			Run = { 
				{ "RunEnd", condition = function(self) return self:require_input("5") end, },
				{ "Jump", condition = function(self) return self:require_input("8") end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
			},
			RunEnd = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
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
				{ "Fall", condition = function(self) return self:require_animationfinish() end, },
			},
			Fall = { 
				{ "FallEnd", condition = function(self) return self.position.GetY() <= 0.5 end, },
			},
			FallEnd = { 
				{ "Idle", condition = function(self) return self.position.GetY() <= 0 and self:require_animationfinish() end, },
			},
			CrouchStart = { 
				{ "Idle", condition = function(self) return self:require_input("5") end, },
				{ "Crouch", condition = function(self) return (self:require_input("1") or self:require_input("2") or self:require_input("3")) and self:require_animationfinish() end, },
			},
			Crouch = { 
				{ "CrouchEnd", condition = function(self) return self:require_input("5") or self:require_input("4") or self:require_input("6") or self:require_input("7") or self:require_input("8") or self:require_input("9") end, },
				{ "LowPunch", condition = function(self) return self:require_input("2A") or self:require_input("1A") or self:require_input("3A") end, },
				{ "LowKick", condition = function(self) return self:require_input("2C") or self:require_input("1C") or self:require_input("3C") end, },
				{ "Uppercut", condition = function(self) return self:require_input("2B") or self:require_input("1B") or self:require_input("3B") end, },
			},
			CrouchEnd = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Turn = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			LightPunch = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			ForwardLightPunch = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			HeavyPunch = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			LowPunch = { 
				{ "Crouch", condition = function(self) return self:require_animationfinish() end, },
			},
			LightKick = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			HeavyKick = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			LowKick = { 
				{ "Crouch", condition = function(self) return self:require_animationfinish() end, },
			},
			Uppercut = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			SpearJaunt = { 
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Shoryuken = { 
				{ "FallStart", condition = function(self) return self:require_animationfinish() end, },
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
		-- Execute the currently active state:
		ExecuteCurrentState = function(self)
			local current_state = self.states[self.state]
			if(current_state ~= nil) then
				if(current_state.update ~= nil) then
					current_state.update(self)
				end
			end
		end,
	

		Create = function(self, face, shirt_color)

			-- Load the model into a custom scene:
			--	We use a custom scene because if two models are loaded into the global scene, they will have name collisions
			--	and thus we couldn't properly query entities by name
			local model_scene = Scene()
			self.model = LoadModel(model_scene, "../models/havoc/havoc.wiscene")

			-- Place model according to starting facing direction:
			self.face = face
			self.request_face = face
			self.position = Vector(self.face * -4)

			-- Set shirt color todifferentiate between characters:
			local shirt_material_entity = model_scene.Entity_FindByName("material_shirt")
			model_scene.Component_GetMaterial(shirt_material_entity).SetBaseColor(shirt_color)
		
			-- Initialize states:
			for i,state in pairs(self.states) do
				state.anim = model_scene.Entity_FindByName(state.anim_name)
				if(state.looped ~= nil) then
					model_scene.Component_GetAnimation(state.anim).SetLooped(state.looped)
				end
			end

			-- Move the custom scene into the global scene:
			scene.Merge(model_scene)

			self:StartState(self.state)

		end,
	
		AI = function(self)
			-- todo some AI bot behaviour
			table.insert(self.input_buffer, {age = 0, command = "5"})
		end,

		Input = function(self)

			-- read input (todo gamepad):
			local left = input.Down(string.byte('A'))
			local right = input.Down(string.byte('D'))
			local up = input.Down(string.byte('W'))
			local down = input.Down(string.byte('S'))
			local A = input.Press(VK_RIGHT)
			local B = input.Press(VK_UP)
			local C = input.Press(VK_LEFT)
			local D = input.Press(VK_DOWN)

			-- swap left and right if facing is opposite side:
			if(self.face < 0) then
				local tmp = right
				right = left
				left = tmp
			end

			if(up and left) then
				table.insert(self.input_buffer, {age = 0, command = "7"})
			elseif(up and right) then
				table.insert(self.input_buffer, {age = 0, command = "9"})
			elseif(up) then
				table.insert(self.input_buffer, {age = 0, command = "8"})
			elseif(down and left) then
				table.insert(self.input_buffer, {age = 0, command = "1"})
			elseif(down and right) then
				table.insert(self.input_buffer, {age = 0, command = "3"})
			elseif(down) then
				table.insert(self.input_buffer, {age = 0, command = "2"})
			elseif(left) then
				table.insert(self.input_buffer, {age = 0, command = "4"})
			elseif(right) then
				table.insert(self.input_buffer, {age = 0, command = "6"})
			else
				table.insert(self.input_buffer, {age = 0, command = "5"})
			end
			
			if(A) then
				table.insert(self.input_buffer, {age = 0, command = "A"})
			end
			if(B) then
				table.insert(self.input_buffer, {age = 0, command = "B"})
			end
			if(C) then
				table.insert(self.input_buffer, {age = 0, command = "C"})
			end
			if(D) then
				table.insert(self.input_buffer, {age = 0, command = "D"})
			end



		end,

		Update = function(self)
			self.frame = self.frame + 1

			self:StepStateMachine()
			self:ExecuteCurrentState()

			-- Manage input buffer:
			for i,element in pairs(self.input_buffer) do -- every input gets older by one frame
				element.age = element.age + 1
			end
			if(#self.input_buffer > 60) then -- only keep the last 60 inputs
				table.remove(self.input_buffer, 1)
			end
		
			-- force from gravity:
			self.force = vector.Add(self.force, Vector(0,-0.04,0))

			-- apply force:
			self.velocity = vector.Add(self.velocity, self.force)
			self.force = Vector()

			-- aerial drag:
			self.velocity = vector.Multiply(self.velocity, 0.98)
		
			-- apply velocity:
			self.position = vector.Add(self.position, self.velocity)
		
			-- check if we are below or on the ground:
			if(self.position.GetY() <= 0 and self.velocity.GetY()<=0) then
				self.position.SetY(0) -- snap to ground
				self.velocity.SetY(0) -- don't fall below ground
				self.velocity = vector.Multiply(self.velocity, 0.8) -- ground drag:
			end
			
			-- Transform component gets set as absolute coordinates every frame:
			local model_transform = scene.Component_GetTransform(self.model)
			model_transform.ClearTransform()
			model_transform.Translate(self.position)
			model_transform.Rotate(Vector(0, 3.1415 * ((self.face - 1) * 0.5)))
			model_transform.UpdateTransform()

			-- Some debug draw:
			DrawPoint(model_transform.GetPosition(), 0.1, Vector(1,0,0,1))
			DrawLine(model_transform.GetPosition(),model_transform.GetPosition():Add(self.velocity), Vector(0,1,0,1))
			DrawLine(model_transform.GetPosition(),model_transform.GetPosition():Add(Vector(self.face)), Vector(0,0,1,1))
		
		end

	}

	self:Create(face, shirt_color)
	return self
end


-- script camera state:
local camera_position = Vector()
local camera_transform = TransformComponent()

-- Interaction between two characters:
local ResolveCharacters = function(player1, player2)

	-- Facing direction requests:
	if(player1.position.GetX() < player2.position.GetX()) then
		player1.request_face = 1
		player2.request_face = -1
	else
		player1.request_face = -1
		player2.request_face = 1
	end

	-- Camera:
	local CAMERA_HEIGHT = 4 -- camera height from ground
	local DEFAULT_CAMERADISTANCE = -9.5 -- the default camera distance when characters are close to each other
	local MODIFIED_CAMERADISTANCE = -11.5 -- if the two players are far enough from each other, the camera will zoom out to this distance
	local CAMERA_DISTANCE_MODIFIER = 10 -- the required distance between the characters when the camera should zoom out
	local XBOUNDS = 20 -- play area horizontal bounds
	local CAMERA_SIDE_LENGTH = 11 -- play area inside the camera (character can't move outside camera even if inside the play area)

	-- Clamp the players inside the camera:
	local camera_side_left = camera_position.GetX() - CAMERA_SIDE_LENGTH
	local camera_side_right = camera_position.GetX() + CAMERA_SIDE_LENGTH
	player1.position.SetX(math.clamp(player1.position.GetX(), camera_side_left, camera_side_right))
	player2.position.SetX(math.clamp(player2.position.GetX(), camera_side_left, camera_side_right))
	
	local camera_position_new = Vector()
	local distanceX = math.abs(player1.position.GetX() - player2.position.GetX())
	local distanceY = math.abs(player1.position.GetY() - player2.position.GetY())

	-- camera height:
	if(player1.position.GetY() > 4 or player2.position.GetY() > 4) then
		camera_position_new.SetY( math.min(player1.position.GetY(), player2.position.GetY()) + distanceY )
	else
		camera_position_new.SetY(CAMERA_HEIGHT)
	end

	-- camera distance:
	if(distanceX > CAMERA_DISTANCE_MODIFIER) then
		camera_position_new.SetZ(MODIFIED_CAMERADISTANCE)
	else
		camera_position_new.SetZ(DEFAULT_CAMERADISTANCE)
	end

	-- camera horizontal position:
	local centerX = math.clamp((player1.position.GetX() + player2.position.GetX()) * 0.5, -XBOUNDS, XBOUNDS)
	camera_position_new.SetX(centerX)

	-- smooth camera:
	camera_position = vector.Lerp(camera_position, camera_position_new, 0.1)

	-- finally update the global camera with current values:
	camera_transform.ClearTransform()
	camera_transform.Translate(camera_position)
	camera_transform.UpdateTransform()
	GetCamera().TransformCamera(camera_transform)

end

-- Main loop:
runProcess(function()

	ClearWorld()
	
	-- Fighting game needs stable frame rate and deterministic controls at all times. We will also refer to frames in this script instead of time units.
	--	We lock the framerate to 60 FPS, so if frame rate goes below, game will play slower
	--	
	--	There is also the possibility to implement game logic in fixed_update() instead, but that is not common for fighting games
	main.SetTargetFrameRate(60)
	main.SetFrameRateLock(true)

	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = main.GetActivePath()
	local path = RenderPath3D_TiledForward()
	main.SetActivePath(path)

	local help_text = ""
	help_text = help_text .. "This script is showcasing how to write a simple fighting game."
	help_text = help_text .. "\nControls:\n#####################\nESCAPE key: quit\nR: reload script"
	help_text = help_text .. "\nWASD: move"
	help_text = help_text .. "\nRight: action A"
	help_text = help_text .. "\nUp: action B"
	help_text = help_text .. "\nLeft: action C"
	help_text = help_text .. "\nDown: action D"
	help_text = help_text .. "\n\nMovelist:"
	help_text = help_text .. "\n\t A : Light Punch"
	help_text = help_text .. "\n\t B : Heavy Punch"
	help_text = help_text .. "\n\t C : Light Kick"
	help_text = help_text .. "\n\t 6A : Forward Light Punch"
	help_text = help_text .. "\n\t 6C : Heavy Kick"
	help_text = help_text .. "\n\t 2A : Low Punch"
	help_text = help_text .. "\n\t 2B : Uppercut"
	help_text = help_text .. "\n\t 2C : Low Kick"
	help_text = help_text .. "\n\t 623D: Shoryuken"
	help_text = help_text .. "\n\t 236D: Jaunt"
	local font = Font(help_text);
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

	LoadModel("../models/dojo.wiscene")
	
	-- Create the two player characters. Parameters are facing direction and shirt material color to differentiate between them:
	local player1 = Character(1, Vector(1,1,1,1)) -- facing to right, white shirt
	local player2 = Character(-1, Vector(1,0,0,1)) -- facing to left, red shirt
	
	while true do
		
		player1:Input()
		player2:AI()

		player1:Update()
		player2:Update()

		ResolveCharacters(player1, player2)

		local inputString = "input: "
		for i,element in ipairs(player1.input_buffer) do
			if(element.command ~= "5") then
				inputString = inputString .. element.command
			end
		end
		info.SetText(inputString .. "\nstate = " .. player1.state .. "\nframe = " .. player1.frame)
		
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

