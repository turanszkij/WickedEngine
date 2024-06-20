-- Integration test for physics

local scene = GetScene()

local function Test()
	local self = {
		entity = INVALID_ENTITY,
		staticEntity = INVALID_ENTITY,

		SetUp = function(self)
			do
				local scalingMat = matrix.Scale(Vector(1,1,1))
				local rotMat = matrix.RotationY(0.0)
				local pos = Vector(0,10,0)
				local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
				
				self.entity = LoadModel(script_dir() .. "../../models/cube.wiscene", transformMat)
				local rigidBody = scene.Component_CreateRigidBodyPhysics(self.entity)
				rigidBody.Shape = RigidBodyShape.Box
				-- cube.wiscene is not a unit-1 cube. it is the Blender cube, which has side-length 2.
				rigidBody.BoxParams_HalfExtents = Vector(1,1,1)
				rigidBody.SetDisableDeactivation(true)
			end

			do
				local scalingMat = matrix.Scale(Vector(1,1,1))
				local rotMat = matrix.RotationY(0.0)
				local pos = Vector(0,0,0)
				local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
				
				self.staticEntity = LoadModel(script_dir() .. "../../models/cube.wiscene", transformMat)
				local rigidBody = scene.Component_CreateRigidBodyPhysics(self.staticEntity)
				rigidBody.Shape = RigidBodyShape.Box
				rigidBody.BoxParams_HalfExtents = Vector(1,1,1)
				rigidBody.SetKinematic(true)
			end

		end,

		TearDown = function(self)
			scene.Entity_Remove(self.entity)
			scene.Entity_Remove(self.staticEntity)
		end,
	}
	return self
end

local test = Test()
test:SetUp()
runProcess(function()

	waitSeconds(2)
	local transform = scene.Component_GetTransform(test.entity)

	local expectedRestingPosY = 2.0
    local expectedRestingPos = Vector(0, expectedRestingPosY, 0)
	local epsilon = 0.05
	if (math.abs(expectedRestingPosY - transform.GetPosition().GetY()) > epsilon) then 
		backlog_post("Test FAIL " .. script_file())
        signal("integration_test_fail")
		vec = expectedRestingPos.Subtract(transform.GetPosition(), expectedRestingPos)
		backlog_post("length: " .. length .. "," .. "pos: " .. vec.GetX() .. " " .. vec.GetY() .. " " .. vec.GetZ())
	else 
		backlog_post("Test PASS " .. script_file())
        signal("integration_test_pass")
	end 
end)

runProcess(function()
	waitSeconds(2.1)
    -- teardown regardless after 2.1 seconds for predictable testing
	test:TearDown()
    signal("integration_test_complete")
end)

