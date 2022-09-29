local distance = 1
local scene = GetScene()
local zone_transform = scene.Component_GetTransform(GetEntity())

if zone_transform ~= nil then
    local zone_pos = zone_transform.GetPosition()
    local zone_active = false

    for i,entity in ipairs(scene.Entity_GetExpressionArray()) do
        local transform = scene.Component_GetTransform(entity)
        local expression = scene.Component_GetExpression(entity)
        if expression ~= nil and transform ~= nil then
            local expression_pos = transform.GetPosition()
            if vector.Subtract(expression_pos, zone_pos).Length() < distance then
                expression.SetPresetWeight(ExpressionPreset.Happy, 1)
                zone_active = true
            else
                expression.SetPresetWeight(ExpressionPreset.Happy, 0)
            end
        end
    end

    local str = "If character gets close to this, it will be happy\n"
    if zone_active then
        str = str .. "...and a character is currently happy!"
    else
        str = str .. "...but no one is happy currently."
    end
    DrawDebugText(str, vector.Add(zone_pos, Vector(0,1,0)), Vector(1,1,1,1), 0.1, DEBUG_TEXT_DEPTH_TEST | DEBUG_TEXT_CAMERA_FACING)
end
