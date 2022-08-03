-- Lua script live reload demo
-- This is a script which showcases the new live-reload behavior and also the new path system for scripts
-- Live reload is done by loading the script by file, editing the script, and focus again on the editor window to see the changes

-- There also exist an API to create a custom live-reload function if you have your own, 
-- for example: live reload with efsw to immediately see changes without the need to focus again on the window

-- There are new APIs exist too for managing scripts
-- KillProcess(coroutine)  >>> To kill exactly one process if you know the coroutine, 
--    runProcess exposes this by returning two variables: success, and coroutine
--    coroutine holds the index to kill this exact runProcess you're running
-- KillProcessPID(SCRIPT_PID)  >>> To kill all processes within one instance of the script
--    SCRIPT_PID are a local exposed to each script for the user to track and kill one full script
-- KillProcessFile(SCRIPT_FILE)  >> To kill all instances of scripts which originates from one file
--    SCRIPT_PID are a local exposed to each script for the user to track and kill all instances of scripts that uses the same file

local counter = 0
-- Use this code to preserve data when the script reloads
-- Details of usage:
--     syncData("Header_Name", your_variable)
-- your_variable can be any varible, including a table
-- specifically with table, syncData will try to preserve the overall data structure even if you modify only a section of the table
-- use it at the initialization part of the script (execute it only once)
-- Header namespace are specific to one script PID, so other scripts and instances can use the same namespace
syncData("counter",counter)

local proc_success, proc_coroutine = runProcess(function() 
    ClearWorld()
	local prevPath = application.GetActivePath()
	local path = RenderPath3D()
	path.SetLightShaftsEnabled(true)
	application.SetActivePath(path)

	local font = SpriteFont("");
	font.SetSize(30)
	font.SetPos(Vector(10, 10))
	font.SetAlign(WIFALIGN_LEFT, WIFALIGN_TOP)
	font.SetColor(0xFFADA3FF)
	font.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(font)

	-- If you use the new path mode, you can use SCRIPT_DIR string variable to load files relative to the script directory.
	-- New path mode and old path mode cannot be used interchangeably.

	-- To run script in the new path mode use dofile("your_script.lua", true)
	-- To run script in the old path mode use dofile("your_script.lua") as usual
	LoadModel(SCRIPT_DIR .. "../models/dojo.wiscene")

	while true do
		counter = counter + 0.00001
		font.SetText(SCRIPT_PID.."\nHello there! I'm Dr. Okabe, THE mad scientist. \nEl. Psy. Kongroo. \nUniverse alteration index: " .. counter)
		fixedupdate()
	end
end)