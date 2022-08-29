local scene = GetScene() -- GetScene() is from global scope
local entity = GetEntity() -- GetEntity() is local to this script (only available if this is running from a ScriptComponent)
local transform = scene.Component_GetTransform(entity)
if transform ~= nil then
	transform.Rotate(Vector(0, math.pi * getDeltaTime(), 0))
	
	local text = "This sample shows usage of a ScriptComponent, which is attached to a scene entity.\n"
	text = text .. "Each ScriptComponent will get these functions that can be used to reference data:\n"
	text = text .. "script_file() : " .. script_file() .. "\n"
	text = text .. "script_dir() : " .. script_dir() .. "\n"
	text = text .. "script_pid() : " .. script_pid() .. "\n"
	text = text .. "GetEntity() : " .. GetEntity() .. "\n"
	DrawDebugText(text, vector.Add(transform.GetPosition(), Vector(0,3,0)), Vector(1,1,1,1), 0.25)
end
