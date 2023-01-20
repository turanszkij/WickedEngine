-- Lua Third person camera and character controller script
--	This script will load a simple scene with a character that can be controlled
--
-- 	CONTROLS:
--		WASD/left thumbstick: walk
--		SHIFT/right shoulder button: walk -> jog
--		E/left shoulder button: jog -> run
--		SPACE/gamepad X/gamepad button 2: jump
--		Right Mouse Button/Right thumbstick: rotate camera
--		Scoll middle mouse/Left-Right triggers: adjust camera distance
--		H: toggle debug draw
--		ESCAPE key: quit
--		ENTER: reload script

-- Debug Draw Helper
local DrawAxis = function(point,f)
	DrawLine(point,point:Add(Vector(f,0,0)),Vector(1,0,0,1))
	DrawLine(point,point:Add(Vector(0,f,0)),Vector(0,1,0,1))
	DrawLine(point,point:Add(Vector(0,0,f)),Vector(0,0,1,1))
end

local debug = true -- press H to toggle
local allow_pushaway_NPC = true -- You can decide whether NPCs can be pushed away by player

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
	DANCE = "dance"
}

local character_capsules = {}

local function Character(model_name, start_position, face, controllable)
	local self = {
		model = INVALID_ENTITY,
		target = INVALID_ENTITY, -- Camera will look at this location, rays will be started from this location, etc.
		current_anim = INVALID_ENTITY,
		idle_anim = INVALID_ENTITY,
		walk_anim = INVALID_ENTITY,
		jog_anim = INVALID_ENTITY,
		run_anim = INVALID_ENTITY,
		jump_anim = INVALID_ENTITY,
		swim_idle_anim = INVALID_ENTITY,
		dance_anim = INVALID_ENTITY,
		swim_anim = INVALID_ENTITY,
		all_anims = {},
		neck = INVALID_ENTITY,
		head = INVALID_ENTITY,
		left_hand = INVALID_ENTITY,
		right_hand = INVALID_ENTITY,
		left_foot = INVALID_ENTITY,
		right_foot = INVALID_ENTITY,
		face = Vector(0,0,1), -- forward direction
		force = Vector(),
		velocity = Vector(),
		savedPointerPos = Vector(),
		walk_speed = 0.2,
		jog_speed = 0.4,
		run_speed = 0.8,
		jump_speed = 14,
		swim_speed = 0.5,
		layerMask = ~0, -- layerMask will be used to filter collisions
		scale = Vector(1, 1, 1),
		rotation = Vector(0,math.pi,0),
		start_position = Vector(0, 1, 0),
		controllable = true,

		patrol_waypoints = {},
		patrol_next = 0,
		patrol_wait = 0,
		patrol_reached = false,
		hired = nil,
		
		Create = function(self, model_name, start_position, face, controllable)
			self.start_position = start_position
			self.face = face
			self.controllable = controllable
			if controllable then
				self.layerMask = Layers.Player
			else
				self.layerMask = Layers.NPC
			end
			local character_scene = Scene()
			self.model = LoadModel(character_scene, model_name)
			local layer = character_scene.Component_GetLayer(self.model)
			layer.SetLayerMask(self.layerMask)

			self.state = States.IDLE
			self.state_prev = self.state
			
			for i,entity in ipairs(character_scene.Entity_GetAnimationArray()) do
				table.insert(self.all_anims, entity)
			end
			
			self.idle_anim = character_scene.Entity_FindByName("idle")
			self.walk_anim = character_scene.Entity_FindByName("walk")
			self.jog_anim = character_scene.Entity_FindByName("jog")
			self.run_anim = character_scene.Entity_FindByName("run")
			self.jump_anim = character_scene.Entity_FindByName("jump")
			self.swim_idle_anim = character_scene.Entity_FindByName("swim_idle")
			self.swim_anim = character_scene.Entity_FindByName("swim")
			self.dance_anim = character_scene.Entity_FindByName("dance")

			self.collider = character_scene.Entity_GetHumanoidArray()[1]
			self.expression = character_scene.Entity_GetExpressionArray()[1]
			self.happy=0
			self.humanoid = character_scene.Entity_GetHumanoidArray()[1]
			local humanoid = character_scene.Component_GetHumanoid(self.humanoid)
			humanoid.SetLookAtEnabled(false)
			self.neck = humanoid.GetBoneEntity(HumanoidBone.Neck)
			self.head = humanoid.GetBoneEntity(HumanoidBone.Head)
			self.left_hand = humanoid.GetBoneEntity(HumanoidBone.LeftHand)
			self.right_hand = humanoid.GetBoneEntity(HumanoidBone.RightHand)
			self.left_foot = humanoid.GetBoneEntity(HumanoidBone.LeftFoot)
			self.right_foot = humanoid.GetBoneEntity(HumanoidBone.RightFoot)
			
			local model_transform = character_scene.Component_GetTransform(self.model)
			model_transform.ClearTransform()
			model_transform.Scale(self.scale)
			model_transform.Rotate(self.rotation)
			model_transform.Translate(self.start_position)
			model_transform.UpdateTransform()
			
			self.target = CreateEntity()
			local target_transform = character_scene.Component_CreateTransform(self.target)
			target_transform.Translate(character_scene.Component_GetTransform(self.neck).GetPosition())
			target_transform.Translate(self.start_position)
			
			character_scene.Component_Attach(self.target, self.model)

			self.root = character_scene.Entity_FindByName("Root")
			self.root_bone_offset = 0

			scene.Merge(character_scene)
		end,
		
		Jump = function(self,f)
			self.force.SetY(f)
			self.state = States.JUMP
		end,
		MoveDirection = function(self,dir)
			local target_transform = scene.Component_GetTransform(self.target)
			dir = vector.Transform(dir:Normalize(), target_transform.GetMatrix())
			dir.SetY(0)
			local dot = vector.Dot(self.face, dir)
			if(dot < 0) then
				self.face = vector.TransformNormal(self.face, matrix.RotationY(math.pi * 0.01))
			end
			self.face = vector.Lerp(self.face, dir, 0.2);
			self.face.Normalize()
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
				self.force = vector.Add(self.force, self.face:Multiply(Vector(speed,speed,speed)))
			end
		end,

		Update = function(self)

			local dt = getDeltaTime()

			local humanoid = scene.Component_GetHumanoid(self.humanoid)
			humanoid.SetLookAtEnabled(false)

			local model_transform = scene.Component_GetTransform(self.model)
			local savedPos = model_transform.GetPosition()
			model_transform.ClearTransform()
			model_transform.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
			model_transform.Scale(self.scale)
			model_transform.Rotate(self.rotation)
			model_transform.Translate(savedPos)
			model_transform.UpdateTransform()
			scene.Component_Detach(self.target)
			scene.Component_Attach(self.target, self.model)

			
			if controllable then
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
				
				local target_transform = scene.Component_GetTransform(self.target)
				target_transform.Rotate(Vector(diff.GetY(),diff.GetX()))

			
				if self.dance_anim ~= INVALID_ENTITY and self.state == States.IDLE and input.Press(string.byte('C')) then
					self.state = States.DANCE
				end
			end

			if self.state == States.DANCE then
				self.happy = math.lerp(self.happy, 1, 0.1)
			else
				self.happy = math.lerp(self.happy, 0, 0.1)
			end
			local expression = scene.Component_GetExpression(self.expression)
			expression.SetPresetWeight(ExpressionPreset.Happy, self.happy)
			
			-- state and animation update
			if(self.state == States.IDLE) then
				self.current_anim = self.idle_anim
			elseif(self.state == States.WALK) then
				self.current_anim = self.walk_anim
			elseif(self.state == States.JOG) then
				self.current_anim = self.jog_anim
			elseif(self.state == States.RUN) then
				self.current_anim = self.run_anim
			elseif(self.state == States.JUMP) then
				self.current_anim = self.jump_anim
			elseif(self.state == States.SWIM_IDLE) then
				self.current_anim = self.swim_idle_anim
			elseif(self.state == States.SWIM) then
				self.current_anim = self.swim_anim
			elseif(self.state == States.DANCE) then
				self.current_anim = self.dance_anim
			end
			
			local current_anim = scene.Component_GetAnimation(self.current_anim)
			if current_anim ~= nil then

				-- Blend in current animation:
				current_anim.SetAmount(math.lerp(current_anim.GetAmount(), 1, 0.1))
				current_anim.Play()
				if self.state_prev ~= self.state then
					-- If anim just started in this frame, reset timer to beginning:
					current_anim.SetTimer(current_anim.GetStart())
					self.state_prev = self.state
				end
				
				-- Ease out other animations:
				for i,anim in ipairs(self.all_anims) do
					if (anim ~= INVALID_ENTITY) and (anim ~= self.current_anim) then
						local prev_anim = scene.Component_GetAnimation(anim)
						prev_anim.SetAmount(math.lerp(prev_anim.GetAmount(), 0, 0.1))
						if prev_anim.GetAmount() <= 0 then
							prev_anim.Stop()
						end
					end
				end
				
				-- Simple state transition to idle:
				if self.state == States.JUMP then
					if current_anim.GetTimer() > current_anim.GetEnd() then
						self.state = States.IDLE
					end
				else
					if self.velocity.Length() < 0.1 and self.state ~= States.SWIM_IDLE and self.state ~= States.SWIM and self.state ~= States.DANCE then
						self.state = States.IDLE
					end
				end
			end

			if dt > 0.1 then
				return -- avoid processing too large delta times to avoid instability
			end

			-- swim test:
			if self.neck ~= INVALID_ENTITY then
				local neck_pos = scene.Component_GetTransform(self.neck).GetPosition()
				local water_threshold = 0.1
				neck_pos = vector.Add(neck_pos, Vector(0, -water_threshold, 0))
				local swim_ray = Ray(neck_pos, Vector(0,1,0), 0, 100)
				local water_entity, water_pos, water_normal, water_distance = scene.Intersects(swim_ray, FILTER_WATER)
				if water_entity ~= INVALID_ENTITY then
					model_transform.Translate(Vector(0,water_distance - water_threshold,0))
					model_transform.UpdateTransform()
					self.force.SetY(0)
					self.force = vector.Multiply(self.force, 0.8) -- water friction
					self.velocity.SetY(0)
					self.state = States.SWIM_IDLE
				end
			end

			if controllable then
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
					
				if self.state ~= States.JUMP and self.state_prev ~= States.JUMP and self.velocity.GetY() == 0 then
					if(lookDir.Length() > 0) then
						if self.state == States.SWIM_IDLE then
							self.state = States.SWIM
							self:MoveDirection(lookDir)
						else
							if(input.Down(KEYBOARD_BUTTON_LSHIFT) or input.Down(GAMEPAD_BUTTON_6)) then
								if input.Down(string.byte('E')) or input.Down(GAMEPAD_BUTTON_5) then
									self.state = States.RUN
									self:MoveDirection(lookDir)
								else
									self.state = States.JOG
									self:MoveDirection(lookDir)
								end
							else
								self.state = States.WALK
								self:MoveDirection(lookDir)
							end
						end
					end
					
					if(input.Press(string.byte('J')) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2)) then
						self:Jump(self.jump_speed)
					end
				elseif self.velocity.GetY() > 0 then
					self:MoveDirection(lookDir)
				end

			else

				-- NPC patrol behavior:
				local patrol_count = len(self.patrol_waypoints)
				if patrol_count > 0 then
					local pos = savedPos
					pos.SetY(0)
					local patrol = self.patrol_waypoints[self.patrol_next % patrol_count + 1]
					local patrol_transform = scene.Component_GetTransform(patrol.entity)
					if patrol_transform ~= nil then
						local patrol_pos = patrol_transform.GetPosition()
						patrol_pos.SetY(0)
						local patrol_wait = patrol.wait or 0 -- default: 0
						local patrol_vec = vector.Subtract(patrol_pos, pos)
						local distance = patrol_vec.Length()
						local patrol_dist = patrol.distance or 0.2 -- default : 0.2
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

			self.face.SetY(0)
			self.face=self.face:Normalize()

			self.force = vector.Add(self.force, Vector(0, -1, 0)) -- gravity
			self.velocity = vector.Multiply(self.force, dt)

			
			-- Capsule collision for character:		
			local capsule = scene.Component_GetCollider(self.collider).GetCapsule()
			local original_capsulepos = model_transform.GetPosition()
			local capsulepos = original_capsulepos
			local capsuleheight = vector.Subtract(capsule.GetTip(), capsule.GetBase()).Length()
			local radius = capsule.GetRadius()
			local ground_intersect = false

			local fixed_update_remain = dt
			local fixed_update_fps = 120
			local fixed_update_rate = 1.0 / fixed_update_fps
			local fixed_dt = fixed_update_rate / dt

			while fixed_update_remain > 0 do
				fixed_update_remain = fixed_update_remain - fixed_update_rate
			
				local step = vector.Multiply(self.velocity, fixed_dt)

				capsulepos = vector.Add(capsulepos, step)
				capsule = Capsule(capsulepos, vector.Add(capsulepos, Vector(0, capsuleheight)), radius)
				local collision_layer = ~self.layerMask
				if not controllable and not allow_pushaway_NPC then
					-- For NPC, this makes it not pushable away by player:
					--	This works by disabling NPC's collision to player
					--	But the player can still collide with NPC and can be blocked
					collision_layer = collision_layer & ~Layers.Player
				end
				local o2, p2, n2, depth, velocity = scene.Intersects(capsule, FILTER_NAVIGATION_MESH | FILTER_COLLIDER, collision_layer) -- scene/capsule collision
				if(o2 ~= INVALID_ENTITY) then

					if debug then
						DrawPoint(p2,0.1,Vector(1,1,0,1))
						DrawLine(p2, vector.Add(p2, n2), Vector(1,1,0,1))
					end

					local ground_slope = vector.Dot(n2, Vector(0,1,0))
					local slope_threshold = 0.1

					if self.velocity.GetY() < 0 and ground_slope > slope_threshold then
						-- Ground intersection, remove falling motion:
						self.velocity.SetY(0)
						capsulepos = vector.Add(capsulepos, Vector(0, depth, 0))
						capsulepos = vector.Add(capsulepos, velocity) -- apply moving platform velocity
						self.force = vector.Multiply(self.force, 0.85) -- ground friction
						ground_intersect = true
					elseif ground_slope <= slope_threshold then
						-- Slide on contact surface:
						local velocityLen = self.velocity.Length()
						local velocityNormalized = self.velocity.Normalize()
						local undesiredMotion = n2:Multiply(vector.Dot(velocityNormalized, n2))
						local desiredMotion = vector.Subtract(velocityNormalized, undesiredMotion)
						if ground_intersect then
							desiredMotion.SetY(0)
						end
						self.velocity = vector.Multiply(desiredMotion, velocityLen)
						capsulepos = vector.Add(capsulepos, vector.Multiply(n2, depth))
					end

				end

			end

			model_transform.Translate(vector.Subtract(capsulepos, original_capsulepos)) -- transform by the capsule offset
			model_transform.UpdateTransform()
			
			-- try to put water ripple:
			if self.velocity.Length() > 0.01 and self.state ~= States.SWIM_IDLE then
				local w,wp = scene.Intersects(capsule, FILTER_WATER)
				if w ~= INVALID_ENTITY then
					PutWaterRipple(script_dir() .. "assets/ripple.png", wp)
				end
			end

			character_capsules[self.model] = capsule
			
		end,

		Update_IK = function(self)
			-- IK foot placement:
			local root_bone_transform = scene.Component_GetTransform(self.root)
			root_bone_transform.ClearTransform()
			self.root_bone_offset = math.lerp(self.root_bone_offset, 0, 0.1)
			local ik_foot = INVALID_ENTITY
			local ik_pos = Vector()
			-- Compute root bone offset:
			--	I determine which foot wants to step on lower ground, that will offset root bone of skeleton downwards
			--	The other foot will be the upper foot which will be later attached an Inverse Kinematics (IK) effector
			if (self.state == States.IDLE or self.state == States.DANCE) and self.velocity.GetY() == 0 then
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
					diff_left = vector.Subtract(collPos_left, pos_left).GetY()
				end
				if collEntity_right ~= INVALID_ENTITY then
					diff_right = vector.Subtract(collPos_right, pos_right).GetY()
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
				self.root_bone_offset = math.lerp(self.root_bone_offset, diff + 0.1, 0.2)
			end
			root_bone_transform.Translate(Vector(0, self.root_bone_offset))

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
		end

	}

	self:Create(model_name, start_position, face, controllable)
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

			if characterB.controllable then
				if input.Press(KEYBOARD_BUTTON_ENTER) then
					characterA.hired = characterB
				end

				if characterA.hired == nil then
					DrawDebugText("Hello there!\nPress ENTER to hire me!", vector.Add(headA, Vector(0,0.4)), Vector(1,0.2,1,1), 0.1, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
				else
					DrawDebugText("Where are we going?", vector.Add(headA, Vector(0,0.4)), Vector(0.2,1,0.2,1), 0.1, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
				end
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

			local camera_transform = scene.Component_GetTransform(self.camera)
			local target_transform = scene.Component_GetTransform(self.character.target)

			
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
			
			camera.ApertureSize = self.character.happy
			camera.FocalLength = vector.Subtract(scene.Component_GetTransform(self.character.head).GetPosition(), camera.GetPosition()).Length()

				
		end,
	}

	self:Create(character)
	return self
end

	
ClearWorld()
LoadModel(script_dir() .. "assets/level.wiscene")
--LoadModel(script_dir() .. "assets/terrain.wiscene")
--LoadModel(script_dir() .. "assets/waypoints.wiscene", matrix.Translation(Vector(1,0,2)))
--dofile(script_dir() .. "../dungeon_generator/dungeon_generator.lua")

local player = Character(script_dir() .. "assets/character.wiscene", Vector(0,0.5,0), Vector(0,0,1), true)
local npcs = {
	-- Patrolling NPC IDs: 1,2,3
	Character(script_dir() .. "assets/character.wiscene", Vector(4,0.1,4), Vector(0,0,-1), false),
	Character(script_dir() .. "assets/character.wiscene", Vector(-8,1,4), Vector(-1,0,0), false),
	Character(script_dir() .. "assets/character.wiscene", Vector(-2,0.1,8), Vector(-1,0,0), false),

	-- stationary NPC IDs: 3,4....
	Character(script_dir() .. "assets/character.wiscene", Vector(-1,0.1,-6), Vector(0,0,1), false),
	--Character(script_dir() .. "assets/character.wiscene", Vector(10.8,0.1,4.1), Vector(0,0,-1), false),
	--Character(script_dir() .. "assets/character.wiscene", Vector(11.1,4,7.2), Vector(-1,0,0), false),
}

local camera = ThirdPersonCamera(player)

-- Main loop:
runProcess(function()
	
	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = application.GetActivePath()
	local path = RenderPath3D()
	--path.SetLightShaftsEnabled(true)
	path.SetLightShaftsStrength(0.01)
	path.SetAO(AO_MSAO)
	path.SetAOPower(0.25)
	--path.SetOutlineEnabled(true)
	path.SetOutlineThreshold(0.11)
	path.SetOutlineThickness(1.7)
	path.SetOutlineColor(0,0,0,0.6)
	path.SetBloomThreshold(5)
	application.SetActivePath(path)

	--application.SetInfoDisplay(false)
	application.SetFPSDisplay(true)
	--path.SetResolutionScale(0.5)
	--path.SetFSR2Enabled(true)
	--path.SetFSR2Preset(FSR2_Preset.Performance)
	--SetProfilerEnabled(true)

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

		camera:Update()

		update()
		
		if(input.Press(KEYBOARD_BUTTON_ESCAPE)) then
			-- restore previous component
			--	so if you loaded this script from the editor, you can go back to the editor with ESC
			backlog_post("EXIT")
			killProcesses()
			application.SetActivePath(prevPath)
			return
		end
		if(not backlog_isactive() and input.Press(string.byte('R'))) then
			-- reload script
			backlog_post("RELOAD")
			killProcesses()
			application.SetActivePath(prevPath)
			dofile(script_file())
			return
		end

	end
	
end)

-- Draw
runProcess(function()

	local help_text = "Wicked Engine Character demo (LUA)\n\n"
	help_text = help_text .. "Controls:\n"
	help_text = help_text .. "#############\n"
	help_text = help_text .. "WASD/arrows/left analog stick: walk\n"
	help_text = help_text .. "SHIFT/right shoulder button: walk -> jog\n"
	help_text = help_text .. "E/left shoulder button: jog -> run\n"
	help_text = help_text .. "SPACE/gamepad X/gamepad button 2: Jump\n"
	help_text = help_text .. "Right Mouse Button/Right thumbstick: rotate camera\n"
	help_text = help_text .. "Scoll middle mouse/Left-Right triggers: adjust camera distance\n"
	help_text = help_text .. "ESCAPE key: quit\n"
	help_text = help_text .. "R: reload script\n"
	help_text = help_text .. "H: toggle debug draw\n"
	
	while true do

		-- Do some debug draw geometry:
			
		DrawDebugText(help_text, Vector(0,2,2), Vector(1,1,1,1), 0.1, DEBUG_TEXT_DEPTH_TEST)

		if input.Press(string.byte('H')) then
			debug = not debug
		end

		if debug and backlog_isactive() == false then

			local model_transform = scene.Component_GetTransform(player.model)
			local target_transform = scene.Component_GetTransform(player.target)
			
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
			--str = str .. "Force = " .. player.force.GetX() .. ", " .. player.force.GetY() .. "," .. player.force.GetZ() .. "\n"
			DrawDebugText(str, vector.Add(capsule.GetBase(), Vector(0,0.4)), Vector(1,1,1,1), 1, DEBUG_TEXT_CAMERA_FACING | DEBUG_TEXT_CAMERA_SCALING)

		end

		-- Wait for the engine to render the scene
		render()
	end
end)



-- Patrol waypoints:

local waypoints = {
	scene.Entity_FindByName("waypoint1"),
	scene.Entity_FindByName("waypoint2"),

	scene.Entity_FindByName("waypoint3"),
	scene.Entity_FindByName("waypoint4"),
	scene.Entity_FindByName("waypoint5"),
	scene.Entity_FindByName("waypoint6"),

	scene.Entity_FindByName("waypoint7"),
	scene.Entity_FindByName("waypoint8"),
	scene.Entity_FindByName("waypoint9"),
	scene.Entity_FindByName("waypoint10"),
	scene.Entity_FindByName("waypoint11"),
	scene.Entity_FindByName("waypoint12"),
	scene.Entity_FindByName("waypoint13"),
	scene.Entity_FindByName("waypoint14"),
	scene.Entity_FindByName("waypoint15"),
	scene.Entity_FindByName("waypoint16"),
	scene.Entity_FindByName("waypoint17"),
	scene.Entity_FindByName("waypoint18"),
	scene.Entity_FindByName("waypoint19"),
	scene.Entity_FindByName("waypoint20"),
	scene.Entity_FindByName("waypoint21"),
	scene.Entity_FindByName("waypoint22"),
	scene.Entity_FindByName("waypoint23"),
	scene.Entity_FindByName("waypoint24"),
	scene.Entity_FindByName("waypoint25"),
}

-- Simplest 1-2 patrol:
if(
	waypoints[1] ~= INVALID_ENTITY and 
	waypoints[2] ~= INVALID_ENTITY
) then
	npcs[1].patrol_waypoints = {
		{
			entity = waypoints[1],
			wait = 0,
		},
		{
			entity = waypoints[2],
			wait = 2,
		},
	}
end

-- Some more advanced, toggle between walk and jog, also swimming (because waypoints are across water mesh in test level):
if(
	waypoints[3] ~= INVALID_ENTITY and 
	waypoints[4] ~= INVALID_ENTITY and
	waypoints[5] ~= INVALID_ENTITY and
	waypoints[6] ~= INVALID_ENTITY
) then
	npcs[2].patrol_waypoints = {
		{
			entity = waypoints[3],
			wait = 0,
			state = States.JOG,
		},
		{
			entity = waypoints[4],
			wait = 0,
		},
		{
			entity = waypoints[5],
			wait = 2,
		},
		{
			entity = waypoints[6],
			wait = 0,
			state = States.JOG,
		},
	}
end


-- Run long circle:
if(
	waypoints[7] ~= INVALID_ENTITY and 
	waypoints[8] ~= INVALID_ENTITY and 
	waypoints[9] ~= INVALID_ENTITY and 
	waypoints[10] ~= INVALID_ENTITY and 
	waypoints[11] ~= INVALID_ENTITY and 
	waypoints[12] ~= INVALID_ENTITY and 
	waypoints[13] ~= INVALID_ENTITY and 
	waypoints[14] ~= INVALID_ENTITY and 
	waypoints[15] ~= INVALID_ENTITY and 
	waypoints[16] ~= INVALID_ENTITY and 
	waypoints[17] ~= INVALID_ENTITY and 
	waypoints[18] ~= INVALID_ENTITY and 
	waypoints[19] ~= INVALID_ENTITY and 
	waypoints[20] ~= INVALID_ENTITY and 
	waypoints[21] ~= INVALID_ENTITY and 
	waypoints[22] ~= INVALID_ENTITY and 
	waypoints[23] ~= INVALID_ENTITY and 
	waypoints[24] ~= INVALID_ENTITY and 
	waypoints[25] ~= INVALID_ENTITY
) then
	npcs[3].patrol_waypoints = {
		{
			entity = waypoints[7],
			state = States.JOG,
		},
		{
			entity = waypoints[8],
			state = States.JOG,
		},
		{
			entity = waypoints[9],
			state = States.JOG,
		},
		{
			entity = waypoints[10],
			state = States.JOG,
		},
		{
			entity = waypoints[11],
			state = States.JOG,
		},
		{
			entity = waypoints[12],
			state = States.JOG,
		},
		{
			entity = waypoints[13],
			state = States.JOG,
		},
		{
			entity = waypoints[14],
			state = States.JOG,
		},
		{
			entity = waypoints[15],
			state = States.JOG,
		},
		{
			entity = waypoints[16],
			state = States.JOG,
		},
		{
			entity = waypoints[17],
			state = States.JOG,
		},
		{
			entity = waypoints[18],
			state = States.JOG,
			wait = 2, -- little wait at top of slope
		},
		{
			entity = waypoints[19],
			state = States.JOG,
		},
		{
			entity = waypoints[20],
			state = States.JOG,
		},
		{
			entity = waypoints[21],
			state = States.JOG,
		},
		{
			entity = waypoints[22],
			state = States.JOG,
		},
		{
			entity = waypoints[23],
			state = States.JOG,
		},
		{
			entity = waypoints[24],
			state = States.JOG,
		},
		{
			entity = waypoints[25],
			state = States.JOG,
		},
	}
end

