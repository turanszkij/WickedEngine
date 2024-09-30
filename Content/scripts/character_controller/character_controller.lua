-- Lua Third person camera and character controller script
--	This script will load a simple scene with a character that can be controlled
--
--	Tips:
--		- Character models that you use should have a humanoid rig (this is created automatically from VRM and Mixamo model imports)
--		- The level objects should be tagged as "Navmesh" because the characters will collide with only those, and these are optimized for intersections
--		- To set the player start position, you can put a metadata component to the level scene and set it to "Player" preset
--		- To add NPCs you can put a metadata component to the level scene and set it to "NPC" preset
--		- To specify which model a character in the level (Player or NPC) uses, add a string property to its metadata named "name" and its value is the name of the character, for example name = johnny will use assets/johnny.wiscene
--		- To specify which animation set a character should use, add "animset" string property to metadata, the value should be the name of the animset which will be concatenated to anim names with "_". For example: animset = male will load animations like "walk_male", etc. if available, else fall back to "walk"
--		- To set patrol route for an NPC character, add a "waypoint" named string property to its metadata and the value is a name of the target waypoint entity. Waypoints can be chained by having a metadata for them and having "waypoint" string properties on each.
--		- To set conversation dialog for an NPC character, add a "dialog" named string property to its metadata and the value is a name of the lua file in assets/dialogs/. For example dialog = hello will make the character use the assets/dialogs/hello.lua dialog script
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

local debug = false -- press H to toggle
local framerate_lock = false
local framerate_lock_target = 20
local slope_threshold = 0.2 -- How much slopeiness will cause character to slide down instead of standing on it
local gravity = -30
local dynamic_voxelization = false -- Set to true to revoxelize navigation every frame

dofile(script_dir() .. "/assets/conversation.lua") -- load conversation system logic from separate file
conversation = Conversation() -- conversation will be accessible from dialog scripts

local scene = GetScene()

local Layers = {
	Player = 1 << 0,
	NPC = 1 << 1,
}
States = {
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

Mood = {
	Neutral = ExpressionPreset.Neutral,
	Happy = ExpressionPreset.Happy,
	Angry = ExpressionPreset.Angry,
	Sad = ExpressionPreset.Sad,
	Relaxed = ExpressionPreset.Relaxed,
	Surprised = ExpressionPreset.Surprised
}

player = nil
npcs = {}

local enable_footprints = true
local footprint_texture = Texture(script_dir() .. "assets/footprint.dds")
local footprints = {}

local character_capsules = {}
local voxelgrid = VoxelGrid(128,32,128)
voxelgrid.SetVoxelSize(0.25)
voxelgrid.SetCenter(Vector(0,0.1,0))

local function Character(model_scene, start_transform, controllable, anim_scene, animset)
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
		leaning_next = 0, -- leaning sideways when turning
		leaning = 0, -- leaning sideways when turning (smoothed)
		savedPointerPos = Vector(),
		walk_speed = 0.1,
		jog_speed = 0.2,
		run_speed = 0.4,
		jump_speed = 8,
		swim_speed = 0.2,
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
		object_entities = {},

		patrol_waypoints = {},
		patrol_waypoints_original = {},
		patrol_next = 0,
		patrol_wait = 0,
		patrol_reached = false,
		hired = nil,
		dialogs = {},
		next_dialog = 1,
		
		GetFacing = function(self)
			local charactercomponent = scene.Component_GetCharacter(self.model)
			return charactercomponent.GetFacingSmoothed()
		end,
		Jump = function(self,f)
			local charactercomponent = scene.Component_GetCharacter(self.model)
			charactercomponent.Jump(f)
			self.state = States.JUMP
		end,
		Turn = function(self,dir)
			local charactercomponent = scene.Component_GetCharacter(self.model)
			charactercomponent.Turn(dir)
		end,
		MoveDirection = function(self,dir)
			local charactercomponent = scene.Component_GetCharacter(self.model)
			local rotation_matrix = matrix.Multiply(matrix.RotationY(self.target_rot_horizontal), matrix.RotationX(self.target_rot_vertical))
			dir = vector.TransformNormal(dir.Normalize(), rotation_matrix)
			dir.SetY(0)
			charactercomponent.Turn(dir.Normalize())
			local speed = self.walk_speed
			if self.state == States.WALK then
				speed = self.walk_speed
			elseif self.state == States.JOG then
				speed = self.jog_speed
			elseif self.state == States.RUN then
				speed = self.run_speed
			elseif self.state == States.SWIM then
				speed = self.swim_speed
			end
			charactercomponent.Move(self:GetFacing():Multiply(Vector(speed,speed,speed)))
		end,
		SetAnimationAmount = function(self,amount)
			local charactercomponent = scene.Component_GetCharacter(self.model)
			charactercomponent.SetAnimationAmount(amount)
		end,

		Update = function(self)

			local dt = getDeltaTime()

			local charactercomponent = scene.Component_GetCharacter(self.model)
			self.ground_intersect = charactercomponent.IsGrounded()
			self.position = charactercomponent.GetPositionInterpolated()
			local velocity = charactercomponent.GetVelocity()
			local capsule = charactercomponent.GetCapsule()
			character_capsules[self.model] = capsule
			--DrawCapsule(capsule)

			local humanoid = scene.Component_GetHumanoid(self.humanoid)
			humanoid.SetLookAtEnabled(false)
			
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
			charactercomponent.PlayAnimation(self.anims[self.state])
			if self.state == States.JUMP then
				if charactercomponent.IsAnimationEnded() then
					self.state = States.IDLE
				end
			else
				if self.ground_intersect and self.state ~= States.DANCE and self.state ~= States.WAVE then
					self.state = States.IDLE
				end
			end

			if charactercomponent.IsSwimming() then
				self.state = States.SWIM_IDLE
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
						elseif self.ground_intersect or charactercomponent.IsWallIntersect() then
							if self.ground_intersect then
								if(input.Down(KEYBOARD_BUTTON_LSHIFT) or input.Down(GAMEPAD_BUTTON_6)) then
									if input.Down(string.byte('E')) or input.Down(GAMEPAD_BUTTON_5) then
										self.state = States.RUN
									else
										self.state = States.JOG
									end
								else
									self.state = States.WALK
								end
							end
							self:MoveDirection(lookDir)
						end
					end
					
					if (input.Press(string.byte('J')) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_3)) and self.ground_intersect then
						self:Jump(self.jump_speed)
					end
				end

			elseif not(conversation.override_input and conversation.character == self) then

				-- NPC patrol behavior:
				local patrol_count = len(self.patrol_waypoints)
				if patrol_count > 0 then
					local pos = self.position
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
							local charactercomponent = scene.Component_GetCharacter(self.model)
							charactercomponent.SetPathGoal(patrol_pos, voxelgrid) -- this is deferred into scene update
							local pathquery = charactercomponent.GetPathQuery()
							pathquery.SetAgentHeight(3)
							if pathquery.IsSuccessful() then
								-- If there is a valid pathfinding result for waypoint, replace heading direction by that:
								patrol_vec = vector.Subtract(pathquery.GetNextWaypoint(), pos)
								patrol_vec.SetY(0)
							end
							if debug then
								DrawPathQuery(pathquery)
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

			-- If camera is inside character capsule, fade out the character, otherwise fade in:
			if capsule.Intersects(GetCamera().GetPosition()) then
				for i,entity in ipairs(self.object_entities) do
					local object = scene.Component_GetObject(entity)
					local color = object.GetColor()
					local opacity = color.GetW()
					opacity = math.lerp(opacity, 0, 0.1)
					color.SetW(opacity)
					object.SetColor(color)
				end
			else
				for i,entity in ipairs(self.object_entities) do
					local object = scene.Component_GetObject(entity)
					local color = object.GetColor()
					local opacity = color.GetW()
					opacity = math.lerp(opacity, 1, 0.1)
					color.SetW(opacity)
					object.SetColor(color)
				end
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
					transform.MatrixTransform(matrix.LookTo(Vector(),self:GetFacing()):Inverse())
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
					transform.MatrixTransform(matrix.LookTo(Vector(),self:GetFacing()):Inverse())
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

        SetPatrol = function(self, patrol)
            self.patrol_waypoints_original = patrol
            self.patrol_waypoints = patrol
        end,
        ReturnToPatrol = function(self) -- return to original patrol route
            self.patrol_waypoints = self.patrol_waypoints_original
        end,

		Follow = function(self, leader)
			self.hired = leader
		end,
		Unfollow = function(self)
			self.hired = nil
			self.patrol_waypoints = {}
		end,

	}

	-- Character initialization:
	self.position = start_transform.GetPosition()
	local facing = vector.Rotate(start_transform.GetForward(), vector.QuaternionFromRollPitchYaw(self.rotation))
	self.controllable = controllable
	if controllable then
		self.layerMask = Layers.Player
		self.target_rot_horizontal = vector.GetAngle(Vector(0,0,1), facing, Vector(0,1,0)) -- only modify camera rot for player
	else
		self.layerMask = Layers.NPC
	end

	-- Note: we instantiate the model_scene into the main scene, start_transform stores a component pointer which gets invalidated after this!!
	self.model = scene.Instantiate(model_scene, true)

	local charactercomponent = scene.Component_CreateCharacter(self.model) -- some character features will be handled by the built-in CharacterComponent in Wicked Engine
	charactercomponent.SetPosition(self.position)
	charactercomponent.SetFacing(facing)
	charactercomponent.SetWidth(0.3)
	charactercomponent.SetHeight(1.8)
	--charactercomponent.SetFootPlacementEnabled(false)

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
			table.insert(self.object_entities, entity) -- save object array for later use
		end
	end

	-- Create a base capsule collider for character:
	local collider = scene.Component_CreateCollider(self.model)
	self.collider = self.model
	collider.SetCPUEnabled(false)
	collider.SetGPUEnabled(true)
	collider.Shape = ColliderShape.Capsule
	collider.Radius = charactercomponent.GetWidth()
	collider.Offset = Vector(0, collider.Radius, 0)
	collider.Tail = Vector(0, charactercomponent.GetHeight() - collider.Radius, 0)
	local head_transform = scene.Component_GetTransform(self.head)
	if head_transform ~= nil then
		collider.Tail = head_transform.GetPosition()
	end

	self.root = self.humanoid
	
	scene.ResetPose(self.model)

	-- The base animation entities are queried from the anim_scene, they are not yet tergeting our character:
	self.anims[States.IDLE] = anim_scene.Entity_FindByName("idle")
	self.anims[States.WALK] = anim_scene.Entity_FindByName("walk")
	self.anims[States.JOG] = anim_scene.Entity_FindByName("jog")
	self.anims[States.RUN] = anim_scene.Entity_FindByName("run")
	self.anims[States.JUMP] = anim_scene.Entity_FindByName("jump")
	self.anims[States.SWIM_IDLE] = anim_scene.Entity_FindByName("swim_idle")
	self.anims[States.SWIM] = anim_scene.Entity_FindByName("swim")
	self.anims[States.DANCE] = anim_scene.Entity_FindByName("dance")
	self.anims[States.WAVE] = anim_scene.Entity_FindByName("wave")

	-- Retarget animation entities onto the character, and add them to the CharacterComponent:
	--	Also see if a requested animation set (animset) is available for each animation, and use that instead if yes
	animset = "_" .. animset
	for state,anim in pairs(self.anims) do
		local anim_name_component = anim_scene.Component_GetName(anim)
		if anim_name_component ~= nil then
			local anim_set_name = anim_name_component.GetName() .. animset
			local anim_set_entity = anim_scene.Entity_FindByName(anim_set_name)
			if anim_set_entity ~= INVALID_ENTITY then
				anim = anim_set_entity -- if there is animation with requested animset, replace default anim to that
			end
			self.anims[state] = scene.RetargetAnimation(self.humanoid, anim, false, anim_scene)
			charactercomponent.AddAnimation(self.anims[state]) -- 
		end
	end
	
	local model_transform = scene.Component_GetTransform(self.model)
	model_transform.ClearTransform()
	model_transform.Scale(self.scale)
	model_transform.Rotate(self.rotation)
	model_transform.Translate(self.position)
	model_transform.UpdateTransform()

	self.target_height = scene.Component_GetTransform(self.neck).GetPosition().GetY()

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

			local facing_amount = vector.Dot(characterB:GetFacing(), vector.Subtract(characterA.position, characterB.position).Normalize())
			if #characterA.dialogs > 0 and conversation.state == ConversationState.Disabled and facing_amount > 0.8 then
				if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(GAMEPAD_BUTTON_2) then
					characterA:Turn(vector.Subtract(headB, headA):Normalize())
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
	SetDebugCamerasEnabled(false)

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
	
    if len(scene.Component_GetVoxelGridArray()) > 0 then
        voxelgrid = scene.Component_GetVoxelGridArray()[1] -- take existing voxel grid from scene if available
    else
        scene.VoxelizeScene(voxelgrid, false, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, ~(Layers.Player | Layers.NPC)) -- generate a voxel grid in code, player and NPCs not included
    end
	
	-- Create characters from scene metadata components:
    local character_scenes = {}
	for i,entity in ipairs(scene.Entity_GetMetadataArray()) do
		local metadata = scene.Component_GetMetadata(entity)
		local transform = scene.Component_GetTransform(entity)
		if metadata ~= nil and transform ~= nil then

            -- Determine name of the placed character:
			local name = "character" -- default name
			if metadata.HasString("name") then
				name = metadata.GetString("name")
			end

			-- Load character model if doesn't exist yet:
			if character_scenes[name] == nil then
                character_scenes[name] = Scene()
				LoadModel(character_scenes[name], script_dir() .. "assets/" .. name .. ".wiscene")
			end

			if player == nil and metadata.GetPreset() == MetadataPreset.Player then
				player = Character(character_scenes[name], transform, true, anim_scene, metadata.GetString("animset"))
			end
			if metadata.GetPreset() == MetadataPreset.NPC then
				local npc = Character(character_scenes[name], transform, false, anim_scene, metadata.GetString("animset"))

				-- add dialog tree if found:
				if metadata.HasString("dialog") then
					npc.dialogs = dofile(script_dir() .. "assets/dialogs/" .. metadata.GetString("dialog") .. ".lua")
				end

				-- Add patrol waypoints if found:
				--	It will be looking for "waypoint" named string values in metadata components, and they can be chained by their value
				local patrol_waypoints = {}
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
						table.insert(patrol_waypoints, waypoint) -- add waypoint to NPC
					else
						break
					end
				end
				npc:SetPatrol(patrol_waypoints)
				table.insert(npcs, npc) -- add NPC
			end
		end
	end

	-- if player was not created from a metadata component, create a default player:
	if player == nil then
		player = Character(character_scenes["character"], TransformComponent(), true, anim_scene)
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

	-- Main loop:
	while true do

		player:Update()
		for k,npc in pairs(npcs) do
			npc:Update()
			ResolveCharacters(npc, player)
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
				for i,character in ipairs(scene.Component_GetCharacterArray()) do
					character.StopAnimation()
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
