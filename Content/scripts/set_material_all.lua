-- You can use this script to iterate through all materials in the scene and set the same parameters for each one
backlog_post("---> START SCRIPT: set_material_all.lua")

for i,material in ipairs(GetScene().Component_GetMaterialArray()) do
	material.SetBaseColor(Vector(1,1,1,1))
end

backlog_post("---> END SCRIPT: set_material_all.lua")
