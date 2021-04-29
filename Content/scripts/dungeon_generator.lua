-- Simplistic random dungeon generator lua script
--	You can call this from Editor for example, and it will load a complete dungeon from set pieces

local modelpath = "../models/"

dungeon={
	
	Generate = function(complexity)
	
		local scalingMat = matrix.Scale(Vector(6,6,6))
		
		-- This will hold the bounding boxes of the placed segments
		local boxes = {}
		-- This holds unconnected exits
		local exits = {}
		-- If an exit should be placed, look up its transform here
		local exitTransforms = {}
		
		-- threshold so that aabbs snapped to each other are not detected as intersecting
		-- because segments are needed to be snapped to each other
		-- chose this value so that it also eliminates floating point errors
		local aabb_threshold = Vector(0.05,0.05,0.05)
		
		-- Shrink an AABB by a threshold vector
		local function ShrinkAABBThreshold(aabb, threshold)
			return AABB(aabb.GetMin():Add(threshold), aabb.GetMax():Subtract(threshold))
		end
		
		-- Check if a segment can be placed or not by specifying its axis aligned bounding box
		local function CheckSegmentCanBePlaced(aabb)
			aabb = ShrinkAABBThreshold(aabb, aabb_threshold)
			
			for i,box in ipairs(boxes) do
				if box.Intersects(aabb) then
					return false
				end
			end
			
			table.insert(boxes,aabb)
			
			return true
		end
		
		-- Merge rooms if their exits are intersecting, but close them if not
		local function MergeExits()
			for i = 1, #exits do
				local place = true
				for j = 1, #exits do
					if i~=j and exits[i].Intersects(exits[j]) then
						place = false
						break
					end
				end
				
				if place then
					LoadModel(modelpath .. "dungeon/end.wiscene",exitTransforms[i])
				end
			end
		end
		
		-- Create physical dungeon segments recursively
		local function GenerateDungeon(remaining, pos, rotY)
			remaining = remaining - 1
			local rotMat = matrix.RotationY(rotY)
			local transformMat = matrix.Multiply(rotMat, matrix.Multiply(matrix.Translation(pos), scalingMat))
			
			-- Mark exit point if the generation ended and return from recursion
			if (remaining < 0) then
				table.insert(exits, ShrinkAABBThreshold( AABB(Vector(-0.5,0,0),Vector(0.5,1,0.5)).Transform(transformMat), aabb_threshold) )
				table.insert(exitTransforms, transformMat)
				return;
			end
			
			local select = math.random(0, 100)
			if(select < 80) then -- common pieces
				local select2 = math.random(0, 6)
				if(select2 < 1) then --left turn
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,2,2)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/turnleft.wiscene",transformMat)
						GenerateDungeon(remaining,pos:Add(Vector(-1,0,1):Transform(rotMat)),rotY-0.5*math.pi)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				elseif(select2 < 2) then --right turn
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,2,2)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/turnright.wiscene",transformMat)
						GenerateDungeon(remaining,pos:Add(Vector(1,0,1):Transform(rotMat)),rotY+0.5*math.pi)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				elseif(select2 < 3) then --t-junction
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,2,2)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/tjunction.wiscene",transformMat)
						-- right
						GenerateDungeon(remaining,pos:Add(Vector(1,0,1):Transform(rotMat)),rotY+0.5*math.pi)
						-- left
						GenerateDungeon(remaining,pos:Add(Vector(-1,0,1):Transform(rotMat)),rotY-0.5*math.pi)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				elseif(select2 < 4) then --cross-junction
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,2,2)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/crossjunction.wiscene",transformMat)
						-- right
						GenerateDungeon(remaining,pos:Add(Vector(1,0,1):Transform(rotMat)),rotY+0.5*math.pi)
						-- left
						GenerateDungeon(remaining,pos:Add(Vector(-1,0,1):Transform(rotMat)),rotY-0.5*math.pi)
						-- straight
						GenerateDungeon(remaining,pos:Add(Vector(0,0,2):Transform(rotMat)),rotY)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				else --straight block
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,2,2)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/block.wiscene",transformMat)
						GenerateDungeon(remaining,pos:Add(Vector(0,0,2):Transform(rotMat)),rotY)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				end
			elseif(select < 98) then -- average pieces
				local select2 = math.random(0, 100)
				if( select2 < 20 ) then --small room left
					if CheckSegmentCanBePlaced(AABB(Vector(-5,0,0),Vector(1,2,6)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/smallroomleft.wiscene",transformMat )
						GenerateDungeon(remaining,pos:Add(Vector(-5,0,5):Transform(rotMat)),rotY-0.5*math.pi)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				elseif( select2 < 30 ) then --odd corridor
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(3,2,8)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/oddcorridor.wiscene",transformMat )
						GenerateDungeon(remaining,pos:Add(Vector(2,0,8):Transform(rotMat)),rotY)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				elseif( select2 < 60 ) then --up corridor
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,4,6)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/upcorridor.wiscene",transformMat )
						GenerateDungeon(remaining,pos:Add(Vector(0,2,6):Transform(rotMat)),rotY)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				else --corridor
					if CheckSegmentCanBePlaced(AABB(Vector(-1,0,0),Vector(1,2,6)).Transform(transformMat)) then
						LoadModel(modelpath .. "dungeon/corridor.wiscene",transformMat )
						GenerateDungeon(remaining,pos:Add(Vector(0,0,6):Transform(rotMat)),rotY)
					else
						GenerateDungeon(remaining,pos,rotY)
					end
				end
			else -- rare pieces
				if CheckSegmentCanBePlaced(AABB(Vector(-4,0,0),Vector(4,8,8)).Transform(transformMat)) then
					LoadModel(modelpath .. "dungeon/room.wiscene",transformMat )
					-- right
					GenerateDungeon(remaining,pos:Add(Vector(4,0,4):Transform(rotMat)),rotY + 0.5*math.pi)
					-- left
					GenerateDungeon(remaining,pos:Add(Vector(-4,0,4):Transform(rotMat)),rotY - 0.5*math.pi)
					-- straight
					GenerateDungeon(remaining,pos:Add(Vector(0,0,8):Transform(rotMat)),rotY)
				else
					GenerateDungeon(remaining,pos,rotY)
				end
			end
		end
		
		
		-- Place the start piece of the dungeon
		LoadModel(modelpath .. "dungeon/start.wiscene",scalingMat)
		CheckSegmentCanBePlaced(AABB(Vector(-1,0,-2),Vector(1,2,0)))
		-- Call recursive generator function
		GenerateDungeon(complexity,Vector(0,0,0),0)
		MergeExits()
		
	
	end,

}

-- Just call this to generate a dungeon of some complexity
dungeon.Generate(15)