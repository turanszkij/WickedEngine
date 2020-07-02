#pragma once

static const char* wiLua_Globals = R"(
------------------------------------------
-- Wicked Engine lua globals
------------------------------------------

-- Get table element count (list length)
function len(T)
  local count = 0
  for _ in pairs(T) do 
	count = count + 1 end
  return count
end

-- Post list elements to backlog
backlog_post_list = function(a)
	for key,value in pairs(a) do
		backlog_post(value)
	end
end

-- Get object properties
getprops = function(object)
	for key,value in pairs(getmetatable(object)) do
		backlog_post(key, " ",  value)
	end
end

-- seeding the system random
math.randomseed( os.time() )

-- This table is indexed by coroutine and simply contains the time at which the coroutine
-- should be woken up.
local WAITING_ON_TIME = {}

-- Keep track of how long the game has been running.
local CURRENT_TIME = 0
function waitSeconds(seconds)  
    -- Grab a reference to the current running coroutine.
    local co = coroutine.running()

    -- If co is nil, that means we're on the main process, which isn't a coroutine and can't yield
    assert(co ~= nil, "The main thread cannot wait!")

    -- Store the coroutine and its wakeup time in the WAITING_ON_TIME table
    local wakeupTime = CURRENT_TIME + seconds
    WAITING_ON_TIME[co] = wakeupTime

    -- And suspend the process
    return coroutine.yield(co)
end

-- This function should be called once per game logic update with the amount of time
-- that has passed since it was last called
function wakeUpWaitingThreads(deltaTime)  
    CURRENT_TIME = CURRENT_TIME + deltaTime

    -- First, grab a list of the threads that need to be woken up. They'll need to be removed
    -- from the WAITING_ON_TIME table which we don't want to try and do while we're iterating
    -- through that table, hence the list.
    local threadsToWake = {}
    for co, wakeupTime in pairs(WAITING_ON_TIME) do
        if wakeupTime < CURRENT_TIME then
            table.insert(threadsToWake, co)
        end
    end

    -- Now wake them all up.
    for _, co in ipairs(threadsToWake) do
        WAITING_ON_TIME[co] = nil -- Setting a field to nil removes it from the table
        local success, errorMsg = coroutine.resume(co)
		if not success then
			error("[Lua Error] "..errorMsg)
		end
    end
end

-- This function is just a quick wrapper to start a coroutine.
function runProcess(func)  
    local co = coroutine.create(func)
    local success, errorMsg = coroutine.resume(co)
	if not success then
		error("[Lua Error] "..errorMsg)
	end
	return success
end

-- Signal helpers
local WAITING_ON_SIGNAL = {}

-- Block coroutine while waiting for a signal
function waitSignal(signalName)  
    -- Same check as in waitSeconds; the main thread cannot wait
    local co = coroutine.running()
    assert(co ~= nil, "The main thread cannot wait!")

    if WAITING_ON_SIGNAL[signalStr] == nil then
        -- If there wasn't already a list for this signal, start a new one.
        WAITING_ON_SIGNAL[signalName] = { co }
    else
        table.insert(WAITING_ON_SIGNAL[signalName], co)
    end

    return coroutine.yield()
end

-- Send a signal on which a coroutine can be blocked
function signal(signalName)  
    local threads = WAITING_ON_SIGNAL[signalName]
    if threads == nil then return end

    WAITING_ON_SIGNAL[signalName] = nil
    for _, co in ipairs(threads) do
        local success, errorMsg = coroutine.resume(co)
		if not success then
			error("[Lua Error] "..errorMsg)
		end
    end
end

-- Kill all processes
function killProcesses()
	WAITING_ON_SIGNAL = {}
	WAITING_ON_TIME = {}
end

-- Store the delta time for the current frame
local lastDeltaTime = 0
function setDeltaTime(dt)
	lastDeltaTime = dt
	wakeUpWaitingThreads(dt)
end
function getDeltaTime()
	return lastDeltaTime
end

-- Wait until the game engine fixed update function runs again
function fixedupdate()
	waitSignal("wickedengine_fixed_update_tick")
end

-- Wait until the game engine update function runs again
function update()
	waitSignal("wickedengine_update_tick")
end

-- Wait until the game engine drawing function runs again
function render()
	waitSignal("wickedengine_render_tick")
end

-- linear interpolation
function math.lerp(a,b,t)
	return (a + (b-a)*t);
end
-- clamp number between min,max
function math.clamp(x,min,max)
	if(x < min) then
		return min
	elseif(x > max) then
		return max
	else
		return x
	end
end
-- clamp number between 0,1
function math.saturate(x)
	return math.clamp(x,0,1)
end
-- round number to nearest integer
function math.round(x)
	return x + 0.5 - ( x + 0.5 ) % 1;
end

)";
