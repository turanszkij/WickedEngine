-- Wicked Engine Test Framework lua script

debugout("Begin script: test_script.lua");


-- Load a model:
local model = LoadModel("../models/Stormtrooper/", "Stormtrooper");

-- Load an image:
local sprite = Sprite("images/HelloWorld.png");
sprite.SetEffects(ImageEffects(100,100,100,50));
-- Set this image as renderable to the active component:
local component = main.GetActiveComponent();
component.AddSprite(sprite);


-- Start a background task to rotate the model and update the sprite:
runProcess(function()
	local velocity = Vector((math.random() * 2 - 1) * 4, (math.random() * 2 - 1) * 4); -- starting velocity for our sprite
	local screenW = GetScreenWidth();
	local screenH = GetScreenHeight();
	
	while(true) do
	
		-- Bounce our sprite across the screen:
		local fx = sprite.GetEffects();
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
		sprite.SetEffects(fx);
		
		-- Rotate the model:
		model.Rotate(Vector(0, -0.01, 0));
		
		fixedupdate(); -- wait for new frame
	end
end);


debugout("Script complete.");
