-- Lua Third person camera and character controller script
--	To use this, first load a scene so our character can stand on something. Otherwise, it will be undefined behaviour
--	For example, you can load the dungeon generator script first, then this and you will have a scene to try navigate.
--
-- 	CONTROLS:
--		WASD: walk
--		SHIFT: speed
--		SPACE: Jump
--		Right Mouse Button: Camera control
--		Mouse: Rotate camera after pressing Right Mouse Button

-- Very simple additive gravity for now
local gravity = Vector(0,-0.076,0)

-- Character controller class:
characterController = {
	skeleton = Armature(),
	target = Transform(), -- Camera will look at this location
	face = Vector(0,0,1),
	velocity = Vector(),
	ray = Ray(),
	o,p,n = Vector(0,0,0), -- collision props with scene (object,position,normal)
	savedPointerPos = Vector(),
	moveSpeed = 0.28,
	layerMask = 0x2, -- The character will be tagged to use this layer, so scene Picking can filter out the character
	
	states = {
		STAND = 0,
		TURN = 1,
		WALK = 2,
		JUMP = 3,
	},
	state = STAND,
	
	Load = function(self,fileName,objectname,armaturename)
		local model = LoadModel(fileName)
		model.SetLayerMask(self.layerMask)
		self.skeleton = GetArmature(armaturename)
	end,
	
	Reset = function(self)
		self.skeleton.ClearTransform()
		self.target.ClearTransform()
		self.skeleton.Rotate(Vector(0,3.1415))
		self.skeleton.Scale(Vector(1.9,1.9,1.9))
		self.target.Translate(Vector(0,7))
		self.target.AttachTo(self.skeleton,1,0,1)
	end,
	Reposition = function(self, pos)
		self.skeleton.Translate(pos)
	end,
	MoveForward = function(self,f)
		local velocityPrev = self.velocity;
		self.velocity = self.face:Multiply(Vector(f,f,f))
		self.velocity.SetY(velocityPrev.GetY())
		self.ray = Ray(self.skeleton.GetPosition():Add(self.velocity):Add(Vector(0,4)),Vector(0,-1,0))
		self.o,self.p,self.n = Pick(self.ray, PICK_OPAQUE, ~self.layerMask)
		if(self.o.IsValid()) then
			self.state = self.states.WALK
		else
			self.state = self.states.STAND
			self.velocity = velocityPrev
		end
		
		--front block
		local ray2 = Ray(self.skeleton.GetPosition():Add(self.velocity:Normalize():Multiply(1.2)):Add(Vector(0,4)),self.velocity)
		local o2,p2,n2 = Pick(ray2, PICK_OPAQUE, ~self.layerMask)
		local dist = vector.Subtract(self.skeleton.GetPosition():Add(Vector(0,4)),p2):Length()
		if(o2.IsValid() and dist < 2.8) then
			-- run along wall instead of going through it
			local velocityLen = self.velocity.Length()
			local velocityNormalized = self.velocity.Normalize()
			local undesiredMotion = n2:Multiply(vector.Dot(velocityNormalized, n2))
			local desiredMotion = vector.Subtract(velocityNormalized, undesiredMotion)
			self.velocity = vector.Multiply(desiredMotion, velocityLen)
		end
		
	end,
	Turn = function(self,f)
		self.skeleton.Rotate(Vector(0,f))
		self.face = self.face.Transform(matrix.RotationY(f))
		self.state = self.states.TURN
	end,
	Jump = function(self,f)
		self.velocity = self.velocity:Add(Vector(0,f,0))
		self.state = self.states.JUMP
	end,
	MoveDirection = function(self,dir,f)
		local savedPos = self.skeleton.GetPosition()
		self.target.Detach()
		self.skeleton.ClearTransform()
		self.face = dir:Normalize().Transform(self.target.GetMatrix())
		self.face.SetY(0)
		self.face = self.face.Normalize()
		self.skeleton.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
		self.skeleton.Scale(Vector(1.9,1.9,1.9))
		self.skeleton.Rotate(Vector(0,3.1415))
		self.skeleton.Translate(savedPos)
		self.skeleton.GetMatrix()
		self.target.AttachTo(self.skeleton)
		self:MoveForward(f)
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
			
			if(lookDir:Length()>0) then
				if(input.Down(VK_LSHIFT)) then
					self:MoveDirection(lookDir,self.moveSpeed*3)
				else
					self:MoveDirection(lookDir,self.moveSpeed)
				end
			end
		
		end
		
		if( input.Press(string.byte('J'))  or input.Press(VK_SPACE) ) then
			self:Jump(1.3)
		end
		
		
		if(input.Down(VK_RBUTTON)) then
			local mousePosNew = input.GetPointer()
			local mouseDif = vector.Subtract(mousePosNew,self.savedPointerPos)
			mouseDif = mouseDif:Multiply(0.01)
			self.target.Rotate(Vector(mouseDif.GetY(),mouseDif.GetX()))
			self.face.SetY(0)
			self.face=self.face:Normalize()
			input.SetPointer(self.savedPointerPos)
		else
			self.savedPointerPos = input.GetPointer()
		end
	end,
	
	Update = function(self)
		
		if(self.state == self.states.STAND) then
			self.skeleton.PauseAction()
			self.state = self.states.STAND
		elseif(self.state == self.states.TURN) then
			self.skeleton.PlayAction()
			self.state = self.states.STAND
		elseif(self.state == self.states.WALK) then
			self.skeleton.PlayAction()
			self.state = self.states.STAND
		elseif(self.state == self.states.JUMP) then
			self.skeleton.PauseAction()
			self.state = self.states.STAND
		end
		
		local w,wp,wn = Pick(self.ray,PICK_WATER)
		if(w.IsValid() and self.velocity.Length()>0) then
			PutWaterRipple("../Editor/images/ripple.png",wp)
		end
		
		
		self.velocity = vector.Add(self.velocity, gravity)
		self.skeleton.Translate(vector.Multiply(getDeltaTime() * 60, self.velocity))
		self.ray = Ray(self.skeleton.GetPosition():Add(Vector(0,4)),Vector(0,-1,0))
		local pPrev = self.p
		self.o,self.p,self.n = Pick(self.ray, PICK_OPAQUE, ~self.layerMask)
		if(not self.o.IsValid()) then
			self.p=pPrev
		end
		if(self.skeleton.GetPosition().GetY() < self.p.GetY() and self.velocity.GetY()<=0) then
			self.state = self.states.STAND
			self.skeleton.Translate(vector.Subtract(self.p,self.skeleton.GetPosition()))
			self.velocity=Vector()
		end
		
	end
}

-- Third person camera controller class:
-- It references a characterController (targetPlayer)
tpCamera = {
	camera = GetCamera(),
	targetPlayer = nil,
	
	Reset = function(self)
		self.targetPlayer = nil
		self.camera.ClearTransform()
	end,
	Follow = function(self, targetPlayer)
		self.targetPlayer = targetPlayer
		self.camera.Rotate(Vector(0.1))
		self.camera.Translate(Vector(2,7,-12))
		self.camera.AttachTo(targetPlayer.target)
	end,
	UnFollow = function(self)
		self.targetPlayer = nil
		self.camera.Detach()
	end,
	
	Update = function(self)
		if(self.targetPlayer ~= nil) then
			local camRestDistance = 12.0
			local camTargetDiff = vector.Subtract(self.targetPlayer.target.GetPosition(), self.camera.GetPosition())
			local camTargetDistance = camTargetDiff.Length()
			
			if(camTargetDistance < camRestDistance) then
				-- Camera is closer to target than rest distance, push it back some amount...
				self.camera.Translate(vector.Multiply(getDeltaTime() * 60, Vector(0,0,-0.14)))
			end
			
			-- Cast a ray from the camera eye and check if it hits something other than the player...
			local camRay = Ray(self.camera.GetPosition(),camTargetDiff.Normalize())
			local camCollObj,camCollPos,camCollNor = Pick(camRay, PICK_OPAQUE, ~self.targetPlayer.layerMask)
			if(camCollObj.IsValid()) then
				-- It hit something, see if it is between the player and camera:
				local camCollDiff = vector.Subtract(camCollPos, self.camera.GetPosition())
				local camCollDistance = camCollDiff.Length()
				if(camCollDistance < camTargetDistance) then
					-- If there was something between player and camera, clamp camera position inside:
					self.camera.Translate(Vector(0,0,camCollDistance))
				end
			end
			
		end
	end,
}


-- Player Controller
local player = characterController
-- Third Person camera
local camera = tpCamera

-- Main loop:
runProcess(function()

	-- We will override the render component so we can invoke the script from Editor and controls won't collide with editor scripts
	--	TODO: for example ESC key could bring us back to editor component and destroy this?
	local component = TiledForwardRenderableComponent()
	component.SetSSREnabled(false)
	component.SetSSAOEnabled(false)
	main.SetActiveComponent(component)
	
	--ClearWorld()
	--LoadModel("../models/Misc/scene.wimf")

	player:Load("../models/girl/girl.wimf","omino_common","Armature_common")
	player:Reset()
	--player:Reposition(Vector(0,4,-20))
	camera:Reset()
	camera:Follow(player)
	
	while true do

		player:Input()
		
		player:Update()
		
		camera:Update()
		
		-- Wait for Engine update tick
		update()
		
	end
end)




-- Debug draw:

-- Draw Helpers
local DrawAxis = function(point,f)
	DrawLine(point,point:Add(Vector(f,0,0)),Vector(1,0,0,1))
	DrawLine(point,point:Add(Vector(0,f,0)),Vector(0,1,0,1))
	DrawLine(point,point:Add(Vector(0,0,f)),Vector(0,0,1,1))
end
local DrawAxisTransformed = function(point,f,transform)
	DrawLine(point,point:Add( Vector(f,0,0).Transform(transform) ),Vector(1,0,0,1))
	DrawLine(point,point:Add( Vector(0,f,0).Transform(transform) ),Vector(0,1,0,1))
	DrawLine(point,point:Add( Vector(0,0,f).Transform(transform) ),Vector(0,0,1,1))
end

-- Draw
runProcess(function()
	while true do
	
		while( backlog_isactive() ) do
			waitSeconds(1)
		end
		
		-- Drawing additional render data (slow, only for debug purposes)
		
		--velocity
		DrawLine(player.skeleton.GetPosition():Add(Vector(0,4)),player.skeleton.GetPosition():Add(Vector(0,4)):Add(player.velocity))
		--face
		DrawLine(player.skeleton.GetPosition():Add(Vector(0,4)),player.skeleton.GetPosition():Add(Vector(0,4)):Add(player.face:Normalize()),Vector(1,0,0,1))
		--intersection
		DrawAxis(player.p,0.5)
		
		
		-- Wait for the engine to render the scene
		render()
	end
end)

