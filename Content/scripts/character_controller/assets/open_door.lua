local distance = 1
local scene = GetScene()
local zone_transform = scene.Component_GetTransform(GetEntity())
local zone_animation = scene.Component_GetAnimation(GetEntity())

if zone_transform ~= nil and zone_animation ~= nil then
    local zone_pos = zone_transform.GetPosition()

    for i,entity in ipairs(scene.Entity_GetHumanoidArray()) do
        local transform = scene.Component_GetTransform(entity)
        if transform ~= nil then
            local humanoid_pos = transform.GetPosition()
            if vector.Subtract(humanoid_pos, zone_pos).Length() < distance then
                zone_animation.Play()
            end
        end
    end

end
