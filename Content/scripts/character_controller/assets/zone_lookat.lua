local distance = 1
local scene = GetScene()
local zone_transform = scene.Component_GetTransform(GetEntity())

if zone_transform ~= nil then
    local zone_pos = zone_transform.GetPosition()
    local zone_active = false

    for i,entity in ipairs(scene.Entity_GetHumanoidArray()) do
        local transform = scene.Component_GetTransform(entity)
        local humanoid = scene.Component_GetHumanoid(entity)
        if humanoid ~= nil and transform ~= nil then
            local humanoid_pos = transform.GetPosition()
            if vector.Subtract(humanoid_pos, zone_pos).Length() < distance then
                humanoid.SetLookAtEnabled(true)
                humanoid.SetLookAt(zone_pos)
                zone_active = true
            end
        end
    end

    local str = "If character gets close to this, it will look at here...\n"
    if zone_active then
        str = str .. "...and a character is currently looking at this!"
    else
        str = str .. "...but no one is looking at this currently."
    end
    DrawDebugText(str, vector.Add(zone_pos, Vector(0,1,0)), Vector(1,1,1,1), 0.1, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
end
