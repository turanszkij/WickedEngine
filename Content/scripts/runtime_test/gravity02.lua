-- Integration test for physics

-- gravity should roll a sphere down a canal formed of diagonal boxes.

local scene = GetScene()

local function Test()
	local self = {
		entity = INVALID_ENTITY,
		canalWall01 = INVALID_ENTITY,
		canalWall02 = INVALID_ENTITY,
        canalWallEnd = INVALID_ENTITY,


		SetUp = function(self)
			do
				local scalingMat = matrix.Scale(Vector(1,1,1))
				local rotMat = matrix.RotationY(0.0)
				local pos = Vector(0,10,0)
				local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
				-- for lack of a icosphere.wiscene, use the sample content from the fighter game
				self.entity = LoadModel(script_dir() .. "../fighting_game/assets/emitter_fireball.wiscene", transformMat)
				local rigidBody = scene.Component_CreateRigidBodyPhysics(self.entity)
				rigidBody.Shape = RigidBodyShape.Sphere
				-- emitter_fireball.wiscene has unit-sphere (radius is 0.5)
				rigidBody.SphereParams_Radius = 0.5
				rigidBody.SetDisableDeactivation(true)
			end

			do
				local scalingMat = matrix.Scale(Vector(1.32,1,1))
				local rotMat = matrix.Multiply(matrix.RotationZ(math.rad(-15.0)), matrix.RotationX(math.rad(45.0)))
				local pos = Vector(0,0,-1)
				local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
				
				self.canalWall01 = LoadModel(script_dir() .. "../../models/cube.wiscene", transformMat)
				local rigidBody = scene.Component_CreateRigidBodyPhysics(self.canalWall01)
				rigidBody.Shape = RigidBodyShape.Box
				rigidBody.BoxParams_HalfExtents = Vector(1.32,1,1)
				rigidBody.SetKinematic(true)
			end

			do
				local scalingMat = matrix.Scale(Vector(1.32,1,1))
				local rotMat = matrix.Multiply(matrix.RotationZ(math.rad(-15.0)), matrix.RotationX(math.rad(-45.0)))
				local pos = Vector(0,0,1)
				local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
				
				self.canalWall02 = LoadModel(script_dir() .. "../../models/cube.wiscene", transformMat)
				local rigidBody = scene.Component_CreateRigidBodyPhysics(self.canalWall02)
				rigidBody.Shape = RigidBodyShape.Box
				rigidBody.BoxParams_HalfExtents = Vector(1.32,1,1)
				rigidBody.SetKinematic(true)
			end

            do
				local scalingMat = matrix.Scale(Vector(1,1,1))
				local rotMat = matrix.RotationZ(math.rad(-45.0))
				local pos = Vector(3.5,-0.5,0)
				local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
				
				self.canalWallEnd = LoadModel(script_dir() .. "../../models/cube.wiscene", transformMat)
				local rigidBody = scene.Component_CreateRigidBodyPhysics(self.canalWallEnd)
				rigidBody.Shape = RigidBodyShape.Box
				rigidBody.BoxParams_HalfExtents = Vector(1,1,1)
				rigidBody.SetKinematic(true)
			end

		end,

		TearDown = function(self)
			scene.Entity_Remove(self.entity)
			scene.Entity_Remove(self.canalWall01)
			scene.Entity_Remove(self.canalWall02)
			scene.Entity_Remove(self.canalWallEnd)
		end,
	}
	return self
end

local test = Test()
test:SetUp()
runProcess(function()

	waitSeconds(5)
	local transform = scene.Component_GetTransform(test.entity)

	local expectedRestingPos = Vector(2.2, 0.32, 0)
	local epsilon = 0.05
	local length = expectedRestingPos.Subtract(transform.GetPosition(), expectedRestingPos).Length() 
	if (length > epsilon) then 
		backlog_post("Test FAIL " .. script_file())
        signal("integration_test_fail")
		vec = transform.GetPosition()
		backlog_post("length: " .. length .. "," .. "pos: " .. vec.GetX() .. " " .. vec.GetY() .. " " .. vec.GetZ())
	else 
		backlog_post("Test PASS " .. script_file())
        signal("integration_test_pass")
	end 
end)

runProcess(function()
	waitSeconds(5.1)
    -- teardown regardless after 2.1 seconds for predictable testing
	test:TearDown()
    signal("integration_test_complete")
end)

