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

local scene = GetScene()

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
	velocity = Vector(),
	velocityPrev = Vector(),
	ray = Ray(Vector(),Vector()),
	o = INVALID_ENTITY, -- collision prop with scene (entity)
	p,n = Vector(), -- collision props with scene (position,normal)
	savedPointerPos = Vector(),
	moveSpeed = 0.2,
	layerMask = 0x2, -- The character will be tagged to use this layer, so scene Picking can filter out the character
	scale = Vector(0.8,0.8,0.8),
	rotation = Vector(0,3.1415,0),
	
	states = {
		STAND = 0,
		WALK = 1,
		JUMP = 2,
	},
	state = STAND,
	
	Create = function(self,entity)
		self.model = entity
		local layer = scene.Component_GetLayer(self.model)
		layer.SetLayerMask(self.layerMask)
		
		self.idle_anim = scene.Entity_FindByName("idle")
		self.walk_anim = scene.Entity_FindByName("walk")
		self.head = scene.Entity_FindByName("testa")
		self.left_hand = scene.Entity_FindByName("mano_L")
		self.right_hand = scene.Entity_FindByName("mano_R")
		self.left_foot = scene.Entity_FindByName("avampiede_L")
		self.right_foot = scene.Entity_FindByName("avampiede_R")
		
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
		self.velocity = self.velocity:Add(Vector(0,f,0))
		self.state = self.states.JUMP
	end,
	MoveDirection = function(self,dir,f)
		local model_transform = scene.Component_GetTransform(self.model)
		local target_transform = scene.Component_GetTransform(self.target)
		local savedPos = model_transform.GetPosition()
		model_transform.ClearTransform()
		self.face = vector.Transform(dir:Normalize(), target_transform.GetMatrix())
		self.face.SetY(0)
		self.face = self.face.Normalize()
		model_transform.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
		model_transform.Scale(self.scale)
		model_transform.Rotate(self.rotation)
		model_transform.Translate(savedPos)
		model_transform.UpdateTransform()
		scene.Component_Detach(self.target)
		scene.Component_Attach(self.target, self.model)
		self.velocityPrev = self.velocity;
		self.velocity = self.face:Multiply(Vector(f,f,f))
		self.velocity.SetY(self.velocityPrev.GetY())
		self.state = self.states.WALK
	end,
	
	Input = function(self)
		
		if(self.state==self.states.STAND) then
			local lookDir = Vector()
			if(input.Down(VK_LEFT) or input.Down(string.byte('A'))) then
				lookDir = lookDir:Add( Vector(-1) )
			end
			if(input.Down(VK_RIGHT) or input.Down(string.byte('D'))) then
				lookDir = lookDir:Add( Vector(1) )
			end
		
			if(input.Down(VK_UP) or input.Down(string.byte('W'))) then
				lookDir = lookDir:Add( Vector(0,0,1) )
			end
			if(input.Down(VK_DOWN) or input.Down(string.byte('S'))) then
				lookDir = lookDir:Add( Vector(0,0,-1) )
			end

			local analog = input.GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_L)
			lookDir = vector.Add(lookDir, Vector(analog.GetX(), 0, analog.GetY()))
			
			if(lookDir:Length()>0) then
				if(input.Down(VK_LSHIFT) or input.Down(GAMEPAD_BUTTON_6, INPUT_TYPE_GAMEPAD)) then
					self:MoveDirection(lookDir,self.moveSpeed*2)
				else
					self:MoveDirection(lookDir,self.moveSpeed)
				end
			end
		
		end
		
		if( input.Press(string.byte('J'))  or input.Press(VK_SPACE) or input.Press(GAMEPAD_BUTTON_2, INPUT_TYPE_GAMEPAD) ) then
			self:Jump(0.6)
		end

		-- Camera target control:

		-- read from gamepad analog stick:
		local diff = input.GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R)
		diff = vector.Multiply(diff, getDeltaTime() * 4)
		
		-- read from mouse:
		if(input.Down(VK_RBUTTON)) then
			local mousePosNew = input.GetPointer()
			local mouseDif = vector.Subtract(mousePosNew,self.savedPointerPos)
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
			scene.Component_GetAnimation(self.idle_anim).Play()
			scene.Component_GetAnimation(self.walk_anim).Stop()
			self.state = self.states.STAND
		elseif(self.state == self.states.WALK) then
			scene.Component_GetAnimation(self.idle_anim).Stop()
			scene.Component_GetAnimation(self.walk_anim).Play()
			self.state = self.states.STAND
		elseif(self.state == self.states.JUMP) then
			scene.Component_GetAnimation(self.idle_anim).Play()
			scene.Component_GetAnimation(self.walk_anim).Stop()
			self.state = self.states.STAND
		end
		
		-- front block shoots multiple rays in front to try to find obstruction
		local rotations = {0, 3.1415*0.3, -3.1415*0.3}
		for i,rot in ipairs(rotations) do
			local origin = vector.Add(model_transform.GetPosition(), Vector(0,1,0)) -- this ray starts a little above character ground position
			local dir = vector.Transform(self.face, matrix.RotationY(rot))
			local ray2 = Ray(origin,dir)
			local o2,p2,n2 = Pick(ray2, PICK_OPAQUE, ~self.layerMask)
			local dist = vector.Subtract(origin,p2):Length()
			if(o2 ~= INVALID_ENTITY and dist < 1) then
				-- run along wall instead of going through it
				local velocityLen = self.velocity.Length()
				local velocityNormalized = self.velocity.Normalize()
				local undesiredMotion = n2:Multiply(vector.Dot(velocityNormalized, n2))
				local desiredMotion = vector.Subtract(velocityNormalized, undesiredMotion)
				self.velocity = vector.Multiply(desiredMotion, velocityLen)
			end
		end
		
		-- check what is below the character
		self.ray = Ray(target_transform.GetPosition(),Vector(0,-1,0))
		local pPrev = self.p
		self.o,self.p,self.n = Pick(self.ray, PICK_OPAQUE, ~self.layerMask)
		if(self.o == INVALID_ENTITY) then
			self.p=pPrev -- if nothing, go back to previous position
		end
		
		-- try to put water ripple if character head is directly above water
		local head_transform = scene.Component_GetTransform(self.head)
		local waterRay = Ray(head_transform.GetPosition(),Vector(0,-1,0))
		local w,wp,wn = Pick(waterRay,PICK_WATER)
		if(w ~= INVALID_ENTITY and self.velocity.Length()>0.1) then
			PutWaterRipple("../Editor/images/ripple.png",wp)
		end
		
		-- add gravity:
		self.velocity = vector.Add(self.velocity, Vector(0,-0.04,0))
		
		-- apply velocity:
		model_transform.Translate(vector.Multiply(getDeltaTime() * 60, self.velocity))
		model_transform.UpdateTransform()
		
		-- check if we are below or on the ground:
		local posY = model_transform.GetPosition().GetY()
		if(posY <= self.p.GetY() and self.velocity.GetY()<=0) then
			self.state = self.states.STAND
			if(self.o == INVALID_ENTITY) then
				model_transform.Translate(vector.Subtract(self.p,model_transform.GetPosition())) -- snap back to last succesfully traced position
			else
				model_transform.Translate(Vector(0,self.p.GetY()-posY,0)) -- snap to ground
			end
			self.velocity.SetY(0) -- don't fall below ground
			self.velocity = vector.Multiply(self.velocity, 0.8) -- slow down gradually on ground
		end
		
	end
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
	local path = RenderPath3D_TiledForward()
	path.SetLightShaftsEnabled(true)
	main.SetActivePath(path)

	local font = Font("This script is showcasing how to perform scene collision with raycasts for character and camera.\nControls:\n#####################\n\nWASD/arrows/left analog stick: walk\nSHIFT/right shoulder button: movement speed\nSPACE/gamepad X/gamepad button 2: Jump\nRight Mouse Button/Right thumbstick: rotate camera\nScoll middle mouse/Left-Right triggers: adjust camera distance\nESCAPE key: quit\nR: reload script");
	font.SetSize(24)
	font.SetPos(Vector(10, GetScreenHeight() - 10))
	font.SetAlign(WIFALIGN_LEFT, WIFALIGN_BOTTOM)
	font.SetColor(0xFFADA3FF)
	font.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(font)

	LoadModel("../models/playground.wiscene")
	
	player:Create(LoadModel("../models/girl.wiscene"))
	camera:Create(player)
	
	while true do

		player:Input()
		
		player:Update()
		
		camera:Update()
		
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
			dofile("character_controller_tps.lua")
			return
		end
		
	end
end)




-- Debug Draw Helper
local DrawAxis = function(point,f)
	DrawLine(point,point:Add(Vector(f,0,0)),Vector(1,0,0,1))
	DrawLine(point,point:Add(Vector(0,f,0)),Vector(0,1,0,1))
	DrawLine(point,point:Add(Vector(0,0,f)),Vector(0,0,1,1))
end

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
		DrawAxis(player.p,0.5)
		
		-- camera target box and axis
		DrawBox(target_transform.GetMatrix())
		
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
		
		
		-- Wait for the engine to render the scene
		render()
	end
end)

