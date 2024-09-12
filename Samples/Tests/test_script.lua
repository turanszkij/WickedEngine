-- Wicked Engine Test Framework lua script

backlog_post("Begin script: test_script.lua");


-- Load a model:
local parent = LoadModel("../Content/models/teapot.wiscene");
LoadModel("../Content/models/cameras.wiscene");

-- Load camera sample script:
dofile("../Content/scripts/camera_animation_repeat.lua");
ToggleCameraAnimation();

-- Load an image:
local sprite = Sprite("../Content/logo_small.png");
--sprite.SetTexture(
--	texturehelper.CreateGradientTexture(
--		GradientType.Circular, -- gradient type
--		256, 256, -- resolution of the texture
--		Vector(0.5, 0.5), Vector(0.5, 0), -- start and end uv coordinates will specify the gradient direction and extents
--		GradientFlags.Inverse | GradientFlags.Smoothstep | GradientFlags.PerlinNoise, -- modifier flags bitwise combination
--		"111R", -- for each channel ,you can specify one of the following characters: 0, 1, r, g, b, a
--		2, -- perlin noise scale
--		123, -- perlin noise seed
--		6, -- perlin noise octaves
--		0.8 -- perlin noise persistence
--	)
--)
--sprite.SetTexture(texturehelper.GetLogo())
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
