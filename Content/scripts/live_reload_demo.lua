-- Lua script live reload demo
-- This is a script which showcases the new live-reload behavior and also the new path system for scripts
-- Live reload is done by loading the script by file, editing the script, and focus again on the 
-- editor window to see the changes

-- There also exist an API to create a custom live-reload function if you have your own, 
-- for example: live reload with efsw to immediately see changes without the need to focus again on the window

-- There are new APIs exist too for managing scripts
-- KillProcess(coroutine)  >>> To kill exactly one process if you know the coroutine, 
--    runProcess exposes this by returning two variables: success, and coroutine
--    coroutine holds the index to kill this exact runProcess you're running
-- KillProcessPID(SCRIPT_PID)  >>> To kill all processes within one instance of the script
--    SCRIPT_PID are a local exposed to each script for the user to track and kill one full script
-- KillProcessFile(SCRIPT_FILE)  >> To kill all instances of scripts which originates from one file
--    SCRIPT_PID are a local exposed to each script for the user to track and kill all 
--    instances of scripts that uses the same file

-- Here's an example on how to keep script data among reloads
-- Use D table to keep track of variables across reloads
-- To initialize data, use the code below, can be used for normal variables or table
-- Initialization can be done anywhere, just make sure the data are'nt accessed first before it is being used
D("counter",0)
-- To access the data you can use D.counter or D["counter"]
-- Specifically for tables, if you modify a subtree of the table, 
-- only that subtree will be reset/modified while the others are kept persistent
-- Example for table:
D("tt", {
	counter = 0,
	hello = "Hi!"
})
-- Change the D.counter in the example below into D.tt.counter, and then either change the data type of hello OR
-- add a new member to the table and see if D.tt.counter are kept persistent!

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

	-- Down below is a small demo to open a file on another script and open it relative to that script's path
	dofile(SCRIPT_DIR .. "subscript_demo/load_dojo.lua", true)

	while true do
		D.counter = D.counter + 0.00001
		font.SetText("File source: " .. SCRIPT_DIR .. "\nHello there! I'm Dr. Okabe, THE mad scientist. \nEl. Psy. Kongroo. \nUniverse alteration index: " .. D.counter)
		update()
	end
end)