-- Wicked Engine Test Framework lua script

backlog_post("Begin script: test_script.lua");


-- Load a model:
local parent = LoadModel("../models/teapot.wiscene");
LoadModel("../models/cameras.wiscene");

-- Load camera sample script:
dofile("../scripts/camera_animation_repeat.lua");
ToggleCameraAnimation();

-- Load an image:
local sprite = Sprite("../images/logo_small.png");
sprite.SetParams(ImageParams(100,100,128,128));
-- Set this image as renderable to the active component:
local component = main.GetActivePath();
component.AddSprite(sprite);


-- Start a background task to rotate the model and update the sprite:
runProcess(function()
	local velocity = Vector((math.random() * 2 - 1) * 4, (math.random() * 2 - 1) * 4); -- starting velocity for our sprite
	local screenW = GetScreenWidth();
	local screenH = GetScreenHeight();

	local scene = GetScene();

	-- This shows how to handle attachments:

	-- -- Create parent transform, this will be rotated:
	-- local parent = CreateEntity();
	-- scene.Component_CreateTransform(parent);
    -- 
	-- -- Retrieve teapot base and lid entity IDs:
	-- local teapot_base = scene.Entity_FindByName("Base");
	-- local teapot_top = scene.Entity_FindByName("Top");
    -- 
	-- -- Attach base to parent, lid to base:
	-- scene.Component_Attach(teapot_base, parent);
	-- scene.Component_Attach(teapot_top, teapot_base);
	
	while(true) do
	
		-- Bounce our sprite across the screen:
		local fx = sprite.GetParams();
		local pos = fx.GetPos();
		local size = fx.GetSize();
		
		-- if it hits a wall, reverse velocity:
		if(pos.GetX()+size.GetX() >= screenW) then
			velocity.SetX(velocity.GetX() * -1);
		end
		if(pos.GetY()+size.GetY() >= screenH) then
			velocity.SetY(velocity.GetY() * -1);
		end
		if(pos.GetX() <= 0) then
			velocity.SetX(velocity.GetX() * -1);
		end
		if(pos.GetY() < 0) then
			velocity.SetY(velocity.GetY() * -1);
		end
		
		pos = vector.Add(pos, velocity);
		
		fx.SetPos(pos);
		sprite.SetParams(fx);

		-- Rotate teapot by parent transform:
		local transform = scene.Component_GetTransform(parent);
		transform.Rotate(Vector(0, 0, 0.01));
		
		
		fixedupdate(); -- wait for new frame
	end
end);


backlog_post("Script complete.");
