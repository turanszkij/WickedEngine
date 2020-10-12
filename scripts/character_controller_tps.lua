-- Lua Third person camera and character controller script
--	This script will load a simple scene with a character that can be controlled
--
-- 	CONTROLS:
--		WASD/left thumbstick: walk
--		SHIFT/right shoulder button: speed
--		SPACE/gamepad X/gamepad button 2: Jump
--		Right Mouse Button/Right thumbstick: rotate camera
--		Scoll middle mouse/Left-Right triggers: adjust camera distance
--		ESCAPE key: quit
--		ENTER: reload script

-- Debug Draw Helper
local DrawAxis = function(point,f)
	DrawLine(point,point:Add(Vector(f,0,0)),Vector(1,0,0,1))
	DrawLine(point,point:Add(Vector(0,f,0)),Vector(0,1,0,1))
	DrawLine(point,point:Add(Vector(0,0,f)),Vector(0,0,1,1))
end

local scene = GetScene()

local testcapsule = Capsule(Vector(-4,3),Vector(-4,6,-20),0.8) -- just place a test capsule into the scene

Character = {
	model = INVALID_ENTITY,
	target = INVALID_ENTITY, -- Camera will look at this location, rays will be started from this location, etc.
	idle_anim = INVALID_ENTITY,
	walk_anim = INVALID_ENTITY,
	head = INVALID_ENTITY,
	left_hand = INVALID_ENTITY,
	right_hand = INVALID_ENTITY,
	left_foot = INVALID_ENTITY,
	right_foot = INVALID_ENTITY,
	face = Vector(0,0,1), -- forward direction
	force = Vector(),
	velocity = Vector(),
	gravity = Vector(),
	savedPointerPos = Vector(),
	moveSpeed = 90,
	layerMask = 0x2, -- The character will be tagged to use this layer, so scene Picking can filter out the character
	scale = Vector(0.8,0.8,0.8),
	rotation = Vector(0,3.1415,0),
	capsule = Capsule(Vector(), Vector(), 0),
	
	states = {
		STAND = 0,
		WALK = 1,
		JUMP = 2,
	},
	state = STAND,
	state_prev = STAND,
	
	Create = function(self,entity)
		self.model = entity
		local layer = scene.Component_GetLayer(self.model)
		layer.SetLayerMask(self.layerMask)
		
		self.idle_anim = scene.Entity_FindByName("idle")
		self.walk_anim = scene.Entity_FindByName("walk")
		self.head = scene.Entity_FindByName("testa")
		self.left_hand = scene.Entity_FindByName("mano_L")
		self.right_hand = scene.Entity_FindByName("mano_R")
		self.left_foot = scene.Entity_FindByName("piede_L")
		self.right_foot = scene.Entity_FindByName("piede_R")
		
		local model_transform = scene.Component_GetTransform(self.model)
		model_transform.ClearTransform()
		model_transform.Scale(self.scale)
		model_transform.Rotate(self.rotation)
		model_transform.UpdateTransform()
		
		self.target = CreateEntity()
		local target_transform = scene.Component_CreateTransform(self.target)
		target_transform.ClearTransform()
		target_transform.Translate(Vector(0,3))
		
		scene.Component_Attach(self.target, self.model)
	end,
	
	Jump = function(self,f)
		self.force = vector.Add(self.force, Vector(0,f,0))
		self.state = self.states.JUMP
	end,
	MoveDirection = function(self,dir,f)
		local model_transform = scene.Component_GetTransform(self.model)
		local target_transform = scene.Component_GetTransform(self.target)
		local savedPos = model_transform.GetPosition()
		model_transform.ClearTransform()
		dir = vector.Transform(dir:Normalize(), target_transform.GetMatrix())
		dir.SetY(0)
		local dot = vector.Dot(self.face, dir)
		if(dot < 0) then
			self.face = vector.TransformNormal(self.face, matrix.RotationY(3.1415 * 0.01))
		end
		self.face = vector.Lerp(self.face, dir, 0.2);
		self.face.Normalize()
		model_transform.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
		model_transform.Scale(self.scale)
		model_transform.Rotate(self.rotation)
		model_transform.Translate(savedPos)
		model_transform.UpdateTransform()
		scene.Component_Detach(self.target)
		scene.Component_Attach(self.target, self.model)
		if(dot > 0) then 
			self.force = vector.Add(self.force, self.face:Multiply(Vector(f,f,f)))
		end
		self.state = self.states.WALK
	end,
	
	Input = function(self)
		
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
			
		if(lookDir:Length()>0) then
			if(input.Down(KEYBOARD_BUTTON_LSHIFT) or input.Down(GAMEPAD_BUTTON_6)) then
				self:MoveDirection(lookDir,self.moveSpeed*2)
			else
				self:MoveDirection(lookDir,self.moveSpeed)
			end
		end
		
		if( input.Press(string.byte('J'))  or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) ) then
			self:Jump(2000)
		end

		-- Camera target control:

		-- read from gamepad analog stick:
		local diff = input.GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R)
		diff = vector.Multiply(diff, getDeltaTime() * 4)
		
		-- read from mouse:
		if(input.Down(MOUSE_BUTTON_RIGHT)) then
			local mouseDif = input.GetPointerDelta()
			mouseDif = mouseDif:Multiply(getDeltaTime() * 0.3)
			diff = vector.Add(diff, mouseDif)
			input.SetPointer(self.savedPointerPos)
			input.HidePointer(true)
		else
			self.savedPointerPos = input.GetPointer()
			input.HidePointer(false)
		end
		
		local target_transform = scene.Component_GetTransform(self.target)
		target_transform.Rotate(Vector(diff.GetY(),diff.GetX()))
		self.face.SetY(0)
		self.face=self.face:Normalize()
		
	end,
	
	Update = function(self)
		local model_transform = scene.Component_GetTransform(self.model)
		local target_transform = scene.Component_GetTransform(self.target)
		
		-- state and animation update
		if(self.state == self.states.STAND) then
			scene.Component_GetAnimation(self.walk_anim).Stop()
			local anim = scene.Component_GetAnimation(self.idle_anim)
			anim.Play()
			anim.SetAmount(math.lerp(anim.GetAmount(), 1, 0.1))
			if(self.state ~= self.state_prev) then
				anim.SetAmount(0)
			end
		elseif(self.state == self.states.WALK) then
			scene.Component_GetAnimation(self.idle_anim).Stop()
			local anim = scene.Component_GetAnimation(self.walk_anim)
			anim.Play()
			anim.SetAmount(math.lerp(anim.GetAmount(), 1, 0.1))
			if(self.state ~= self.state_prev) then
				anim.SetAmount(0)
			end
		elseif(self.state == self.states.JUMP) then
			scene.Component_GetAnimation(self.walk_anim).Stop()
			local anim = scene.Component_GetAnimation(self.idle_anim)
			anim.Play()
			anim.SetAmount(math.lerp(anim.GetAmount(), 1, 0.1))
			if(self.state ~= self.state_prev) then
				anim.SetAmount(0)
			end
		end
		self.state_prev = self.state
		
		-- apply force:
		self.velocity = vector.Add(self.velocity, vector.Multiply(self.force, 0.016))
		self.force = Vector(0,0,0,0)
		
		-- Capsule collision for character:
		local original_capsulepos = model_transform.GetPosition()
		local capsulepos = original_capsulepos
		local capsuleheight = 4
		local radius = 0.7
		local ground_intersection = false
		local ccd_max = 5
		local ccd = 0
		while(ccd < ccd_max) do
			ccd = ccd + 1
			local step = vector.Multiply(self.velocity, 1.0 / ccd_max * 0.016)

			capsulepos = vector.Add(capsulepos, step)
			self.capsule = Capsule(capsulepos, vector.Add(capsulepos, Vector(0, capsuleheight)), radius)
			local o2, p2, n2, depth = self.capsule.Intersects(testcapsule) -- capsule/capsule collision
			if(not o2) then
				o2, p2, n2, depth = SceneIntersectCapsule(self.capsule, PICK_OPAQUE, ~self.layerMask) -- scene/capsule collision
			end
			if(o2 ~= INVALID_ENTITY) then
				DrawPoint(p2,0.1,Vector(1,1,0,1))
				DrawLine(p2, vector.Add(p2, n2), Vector(1,1,0,1))

				-- Slide on contact surface:
				local velocityLen = self.velocity.Length()
				local velocityNormalized = self.velocity.Normalize()
				local undesiredMotion = n2:Multiply(vector.Dot(velocityNormalized, n2))
				local desiredMotion = vector.Subtract(velocityNormalized, undesiredMotion)
				self.velocity = vector.Multiply(desiredMotion, velocityLen)

				-- Remove penetration (penetration epsilon added to handle infinitely small penetration):
				capsulepos = vector.Add(capsulepos, vector.Multiply(n2, depth + 0.0001))

			end
		end
		
		-- Gravity collision is separate:
		--	This is to avoid sliding down slopes and easier traversing of slopes/stairs
		--	Unlike normal character motion collision, surface sliding is not computed
		self.gravity = vector.Add(self.gravity, Vector(0,-9.8*0.1,0))
		ccd = 0
		ccd_max = 5
		while(ccd < ccd_max) do
			ccd = ccd + 1
			local step = vector.Multiply(self.gravity, 1.0 / ccd_max * 0.016)

			capsulepos = vector.Add(capsulepos, step)
			self.capsule = Capsule(capsulepos, vector.Add(capsulepos, Vector(0, capsuleheight)), radius)
			local o2, p2, n2, depth = self.capsule.Intersects(testcapsule) -- capsule/capsule collision
			if(not o2) then
				o2, p2, n2, depth = SceneIntersectCapsule(self.capsule, PICK_OPAQUE, ~self.layerMask) -- scene/capsule collision
			end
			if(o2 ~= INVALID_ENTITY) then
				DrawPoint(p2,0.1,Vector(1,1,0,1))
				DrawLine(p2, vector.Add(p2, n2), Vector(1,1,0,1))

				-- Remove penetration (penetration epsilon added to handle infinitely small penetration):
				capsulepos = vector.Add(capsulepos, vector.Multiply(n2, depth + 0.0001))

				-- Check whether it is intersecting the ground (ground normal is upwards)
				if(vector.Dot(n2, Vector(0,1,0)) > 0.3) then
					ground_intersection = true
					self.gravity = vector.Multiply(self.gravity, 0)
					break
				end

			end
		end

		if ground_intersection then
			self.velocity = vector.Multiply(self.velocity, 0.85) -- ground friction
		else
			self.velocity = vector.Multiply(self.velocity, 0.94) -- air friction
		end
		if(vector.Length(self.velocity) < 0.001) then
			self.state = self.states.STAND
		end
		model_transform.Translate(vector.Subtract(capsulepos, original_capsulepos)) -- transform by the capsule offset
		model_transform.UpdateTransform()
		
		-- try to put water ripple if character head is directly above water
		local head_transform = scene.Component_GetTransform(self.head)
		local waterRay = Ray(head_transform.GetPosition(),Vector(0,-1,0))
		local w,wp,wn = Pick(waterRay,PICK_WATER)
		if(w ~= INVALID_ENTITY and self.velocity.Length() > 2) then
			PutWaterRipple("../models/ripple.png",wp)
		end
		
	end,

}

-- Third person camera controller class:
ThirdPersonCamera = {
	camera = INVALID_ENTITY,
	character = nil,
	side_offset = 1,
	height = 0,
	rest_distance = 6,
	rest_distance_new = 6,
	
	Create = function(self, character)
		self.character = character
		
		self.camera = CreateEntity()
		local camera_transform = scene.Component_CreateTransform(self.camera)
	end,
	
	Update = function(self)
		if(self.character ~= nil) then

			-- Mouse scroll or gamepad triggers will move the camera distance:
			local scroll = input.GetPointer().GetZ() -- pointer.z is the mouse wheel delta this frame
			scroll = scroll + input.GetAnalog(GAMEPAD_ANALOG_TRIGGER_R).GetX()
			scroll = scroll - input.GetAnalog(GAMEPAD_ANALOG_TRIGGER_L).GetX()
			self.rest_distance_new = math.max(self.rest_distance_new - scroll, 2) -- do not allow too close using max
			self.rest_distance = math.lerp(self.rest_distance, self.rest_distance_new, 0.1) -- lerp will smooth out the zooming

			-- We update the scene so that character's target_transform will be using up to date values
			scene.Update(0)

			local camera_transform = scene.Component_GetTransform(self.camera)
			local target_transform = scene.Component_GetTransform(self.character.target)

			
			-- First calculate the rest orientation (transform) of the camera:
			local mat = matrix.Translation(Vector(self.side_offset, self.height, -self.rest_distance))
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

			-- Update global camera matrices for rest position:
			GetCamera().TransformCamera(camera_transform)
			GetCamera().UpdateCamera()

			-- Cast rays from target to clip space points on the camera near plane to avoid clipping through objects:
			local unproj = GetCamera().GetInvViewProjection()	-- camera matrix used to unproject from clip space to world space
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

				local ray = Ray(targetPos, target_to_corner.Normalize())

				local collObj,collPos,collNor = Pick(ray, PICK_OPAQUE, ~self.character.layerMask)
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
			GetCamera().TransformCamera(camera_transform)
			GetCamera().UpdateCamera()
			
		end
	end,
}


-- Player Controller
local player = Character
-- Third Person camera
local camera = ThirdPersonCamera

-- Main loop:
runProcess(function()

	ClearWorld()

	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = main.GetActivePath()
	local path = RenderPath3D()
	path.SetLightShaftsEnabled(true)
	main.SetActivePath(path)

	local font = SpriteFont("This script is showcasing how to perform scene collision with raycasts for character and camera.\nControls:\n#####################\n\nWASD/arrows/left analog stick: walk\nSHIFT/right shoulder button: movement speed\nSPACE/gamepad X/gamepad button 2: Jump\nRight Mouse Button/Right thumbstick: rotate camera\nScoll middle mouse/Left-Right triggers: adjust camera distance\nESCAPE key: quit\nR: reload script");
	font.SetSize(14)
	font.SetPos(Vector(10, GetScreenHeight() - 10))
	font.SetAlign(WIFALIGN_LEFT, WIFALIGN_BOTTOM)
	font.SetColor(0xFFADA3FF)
	font.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(font)

	LoadModel("../models/playground.wiscene")
	
	player:Create(LoadModel("../models/girl.wiscene"))
	camera:Create(player)
	
	while true do
		player:Update()
		camera:Update()
		fixedupdate()
		
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
			dofile("character_controller_tps.lua")
			return
		end
		
	end
end)

runProcess(function()
	
	while true do
		player:Input()

		update()
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
		
		-- Head bone
		DrawPoint(scene.Component_GetTransform(player.head).GetPosition(),0.2, Vector(0,1,1,1))
		-- Left hand bone
		DrawPoint(scene.Component_GetTransform(player.left_hand).GetPosition(),0.2, Vector(0,1,1,1))
		-- Right hand bone
		DrawPoint(scene.Component_GetTransform(player.right_hand).GetPosition(),0.2, Vector(0,1,1,1))
		-- Left foot bone
		DrawPoint(scene.Component_GetTransform(player.left_foot).GetPosition(),0.2, Vector(0,1,1,1))
		-- Right foot bone
		DrawPoint(scene.Component_GetTransform(player.right_foot).GetPosition(),0.2, Vector(0,1,1,1))
		
		DrawCapsule(player.capsule)
		DrawCapsule(testcapsule, Vector(1,0,0,100))
		
		-- Wait for the engine to render the scene
		render()
	end
end)

