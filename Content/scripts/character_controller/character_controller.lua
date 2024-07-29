-- Lua Third person camera and character controller script
--	This script will load a simple scene with a character that can be controlled
--
-- 	CONTROLS:
--		WASD/left thumbstick: walk
--		SHIFT/right shoulder button: walk -> jog
--		E/left shoulder button: jog -> run
--		SPACE/gamepad X/gamepad button 3: jump
--		Right Mouse Button/Right thumbstick: rotate camera
--		Scoll middle mouse/Left-Right triggers: adjust camera distance
--		H: toggle debug draw
--		L: toggle framerate lock
--		ESCAPE key: quit
--		R: reload script
--		ENTER: interaction

-- Debug Draw Helper
local DrawAxis = function(point,f)
	DrawLine(point,point:Add(Vector(f,0,0)),Vector(1,0,0,1))
	DrawLine(point,point:Add(Vector(0,f,0)),Vector(0,1,0,1))
	DrawLine(point,point:Add(Vector(0,0,f)),Vector(0,0,1,1))
end

local debug = false -- press H to toggle
local allow_pushaway_NPC = true -- You can decide whether NPCs can be pushed away by player
local framerate_lock = false
local framerate_lock_target = 20
local slope_threshold = 0.2 -- How much slopeiness will cause character to slide down instead of standing on it
local gravity = -30
local dynamic_voxelization = false -- Set to true to revoxelize navigation every frame

local ConversationState = {
	Disabled = 0,
	Talking = 1,
	Waiting = 2,
}
local function Conversation()
	local self = {
		state = ConversationState.Disabled,
		percent = 0,
		character = nil,
		font = SpriteFont(),
		advance_font = SpriteFont(""),
		choice_fonts = {},
		font_blink_timer = 0,
		dialog = {},
		speed = 30,
		choice = 1,
		override_input = false,

		Update = function(self, path, scene, player)

			local crop_height = GetScreenHeight() * 0.18
			local text_offset = GetScreenWidth() * 0.2
			self.font.SetPos(Vector(text_offset, GetScreenHeight() - crop_height + 10))
			self.font.SetHorizontalWrapping(GetScreenWidth() - self.font.GetPos().GetX() * 2)
			self.advance_font.SetPos(Vector(GetScreenWidth() - self.font.GetPos().GetX(), GetScreenHeight() - 50))

			-- Update conversation percentage (fade in/out of conversation)
			if self.state == ConversationState.Disabled then
				self.percent = math.lerp(self.percent, 0, getDeltaTime() * 4)
			else
				self.percent = math.lerp(self.percent, 1, getDeltaTime() * 4)
			end
			path.SetCropTop(self.percent * crop_height)
			path.SetCropBottom(self.percent * crop_height)
			local cam = GetCamera()
			cam.SetApertureSize(self.percent)

			if self.percent < 0.9 then
				self.font.SetHidden(true)
				self.font.ResetTypewriter()
			else
				self.font.SetHidden(false)
			end
			self.advance_font.SetColor(Vector())
			self.override_input = false
			for i,choice_font in ipairs(self.choice_fonts) do
				choice_font.SetColor(Vector())
			end
			self.font_blink_timer = self.font_blink_timer + getDeltaTime()

			-- Focus on character:
			if self.character ~= nil then
				cam.SetFocalLength(vector.Subtract(scene.Component_GetTransform(self.character.head).GetPosition(), cam.GetPosition()).Length())
			end

			-- State flow:
			if self.state == ConversationState.Disabled then
				self.font.SetHidden(true)
			elseif self.state == ConversationState.Talking then

				self.choice = 1
				self.override_input = true
				self:CinematicCamera(self.character, player, scene, cam)

				-- End of talking:
				if self.font.IsTypewriterFinished() then
					self.state = ConversationState.Waiting
					self.font_blink_timer = 0
				end

				-- Skip talking:
				if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) then
					self.font.TypewriterFinish()
				end

				-- Turn on talking animation:
				if self.character.expression ~= INVALID_ENTITY and self.percent >= 0.9 then
					scene.Component_GetExpression(self.character.expression).SetForceTalkingEnabled(true)
				end

			elseif self.state == ConversationState.Waiting then

				-- End of talking, waiting for input:
				
				if self.dialog.choices ~= nil then
					-- Dialog choices:
					self.override_input = true
					self:CinematicCamera(player, self.character, scene, cam)

					local pos = vector.Add(self.font.GetPos(), Vector(20, 10 + self.font.TextSize().GetY()))
					for i,choice in ipairs(self.dialog.choices) do
						if self.choice_fonts[i] == nil then
							self.choice_fonts[i] = SpriteFont()
							path.AddFont(self.choice_fonts[i])
						end
						self.choice_fonts[i].SetPos(pos)
						self.choice_fonts[i].SetSize(self.font.GetSize())
						self.choice_fonts[i].SetHorizontalWrapping(GetScreenWidth() - self.choice_fonts[i].GetPos().GetX() * 2)
						if i == self.choice then
							self.choice_fonts[i].SetText(" " .. choice[1])
							self.choice_fonts[i].SetColor(vector.Lerp(Vector(1,1,1,1), self.font.GetColor(), math.abs(math.sin(self.font_blink_timer * math.pi))))
						else
							self.choice_fonts[i].SetText("     " .. choice[1])
							self.choice_fonts[i].SetColor(self.font.GetColor())
						end
						pos = vector.Add(pos, Vector(0, self.choice_fonts[i].TextSize().GetY() + 5))
					end

					-- Dialog input:
					if input.Press(KEYBOARD_BUTTON_UP) or input.Press(string.byte('W')) or input.Press(GAMEPAD_BUTTON_UP) then
						self.choice = self.choice - 1
						self.font_blink_timer = 0
					end
					if input.Press(KEYBOARD_BUTTON_DOWN) or input.Press(string.byte('S')) or input.Press(GAMEPAD_BUTTON_DOWN) then
						self.choice = self.choice + 1
						self.font_blink_timer = 0
					end
					if self.choice < 1 then
						self.choice = #self.dialog.choices
					end
					if self.choice > #self.dialog.choices then
						self.choice = 1
					end
					if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) then
						if self.dialog.choices[self.choice].action ~= nil then
							self.dialog.choices[self.choice].action() -- execute dialog choice action
						end
						self:Next()
					end
				else
					-- No dialog choices:
					self.override_input = true
					self:CinematicCamera(self.character, player, scene, cam)

					if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) then
						if self.dialog.action_after ~= nil then
							self.dialog.action_after() -- execute dialog action after ended
						end
						self:Next()
						self.font.ResetTypewriter()
					end
					-- Blinking advance conversation icon:
					self.advance_font.SetColor(vector.Lerp(Vector(0,0,0,0), self.font.GetColor(), math.abs(math.sin(self.font_blink_timer * math.pi))))	
				end

				-- Turn off talking animation:
				if self.character.expression ~= INVALID_ENTITY then
					scene.Component_GetExpression(self.character.expression).SetForceTalkingEnabled(false)
				end
			end

		end,
		
		Enter = function(self, character)
			backlog_post("Entering conversation")
			self.state = ConversationState.Talking
			self.character = character
			if #self.character.dialogs < self.character.next_dialog then
				self.character.next_dialog = 1
			end
			self:Next()
		end,
	
		Exit = function(self)
			backlog_post("Exiting conversation")
			self.state = ConversationState.Disabled
			self.dialog = {}
			self.override_input = false
			self.font.ResetTypewriter()
		end,

		Next = function(self)
			if #self.character.dialogs < self.character.next_dialog then
				self:Exit()
			end
			if self.state == ConversationState.Disabled then
				return
			end
			self.dialog = self.character.dialogs[self.character.next_dialog]
			self.character.next_dialog = self.character.next_dialog + 1
			self.state = ConversationState.Talking
			self.font.SetText(self.dialog[1])
			self.font.SetTypewriterTime(string.len(self.dialog[1] or "") / self.speed)
			self.font.ResetTypewriter()
			if self.dialog.action ~= nil then
				self.dialog.action() -- execute dialog action
			end
		end,

		CinematicCamera = function(self, character_foreground, character_background, scene, camera)
			local head_pos = scene.Component_GetTransform(character_foreground.head).GetPosition()
			local forward_dir = vector.Subtract(character_background.position, character_foreground.position).Normalize()
			forward_dir = vector.TransformNormal(forward_dir, matrix.RotationY(math.pi * 0.1))
			local up_dir = Vector(0,1,0)
			local right_dir = vector.Cross(up_dir, forward_dir).Normalize()
			local cam_pos = vector.Add(head_pos, forward_dir)
			local cam_target = vector.Add(vector.Add(head_pos, vector.Multiply(right_dir, -0.4)), Vector(0,-0.1))
			local lookat = matrix.Inverse(matrix.LookAt(cam_pos, cam_target))
			camera.TransformCamera(lookat)
			camera.UpdateCamera()
			camera.SetFocalLength(vector.Subtract(head_pos, camera.GetPosition()).Length())
		end
	}
	self.font.SetSize(20)
	self.font.SetColor(Vector(0.6,0.8,1,1))
	self.advance_font.SetSize(24)
	local sound = Sound()
	local soundinstance = SoundInstance()
	audio.CreateSound(script_dir() .. "assets/typewriter.wav", sound)
	audio.CreateSoundInstance(sound, soundinstance)
	audio.SetVolume(0.2, soundinstance)
	self.font.SetTypewriterSound(sound, soundinstance)
	return self
end
local conversation = Conversation()

local scene = GetScene()

local Layers = {
	Player = 1 << 0,
	NPC = 1 << 1,
}
local States = {
	IDLE = "idle",
	WALK = "walk",
	JOG = "jog",
	RUN = "run",
	JUMP = "jump",
	SWIM_IDLE = "swim_idle",
	SWIM = "swim",
	DANCE = "dance",
	WAVE = "wave"
}

local Mood = {
	Neutral = ExpressionPreset.Neutral,
	Happy = ExpressionPreset.Happy,
	Angry = ExpressionPreset.Angry,
	Sad = ExpressionPreset.Sad,
	Relaxed = ExpressionPreset.Relaxed,
	Surprised = ExpressionPreset.Surprised
}

local enable_footprints = true
local footprint_texture = Texture(script_dir() .. "assets/footprint.dds")
local footprints = {}
	
local animations = {}
local function LoadAnimations(anim_scene)
	animations = {
		IDLE = anim_scene.Entity_FindByName("idle"),
		WALK = anim_scene.Entity_FindByName("walk"),
		JOG = anim_scene.Entity_FindByName("jog"),
		RUN = anim_scene.Entity_FindByName("run"),
		JUMP = anim_scene.Entity_FindByName("jump"),
		SWIM_IDLE = anim_scene.Entity_FindByName("swim_idle"),
		SWIM = anim_scene.Entity_FindByName("swim"),
		DANCE = anim_scene.Entity_FindByName("dance"),
		WAVE = anim_scene.Entity_FindByName("wave"),
	}
end

local character_capsules = {}
local voxelgrid = VoxelGrid(128,32,128)
voxelgrid.SetVoxelSize(0.25)
voxelgrid.SetCenter(Vector(0,0.1,0))

local function Character(model_scene, start_transform, controllable, anim_scene)
	local self = {
		model = INVALID_ENTITY,
		target_rot_horizontal = 0,
		target_rot_vertical = 0,
		target_height = 0,
		anims = {},
		anim_amount = 1,
		neck = INVALID_ENTITY,
		head = INVALID_ENTITY,
		left_hand = INVALID_ENTITY,
		right_hand = INVALID_ENTITY,
		left_foot = INVALID_ENTITY,
		right_foot = INVALID_ENTITY,
		left_toes = INVALID_ENTITY,
		right_toes = INVALID_ENTITY,
		face = Vector(0,0,1), -- forward direction (smoothed)
		face_next = Vector(0,0,1), -- forward direction in current frame
		movement_velocity = Vector(),
		velocity = Vector(),
		leaning_next = 0, -- leaning sideways when turning
		leaning = 0, -- leaning sideways when turning (smoothed)
		savedPointerPos = Vector(),
		walk_speed = 0.1,
		jog_speed = 0.2,
		run_speed = 0.4,
		jump_speed = 8,
		swim_speed = 0.5,
		layerMask = ~0, -- layerMask will be used to filter collisions
		scale = Vector(1, 1, 1),
		rotation = Vector(0,math.pi,0),
		position = Vector(),
        ground_intersect = false,
		controllable = true,
		fixed_update_remain = 0,
		timestep_occured = false,
		root_offset = 0,
		foot_placed_left = false,
		foot_placed_right = false,
		mood = Mood.Neutral,
		mood_amount = 1,
		expression = INVALID_ENTITY,

		pathquery = PathQuery(),
		patrol_waypoints = {},
		patrol_next = 0,
		patrol_wait = 0,
		patrol_reached = false,
		hired = nil,
		dialogs = {},
		next_dialog = 1,
		
		Create = function(self, model_scene, start_transform, controllable, anim_scene)
			self.position = start_transform.GetPosition()
			self.face = vector.Rotate(start_transform.GetForward(), vector.QuaternionFromRollPitchYaw(self.rotation))
			self.face_next = self.face
			self.controllable = controllable
			if controllable then
				self.layerMask = Layers.Player
				self.target_rot_horizontal = vector.GetAngle(Vector(0,0,1), self.face, Vector(0,1,0)) -- only modify camera rot for player
			else
				self.layerMask = Layers.NPC
			end

			-- Note: we instantiate the model_scene into the main scene, start_transform stores a component pointer which gets invalidated after this!!
			self.model = scene.Instantiate(model_scene, true)

			local layer = scene.Component_GetLayer(self.model)
			layer.SetLayerMask(self.layerMask)

			self.state = States.IDLE
			self.state_prev = self.state

			for i,entity in ipairs(scene.Entity_GetHumanoidArray()) do
				if scene.Entity_IsDescendant(entity, self.model) then
					self.humanoid = entity
					local humanoid = scene.Component_GetHumanoid(self.humanoid)
					humanoid.SetLookAtEnabled(false)
					self.neck = humanoid.GetBoneEntity(HumanoidBone.Neck)
					self.head = humanoid.GetBoneEntity(HumanoidBone.Head)
					self.left_hand = humanoid.GetBoneEntity(HumanoidBone.LeftHand)
					self.right_hand = humanoid.GetBoneEntity(HumanoidBone.RightHand)
					self.left_foot = humanoid.GetBoneEntity(HumanoidBone.LeftFoot)
					self.right_foot = humanoid.GetBoneEntity(HumanoidBone.RightFoot)
					self.left_toes = humanoid.GetBoneEntity(HumanoidBone.LeftToes)
					self.right_toes = humanoid.GetBoneEntity(HumanoidBone.RightToes)
					break
				end
			end
			for i,entity in ipairs(scene.Entity_GetExpressionArray()) do
				if scene.Entity_IsDescendant(entity, self.model) then
					self.expression = entity

					-- Set up some overrides to avoid bad looking expression combinations:
					local expression = scene.Component_GetExpression(self.expression)
					expression.SetPresetOverrideBlink(ExpressionPreset.Happy, ExpressionOverride.Block)
					expression.SetPresetOverrideMouth(ExpressionPreset.Happy, ExpressionOverride.Blend)
					expression.SetPresetOverrideBlink(ExpressionPreset.Surprised, ExpressionOverride.Block)
				end
			end

			-- Enable wetmap for all objects of character, so it can get wet in the ocean (if the weather has it):
			for i,entity in ipairs(scene.Entity_GetObjectArray()) do
				if scene.Entity_IsDescendant(entity, self.model) then
					local object = scene.Component_GetObject(entity)
					object.SetWetmapEnabled(true)
				end
			end

			-- Create a base capsule collider for character:
			local collider = scene.Component_CreateCollider(self.model)
			self.collider = self.model
			collider.SetCPUEnabled(false)
			collider.SetGPUEnabled(true)
			collider.Shape = ColliderShape.Capsule
			collider.Radius = 0.3
			collider.Offset = Vector(0, collider.Radius, 0)
			collider.Tail = Vector(0, 1.4, 0)
			local head_transform = scene.Component_GetTransform(self.head)
			if head_transform ~= nil then
				collider.Tail = head_transform.GetPosition()
			end

			self.root = self.humanoid
			
			scene.ResetPose(self.model)
			self.anims[States.IDLE] = scene.RetargetAnimation(self.humanoid, animations.IDLE, false, anim_scene)
			self.anims[States.WALK] = scene.RetargetAnimation(self.humanoid, animations.WALK, false, anim_scene)
			self.anims[States.JOG] = scene.RetargetAnimation(self.humanoid, animations.JOG, false, anim_scene)
			self.anims[States.RUN] = scene.RetargetAnimation(self.humanoid, animations.RUN, false, anim_scene)
			self.anims[States.JUMP] = scene.RetargetAnimation(self.humanoid, animations.JUMP, false, anim_scene)
			self.anims[States.SWIM_IDLE] = scene.RetargetAnimation(self.humanoid, animations.SWIM_IDLE, false, anim_scene)
			self.anims[States.SWIM] = scene.RetargetAnimation(self.humanoid, animations.SWIM, false, anim_scene)
			self.anims[States.DANCE] = scene.RetargetAnimation(self.humanoid, animations.DANCE, false, anim_scene)
			self.anims[States.WAVE] = scene.RetargetAnimation(self.humanoid, animations.WAVE, false, anim_scene)
			
			local model_transform = scene.Component_GetTransform(self.model)
			model_transform.ClearTransform()
			model_transform.Scale(self.scale)
			model_transform.Rotate(self.rotation)
			model_transform.Translate(self.position)
			model_transform.UpdateTransform()

			self.target_height = scene.Component_GetTransform(self.neck).GetPosition().GetY()

		end,
		
		Jump = function(self,f)
			self.velocity.SetY(f)
			self.state = States.JUMP
		end,
		MoveDirection = function(self,dir)
			local rotation_matrix = matrix.Multiply(matrix.RotationY(self.target_rot_horizontal), matrix.RotationX(self.target_rot_vertical))
			dir = vector.TransformNormal(dir.Normalize(), rotation_matrix)
			dir.SetY(0)
			local dot = vector.Dot(self.face, dir)
			if(dot < 0) then
				self.face = vector.TransformNormal(self.face, matrix.RotationY(math.pi * 0.01)) -- Turn around 180 degrees easily when wanting to go backwards
			end
			self.face_next = dir
			self.face_next = self.face_next.Normalize()
			if(dot > 0) then 
				local speed = 0
				if self.state == States.WALK then
					speed = self.walk_speed
				elseif self.state == States.JOG then
					speed = self.jog_speed
				elseif self.state == States.RUN then
					speed = self.run_speed
				elseif self.state == States.SWIM then
					speed = self.swim_speed
				end
				self.movement_velocity = self.face:Multiply(Vector(speed,speed,speed))
			end
		end,

		Update = function(self)

			local dt = getDeltaTime()

			local humanoid = scene.Component_GetHumanoid(self.humanoid)
			humanoid.SetLookAtEnabled(false)

			local face_rotation = matrix.LookTo(Vector(),self.face)

			local model_transform = scene.Component_GetTransform(self.model)
			local savedPos = model_transform.GetPosition()
			model_transform.ClearTransform()
			model_transform.MatrixTransform(matrix.Inverse(face_rotation))
			model_transform.Scale(self.scale)
			model_transform.Rotate(self.rotation)
			model_transform.Translate(savedPos)
			model_transform.UpdateTransform()
			
			if self.controllable then
				-- Camera target control:

				-- read from gamepad analog stick:
				local diff = input.GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R)
				diff = vector.Multiply(diff, dt * 4)
				
				-- read from mouse:
				if(input.Down(MOUSE_BUTTON_RIGHT)) then
					local mouseDif = input.GetPointerDelta()
					mouseDif = mouseDif:Multiply(dt * 0.3)
					diff = vector.Add(diff, mouseDif)
					input.SetPointer(self.savedPointerPos)
					input.HidePointer(true)
				else
					self.savedPointerPos = input.GetPointer()
					input.HidePointer(false)
				end
				
				self.target_rot_horizontal = self.target_rot_horizontal + diff.GetX()
				self.target_rot_vertical = math.clamp(self.target_rot_vertical + diff.GetY(), -math.pi * 0.3, math.pi * 0.4) -- vertical camers limits
			end

			-- Blend to current mood, blend out other moods:
			local expression = scene.Component_GetExpression(self.expression)
			if expression ~= nil then
				for i,preset in pairs(Mood) do
					local weight = expression.GetPresetWeight(preset)
					if preset == self.mood then
						weight = math.lerp(weight, self.mood_amount, 0.1)
					else
						weight = math.lerp(weight, 0, 0.1)
					end
					expression.SetPresetWeight(preset, weight)
				end
			end
			
			-- state and animation update
			local current_anim = scene.Component_GetAnimation(self.anims[self.state])
			if current_anim ~= nil then
				-- Play current anim:
				current_anim.Play()
				if self.state_prev ~= self.state then
					-- If anim just started in this frame, reset timer to beginning:
					current_anim.SetTimer(current_anim.GetStart())
					self.state_prev = self.state
				end
				
				-- Simple state transition to idle:
				if self.state == States.JUMP then
					if current_anim.GetTimer() > current_anim.GetEnd() then
						self.state = States.IDLE
					end
				else
					if self.ground_intersect and self.state ~= States.DANCE and self.state ~= States.WAVE then
						self.state = States.IDLE
					end
				end
			end

			if dt > 0.2 then
				return -- avoid processing too large delta times to avoid instability
			end

			-- swim test:
			local swimming = false
			if self.neck ~= INVALID_ENTITY then
				local neck_pos = scene.Component_GetTransform(self.neck).GetPosition()
				local water_threshold = 0.1
				neck_pos = vector.Add(neck_pos, Vector(0, -water_threshold, 0))
				local oceanpos = scene.GetOceanPosAt(neck_pos)
				local water_distance = oceanpos.GetY() - neck_pos.GetY()
				if water_distance > 0 then
					-- Ocean determined to be above neck:
					model_transform.Translate(Vector(0,water_distance - water_threshold,0))
					model_transform.UpdateTransform()
					self.velocity.SetY(0)
					swimming = true
					self.state = States.SWIM_IDLE
				else
					-- Ray trace water mesh:
					local swim_ray = Ray(neck_pos, Vector(0,1,0), 0, 100)
					local water_entity, water_pos, water_normal, water_distance = scene.Intersects(swim_ray, FILTER_WATER)
					if water_entity ~= INVALID_ENTITY then
						model_transform.Translate(Vector(0,water_distance - water_threshold,0))
						model_transform.UpdateTransform()
						self.velocity.SetY(0)
						swimming = true
						self.state = States.SWIM_IDLE
					end
				end
			end

			if self.controllable then
				if not backlog_isactive() then
					-- Movement input:
					local lookDir = Vector()
					if(input.Down(KEYBOARD_BUTTON_LEFT) or input.Down(string.byte('A'))) then
						lookDir = lookDir:Add( Vector(-1) )
					end
					if(input.Down(KEYBOARD_BUTTON_RIGHT) or input.Down(string.byte('D'))) then
						lookDir = lookDir:Add( Vector(1) )
					end
					
					if(input.Down(KEYBOARD_BUTTON_UP) or input.Down(string.byte('W'))) then
						lookDir = lookDir:Add( Vector(0,0,1) )
					end
					if(input.Down(KEYBOARD_BUTTON_DOWN) or input.Down(string.byte('S'))) then
						lookDir = lookDir:Add( Vector(0,0,-1) )
					end

					local analog = input.GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_L)
					lookDir = vector.Add(lookDir, Vector(analog.GetX(), 0, analog.GetY()))
						
					-- Apply movement:
					if(lookDir.Length() > 0) then
						if self.state == States.SWIM_IDLE then
							self.state = States.SWIM
							self:MoveDirection(lookDir)
						elseif self.ground_intersect then
							if(input.Down(KEYBOARD_BUTTON_LSHIFT) or input.Down(GAMEPAD_BUTTON_6)) then
								if input.Down(string.byte('E')) or input.Down(GAMEPAD_BUTTON_5) then
									self.state = States.RUN
								else
									self.state = States.JOG
								end
							else
								self.state = States.WALK
							end
							self:MoveDirection(lookDir)
						end
					end
					
					if(input.Press(string.byte('J')) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_3)) then
						self:Jump(self.jump_speed)
					end
				end

			elseif not(conversation.override_input and conversation.character == self) then

				-- NPC patrol behavior:
				local patrol_count = len(self.patrol_waypoints)
				if patrol_count > 0 then
					local pos = savedPos
					local patrol = self.patrol_waypoints[self.patrol_next % patrol_count + 1]
					local patrol_transform = scene.Component_GetTransform(patrol.entity)
					if patrol_transform ~= nil then
						local patrol_pos = patrol_transform.GetPosition()
						local patrol_wait = patrol.wait or 0 -- default: 0
						local patrol_vec = vector.Subtract(patrol_pos, pos)
						patrol_vec.SetY(0)
						local distance = patrol_vec.Length()
						local patrol_dist = patrol.distance or 0.5 -- default : 0.5
						local patrol_dist_threshold = patrol.distance_threshold or 0 -- default : 0
						if distance < patrol_dist then
							-- reached patrol waypoint:
							self.patrol_reached = true
							if self.state ~= States.SWIM_IDLE then
								self.state = States.IDLE
							end
							if self.patrol_wait >= patrol_wait then
								self.patrol_next = self.patrol_next + 1
							else
								self.patrol_wait = self.patrol_wait + dt
							end
						elseif self.patrol_reached then
							--  threshold to avoid jerking between moving and reached destination:
							--	 Between distance and distance + threshold, the character will not advance towards waypoint
							--	 Useful for dynamic waypoint, such as following behaviour
							if distance > patrol_dist + patrol_dist_threshold then
								self.patrol_reached = false
							end
						else
							-- move towards patrol waypoint:
							self.pathquery.SetAgentHeight(3)
							self.pathquery.Process(pos, patrol_pos, voxelgrid)
							if self.pathquery.IsSuccessful() then
								-- If there is a valid pathfinding result for waypoint, replace heading direction by that:
								patrol_vec = vector.Subtract(self.pathquery.GetNextWaypoint(), pos)
								patrol_vec.SetY(0)
							end
							if debug then
								DrawPathQuery(self.pathquery)
							end
							self.patrol_wait = 0
							-- check if it's blocked by player collision:
							local capsule = scene.Component_GetCollider(self.collider).GetCapsule()
							local forward_offset = vector.Multiply(patrol_vec.Normalize(), 0.5)
							capsule.SetBase(vector.Add(capsule.GetBase(), forward_offset))
							capsule.SetTip(vector.Add(capsule.GetTip(), forward_offset))
							capsule.SetRadius(capsule.GetRadius() * 2)

							-- Manual check for blocked patrol:
							--	This is manually done because it is difficult to filter out current NPC among other NPCs
							--	Because NPCs can also be blocked by other NPCs but not themselves
							--	And introducing custom layer for every NPC is not very good
							local blocked = false
							for other_entity,other_capsule in pairs(character_capsules) do
								if other_entity ~= self.model then
									if capsule.Intersects(other_capsule) then
										blocked = true
									end
								end
							end
							
							if blocked then
								if debug then
									DrawDebugText("patrol blocked", vector.Add(capsule.GetTip(), Vector(0,0.1)), Vector(1,0,0,1), 0.08, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
									DrawCapsule(capsule, Vector(1,0,0,1))
								end
							else
								if self.state == States.SWIM_IDLE then
									self.state = States.SWIM
								else
									self.state = patrol.state or States.WALK -- default : WALK
								end
								self:MoveDirection(patrol_vec)
								if debug then
									DrawDebugText("patrol blocking check", vector.Add(capsule.GetTip(), Vector(0,0.1)), Vector(0,1,0,1), 0.08, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
									DrawCapsule(capsule, Vector(0,1,0,1))
								end
							end

						end
					end
				end
				
			end
			
			-- Capsule collision for character:
			local capsule = scene.Component_GetCollider(self.collider).GetCapsule()
			local original_capsulepos = model_transform.GetPosition()
			local capsulepos = original_capsulepos
			local capsuleheight = vector.Subtract(capsule.GetTip(), capsule.GetBase()).Length()
			local radius = capsule.GetRadius()
			local collision_layer = ~self.layerMask
			if not controllable and not allow_pushaway_NPC then
				-- For NPC, this makes it not pushable away by player:
				--	This works by disabling NPC's collision to player
				--	But the player can still collide with NPC and can be blocked
				collision_layer = collision_layer & ~Layers.Player
			end
			local current_anim = scene.Component_GetAnimation(self.anims[self.state])
			local platform_velocity_accumulation = Vector()
			local platform_velocity_count = 0

			-- Perform fixed timestep logic:
			self.fixed_update_remain = self.fixed_update_remain + dt
			local fixed_update_fps = 120
			local fixed_dt = 1.0 / fixed_update_fps
			self.timestep_occured = false

            if self.fixed_update_remain >= fixed_dt then
                self.ground_intersect = false
            end

			while self.fixed_update_remain >= fixed_dt do
				self.timestep_occured = true;
				self.fixed_update_remain = self.fixed_update_remain - fixed_dt
				
				if swimming then
					self.velocity = vector.Multiply(self.velocity, 0.8) -- water friction
				end
				if self.velocity.GetY() > -30 then
					self.velocity = vector.Add(self.velocity, Vector(0, gravity * fixed_dt, 0)) -- gravity
				end
				self.velocity = vector.Add(self.velocity, self.movement_velocity)

				capsulepos = vector.Add(capsulepos, vector.Multiply(self.velocity, fixed_dt))

				-- Check ground:
				capsule = Capsule(capsulepos, vector.Add(capsulepos, Vector(0, capsuleheight)), radius)
				local o2, p2, n2, depth, platform_velocity = scene.Intersects(capsule, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, collision_layer) -- scene/capsule collision
				if(o2 ~= INVALID_ENTITY) then

					if debug then
						DrawPoint(p2,0.1,Vector(1,1,0,1))
						DrawLine(p2, vector.Add(p2, n2), Vector(1,1,0,1))
					end

					local slope = vector.Dot(n2, Vector(0,1,0))
					if slope > slope_threshold then
						-- Ground intersection:
                        self.ground_intersect = true
						self.velocity = vector.Multiply(self.velocity, 0.92) -- ground friction
						capsulepos = vector.Add(capsulepos, Vector(0, depth, 0)) -- avoid sliding, instead stand upright
						platform_velocity_accumulation = vector.Add(platform_velocity_accumulation, platform_velocity)
						platform_velocity_count = platform_velocity_count + 1
					end
				end

				-- Check wall:
				capsule = Capsule(capsulepos, vector.Add(capsulepos, Vector(0, capsuleheight)), radius)
				o2, p2, n2, depth, platform_velocity = scene.Intersects(capsule, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, collision_layer) -- scene/capsule collision
				if(o2 ~= INVALID_ENTITY) then

					if debug then
						DrawPoint(p2,0.1,Vector(1,1,0,1))
						DrawLine(p2, vector.Add(p2, n2), Vector(1,1,0,1))
					end

					local slope = vector.Dot(n2, Vector(0,1,0))
					if slope <= slope_threshold then
						-- Wall intersection:
						local velocityLen = self.velocity.Length()
						local velocityNormalized = self.velocity.Normalize()
						local undesiredMotion = n2:Multiply(vector.Dot(velocityNormalized, n2))
						local desiredMotion = vector.Subtract(velocityNormalized, undesiredMotion)
						self.velocity = vector.Multiply(desiredMotion, velocityLen)
						capsulepos = vector.Add(capsulepos, vector.Multiply(n2, depth))
					end
				end
				
				-- Some other things also updated at fixed rate:
				self.face = vector.Lerp(self.face, self.face_next, 0.1) -- smooth the turning in fixed update
				self.face.SetY(0)
				self.face = self.face.Normalize()
				self.leaning_next = math.lerp(self.leaning_next, vector.TransformNormal(vector.Subtract(self.face_next, self.face), face_rotation).GetX() * Vector(self.velocity.GetX(), self.velocity.GetZ()).Length() * 0.08, 0.05)
				self.leaning = math.lerp(self.leaning, self.leaning_next, 0.05)

				-- Animation blending
				if current_anim ~= nil then
					-- Blend in current animation:
					current_anim.SetAmount(math.lerp(current_anim.GetAmount(), self.anim_amount, 0.1))
					
					-- Ease out other animations:
					for i,anim in pairs(self.anims) do
						if (anim ~= INVALID_ENTITY) and (anim ~= self.anims[self.state]) then
							local prev_anim = scene.Component_GetAnimation(anim)
							prev_anim.SetAmount(math.lerp(prev_anim.GetAmount(), 0, 0.1))
							if prev_anim.GetAmount() <= 0 then
								prev_anim.Stop()
							end
						end
					end
				end
				
			end

			if platform_velocity_count > 0 then
				capsulepos = vector.Add(capsulepos, vector.Multiply(platform_velocity_accumulation, 1.0 / platform_velocity_count)) -- apply moving platform velocity
			end

			model_transform.Translate(vector.Subtract(capsulepos, original_capsulepos)) -- transform by the capsule offset
			model_transform.Rotate(Vector(0, 0, self.leaning * math.pi))
			model_transform.UpdateTransform()

			self.movement_velocity = Vector()
			
			-- try to put water ripple:
			if Vector(self.velocity.GetX(), self.velocity.GetZ()).Length() > 0.01 and self.state ~= States.SWIM_IDLE then
				local oceanpos = scene.GetOceanPosAt(self.position)
				if self.position.GetY() < oceanpos.GetY() then
					PutWaterRipple(Vector(self.position.GetX(), oceanpos.GetY(), self.position.GetZ()))
				else
					local w,wp = scene.Intersects(capsule, FILTER_WATER)
					if w ~= INVALID_ENTITY then
						PutWaterRipple(wp)
					end
				end
			end

			character_capsules[self.model] = capsule
			self.position = model_transform.GetPosition()

			-- If camera is inside character capsule, fade out the character, otherwise fade in:
			if capsule.Intersects(GetCamera().GetPosition()) then
				for i,entity in ipairs(scene.Entity_GetObjectArray()) do
					if scene.Entity_IsDescendant(entity, self.model) then
						local object = scene.Component_GetObject(entity)
						local color = object.GetColor()
						local opacity = color.GetW()
						opacity = math.lerp(opacity, 0, 0.1)
						color.SetW(opacity)
						object.SetColor(color)
					end
				end
			else
				for i,entity in ipairs(scene.Entity_GetObjectArray()) do
					if scene.Entity_IsDescendant(entity, self.model) then
						local object = scene.Component_GetObject(entity)
						local color = object.GetColor()
						local opacity = color.GetW()
						opacity = math.lerp(opacity, 1, 0.1)
						color.SetW(opacity)
						object.SetColor(color)
					end
				end
			end
			
		end,

		Update_IK = function(self)
			-- Make sure we only update IK as often as we check for ground collisions etc.
			-- in Update. Otherwise, when the framerate is higher than the step rate, 
			-- self.velocity is unreliable
			if not self.timestep_occured then return end;

			-- IK foot placement:
			local base_y = self.position.GetY()
			local ik_foot = INVALID_ENTITY
			local ik_pos = Vector()
			-- Compute root offset:
			--	I determine which foot wants to step on lower ground, that will offset whole root downwards
			--	The other foot will be the upper foot which will be later attached an Inverse Kinematics (IK) effector
			if (self.state == States.IDLE or self.state == States.DANCE) and self.ground_intersect then
				local pos_left = scene.Component_GetTransform(self.left_foot).GetPosition()
				local pos_right = scene.Component_GetTransform(self.right_foot).GetPosition()
				local ray_left = Ray(vector.Add(pos_left, Vector(0, 1)), Vector(0, -1), 0, 1.8)
				local ray_right = Ray(vector.Add(pos_right, Vector(0, 1)), Vector(0, -1), 0, 1.8)
				-- Ray trace for both feet:
				local collision_layer =  ~(Layers.Player | Layers.NPC) -- specifies that neither NPC, nor Player can collide with feet
				local collEntity_left,collPos_left = scene.Intersects(ray_left, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, collision_layer)
				local collEntity_right,collPos_right = scene.Intersects(ray_right, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, collision_layer)
				local diff_left = 0
				local diff_right = 0
				if collEntity_left ~= INVALID_ENTITY then
					--DrawAxis(collPos_left, 0.2)
					diff_left = collPos_left.GetY() - base_y
				end
				if collEntity_right ~= INVALID_ENTITY then
					--DrawAxis(collPos_right, 0.2)
					diff_right = collPos_right.GetY() - base_y
				end
				local diff = diff_left
				if collPos_left.GetY() > collPos_right.GetY() + 0.01 then
					diff = diff_right
					if collEntity_left ~= INVALID_ENTITY then
						ik_foot = self.left_foot
						ik_pos = collPos_left
					end
				else 
					if collEntity_right ~= INVALID_ENTITY then
						ik_foot = self.right_foot
						ik_pos = collPos_right
					end
				end
				self.root_offset = math.lerp(self.root_offset, diff, 0.1)
			else
				self.root_offset = math.lerp(self.root_offset, 0, 0.1)
			end

			-- Offset root transform to lower foot pos:
			local root_transform = scene.Component_GetTransform(self.root)
			if root_transform ~= nil then
				root_transform.Translation_local = Vector(0, self.root_offset)
				root_transform.SetDirty(true)
			end

			-- Remove IK effectors by default:
			if scene.Component_GetInverseKinematics(self.left_foot) ~= nil then
				scene.Component_RemoveInverseKinematics(self.left_foot)
			end
			if scene.Component_GetInverseKinematics(self.right_foot) ~= nil then
				scene.Component_RemoveInverseKinematics(self.right_foot)
			end
			-- The upper foot will use IK effector in IDLE state:
			if ik_foot ~= INVALID_ENTITY then
				if self.foot_placement == nil then
					self.foot_placement = CreateEntity()
					scene.Component_CreateTransform(self.foot_placement)
				end
				--DrawAxis(ik_pos, 0.2)
				local transform = scene.Component_GetTransform(self.foot_placement)
				transform.ClearTransform()
				transform.Translate(vector.Add(ik_pos, Vector(0, 0.15)))
				local ik = scene.Component_CreateInverseKinematics(ik_foot)
				ik.SetTarget(self.foot_placement)
				ik.SetChainLength(2)
				ik.SetIterationCount(10)
			end
		end,

		Update_Footprints = function(self)
			local radius = 0.1

			-- Left footprint:
			local capsule = Capsule(scene.Component_GetTransform(self.left_foot).GetPosition(), scene.Component_GetTransform(self.left_toes).GetPosition(), 0.1)
			--DrawCapsule(capsule, Vector(1,0,0,1))
			local collEntity,collPos = scene.Intersects(capsule, FILTER_NAVIGATION_MESH)
			if collEntity ~= INVALID_ENTITY then
				if not self.foot_placed_left then
					self.foot_placed_left = true
					local entity = CreateEntity()
					local transform = scene.Component_CreateTransform(entity)
					local material = scene.Component_CreateMaterial(entity)
					local decal = scene.Component_CreateDecal(entity)
					local layer = scene.Component_CreateLayer(entity)
					transform.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
					transform.Rotate(Vector(math.pi * 0.5))
					transform.Scale(0.1)
					transform.Translate(collPos)
					material.SetTexture(TextureSlot.BASECOLORMAP, footprint_texture)
					material.SetBaseColor(Vector(0.1,0.1,0.1,1))
					decal.SetSlopeBlendPower(2)
					layer.SetLayerMask(~(Layers.Player | Layers.NPC))
					scene.Component_Attach(entity, collEntity)
					table.insert(footprints, entity)
				end
			else
				self.foot_placed_left = false
			end

			-- Right footprint:
			local capsule = Capsule(scene.Component_GetTransform(self.right_foot).GetPosition(), scene.Component_GetTransform(self.right_toes).GetPosition(), 0.1)
			--DrawCapsule(capsule, Vector(1,0,0,1))
			local collEntity,collPos = scene.Intersects(capsule, FILTER_NAVIGATION_MESH)
			if collEntity ~= INVALID_ENTITY then
				if not self.foot_placed_right then
					self.foot_placed_right = true
					local entity = CreateEntity()
					local transform = scene.Component_CreateTransform(entity)
					local material = scene.Component_CreateMaterial(entity)
					local decal = scene.Component_CreateDecal(entity)
					local layer = scene.Component_CreateLayer(entity)
					transform.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
					transform.Rotate(Vector(math.pi * 0.5))
					transform.Scale(0.1)
					transform.Translate(collPos)
					material.SetTexture(TextureSlot.BASECOLORMAP, footprint_texture)
					material.SetTexMulAdd(Vector(-1, 1, 0, 0)) -- flip left footprint texture to be right
					material.SetBaseColor(Vector(0.1,0.1,0.1,1))
					decal.SetSlopeBlendPower(2)
					layer.SetLayerMask(~(Layers.Player | Layers.NPC))
					scene.Component_Attach(entity, collEntity)
					table.insert(footprints, entity)
				end
			else
				self.foot_placed_right = false
			end
		end,

		Follow = function(self, leader)
			self.hired = leader
		end,
		Unfollow = function(self)
			self.hired = nil
			self.patrol_waypoints = {}
		end,

	}

	self:Create(model_scene, start_transform, controllable, anim_scene)
	return self
end

-- Interaction between two characters:
local ResolveCharacters = function(characterA, characterB)
	local humanoidA = scene.Component_GetHumanoid(characterA.humanoid)
	local humanoidB = scene.Component_GetHumanoid(characterB.humanoid)
	if humanoidA ~= INVALID_ENTITY and humanoidB ~= INVALID_ENTITY then
		local headA = scene.Component_GetTransform(characterA.head).GetPosition()
		local headB = scene.Component_GetTransform(characterB.head).GetPosition()
		local distance = vector.Subtract(headA, headB).Length()
		if distance < 1.5 then
			humanoidA.SetLookAtEnabled(true)
			humanoidB.SetLookAtEnabled(true)
			humanoidA.SetLookAt(headB)
			humanoidB.SetLookAt(headA)

			local facing_amount = vector.Dot(characterB.face, vector.Subtract(characterA.position, characterB.position).Normalize())
			if #characterA.dialogs > 0 and conversation.state == ConversationState.Disabled and facing_amount > 0.8 then
				if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(GAMEPAD_BUTTON_2) then
					characterA.face_next = vector.Subtract(headB, headA):Normalize()
					conversation:Enter(characterA)
				end
				DrawDebugText("", vector.Add(headA, Vector(0,0.4)), Vector(1,1,1,1), 0.1, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
			end
		end

		if characterA.hired ~= nil then
			characterA.patrol_waypoints = {
				{
					entity = characterA.hired.model,
					distance = 2,
					distance_threshold = 1
				}
			}
			if distance < 5 then
				if characterA.hired.state == States.JOG or characterA.hired.state == States.RUN then
					characterA.patrol_waypoints[1].state = States.JOG
				else
					characterA.patrol_waypoints[1].state = States.WALK
				end
			elseif distance < 10 then
				characterA.patrol_waypoints[1].state = States.JOG
			else
				characterA.patrol_waypoints[1].state = States.RUN
			end
		end

	end

end

-- Third person camera controller class:
local function ThirdPersonCamera(character)
	local self = {
		camera = INVALID_ENTITY,
		character = nil,
		side_offset = 0.2,
		rest_distance = 1,
		rest_distance_new = 1,
		min_distance = 0.5,
		zoom_speed = 0.3,
		target_rot_horizontal = 0,
		target_rot_vertical = 0,
		target_height = 1,
		
		Create = function(self, character)
			self.character = character
			
			self.camera = CreateEntity()
			local camera_transform = scene.Component_CreateTransform(self.camera)
		end,
		
		Update = function(self)
			if self.character == nil then
				return
			end

			-- Mouse scroll or gamepad triggers will move the camera distance:
			local scroll = input.GetPointer().GetZ() -- pointer.z is the mouse wheel delta this frame
			scroll = scroll + input.GetAnalog(GAMEPAD_ANALOG_TRIGGER_R).GetX()
			scroll = scroll - input.GetAnalog(GAMEPAD_ANALOG_TRIGGER_L).GetX()
			scroll = scroll * self.zoom_speed
			self.rest_distance_new = math.max(self.rest_distance_new - scroll, self.min_distance) -- do not allow too close using max
			self.rest_distance = math.lerp(self.rest_distance, self.rest_distance_new, 0.1) -- lerp will smooth out the zooming

			-- This will allow some smoothing for certain movements of camera target:
			local character_transform = scene.Component_GetTransform(self.character.model)
			local character_position = character_transform.GetPosition()
			self.target_rot_horizontal = math.lerp(self.target_rot_horizontal, self.character.target_rot_horizontal, 0.1)
			self.target_rot_vertical = math.lerp(self.target_rot_vertical, self.character.target_rot_vertical, 0.1)
			self.target_height = math.lerp(self.target_height, character_position.GetY() + self.character.target_height + self.character.root_offset, 0.1)

			local camera_transform = scene.Component_GetTransform(self.camera)
			local target_transform = TransformComponent()
			target_transform.Translate(Vector(character_position.GetX(), 0, character_position.GetZ()))
			target_transform.Translate(Vector(0, self.target_height))
			target_transform.Rotate(Vector(self.target_rot_vertical, self.target_rot_horizontal))
			target_transform.UpdateTransform()
			
			-- First calculate the rest orientation (transform) of the camera:
			local mat = matrix.Translation(Vector(self.side_offset, 0, -self.rest_distance))
			mat = matrix.Multiply(mat, target_transform.GetMatrix())
			camera_transform.ClearTransform()
			camera_transform.MatrixTransform(mat)
			camera_transform.UpdateTransform()


			-- Camera collision:

			-- Compute the relation vectors between camera and target:
			local camPos = camera_transform.GetPosition()
			local targetPos = target_transform.GetPosition()
			local camDistance = vector.Subtract(camPos, targetPos).Length()

			-- These will store the closest collision distance and required camera position:
			local bestDistance = camDistance
			local bestPos = camPos
			local camera = GetCamera()

			-- Update global camera matrices for rest position:
			camera.TransformCamera(camera_transform)
			camera.UpdateCamera()

			-- Cast rays from target to clip space points on the camera near plane to avoid clipping through objects:
			local unproj = camera.GetInvViewProjection()	-- camera matrix used to unproject from clip space to world space
			local clip_coords = {
				Vector(0,0,1,1),	-- center
				Vector(-1,-1,1,1),	-- bottom left
				Vector(-1,1,1,1),	-- top left
				Vector(1,-1,1,1),	-- bottom right
				Vector(1,1,1,1),	-- top right
			}
			for i,coord in ipairs(clip_coords) do
				local corner = vector.TransformCoord(coord, unproj)
				local target_to_corner = vector.Subtract(corner, targetPos)
				local corner_to_campos = vector.Subtract(camPos, corner)
				local TMin = 0
				local TMax = target_to_corner.Length() -- optimization: limit the ray tracing distance

				local ray = Ray(targetPos, target_to_corner.Normalize(), TMin, TMax)

				local collision_layer =  ~(Layers.Player | Layers.NPC) -- specifies that neither NPC, nor Player can collide with camera
				local collObj,collPos,collNor = scene.Intersects(ray, FILTER_NAVIGATION_MESH | FILTER_COLLIDER,  collision_layer)
				if(collObj ~= INVALID_ENTITY) then
					-- It hit something, see if it is between the player and camera:
					local collDiff = vector.Subtract(collPos, targetPos)
					local collDist = collDiff.Length()
					if(collDist > 0 and collDist < bestDistance) then
						bestDistance = collDist
						bestPos = vector.Add(collPos, corner_to_campos)
						--DrawPoint(collPos, 0.1, Vector(1,0,0,1))
					end
				end
			end
			
			-- We have the best candidate for new camera position now, so offset the camera with the delta between the old and new camera position:
			local collision_offset = vector.Subtract(bestPos, camPos)
			mat = matrix.Multiply(mat, matrix.Translation(collision_offset))
			camera_transform.ClearTransform()
			camera_transform.MatrixTransform(mat)
			camera_transform.UpdateTransform()
			--DrawPoint(bestPos, 0.1, Vector(1,1,0,1))
			
			-- Feed back camera after collision:
			camera.TransformCamera(camera_transform)
			camera.UpdateCamera()
				
		end,
	}

	self:Create(character)
	return self
end

runProcess(function()
	
	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = application.GetActivePath()
	local path = RenderPath3D()
	local loadingscreen = LoadingScreen()

	--path.SetLightShaftsEnabled(true)
	path.SetLightShaftsStrength(0.01)
	path.SetAO(AO_MSAO)
	path.SetAOPower(0.25)
	--path.SetOutlineEnabled(true)
	path.SetOutlineThreshold(0.11)
	path.SetOutlineThickness(1.7)
	path.SetOutlineColor(0,0,0,0.6)
	path.SetBloomThreshold(5)

	--application.SetInfoDisplay(false)
	--application.SetFPSDisplay(true)
	--path.SetResolutionScale(0.75)
	--path.SetFSR2Enabled(true)
	--path.SetFSR2Preset(FSR2_Preset.Performance)
	--SetProfilerEnabled(true)
	SetDebugEnvProbesEnabled(false)
	SetGridHelperEnabled(false)

	-- Configure a simple loading progress bar:
    local loadingbar = Sprite()
	loadingbar.SetMaskTexture(texturehelper.CreateGradientTexture(
		GradientType.Linear,
		2048, 1,
		Vector(0, 0), Vector(1, 0),
		GradientFlags.Inverse,
		"111r"
	))
	local loadingbarparams = loadingbar.GetParams()
	loadingbarparams.SetColor(Vector(1,0.2,0.2,1))
	loadingbarparams.SetBlendMode(BLENDMODE_ALPHA)
    loadingscreen.AddSprite(loadingbar)
	loadingscreen.SetBackgroundTexture(Texture(script_dir() .. "assets/loadingscreen.png"))

	-- All LoadModel tasks are started by the LoadingScreen asynchronously, but we get back root Entity handles immediately:
	local anim_scene = Scene()
	loadingscreen.AddLoadModelTask(anim_scene, script_dir() .. "assets/animations.wiscene")
	local loading_scene = Scene()
	loadingscreen.AddLoadModelTask(loading_scene, script_dir() .. "assets/level.wiscene")
	local character_scene = Scene()
	loadingscreen.AddLoadModelTask(character_scene, script_dir() .. "assets/character.wiscene")
	loadingscreen.AddRenderPathActivationTask(path, application, 0.5)
	application.SetActivePath(loadingscreen, 0.5) -- activate and switch to loading screen

    -- Because we are in a runProcess, we can block loading screen like this while application is still running normally:
	--	Meanwhile, we can update the progress bar sprite
    while not loadingscreen.IsFinished() do
        update() -- gives back control for application for one frame, script waits until next update() phase
		local canvas = application.GetCanvas()
		loadingbarparams.SetPos(Vector(50, canvas.GetLogicalHeight() * 0.8))
		loadingbarparams.SetSize(Vector(canvas.GetLogicalWidth() - 100, 20))
		local progress = 1 - loadingscreen.GetProgress() / 100.0
		loadingbarparams.SetMaskAlphaRange(math.saturate(progress - 0.05), math.saturate(progress))
		loadingbar.SetParams(loadingbarparams)
    end
    
	-- After loading finished, we clear the main scene, and merge loaded scene into it:
    scene.Clear()
    scene.Merge(loading_scene)
    scene.Update(0)
	
	scene.VoxelizeScene(voxelgrid, false, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, ~(Layers.Player | Layers.NPC)) -- player and npc layers not included in voxelization
	
	-- Parse animations from anim_scene, which was loaded by the loading screen:
	LoadAnimations(anim_scene)
	
	-- Create characters from scene metadata components:
	local player = nil
	local npcs = {}
	for i,entity in ipairs(scene.Entity_GetMetadataArray()) do
		local metadata = scene.Component_GetMetadata(entity)
		local transform = scene.Component_GetTransform(entity)
		if metadata ~= nil and transform ~= nil then
			if player == nil and metadata.GetPreset() == MetadataPreset.Player then
				player = Character(character_scene, transform, true, anim_scene)
			end
			if metadata.GetPreset() == MetadataPreset.NPC then
				local npc = Character(character_scene, transform, false, anim_scene)
				-- Add patrol waypoints if found:
				--	It will be looking for "waypoint" named string values in metadata components, and they can be chained by their value
				local visited = {} -- avoid infinite loop
				while metadata ~= nil and metadata.HasString("waypoint") do
					local waypoint_name = metadata.GetString("waypoint")
					local waypoint_entity = scene.Entity_FindByName(waypoint_name)
					if waypoint_entity ~= INVALID_ENTITY then
						if visited[waypoint_entity] then
							break
						else
							metadata = scene.Component_GetMetadata(waypoint_entity) -- chain waypoints
							visited[waypoint_entity] = true
						end
						if metadata == nil then
							break
						end
						local waypoint = {
							entity = waypoint_entity,
							wait = metadata.GetFloat("wait")
						}
						if metadata.HasString("state") then
							waypoint.state = metadata.GetString("state")
						end
						table.insert(npc.patrol_waypoints, waypoint) -- add waypoint to NPC
					else
						break
					end
				end
				table.insert(npcs, npc) -- add NPC
			end
		end
	end

	-- if player was not created from a metadata component, create a default player:
	if player == nil then
		player = Character(character_scene, TransformComponent(), true, anim_scene)
	end
	
	local camera = ThirdPersonCamera(player)

	path.AddFont(conversation.font)
	path.AddFont(conversation.advance_font)
	
	local help_text = "Wicked Engine Character demo (LUA)\n\n"
	help_text = help_text .. "Controls:\n"
	help_text = help_text .. "#############\n"
	help_text = help_text .. "WASD/arrows/left analog stick: walk\n"
	help_text = help_text .. "SHIFT/right shoulder button: walk -> jog\n"
	help_text = help_text .. "E/left shoulder button: jog -> run\n"
	help_text = help_text .. "SPACE/gamepad X/gamepad button 3: Jump\n"
	help_text = help_text .. "Right Mouse Button/Right thumbstick: rotate camera\n"
	help_text = help_text .. "Scoll middle mouse/Left-Right triggers: adjust camera distance\n"
	help_text = help_text .. "ESCAPE key: quit\n"
	help_text = help_text .. "ENTER key: interact\n"
	help_text = help_text .. "R: reload script\n"
	help_text = help_text .. "H: toggle debug draw\n"
	help_text = help_text .. "L: toggle framerate lock\n"

	
	-- Conversation dialogs:
	local dialogtree = {
		-- Dialog starts here:
		{"Hello! Today is a nice day for a walk, isn't it? The sun is shining, the wind blows lightly, and the temperature is just perfect! To be honest, I don't need anything else to be happy."},
		{"I just finished my morning routine and I'm ready for the day. What should I do now...?"},
		{
			"Anything I can do for you?",
			choices = {
				{
					"Follow me!",
					action = function()
						conversation.character:Follow(player)
						conversation.character.next_dialog = 4
					end
				},
				{
					"Never mind.",
					action = function()
						conversation.character.next_dialog = 5
					end
				}
			}
		},

		-- Dialog 4: When chosen [Follow me] or [Just keep following me]
		{"Lead the way!", action_after = function() conversation:Exit() conversation.character.next_dialog = 6 end},

		-- Dialog 5: When chosen [Never mind] - this also modifies mood (expression) and state (anim) while dialog is playing
		{
			"Have a nice day!",
			action = function()
				conversation.character.mood = Mood.Happy
				conversation.character.state = States.WAVE
				conversation.character.anim_amount = 0.1
			end,
			action_after = function()
				conversation.character.mood = Mood.Neutral
				conversation.character.state = States.IDLE
				conversation.character.anim_amount = 1
				conversation:Exit() 
				conversation.character.next_dialog = 1 
			end
		},

		-- Dialog 6: After Dialog 4 finished, so character is following player
		{
			"Where are we going?",
			choices = {
				{"Just keep following me.", action = function() conversation.character.next_dialog = 4 end},
				{"Stay here!", action = function() conversation.character:Unfollow() end}
			}
		},
		{"Gotcha!"}, -- After chosen [Stay here]
	}

	for i,npc in pairs(npcs) do
		npc.dialogs = dialogtree
	end

	-- Main loop:
	while true do

		player:Update()
		for k,npc in pairs(npcs) do
			npc:Update()
			ResolveCharacters(npc, player)
		end

		-- Hierarchy after character positioning is updated, this is needed to place camera and IK afterwards to most up to date locations
		--	But we do it once, not every character!
		scene.UpdateHierarchy() -- Note: if I don't do this, you get foot positions after IK, but we want foot positions after animation, to start from "fresh"
		
		player:Update_IK()
		for k,npc in pairs(npcs) do
			npc:Update_IK()
		end

		if enable_footprints then
			player:Update_Footprints()
			for k,npc in pairs(npcs) do
				npc:Update_Footprints()
			end
			-- Cap the max number of decals:
			while #footprints > 100 do
				local entity = footprints[1]
				scene.Entity_Remove(entity)
				table.remove(footprints, 1)
			end
		end

		conversation:Update(path, scene, player)
		player.controllable = not conversation.override_input

		if not conversation.override_input then
			camera:Update()
		end

		if dynamic_voxelization then
			voxelgrid.ClearData()
			scene.VoxelizeScene(voxelgrid, false, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, ~(Layers.Player | Layers.NPC)) -- player and npc layers not included in voxelization
		end
		
		-- Do some debug draw geometry:
			
		DrawDebugText(help_text, Vector(0,2,2), Vector(1,1,1,1), 0.1, DEBUG_TEXT_DEPTH_TEST)

		if input.Press(string.byte('H')) then
			debug = not debug
		end

		if debug and backlog_isactive() == false then

			local model_transform = scene.Component_GetTransform(player.model)
			local target_transform = scene.Component_GetTransform(player.neck)
			
			--velocity
			DrawLine(target_transform.GetPosition(),target_transform.GetPosition():Add(player.velocity))
			--face
			DrawLine(target_transform.GetPosition(),target_transform.GetPosition():Add(player.face:Normalize()),Vector(0,1,0,1))
			--intersection
			--DrawAxis(player.p,0.5)
			
			-- camera target box and axis
			--DrawBox(target_transform.GetMatrix())
			
			-- Neck bone
			DrawPoint(scene.Component_GetTransform(player.neck).GetPosition(),0.05, Vector(0,1,1,1))
			-- Head bone
			DrawPoint(scene.Component_GetTransform(player.head).GetPosition(),0.05, Vector(0,1,1,1))
			-- Left hand bone
			DrawPoint(scene.Component_GetTransform(player.left_hand).GetPosition(),0.05, Vector(0,1,1,1))
			-- Right hand bone
			DrawPoint(scene.Component_GetTransform(player.right_hand).GetPosition(),0.05, Vector(0,1,1,1))
			-- Left foot bone
			DrawPoint(scene.Component_GetTransform(player.left_foot).GetPosition(),0.05, Vector(0,1,1,1))
			-- Right foot bone
			DrawPoint(scene.Component_GetTransform(player.right_foot).GetPosition(),0.05, Vector(0,1,1,1))

			
			local capsule = scene.Component_GetCollider(player.collider).GetCapsule()
			DrawCapsule(capsule)

			local str = "State: " .. player.state .. "\n"
			--str = str .. "Velocity = " .. player.velocity.GetX() .. ", " .. player.velocity.GetY() .. "," .. player.velocity.GetZ() .. "\n"
			str = str .. "Velocity = " .. player.velocity.GetY() .. "\n"
			DrawDebugText(str, vector.Add(capsule.GetBase(), Vector(0,0.4)), Vector(1,1,1,1), 1, DEBUG_TEXT_CAMERA_FACING | DEBUG_TEXT_CAMERA_SCALING)

			DrawVoxelGrid(voxelgrid)

		end

		update()
		
		if not backlog_isactive() then
			if(input.Press(KEYBOARD_BUTTON_ESCAPE)) then
				-- restore previous component
				--	so if you loaded this script from the editor, you can go back to the editor with ESC
				backlog_post("EXIT")
				for i,anim in ipairs(scene.Component_GetAnimationArray()) do
					anim.Stop() -- stop animations because some of them are retargeted and animation source scene will be lost after we exit this script!
				end
				application.SetActivePath(prevPath)
				killProcesses()
				return
			end
			if(input.Press(string.byte('R'))) then
				-- reload script
				backlog_post("RELOAD")
				application.SetActivePath(prevPath, 0.5)
				while not application.IsFaded() do
					update()
				end
				killProcesses()
				dofile(script_file())
				return
			end
			if input.Press(string.byte('L')) then
				framerate_lock = not framerate_lock
				application.SetFrameRateLock(framerate_lock)
				if framerate_lock then
					application.SetTargetFrameRate(framerate_lock_target)
				end
			end
		end

	end
	
end)
