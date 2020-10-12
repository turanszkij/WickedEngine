-- Lua Fighting game sample script
--
-- README:
--
-- Structure of this script:
--	
--	**) Character "class"	- holds all character specific information, like hitboxes, moves, state machine, and Update(), Input() functions
--	***) ResolveCharacters() function	- updates the two players and checks for collision, moves the camera, etc.
--	****) Main loop process	- initialize script and do call update() in an infinite loop
--
--
-- The script is programmable using common fighting game "numpad notations" (read this if you are unfamiliar: http://www.dustloop.com/wiki/index.php/Notation )
-- There are four action buttons: A, B, C, D
--	So for example a forward motion combined with action D would look like this in code: "6D" 
--	A D action without motion (neutral D) would be: "5D"
--	A quarter circle forward + A would be "236A"
--	"Shoryuken" + A command would be: "623A"
--	For a full circle motion, the input would be: "23698741"
--		But because that full circle motion is difficult to execute properly, we can make it easier by accpeting similar inputs, like:
--			"2684" or "2369874"...
--	The require_input("inputstring") facility will help detect instant input execution
--	The require_input_window("inputstring", allowed_latency_window) facility can detect inputs that are executed over multiple frames
--	Neutral motion is "5", that can help to specify in some cases, for example: double tap right button would need a neutral in between the two presses, like this: 656. Also, "5A" means that A is pressed only once, not continuously.

local scene = GetScene()

local debug_draw = true -- press H button to toggle
local player2_control = "CPU" -- can be "CPU" or "Controller2". player1 will always use Keyboard and Controller1 for now (for simplicity)

-- **The character "class" is a wrapper function that returns a local internal table called "self"
local function Character(face, skin_color, shirt_color, hair_color, shoe_color)
	local self = {
		model = INVALID_ENTITY,
		effect_dust = INVALID_ENTITY,
		effect_hit = INVALID_ENTITY,
		effect_guard = INVALID_ENTITY,
		effect_spark = INVALID_ENTITY,
		model_fireball = INVALID_ENTITY,
		effect_fireball = INVALID_ENTITY,
		effect_fireball_haze = INVALID_ENTITY,
		sprite_hpbar_background = Sprite(),
		sprite_hpbar_hp = Sprite(),
		sprite_hpbar_pattern = Sprite(),
		sprite_hpbar_pattern2 = Sprite(),
		sprite_hpbar_border = Sprite(),
		face = 1, -- face direction (X)
		request_face = 1, -- the suggested facing of this player, it might not be the actual facing if the player haven't been able to turn yet (for example an other action hasn't finished yet)
		position = Vector(), -- the absolute position of this player in the world, a 2D Vector
		velocity = Vector(), -- velocity will affect position
		force = Vector(), -- force will affect velocity
		frame = 0, -- the current animation's elapsed frames starting from 0
		input_buffer = {}, -- list of input history
		clipbox = AABB(), -- AABB that makes the two players not clip into each other
		hurtboxes = {}, -- list of AABBs that the opponent can hit with a hitbox
		hitboxes = {}, -- list of AABBs that can hit the opponent's hurtboxes
		guardboxes = {}, -- list of AABBs that can indicate to the opponent that guarding can be started
		hitconfirms = {}, -- contains the frames in which the opponent was hit. Resets in the beginning of every action
		hitconfirms_guard = {}, -- contains the frames in which the opponent was hit but guarded. Resets in the beginning of every action
		hurt = false, -- will be true in a frame if this player was hit by the opponent
		jumps_remaining = 2, -- for double jump
		push = Vector(), -- will affect opponent's velocity
		can_guard = false, -- true when player is inside opponent's guard box and can initiate guarding state
		guarding = false, -- if true, player can't be hit
		max_hp = 10000, -- maximum health
		hp = 10000, -- current health
		fireball_active = false, -- fireball is on screen or not

		-- Box helpers:
		set_box_local = function(self, boxlist, aabb) -- sets box relative to character orientation
			table.insert(boxlist, aabb.Transform(scene.Component_GetTransform(self.model).GetMatrix()))
		end,
		set_box_global = function(self, boxlist, aabb) -- sets box in absolute orientation
			table.insert(boxlist, aabb)
		end,

		-- Effect helpers:
		spawn_effect_hit = function(self, pos)

			-- depending on if the attack is guarded or not, we will spawn different effects:
			local emitter_entity = INVALID_ENTITY
			local burst_count = 0
			local spark_color = Vector()
			if(self:require_hitconfirm_guard()) then
				emitter_entity = self.effect_guard
				burst_count = 4
				spark_color = Vector(0,0.5,1,1)
			else
				emitter_entity = self.effect_hit
				burst_count = 50
				spark_color = Vector(1,0.5,0,1)
			end

			scene.Component_GetEmitter(emitter_entity).Burst(burst_count)
			local transform_component = scene.Component_GetTransform(emitter_entity)
			transform_component.ClearTransform()
			transform_component.Translate(pos)

			scene.Component_GetEmitter(self.effect_spark).Burst(4)
			transform_component = scene.Component_GetTransform(self.effect_spark)
			transform_component.ClearTransform()
			transform_component.Translate(pos)
			local material_component_spark = scene.Component_GetMaterial(self.effect_spark)
			material_component_spark.SetBaseColor(spark_color)

			runProcess(function() -- this sub-process will spawn a light, wait a bit then remove it
				local entity = CreateEntity()
				local light_transform = scene.Component_CreateTransform(entity)
				light_transform.Translate(pos)
				local light_component = scene.Component_CreateLight(entity)
				light_component.SetType(POINT)
				light_component.SetRange(8)
				light_component.SetEnergy(4)
				if(self:require_hitconfirm_guard()) then
					light_component.SetColor(Vector(0,0.5,1)) -- guarded attack emits blueish light
				else
					light_component.SetColor(Vector(1,0.5,0)) -- successful attack emits orangeish light
				end
				light_component.SetCastShadow(false)
				waitSeconds(0.1)
				scene.Entity_Remove(entity)
			end)
		end,
		spawn_effect_fireball = function(self, pos, velocity)
			self.fireball_active = true
			local fireball_life = 8 -- can hit the player so many times
			runProcess(function() -- first subprocess begins effect
				scene.Component_GetEmitter(self.effect_fireball).SetEmitCount(2000)
				scene.Component_GetEmitter(self.effect_fireball_haze).SetEmitCount(10)
				local transform_component = scene.Component_GetTransform(self.model_fireball)
				transform_component.ClearTransform()
				transform_component.Translate(pos)
			end)
			runProcess(function()
				for i=1,120,1 do -- move the fireball effect for some frames
					local transform_component = scene.Component_GetTransform(self.model_fireball)
					if(fireball_life == 0) then
						break
					end
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(transform_component.GetPosition())
						fireball_life = fireball_life - 1
					else
						transform_component.Translate(velocity)
						transform_component.UpdateTransform()
					end
					waitSignal("subprocess_update" .. self.model)
				end
				scene.Component_GetEmitter(self.effect_fireball).SetEmitCount(0)
				scene.Component_GetEmitter(self.effect_fireball_haze).SetEmitCount(0)
				scene.Component_GetTransform(self.model_fireball).Translate(Vector(0,0,-1000000))
				self.fireball_active = false
			end)
			runProcess(function() -- while there is a fireball, activate hitboxes
				while(self.fireball_active) do
					local transform_component = scene.Component_GetTransform(self.model_fireball)
					self:set_box_global(self.hitboxes, AABB(Vector(-0.5,-0.5), Vector(0.5,0.5)).Transform(transform_component.GetMatrix()) )
					self:set_box_global(self.guardboxes, AABB(Vector(-10,-10), Vector(10,10)).Transform(transform_component.GetMatrix()) )
					waitSignal("subprocess_update_collisions" .. self.model)
				end
			end)
		end,
		spawn_effect_dust = function(self, pos)
			local emitter_component = scene.Component_GetEmitter(self.effect_dust).Burst(10)
			local transform_component = scene.Component_GetTransform(self.effect_dust)
			transform_component.ClearTransform()
			transform_component.Translate(pos)
		end,

		-- Common requirement conditions for state transitions:
		require_input_window = function(self, inputString, window) -- player input notation with some tolerance to input execution window (in frames) (help: see readme on top of this file)
			-- reduce remaining input with non-expired commands:
			for i,element in ipairs(self.input_buffer) do
				if(element.age <= window and element.command == string.sub(inputString, 0, string.len(element.command))) then
					inputString = string.sub(inputString, string.len(element.command) + 1)
					if(inputString == "") then
						return true
					end
				end
			end
			return false -- match failure
		end,
		require_input = function(self, inputString) -- player input notation (immediate) (help: see readme on top of this file)
			return self:require_input_window(inputString, string.len(inputString))
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
		require_hitconfirm = function(self, frame) -- true if this player hit the other, optionally provide frame number to check if hit occured in the past or not
			frame = frame or (self.frame - 1)
			for i,hit in ipairs(self.hitconfirms) do
				if(hit == frame) then
					return true
				end
			end
			return false
		end,
		require_hitconfirm_guard = function(self, frame) -- true if this player hit the opponent but the opponent guarded it, optionally provide frame number to check if hit occured in the past or not
			frame = frame or (self.frame - 1)
			for i,hit in ipairs(self.hitconfirms_guard) do
				if(hit == frame) then
					return true
				end
			end
			return false
		end,
		require_hurt = function(self) -- true if this player was hit by the other
			return self.hurt
		end,
		require_guard = function(self) -- true if player can start guarding
			return self.can_guard
		end,
		require_dead = function(self) -- true if player can start guarding
			return self.hp <= 0
		end,
		
		-- Common motion helpers:
		require_motion_qcf = function(self, button)
			local window = 20
			return 
				self:require_input_window("236" .. button, window) or
				self:require_input_window("26" .. button, window)
		end,
		require_motion_shoryuken = function(self, button)
			local window = 20
			return 
				self:require_input_window("623" .. button, window) or
				self:require_input_window("626" .. button, window)
		end,

		-- List all possible states:
		states = {
			-- Common states:
			--	anim_name			: name of the animation track in the model file
			--	anim				: this will be initialized automatically to animation entity reference if the animation track is found by name
			--	clipbox				: (optional) AABB that describes the clip area for the character in this state. Characters can not clip into each other's clip area
			--	hurtbox				: (optional) AABB that describes the area the character can be hit/hurt
			--	update_collision	: (optional) this function will be executed in the continuous collision detection phase, multiple times per frame. Describe the hitboxes here
			--	update				: (optional) this function will be executed once every frame
			Idle = {
				anim_name = "Idle",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					self.jumps_remaining = 2
				end,
			},
			Walk_Backward = {
				anim_name = "Back",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					self.force = vector.Add(self.force, Vector(-0.025 * self.face, 0))
				end,
			},
			Walk_Forward = {
				anim_name = "Forward",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					self.force = vector.Add(self.force, Vector(0.025 * self.face, 0))
				end,
			},
			Dash_Backward = {
				anim_name = "BDash",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					if(self:require_window(0,2)) then
						self.force = vector.Add(self.force, Vector(-0.07 * self.face, 0.1))
					end
					if(self:require_frame(14)) then
						self:spawn_effect_dust(self.position)
					end
				end,
			},
			RunStart = {
				anim_name = "RunStart",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-0.5), Vector(2, 5)),
				hurtbox = AABB(Vector(-0.7), Vector(2.2, 5.5)),
			},
			Run = {
				anim_name = "Run",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-0.5), Vector(2, 5)),
				hurtbox = AABB(Vector(-0.7), Vector(2.2, 5.5)),
				update = function(self)
					self.force = vector.Add(self.force, Vector(0.08 * self.face, 0))
					if(self.frame % 15 == 0) then
						self:spawn_effect_dust(self.position)
					end
				end,
			},
			RunEnd = {
				anim_name = "RunEnd",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-0.5), Vector(2, 5)),
				hurtbox = AABB(Vector(-0.7), Vector(2.2, 5.5)),
			},
			Jump = {
				anim_name = "Jump",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					if(self.frame == 0) then
						self.jumps_remaining = self.jumps_remaining - 1
						self.velocity.SetY(0)
						self.force = vector.Add(self.force, Vector(0, 0.8))
						if(self.position.GetY() == 0) then
							self:spawn_effect_dust(self.position)
						end
					end
				end,
			},
			JumpBack = {
				anim_name = "Jump",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					if(self.frame == 0) then
						self.jumps_remaining = self.jumps_remaining - 1
						self.velocity.SetY(0)
						self.force = vector.Add(self.force, Vector(-0.2 * self.face, 0.8))
						if(self.position.GetY() == 0) then
							self:spawn_effect_dust(self.position)
						end
					end
				end,
			},
			JumpForward = {
				anim_name = "Jump",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					if(self.frame == 0) then
						self.jumps_remaining = self.jumps_remaining - 1
						self.velocity.SetY(0)
						self.force = vector.Add(self.force, Vector(0.2 * self.face, 0.8))
						if(self.position.GetY() == 0) then
							self:spawn_effect_dust(self.position)
						end
					end
				end,
			},
			FallStart = {
				anim_name = "FallStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			Fall = {
				anim_name = "Fall",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			FallEnd = {
				anim_name = "FallEnd",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					if(self:require_frame(2)) then
						self:spawn_effect_dust(self.position)
					end
				end,
			},
			CrouchStart = {
				anim_name = "CrouchStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 3)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 3.5)),
			},
			Crouch = {
				anim_name = "Crouch",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 3)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 3.5)),
			},
			CrouchEnd = {
				anim_name = "CrouchEnd",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			Turn = {
				anim_name = "Turn",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					if(self.frame == 0) then
						self.face = self.request_face
					end
				end,
			},
			Taunt = {
				anim_name = "Taunt",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			Guard = {
				anim_name = "Block",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				update = function(self)
					self.guarding = true
				end,
			},
			LowGuard = {
				anim_name = "LowBlock",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 3)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 3.5)),
				update = function(self)
					self.guarding = true
				end,
			},
			
			-- Attack states:
			LightPunch = {
				anim_name = "LightPunch",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,8)),
				update_collision = function(self)
					if(self:require_frame(5)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0.5,2), Vector(3,5)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.5 * self.face,4,-1)))
						self.push = Vector(0.1 * self.face)
					end
				end,
			},
			ForwardLightPunch = {
				anim_name = "FLightPunch",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,8)),
				update_collision = function(self)
					if(self:require_window(12,14)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0.5,2), Vector(3.5,6)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.5 * self.face,4,-1)))
						self.push = Vector(0.12 * self.face)
					end
				end,
			},
			HeavyPunch = {
				anim_name = "HeavyPunch",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(8,10)),
				update_collision = function(self)
					if(self:require_window(3,6)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0.5,2), Vector(3.5,5)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.5 * self.face,4,-1)))
						self.push = Vector(0.2 * self.face)
					end
				end,
			},
			LowPunch = {
				anim_name = "LowPunch",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 3)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 3.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,4)),
				update_collision = function(self)
					if(self:require_window(3,6)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0.5,0), Vector(2.8,3)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.5 * self.face,2,-1)))
						self.push = Vector(0.1 * self.face)
					end
				end,
			},
			LightKick = {
				anim_name = "LightKick",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,8)),
				update_collision = function(self)
					if(self:require_window(7,8)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,0), Vector(3,3)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2 * self.face,2,-1)))
						self.push = Vector(0.1 * self.face)
					end
				end,
			},
			HeavyKick = {
				anim_name = "HeavyKick",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,8)),
				update_collision = function(self)
					if(self:require_window(10,13)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,0), Vector(4,3)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.6 * self.face,1.4,-1)))
						self.push = Vector(0.15 * self.face)
					end
				end,
			},
			AirKick = {
				anim_name = "AirKick",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,-6),Vector(6,8)),
				update_collision = function(self)
					if(self:require_window(6,8)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,0), Vector(3,3)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2 * self.face,2,-1)))
						self.push = Vector(0.2 * self.face)
					end
				end,
			},
			AirHeavyKick = {
				anim_name = "AirHeavyKick",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,-6),Vector(6,8)),
				update_collision = function(self)
					if(self:require_window(6,8)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,-3), Vector(3,3)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2 * self.face,1,-1)))
						self.push = Vector(0.25 * self.face)
					end
				end,
			},
			LowKick = {
				anim_name = "LowKick",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 3)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 3.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,4)),
				update_collision = function(self)
					if(self:require_window(3,6)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0.5,0), Vector(3,3)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2 * self.face,1,-1)))
						self.push = Vector(0.1 * self.face)
					end
				end,
			},
			ChargeKick = {
				anim_name = "ChargeKick",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(0), Vector(2, 5)),
				hurtbox = AABB(Vector(0), Vector(2.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(16,8)),
				update_collision = function(self)
					if(self:require_window(11,41)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0.5,0), Vector(5.6,3)) )
					end
				end,
				update = function(self)
					if(self:require_frame(4)) then
						self.force = vector.Add(self.force, Vector(0.9 * self.face))
					end
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(5 * self.face,3,-1)))
						if(self:require_hitconfirm_guard()) then
							self.push = Vector(0.8 * self.face, 0)
						else
							self.push = Vector(0.8 * self.face, 0.2)
						end
					end
				end,
			},
			Uppercut = {
				anim_name = "Uppercut",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(6,8)),
				update_collision = function(self)
					if(self:require_window(3,5)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,3), Vector(2.3,7)) )
					end
				end,
				update = function(self)
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.5 * self.face,4,-1)))
						if(self:require_hitconfirm_guard()) then
							self.push = Vector(0.1 * self.face, 0) -- if guarded, don't push opponent up
						else
							self.push = Vector(0.1 * self.face, 0.6) -- if not guarded, push opponent up
						end
					end
				end,
			},
			Jaunt = {
				anim_name = "SpearJaunt",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1.5), Vector(1.5, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(16,8)),
				update_collision = function(self)
					if(self:require_window(17,40)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,1), Vector(4.5,5)) )
					end
				end,
				update = function(self)
					if(self:require_frame(16)) then
						self.force = vector.Add(self.force, Vector(1.3 * self.face))
					end
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(3 * self.face,3.6,-1)))
						self.push = Vector(0.3 * self.face)
					end
				end,
			},
			Shoryuken = {
				anim_name = "Shoryuken",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(8,15)),
				update_collision = function(self)
					if(self:require_window(0,4)) then -- little invincibility at start
						self.hurtboxes = {}
					end
					if(self:require_window(2,20)) then
						self:set_box_local(self.hitboxes, AABB(Vector(0,2), Vector(2.3,7)) )
					end
				end,
				update = function(self)
					if(self:require_frame(0)) then
						self.force = vector.Add(self.force, Vector(0.3 * self.face, 0.9))
					end
					if(self:require_hitconfirm()) then
						self:spawn_effect_hit(vector.Add(self.position, Vector(2.5 * self.face,4,-1)))
						if(self:require_window(2,3) and not self:require_hitconfirm_guard()) then
							self.push = Vector(0, 1)
						end
					end
				end,
			},
			Fireball = {
				anim_name = "SpearJaunt", -- todo: needs new anim
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1.5), Vector(1.5, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
				guardbox = AABB(Vector(-2,0),Vector(16,8)),
				update_collision = function(self)
					self.velocity = Vector()
				end,
				update = function(self)
					if(self:require_frame(16)) then
						local fireball_velocity = Vector(self.face * 0.3)
						if(self.position.GetY() > 0) then
							fireball_velocity.SetY(-0.18)
						end
						self:spawn_effect_fireball(vector.Add(self.position, Vector(2.5 * self.face, 3.4)), fireball_velocity)
					end
				end,
			},
			
			-- Hurt states:
			StaggerStart = {
				anim_name = "StaggerStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			Stagger = {
				anim_name = "Stagger",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			StaggerEnd = {
				anim_name = "StaggerEnd",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},

			StaggerCrouchStart = {
				anim_name = "StaggerCrouchStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			StaggerCrouch = {
				anim_name = "StaggerCrouch",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			StaggerCrouchEnd = {
				anim_name = "StaggerCrouchEnd",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},

			StaggerAirStart = {
				anim_name = "StaggerAirStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			StaggerAir = {
				anim_name = "StaggerAir",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
			StaggerAirEnd = {
				anim_name = "StaggerAirEnd",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 1)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 1)),
				update = function(self)
					if(self.position.GetY() < 1 and self.velocity.GetY() < 0) then
						self:spawn_effect_dust(self.position)
					end
				end,
			},
			
			Downed = {
				anim_name = "Downed",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(-1), Vector(1, 1)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 1)),
			},
			DownedDieStart = {
				anim_name = "DownedDieStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(), Vector()),
				hurtbox = AABB(Vector(), Vector()),
			},
			DownedDie = {
				anim_name = "DownedDie",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(), Vector()),
				hurtbox = AABB(Vector(), Vector()),
			},
			DieStart = {
				anim_name = "DieStart",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(), Vector()),
				hurtbox = AABB(Vector(), Vector()),
			},
			Die = {
				anim_name = "Die",
				anim = INVALID_ENTITY,
				clipbox = AABB(Vector(), Vector()),
				hurtbox = AABB(Vector(), Vector()),
			},
			Getup = {
				anim_name = "Getup",
				anim = INVALID_ENTITY,
				looped = false,
				clipbox = AABB(Vector(-1), Vector(1, 5)),
				hurtbox = AABB(Vector(-1.2), Vector(1.2, 5.5)),
			},
		},

		-- State machine describes all possible state transitions (item order is priority high->low):
		--	StateFrom = {
		--		{ "StateTo1", condition = function(self) return [requirements that should be met] end },
		--		{ "StateTo2", condition = function(self) return [requirements that should be met] end },
		--	}
		statemachine = {
			Idle = { 
				{ "Guard", condition = function(self) return self:require_guard() and self:require_input("4") end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("B") end, },
				{ "Jaunt", condition = function(self) return self:require_motion_qcf("B") end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self:require_motion_qcf("A") end, },
				{ "Turn", condition = function(self) return self.request_face ~= self.face end, },
				{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
				{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
				{ "Jump", condition = function(self) return self:require_input("8") end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "ChargeKick", condition = function(self) return self:require_input_window("4444444444444444446C", 30) or self:require_input_window("1111111111111111116C", 30) end, },
				{ "Taunt", condition = function(self) return self:require_input_window("5T", 10) end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("5D") end, },
			},
			Walk_Backward = { 
				{ "Guard", condition = function(self) return self:require_guard() and self:require_input("4") end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("B") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
				{ "Dash_Backward", condition = function(self) return self:require_input_window("454", 7) end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "Idle", condition = function(self) return self:require_input("5") end, },
				{ "ChargeKick", condition = function(self) return self:require_input_window("4444444444444444446C", 30) or self:require_input_window("1111111111111111116C", 30) end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("5D") end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
			},
			Walk_Forward = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("B") end, },
				{ "Jaunt", condition = function(self) return self:require_motion_qcf("B") end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self:require_motion_qcf("A") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
				{ "RunStart", condition = function(self) return self:require_input_window("656", 10) end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "Idle", condition = function(self) return self:require_input("5") end, },
				{ "ChargeKick", condition = function(self) return self:require_input_window("4444444444444444446C", 30) or self:require_input_window("1111111111111111116C", 30) end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("5D") end, },
			},
			Dash_Backward = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			RunStart = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Run", condition = function(self) return self:require_animationfinish() end, },
			},
			Run = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Jump", condition = function(self) return self:require_input("8") end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "RunEnd", condition = function(self) return not self:require_input("6") end, },
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("B") end, },
				{ "Jaunt", condition = function(self) return self:require_motion_qcf("B") end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self:require_motion_qcf("A") end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("5D") end, },
			},
			RunEnd = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
				{ "Guard", condition = function(self) return self:require_guard() and self:require_input("4") end, },
				{ "Shoryuken", condition = function(self) return self:require_motion_shoryuken("B") end, },
				{ "Jaunt", condition = function(self) return self:require_motion_qcf("B") end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self:require_motion_qcf("A") end, },
				{ "Turn", condition = function(self) return self.request_face ~= self.face end, },
				{ "Walk_Forward", condition = function(self) return self:require_input("6") end, },
				{ "Walk_Backward", condition = function(self) return self:require_input("4") end, },
				{ "Jump", condition = function(self) return self:require_input("8") end, },
				{ "JumpBack", condition = function(self) return self:require_input("7") end, },
				{ "JumpForward", condition = function(self) return self:require_input("9") end, },
				{ "CrouchStart", condition = function(self) return self:require_input("1") or self:require_input("2") or self:require_input("3") end, },
				{ "ChargeKick", condition = function(self) return self:require_input_window("4444444444444444446C", 30) or self:require_input_window("1111111111111111116C", 30) end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_input("6A") end, },
				{ "LightPunch", condition = function(self) return self:require_input("5A") end, },
				{ "HeavyPunch", condition = function(self) return self:require_input("5B") end, },
				{ "LightKick", condition = function(self) return self:require_input("5C") end, },
				{ "HeavyKick", condition = function(self) return self:require_input("5D") end, },
			},
			Jump = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self.position.GetY() > 3 and self:require_motion_qcf("A") end, },
				{ "AirHeavyKick", condition = function(self) return self.position.GetY() > 4 and self:require_input("D") end, },
				{ "AirKick", condition = function(self) return self.position.GetY() > 2 and self:require_input("C") end, },
				{ "FallStart", condition = function(self) return self.velocity.GetY() <= 0 end, },
			},
			JumpForward = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self.position.GetY() > 3 and self:require_motion_qcf("A") end, },
				{ "AirHeavyKick", condition = function(self) return self.position.GetY() > 4 and self:require_input("D") end, },
				{ "AirKick", condition = function(self) return self.position.GetY() > 2 and self:require_input("C") end, },
				{ "FallStart", condition = function(self) return self.velocity.GetY() <= 0 end, },
			},
			JumpBack = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self.position.GetY() > 3 and self:require_motion_qcf("A") end, },
				{ "AirHeavyKick", condition = function(self) return self.position.GetY() > 4 and self:require_input("D") end, },
				{ "AirKick", condition = function(self) return self.position.GetY() > 2 and self:require_input("C") end, },
				{ "FallStart", condition = function(self) return self.velocity.GetY() <= 0 end, },
			},
			FallStart = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self.position.GetY() > 3 and self:require_motion_qcf("A") end, },
				{ "FallEnd", condition = function(self) return self.position.GetY() <= 0.5 end, },
				{ "Fall", condition = function(self) return self:require_animationfinish() end, },
				{ "AirHeavyKick", condition = function(self) return self.position.GetY() > 4 and self:require_input("D") end, },
				{ "AirKick", condition = function(self) return self.position.GetY() > 2 and self:require_input("C") end, },
			},
			Fall = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self.position.GetY() > 3 and self:require_motion_qcf("A") end, },
				{ "Jump", condition = function(self) return self.jumps_remaining > 0 and self:require_input_window("58", 7) end, },
				{ "JumpBack", condition = function(self) return self.jumps_remaining > 0 and self:require_input_window("57", 7) end, },
				{ "JumpForward", condition = function(self) return self.jumps_remaining > 0 and self:require_input_window("59", 7) end, },
				{ "FallEnd", condition = function(self) return self.position.GetY() <= 0.5 end, },
				{ "AirHeavyKick", condition = function(self) return self.position.GetY() > 4 and self:require_input("D") end, },
				{ "AirKick", condition = function(self) return self.position.GetY() > 2 and self:require_input("C") end, },
			},
			FallEnd = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fireball", condition = function(self) return not self.fireball_active and self:require_motion_qcf("A") end, },
				{ "Turn", condition = function(self) return self.position.GetY() <= 0 and self.request_face ~= self.face end, },
				{ "Idle", condition = function(self) return self.position.GetY() <= 0 and self:require_animationfinish() end, },
				{ "RunStart", condition = function(self) return self.position.GetY() <= 0 and self:require_input_window("656", 7) end, },
				{ "Dash_Backward", condition = function(self) return self.position.GetY() <= 0 and self:require_input_window("454", 7) end, },
			},
			CrouchStart = { 
				{ "StaggerCrouchStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_input("5") end, },
				{ "Crouch", condition = function(self) return (self:require_input("1") or self:require_input("2") or self:require_input("3")) and self:require_animationfinish() end, },
			},
			Crouch = { 
				{ "LowGuard", condition = function(self) return self:require_guard() and self:require_input("1") end, },
				{ "StaggerCrouchStart", condition = function(self) return self:require_hurt() end, },
				{ "CrouchEnd", condition = function(self) return self:require_input("5") or self:require_input("4") or self:require_input("6") or self:require_input("7") or self:require_input("8") or self:require_input("9") end, },
				{ "LowPunch", condition = function(self) return self:require_input("2A") or self:require_input("1A") or self:require_input("3A") end, },
				{ "LowKick", condition = function(self) return self:require_input("2C") or self:require_input("1C") or self:require_input("3C") end, },
				{ "Uppercut", condition = function(self) return self:require_input("2B") or self:require_input("1B") or self:require_input("3B") end, },
			},
			CrouchEnd = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Turn = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Taunt = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Guard = { 
				{ "Turn", condition = function(self) return self.request_face ~= self.face end, },
				{ "Idle", condition = function(self) return not self:require_input("4") end, },
			},
			LowGuard = { 
				{ "Turn", condition = function(self) return self.request_face ~= self.face end, },
				{ "Crouch", condition = function(self) return not self:require_input("1") end, },
			},

			LightPunch = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
				{ "LightPunch", condition = function(self) return self:require_window(11,16) and self:require_hitconfirm(5) and self:require_input_window("5A", 10) end, },
				{ "HeavyPunch", condition = function(self) return self:require_window(11,16) and self:require_hitconfirm(5) and self:require_input_window("5B", 10) end, },
			},
			ForwardLightPunch = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			HeavyPunch = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
				{ "ForwardLightPunch", condition = function(self) return self:require_window(11,21) and self:require_hitconfirm(6) and self:require_input_window("6A", 10) end, },
				{ "LightKick", condition = function(self) return self:require_window(11,21) and self:require_hitconfirm(6) and self:require_input_window("5C", 10) end, },
				{ "HeavyKick", condition = function(self) return self:require_window(11,21) and self:require_hitconfirm(6) and self:require_input_window("5D", 10) end, },
			},
			LowPunch = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Crouch", condition = function(self) return self:require_animationfinish() end, },
			},
			LightKick = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
				{ "HeavyKick", condition = function(self) return self:require_window(11,19) and self:require_hitconfirm(8) and self:require_input_window("5D", 10) end, },
			},
			HeavyKick = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			AirKick = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() end, },
				{ "Fall", condition = function(self) return self:require_animationfinish() end, },
			},
			AirHeavyKick = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() end, },
				{ "Fall", condition = function(self) return self:require_animationfinish() end, },
			},
			LowKick = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Crouch", condition = function(self) return self:require_animationfinish() end, },
			},
			ChargeKick = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Uppercut = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
				{ "Jump", condition = function(self) return self:require_window(10,26) and self:require_hitconfirm(4) and self:require_input_window("8", 20) end, },
				{ "JumpBack", condition = function(self) return self:require_window(10,26) and self:require_hitconfirm(4) and self:require_input_window("7", 20) end, },
				{ "JumpForward", condition = function(self) return self:require_window(10,26) and self:require_hitconfirm(4) and self:require_input_window("9", 20) end, },
			},
			Jaunt = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			Shoryuken = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "FallStart", condition = function(self) return self:require_animationfinish() end, },
			},
			Fireball = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Fall", condition = function(self) return self:require_frame(32) and self.position.GetY() > 0 end, },
				{ "Idle", condition = function(self) return self:require_frame(32) end, },
			},
			
			StaggerStart = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Stagger", condition = function(self) return self:require_animationfinish() end, },
			},
			Stagger = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "StaggerEnd", condition = function(self) return not self:require_hurt() end, },
			},
			StaggerEnd = { 
				{ "DieStart", condition = function(self) return self:require_dead() end, },
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
			
			StaggerCrouchStart = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerCrouchStart", condition = function(self) return self:require_hurt() end, },
				{ "StaggerCrouch", condition = function(self) return self:require_animationfinish() end, },
			},
			StaggerCrouch = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerCrouchStart", condition = function(self) return self:require_hurt() end, },
				{ "StaggerCrouchEnd", condition = function(self) return not self:require_hurt() end, },
			},
			StaggerCrouchEnd = { 
				{ "DieStart", condition = function(self) return self:require_dead() end, },
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() and self.position.GetY() > 0 end, },
				{ "StaggerCrouchStart", condition = function(self) return self:require_hurt() end, },
				{ "Crouch", condition = function(self) return self:require_animationfinish() end, },
			},
			
			StaggerAirStart = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() end, },
				{ "StaggerAir", condition = function(self) return self:require_animationfinish() end, },
			},
			StaggerAir = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() end, },
				{ "StaggerAirEnd", condition = function(self) return not self:require_hurt() and self.position.GetY() < 3 end, },
			},
			StaggerAirEnd = { 
				{ "StaggerAirStart", condition = function(self) return self:require_hurt() end, },
				{ "Downed", condition = function(self) return self:require_animationfinish() end, },
			},

			Downed = { 
				{ "DownedDieStart", condition = function(self) return self:require_dead() end, },
				{ "Getup", condition = function(self) return self:require_input("A") or self:require_input("B") or self:require_input("C") or self.frame > 60 end, },
			},
			DownedDieStart = { 
				{ "DownedDie", condition = function(self) return self:require_animationfinish() end, },
			},
			DownedDie = { 
			},
			DieStart = { 
				{ "Die", condition = function(self) return self:require_animationfinish() end, },
			},
			Die = { 
			},
			Getup = { 
				{ "StaggerStart", condition = function(self) return self:require_hurt() end, },
				{ "Idle", condition = function(self) return self:require_animationfinish() end, },
			},
		},

		state = "Idle", -- starting state

	
		-- Ends the current state:
		EndState = function(self)
			scene.Component_GetAnimation(self.states[self.state].anim).Stop()
		end,
		-- Starts a new state:
		StartState = function(self, dst_state)
			local anim = scene.Component_GetAnimation(self.states[dst_state].anim)
			anim.Play()
			anim.SetAmount(0)
			self.frame = 0
			self.state = dst_state
			self.hitconfirms = {}
			self.hitconfirms_guard = {}
		end,
		-- Step state machine and execute current state:
		ExecuteStateMachine = function(self)
			self.frame = self.frame + 1
			self.guarding = false

			-- Parse state machine at current state and perform transition if applicable:
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
						break
					end
				end
			end

			-- Animation blending:
			local anim = scene.Component_GetAnimation(self.states[self.state].anim)
			local anim_amount = anim.GetAmount()
			anim.SetAmount(math.lerp(anim_amount, 1, 0.2))

			-- Execute the currently active state:
			local current_state = self.states[self.state]
			if(current_state ~= nil) then
				if(current_state.update ~= nil) then
					current_state.update(self)
				end
			end

		end,
	

		Create = function(self, face, skin_color, shirt_color, hair_color, shoe_color)

			-- Load the model into a custom scene:
			--	We use a custom scene because if two models are loaded into the global scene, they will have name collisions
			--	and thus we couldn't properly query entities by name
			local model_scene = Scene()
			self.model = LoadModel(model_scene, "../models/havoc/havoc.wiscene")

			-- Place model according to starting facing direction:
			self.face = face
			self.request_face = face
			self.position = Vector(self.face * -4)

			-- Set material colors to differentiate between characters:
			model_scene.Component_GetMaterial(model_scene.Entity_FindByName("material_skin")).SetBaseColor(skin_color)
			model_scene.Component_GetMaterial(model_scene.Entity_FindByName("material_shirt")).SetBaseColor(shirt_color)
			model_scene.Component_GetMaterial(model_scene.Entity_FindByName("material_hair")).SetBaseColor(hair_color)
			model_scene.Component_GetMaterial(model_scene.Entity_FindByName("material_shoes")).SetBaseColor(shoe_color)

			-- Set user stencil ref for all objects:
			local stencilref_cutout = 1
			for i,object in ipairs(model_scene.Component_GetObjectArray()) do
				object.SetUserStencilRef(stencilref_cutout)
			end
		
			-- Initialize states:
			for i,state in pairs(self.states) do
				state.anim = model_scene.Entity_FindByName(state.anim_name)
				if(state.looped ~= nil) then
					model_scene.Component_GetAnimation(state.anim).SetLooped(state.looped)
				end
			end

			-- Move the custom scene into the global scene:
			scene.Merge(model_scene)



			-- Load effects:
			local effect_scene = Scene()
			
			effect_scene.Clear()
			LoadModel(effect_scene, "../models/emitter_dust.wiscene")
			self.effect_dust = effect_scene.Entity_FindByName("dust")  -- query the emitter entity by name
			effect_scene.Component_GetEmitter(self.effect_dust).SetEmitCount(0)  -- don't emit continuously
			scene.Merge(effect_scene)

			effect_scene.Clear()
			LoadModel(effect_scene, "../models/emitter_hiteffect.wiscene")
			self.effect_hit = effect_scene.Entity_FindByName("hit")  -- query the emitter entity by name
			effect_scene.Component_GetEmitter(self.effect_hit).SetEmitCount(0)  -- don't emit continuously
			scene.Merge(effect_scene)

			effect_scene.Clear()
			LoadModel(effect_scene, "../models/emitter_guardeffect.wiscene")
			self.effect_guard = effect_scene.Entity_FindByName("guard")  -- query the emitter entity by name
			effect_scene.Component_GetEmitter(self.effect_guard).SetEmitCount(0)  -- don't emit continuously
			scene.Merge(effect_scene)

			effect_scene.Clear()
			LoadModel(effect_scene, "../models/emitter_spark.wiscene")
			self.effect_spark = effect_scene.Entity_FindByName("spark")  -- query the emitter entity by name
			effect_scene.Component_GetEmitter(self.effect_spark).SetEmitCount(0)  -- don't emit continuously
			scene.Merge(effect_scene)

			effect_scene.Clear()
			self.model_fireball = LoadModel(effect_scene, "../models/emitter_fireball.wiscene")
			self.effect_fireball = effect_scene.Entity_FindByName("fireball")  -- query the emitter entity by name
			effect_scene.Component_GetEmitter(self.effect_fireball).SetEmitCount(0)  -- don't emit continuously
			self.effect_fireball_haze = effect_scene.Entity_FindByName("haze")  -- query the emitter entity by name
			effect_scene.Component_GetEmitter(self.effect_fireball_haze).SetEmitCount(0)  -- don't emit continuously
			effect_scene.Component_GetTransform(self.model_fireball).Translate(Vector(0,0,-1000000))
			scene.Merge(effect_scene)



			-- HP bar, etc. sprites:
			local renderPath = main.GetActivePath()

			self.sprite_hpbar_background = Sprite("hp_bar.png")
			local fx = self.sprite_hpbar_background.GetParams()
			fx.SetStencilRefMode(STENCILREFMODE_USER) -- we set the stencil ref in user space
			fx.SetStencil(STENCILMODE_NOT, stencilref_cutout)
			fx.EnableDrawRect(Vector(0, 180, 1430, 180))
			fx.SetColor(Vector(0,0,0,0.5))
			if(self.face > 0) then
				fx.EnableMirror()
				fx.SetPivot(Vector(1,0))
			else
				fx.DisableMirror()
				fx.SetPivot(Vector(0,0))
			end
			self.sprite_hpbar_background.SetParams(fx)
			renderPath.AddSprite(self.sprite_hpbar_background)

			self.sprite_hpbar_hp = Sprite("hp_bar.png")
			fx = self.sprite_hpbar_hp.GetParams()
			fx.SetStencilRefMode(STENCILREFMODE_USER) -- we set the stencil ref in user space
			fx.SetStencil(STENCILMODE_NOT, stencilref_cutout)
			fx.EnableDrawRect(Vector(0, 180, 1430, 180))
			fx.SetColor(Vector(0,1,0.5,1))
			if(self.face > 0) then
				fx.EnableMirror()
				fx.SetPivot(Vector(1,0))
			else
				fx.DisableMirror()
				fx.SetPivot(Vector(0,0))
			end
			self.sprite_hpbar_hp.SetParams(fx)
			renderPath.AddSprite(self.sprite_hpbar_hp)

			self.sprite_hpbar_pattern = Sprite("hp_bar.png", "hp_bar.png")
			fx = self.sprite_hpbar_pattern.GetParams()
			fx.SetStencilRefMode(STENCILREFMODE_USER) -- we set the stencil ref in user space
			fx.SetStencil(STENCILMODE_NOT, stencilref_cutout)
			fx.EnableDrawRect(Vector(0, 360, 1430, 180))
			fx.EnableDrawRect2(Vector(0, 180, 1430, 180))
			fx.SetColor(Vector(1,1,1,0.2))
			if(self.face > 0) then
				fx.EnableMirror()
				fx.SetPivot(Vector(1,0))
			else
				fx.DisableMirror()
				fx.SetPivot(Vector(0,0))
			end
			self.sprite_hpbar_pattern.SetParams(fx)
			local pattern_anim = self.sprite_hpbar_pattern.GetAnim()
			local movingtexanim = MovingTexAnim()
			movingtexanim.SetSpeedX(60)
			pattern_anim.SetMovingTexAnim(movingtexanim)
			self.sprite_hpbar_pattern.SetAnim(pattern_anim)
			renderPath.AddSprite(self.sprite_hpbar_pattern)

			self.sprite_hpbar_pattern2 = Sprite("hp_bar.png", "hp_bar.png")
			fx = self.sprite_hpbar_pattern2.GetParams()
			fx.SetStencilRefMode(STENCILREFMODE_USER) -- we set the stencil ref in user space
			fx.SetStencil(STENCILMODE_NOT, stencilref_cutout)
			fx.EnableDrawRect(Vector(0, 360, 1430, 180))
			fx.EnableDrawRect2(Vector(0, 180, 1430, 180))
			fx.SetColor(Vector(1,1,1,0.2))
			if(self.face > 0) then
				fx.EnableMirror()
				fx.SetPivot(Vector(1,0))
			else
				fx.DisableMirror()
				fx.SetPivot(Vector(0,0))
			end
			self.sprite_hpbar_pattern2.SetParams(fx)
			local pattern_anim = self.sprite_hpbar_pattern.GetAnim()
			local movingtexanim = MovingTexAnim()
			movingtexanim.SetSpeedX(-40)
			pattern_anim.SetMovingTexAnim(movingtexanim)
			self.sprite_hpbar_pattern2.SetAnim(pattern_anim)
			renderPath.AddSprite(self.sprite_hpbar_pattern2)

			self.sprite_hpbar_border = Sprite("hp_bar.png")
			fx = self.sprite_hpbar_border.GetParams()
			fx.SetStencilRefMode(STENCILREFMODE_USER) -- we set the stencil ref in user space
			fx.SetStencil(STENCILMODE_NOT, stencilref_cutout)
			fx.EnableDrawRect(Vector(0, 0, 1430, 180))
			if(self.face > 0) then
				fx.EnableMirror()
				fx.SetPivot(Vector(1,0))
			else
				fx.DisableMirror()
				fx.SetPivot(Vector(0,0))
			end
			self.sprite_hpbar_border.SetParams(fx)
			renderPath.AddSprite(self.sprite_hpbar_border)
			
			if(self.face > 0) then
				self.sprite_timer = Sprite("hp_bar.png")
				fx = self.sprite_timer.GetParams()
				fx.SetStencilRefMode(STENCILREFMODE_USER) -- we set the stencil ref in user space
				fx.SetStencil(STENCILMODE_NOT, stencilref_cutout)
				fx.EnableDrawRect(Vector(0,540,360,180))
				fx.SetPivot(Vector(0.5,0))
				self.sprite_timer.SetParams(fx)
				renderPath.AddSprite(self.sprite_timer)
			end

			-- Controller vibration process:
			runProcess(function()
				local fb = ControllerFeedback()
				local playerindex = 0
				local signal = ""
				
				if(self.face > 0) then
					fb.SetLEDColor(Vector(1,0,0))
					playerindex = 0
					signal = "vibrate_player1"
				else
					fb.SetLEDColor(Vector(0,0,1))
					playerindex = 1
					signal = "vibrate_player2"
				end
				input.SetControllerFeedback(fb, playerindex)

				while(true) do
					waitSignal(signal) -- wait until specific signal arrives
					fb.SetVibrationLeft(0.8)
					fb.SetVibrationRight(0.8)
					input.SetControllerFeedback(fb, playerindex) -- start vibrating controller
					waitSeconds(0.1) -- only vibrate for short time
					fb.SetVibrationLeft(0)
					fb.SetVibrationRight(0)
					input.SetControllerFeedback(fb, playerindex) -- stop vibrating controller
				end
			end)

			self:StartState(self.state)

		end,
	
		ai_state = "Idle",
		AI = function(self)
			-- todo some better AI bot behaviour
			if(self.ai_state == "Jump") then
				table.insert(self.input_buffer, {age = 0, command = '8'})
			elseif(self.ai_state == "Crouch") then
				table.insert(self.input_buffer, {age = 0, command = '2'})
			elseif(self.ai_state == "Guard" and self:require_guard()) then
				table.insert(self.input_buffer, {age = 0, command = '4'})
			elseif(self.ai_state == "Attack") then
				table.insert(self.input_buffer, {age = 0, command = "5A"})
			else
				table.insert(self.input_buffer, {age = 0, command = '5'})
			end
		end,

		-- Read input and store in the buffer:
		--	playerindex can differentiate between multiple controllers. eg: controller1 = playerindex0; controller2 = playerindex1
		Input = function(self, playerindex)

			-- read input:
			local left = input.Down(string.byte('A'), playerindex) or input.Down(GAMEPAD_BUTTON_LEFT, playerindex)
			local right = input.Down(string.byte('D'), playerindex) or input.Down(GAMEPAD_BUTTON_RIGHT, playerindex)
			local up = input.Down(string.byte('W'), playerindex) or input.Down(GAMEPAD_BUTTON_UP, playerindex)
			local down = input.Down(string.byte('S'), playerindex) or input.Down(GAMEPAD_BUTTON_DOWN, playerindex)
			local A = input.Down(KEYBOARD_BUTTON_RIGHT, playerindex) or input.Down(GAMEPAD_BUTTON_3, playerindex)
			local B = input.Down(KEYBOARD_BUTTON_UP, playerindex) or input.Down(GAMEPAD_BUTTON_4, playerindex)
			local C = input.Down(KEYBOARD_BUTTON_LEFT, playerindex) or input.Down(GAMEPAD_BUTTON_1, playerindex)
			local D = input.Down(KEYBOARD_BUTTON_DOWN, playerindex) or input.Down(GAMEPAD_BUTTON_2, playerindex)
			local T = input.Down(string.byte('T'), playerindex) or input.Down(GAMEPAD_BUTTON_5, playerindex)

			-- swap left and right if facing the opposite side:
			if(self.face < 0) then
				local tmp = right
				right = left
				left = tmp
			end

			if(up and left) then
				table.insert(self.input_buffer, {age = 0, command = '7'})
			elseif(up and right) then
				table.insert(self.input_buffer, {age = 0, command = '9'})
			elseif(up) then
				table.insert(self.input_buffer, {age = 0, command = '8'})
			elseif(down and left) then
				table.insert(self.input_buffer, {age = 0, command = '1'})
			elseif(down and right) then
				table.insert(self.input_buffer, {age = 0, command = '3'})
			elseif(down) then
				table.insert(self.input_buffer, {age = 0, command = '2'})
			elseif(left) then
				table.insert(self.input_buffer, {age = 0, command = '4'})
			elseif(right) then
				table.insert(self.input_buffer, {age = 0, command = '6'})
			elseif(not A and not B and not C and not D and not T) then
				table.insert(self.input_buffer, {age = 0, command = '5'})
			end
			
			if(A) then
				table.insert(self.input_buffer, {age = 0, command = 'A'})
			end
			if(B) then
				table.insert(self.input_buffer, {age = 0, command = 'B'})
			end
			if(C) then
				table.insert(self.input_buffer, {age = 0, command = 'C'})
			end
			if(D) then
				table.insert(self.input_buffer, {age = 0, command = 'D'})
			end
			if(T) then
				table.insert(self.input_buffer, {age = 0, command = 'T'})
			end

		end,

		-- Update character state and forces once per frame:
		Update = function(self)
			-- force from gravity:
			self.force = Vector(0,-0.04,0)

			self:ExecuteStateMachine()

			-- Manage input buffer:
			for i,element in pairs(self.input_buffer) do -- every input gets older by one frame
				element.age = element.age + 1
			end
			if(#self.input_buffer > 60) then -- only keep the last 60 inputs
				table.remove(self.input_buffer, 1)
			end

			-- apply force:
			self.velocity = vector.Add(self.velocity, self.force)

			-- aerial drag:
			self.velocity = vector.Multiply(self.velocity, 0.98)
		
		
			-- check if we are below or on the ground:
			if(self.position.GetY() <= 0 and self.velocity.GetY()<=0) then
				self.position.SetY(0) -- snap to ground
				self.velocity.SetY(0) -- don't fall below ground
				self.velocity = vector.Multiply(self.velocity, 0.88) -- ground drag
			end



			-- update sprites:

			local scaling = GetScreenWidth() / 3840.0
			local hp_percentage = self.hp / self.max_hp
			local fx = self.sprite_hpbar_background.GetParams()
			local offset_from_center = 200 * scaling
			if(fx.IsMirrorEnabled()) then
				offset_from_center = offset_from_center * -1
			end
			fx.SetPos(Vector(GetScreenWidth() / 2 + offset_from_center, GetScreenHeight() / 16))
			fx.SetSize(vector.Multiply(Vector(1430, 180), scaling))
			self.sprite_hpbar_background.SetParams(fx)
			
			fx = self.sprite_hpbar_hp.GetParams()
			fx.SetPos(Vector(GetScreenWidth() / 2 + offset_from_center, GetScreenHeight() / 16))
			fx.SetSize(vector.Multiply(Vector(1430 * hp_percentage, 180), scaling))
			fx.EnableDrawRect(Vector(0, 180, 1430 * hp_percentage, 180))
			if(hp_percentage < 0.25) then
				fx.SetColor(Vector(1,0.2,0,1))
			elseif(hp_percentage < 1) then
				fx.SetColor(Vector(1,1,0,1))
			else
				fx.SetColor(Vector(0,1,0.6,1))
			end
			self.sprite_hpbar_hp.SetParams(fx)
			
			fx = self.sprite_hpbar_pattern.GetParams()
			fx.SetPos(Vector(GetScreenWidth() / 2 + offset_from_center, GetScreenHeight() / 16))
			fx.SetSize(vector.Multiply(Vector(1430 * hp_percentage, 180), scaling))
			fx.EnableDrawRect(Vector(0, 360, 1430 * hp_percentage, 180))
			fx.EnableDrawRect2(Vector(0, 180, 1430 * hp_percentage, 180))
			self.sprite_hpbar_pattern.SetParams(fx)
			
			fx = self.sprite_hpbar_pattern2.GetParams()
			fx.SetPos(Vector(GetScreenWidth() / 2 + offset_from_center, GetScreenHeight() / 16))
			fx.SetSize(vector.Multiply(Vector(1430 * hp_percentage, 180), scaling))
			fx.EnableDrawRect(Vector(0, 370, 1430 * hp_percentage, 180))
			fx.EnableDrawRect2(Vector(0, 180, 1430 * hp_percentage, 180))
			self.sprite_hpbar_pattern2.SetParams(fx)

			fx = self.sprite_hpbar_border.GetParams()
			fx.SetPos(Vector(GetScreenWidth() / 2 + offset_from_center, GetScreenHeight() / 16))
			fx.SetSize(vector.Multiply(Vector(1430, 180), scaling))
			self.sprite_hpbar_border.SetParams(fx)

			if(self.sprite_timer ~= nil) then
				fx = self.sprite_timer.GetParams()
				fx.SetPos(Vector(GetScreenWidth()/2, GetScreenHeight() / 16))
				fx.SetSize(vector.Multiply(Vector(360, 180), scaling))
				self.sprite_timer.SetParams(fx)
			end

			signal("subprocess_update" .. self.model)
		
		end,

		-- Updates the character bounding boxes that will be used for collision. This will be processed multiple times per frame:
		UpdateCollisionState = function(self, ccd_step)
		
			-- apply velocity:
			self.position = vector.Add(self.position, vector.Multiply(self.velocity, ccd_step))
			
			-- Compute global transform for the model:
			local model_transform = scene.Component_GetTransform(self.model)
			model_transform.ClearTransform()
			model_transform.Translate(self.position)
			model_transform.Rotate(Vector(0, math.pi * ((self.face - 1) * 0.5)))
			model_transform.UpdateTransform()

			-- Reset collision boxes:
			self.clipbox = AABB()
			self.hurtboxes = {}
			self.hitboxes = {}
			self.guardboxes = {}
			
			-- Set collision boxes in local space:
			local current_state = self.states[self.state]
			if(current_state ~= nil) then
				if(current_state.clipbox ~= nil) then
					self.clipbox = current_state.clipbox.Transform(model_transform.GetMatrix())
				end
				if(current_state.hurtbox ~= nil) then
					self:set_box_local(self.hurtboxes, current_state.hurtbox)
				end
				if(current_state.guardbox ~= nil) then
					self:set_box_local(self.guardboxes, current_state.guardbox)
				end
				if(current_state.update_collision ~= nil) then
					current_state.update_collision(self)
				end
			end

			signal("subprocess_update_collisions" .. self.model)

		end,

		-- Draws the hitboxes, etc.
		DebugDraw = function(self)
			if(not debug_draw) then
				return
			end

			DrawPoint(self.position, 0.1, Vector(1,0,0,1))
			DrawLine(self.position,self.position:Add(self.velocity), Vector(0,1,0,10))
			DrawLine(vector.Add(self.position, Vector(0,1)),vector.Add(self.position, Vector(0,1)):Add(Vector(self.face)), Vector(0,0,1,1))
			DrawBox(self.clipbox.GetAsBoxMatrix(), Vector(1,1,0,1))
			for i,hitbox in ipairs(self.hitboxes) do
				DrawBox(self.hitboxes[i].GetAsBoxMatrix(), Vector(1,0,0,1))
			end
			for i,hurtbox in ipairs(self.hurtboxes) do
				DrawBox(self.hurtboxes[i].GetAsBoxMatrix(), Vector(0,1,0,1))
			end
			for i,guardbox in ipairs(self.guardboxes) do
				DrawBox(self.guardboxes[i].GetAsBoxMatrix(), Vector(0,0,1,1))
			end
		end,

	}

	self:Create(face, skin_color, shirt_color, hair_color, shoe_color)
	return self
end


-- script camera state:
local camera_position = Vector(0,6,-25)
local camera_transform = TransformComponent()
local CAMERA_HEIGHT = 4 -- camera height from ground
local DEFAULT_CAMERADISTANCE = -9.5 -- the default camera distance when characters are close to each other
local MODIFIED_CAMERADISTANCE = -11.5 -- if the two players are far enough from each other, the camera will zoom out to this distance
local CAMERA_DISTANCE_MODIFIER = 10 -- the required distance between the characters when the camera should zoom out
local PLAYAREA = 30 -- play area horizontal bounds

-- ***Interaction between two characters:
local ResolveCharacters = function(player1, player2)
		
	player1:Input(0)
	if(player2_control == "CPU") then
		player2:AI()
	else
		player2:Input(1)
	end

	player1:Update()
	player2:Update()

	-- Facing direction requests:
	if(player1.position.GetX() < player2.position.GetX()) then
		player1.request_face = 1
		player2.request_face = -1
	else
		player1.request_face = -1
		player2.request_face = 1
	end

	-- Update the system global camera with current values:
	camera_transform.ClearTransform()
	camera_transform.Translate(camera_position)
	camera_transform.UpdateTransform()
	local camera = GetCamera()
	camera.TransformCamera(camera_transform)
	camera.UpdateCamera()
	
	-- Camera bounds:
	local projection_matrix =  camera.GetViewProjection()
	local z = vector.TransformCoord(Vector(), projection_matrix).GetZ()
	local unprojection_matrix =  camera.GetInvViewProjection()
	local camera_side_left = vector.TransformCoord(Vector(-1,0,z,1), unprojection_matrix).GetX()
	local camera_side_right = vector.TransformCoord(Vector(1,0,z,1), unprojection_matrix).GetX()
	local camera_halfwidth = (camera_side_right - camera_side_left) * 0.5

	-- Push:

	-- opponent on the edge of screen can initiate push transfer:
	--	it means that the opponent cannot be pushed further, so the pushing player will be pushed back instead to compensate:
	if(player2.clipbox.GetMin().GetX() <= camera_side_left and player1.push.GetX() < 0) then
		player2.push.SetX(-player1.push.GetX())
	end
	if(player2.clipbox.GetMax().GetX() >= camera_side_right and player1.push.GetX() > 0) then
		player2.push.SetX(-player1.push.GetX())
	end
	if(player1.clipbox.GetMin().GetX() <= camera_side_left and player2.push.GetX() < 0) then
		player1.push.SetX(-player2.push.GetX())
	end
	if(player1.clipbox.GetMax().GetX() >= camera_side_right and player2.push.GetX() > 0) then
		player1.push.SetX(-player2.push.GetX())
	end

	-- apply push forces:
	if(player1.push.Length() > 0) then
		player2.velocity = player1.push
	end
	if(player2.push.Length() > 0) then
		player1.velocity = player2.push
	end

	-- reset push forces:
	player1.push = Vector()
	player2.push = Vector()
	
	-- Because hitbox checks are in ccd phase, we will add hitconfirms after ccd phase is over, only once per frame:
	local player1_hitconfirm = false
	local player1_hitconfirm_guard = false
	local player2_hitconfirm = false
	local player2_hitconfirm_guard = false

	-- Continuous collision detection will be iterated multiple times to avoid "bullet through paper problem":
	local iterations = 10
	local ccd_step = 1.0 / iterations
	for i=1,iterations, 1 do

		player1:UpdateCollisionState(ccd_step)
		player2:UpdateCollisionState(ccd_step)

		-- Hit/Hurt/Guard:
		player1.hurt = false
		player2.hurt = false
		-- player1 hits player2:
		for i,hitbox in ipairs(player1.hitboxes) do
			for j,hurtbox in ipairs(player2.hurtboxes) do
				if(hitbox.Intersects2D(hurtbox)) then
					player1_hitconfirm = true
					player2.hurt = true
					if(player2.guarding) then
						player1_hitconfirm_guard = true
					else
						player2.hp = math.max(0, player2.hp - 10)
					end
					signal("vibrate_player1")
					break
				end
			end
		end
		-- player2 hits player1:
		for i,hitbox in ipairs(player2.hitboxes) do
			for j,hurtbox in ipairs(player1.hurtboxes) do
				if(hitbox.Intersects2D(hurtbox)) then
					player2_hitconfirm = true
					player1.hurt = true
					if(player1.guarding) then
						player2_hitconfirm_guard = true
					else
						player1.hp = math.max(0, player1.hp - 10)
					end
					signal("vibrate_player2")
					break
				end
			end
		end

		player1.can_guard = false
		player2.can_guard = false
		-- player1 guardbox player2:
		for i,guardbox in ipairs(player1.guardboxes) do
			for j,hurtbox in ipairs(player2.hurtboxes) do
				if(guardbox.Intersects2D(hurtbox)) then
					player2.can_guard = true
					break
				end
			end
		end
		-- player2 guardbox player1:
		for i,guardbox in ipairs(player2.guardboxes) do
			for j,hurtbox in ipairs(player1.hurtboxes) do
				if(guardbox.Intersects2D(hurtbox)) then
					player1.can_guard = true
					break
				end
			end
		end

		-- Clipping:
		if(player1.clipbox.Intersects2D(player2.clipbox)) then
			local center1 = player1.clipbox.GetCenter().GetX()
			local center2 = player2.clipbox.GetCenter().GetX()
			local extent1 = player1.clipbox.GetHalfExtents().GetX()
			local extent2 = player2.clipbox.GetHalfExtents().GetX()
			local diff = math.abs(center2 - center1)
			local target_diff = math.abs(extent2 + extent1)
			local offset = (target_diff - diff) * 0.5
			offset = math.lerp( offset, math.min(offset, 0.3 * ccd_step), math.saturate(math.abs(player1.position.GetY() - player2.position.GetY())) ) -- smooth out clipping in mid-air
			player1.position.SetX(player1.position.GetX() - offset * player1.request_face)
			player2.position.SetX(player2.position.GetX() - offset * player2.request_face)
		end


		-- Clamp the players inside the camera:
		player1.position.SetX(player1.position.GetX() + math.saturate(camera_side_left - player1.clipbox.GetMin().GetX()))
		player1.position.SetX(player1.position.GetX() - math.saturate(player1.clipbox.GetMax().GetX() - camera_side_right))
		player2.position.SetX(player2.position.GetX() + math.saturate(camera_side_left - player2.clipbox.GetMin().GetX()))
		player2.position.SetX(player2.position.GetX() - math.saturate(player2.clipbox.GetMax().GetX() - camera_side_right))
	
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
		local centerX = math.clamp((player1.position.GetX() + player2.position.GetX()) * 0.5, -PLAYAREA + camera_halfwidth, PLAYAREA - camera_halfwidth)
		camera_position_new.SetX(centerX)

		-- smooth camera:
		camera_position = vector.Lerp(camera_position, camera_position_new, 0.1 * ccd_step)

	end


	-- Add hitconfirms to the character's own table
	if(player1_hitconfirm) then
		table.insert(player1.hitconfirms, player1.frame)
	end
	if(player1_hitconfirm_guard) then
		table.insert(player1.hitconfirms_guard, player1.frame)
	end
	if(player2_hitconfirm) then
		table.insert(player2.hitconfirms, player2.frame)
	end
	if(player2_hitconfirm_guard) then
		table.insert(player2.hitconfirms_guard, player2.frame)
	end
	

	-- Update collision state once more (but with ccd_step = 0) so that bounding boxes and system transform is up to date:
	player1:UpdateCollisionState(0)
	player2:UpdateCollisionState(0)

	player1:DebugDraw()
	player2:DebugDraw()

end

-- ****Main loop:
runProcess(function()

	ClearWorld() -- clears global scene and renderer
	SetProfilerEnabled(false) -- have a bit more screen space
	
	-- Fighting game needs stable frame rate and deterministic controls at all times. We will also refer to frames in this script instead of time units.
	--	We lock the framerate to 60 FPS, so if frame rate goes below, game will play slower
	--	
	--	There is also the possibility to implement game logic in fixed_update() instead, but that is not common for fighting games
	main.SetTargetFrameRate(60)
	main.SetFrameRateLock(true)

	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = main.GetActivePath()
	local path = RenderPath3D()
	main.SetActivePath(path)

	local help_text = ""
	help_text = help_text .. "Wicked Engine Fighting game sample script\n"
	help_text = help_text .. "\nESCAPE key: quit\nR: reload script"
	help_text = help_text .. "\nWASD / Gamepad direction buttons: move"
	help_text = help_text .. "\nRight / Gamepad button 1: action A"
	help_text = help_text .. "\nUp	/ Gamepad button 2: action B"
	help_text = help_text .. "\nLeft / Gamepad button 3: action C"
	help_text = help_text .. "\nDown / Gamepad button 4: action D"
	help_text = help_text .. "\nT / Gamepad button 5: Taunt"
	help_text = help_text .. "\nJ: player2 will always jump"
	help_text = help_text .. "\nC: player2 will always crouch"
	help_text = help_text .. "\nG: player2 will always guard"
	help_text = help_text .. "\nK: player2 will always attack"
	help_text = help_text .. "\nI: player2 will be idle"
	help_text = help_text .. "\nH: toggle Debug Draw"
	help_text = help_text .. "\n\nMovelist (using numpad notation):"
	help_text = help_text .. "\n\t A : Light Punch"
	help_text = help_text .. "\n\t B : Heavy Punch"
	help_text = help_text .. "\n\t C : Light Kick"
	help_text = help_text .. "\n\t D : Heavy Kick"
	help_text = help_text .. "\n\t 6A : Forward Light Punch (forward + A)"
	help_text = help_text .. "\n\t 2A : Low Punch (down + A)"
	help_text = help_text .. "\n\t 2B : Uppercut (down + B)"
	help_text = help_text .. "\n\t 2C : Low Kick (down + C)"
	help_text = help_text .. "\n\t 4(charge) 6C : Charge Kick (charge back + 6, also while crouching)"
	help_text = help_text .. "\n\t C : Air Kick (while jumping)"
	help_text = help_text .. "\n\t D : Air Heavy Kick (while jumping)"
	help_text = help_text .. "\n\t 623B: Shoryuken (forward, then quater circle forward + B)"
	help_text = help_text .. "\n\t 236B: Jaunt (quarter circle forward + B)"
	help_text = help_text .. "\n\t 236A: Fireball (quarter circle forward + A, also in mid-air)"
	help_text = help_text .. "\n\nCombos:"
	help_text = help_text .. "\n\t Revolver action: A, B, C, D (Hit action buttons in quick succession)"
	help_text = help_text .. "\n\t Airborne heat: 2B, 8, 8C (Uppercut, then jump cancel into Air Kick)"
	local font = SpriteFont(help_text);
	font.SetSize(14)
	font.SetPos(Vector(10, GetScreenHeight() - 10))
	font.SetAlign(WIFALIGN_LEFT, WIFALIGN_BOTTOM)
	font.SetColor(0xFF4D21FF)
	font.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(font)

	local info = SpriteFont("");
	info.SetSize(14)
	info.SetPos(Vector(GetScreenWidth() / 2.5, GetScreenHeight() - 10))
	info.SetAlign(WIFALIGN_LEFT, WIFALIGN_BOTTOM)
	info.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(info)

	LoadModel("../models/dojo.wiscene")
	
	-- Create the two player characters. Parameters are facing direction and material colors to differentiate between them:
	--						Facing:		skin color:				shirt color				hair color				shoe color
	local player1 = Character(	 1,		Vector(0.7,0.5,0.3,1),	Vector(1,1,1,1),		Vector(1,1,1,1),		Vector(1,1,1,1))
	local player2 = Character(	-1,		Vector(1,1,1,1),		Vector(1,0,0,1),		Vector(0.1,0.1,0.4,1),	Vector(0.3,0.1,0.1,1))
	
	while true do

		ResolveCharacters(player1, player2)

		if(input.Press(string.byte('I'))) then
			player2.ai_state = "Idle"
		elseif(input.Press(string.byte('J'))) then
			player2.ai_state = "Jump"
		elseif(input.Press(string.byte('C'))) then
			player2.ai_state = "Crouch"
		elseif(input.Press(string.byte('G'))) then
			player2.ai_state = "Guard"
		elseif(input.Press(string.byte('K'))) then
			player2.ai_state = "Attack"
		elseif(input.Press(string.byte('H'))) then
			debug_draw = not debug_draw
		elseif(input.Press(GAMEPAD_BUTTON_10, 1)) then
			if(player2_control == "CPU") then
				player2_control = "Controller2"
			else
				player2_control = "CPU"
			end
		end


		-- Print player 1 and player2 specific debug information:
		local infoText = ""
		infoText = infoText .. "Player 1: "
		infoText = infoText .. "\n\tControl: Keyboard / Controller1"
		infoText = infoText .. "\n\tInput: "
		for i,element in ipairs(player1.input_buffer) do
			if(element.command ~= "5") then
				infoText = infoText .. element.command
			end
		end
		infoText = infoText .. "\n\tstate = " .. player1.state .. "\n\tframe = " .. player1.frame .. "\n\n"
		
		infoText = infoText .. "Player 2: "
		infoText = infoText .. "\n\tControl: " .. player2_control .. " (Press START / BUTTON10 on Gamepad2 to join)"
		infoText = infoText .. "\n\tInput: "
		for i,element in ipairs(player2.input_buffer) do
			if(element.command ~= "5") then
				infoText = infoText .. element.command
			end
		end
		infoText = infoText .. "\n\tstate = " .. player2.state .. "\n\tframe = " .. player2.frame

		info.SetText(infoText)

		
		-- Wait for Engine update tick
		update()
		
	
		if(input.Press(KEYBOARD_BUTTON_ESCAPE)) then
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

