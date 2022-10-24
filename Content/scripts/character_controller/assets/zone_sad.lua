if not Script_Initialized(script_pid()) then
    dofile(script_dir() .. "zone_expression.lua")
end
zone_expression(GetEntity(), ExpressionPreset.Sad, "sad")