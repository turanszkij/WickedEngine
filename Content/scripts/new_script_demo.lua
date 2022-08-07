-- Lua new scripting system demo
-- This is a script which showcases the new path system for scripts, and new handling for processes

-- There are new APIs exist too for managing scripts
-- KillProcess(coroutine)  >>> To kill exactly one process if you know the coroutine, 
--    runProcess exposes this by returning two variables: success, and coroutine
--    coroutine holds the index to kill this exact runProcess you're running
-- KillProcessPID(SCRIPT_PID())  >>> To kill all processes within one instance of the script
--    SCRIPT_PID() are a local variable exposed to each script for the user to track and kill one full script
-- KillProcessFile(SCRIPT_FILE())  >> To kill all instances of scripts which originates from one file
--    SCRIPT_PID() are a local variable exposed to each script for the user to track and kill all 
--    instances of scripts that uses the same file

-- Now you can track even the coroutine that the script has
-- You can exactly kill this one coroutine by killing them like this: killProcess(proc_coroutine)
local proc_success, proc_coroutine = runProcess(function()
	-- If you want to do stuff once but never again on the next sequence of reloads, you can do it like this 
	-- (apply to any scripts you have their PID tracked on script too)
	if not Script_Initialized(SCRIPT_PID()) then
		backlog_post("\n")
		backlog_post("== Script INFO ==")
		backlog_post("SCRIPT_FILE(): "..SCRIPT_FILE())
		backlog_post("SCRIPT_PID(): "..SCRIPT_PID())
		backlog_post("SCRIPT_DIR(): "..SCRIPT_DIR())
		backlog_post("\n")

		-- By using a global table you can keep code data across reloads
		-- And if you want to keep each script instance separate you can use SCRIPT_PID() to store the data
		-- e.g D[SCRIPT_PID()].counter = 0
		D = {}
		D.counter = 0

		ClearWorld()
		D.prevPath = application.GetActivePath()
		D.path = RenderPath3D()
		D.path.SetLightShaftsEnabled(true)
		application.SetActivePath(D.path)

		D.font = SpriteFont("");
		D.font.SetSize(30)
		D.font.SetPos(Vector(10, 10))
		D.font.SetAlign(WIFALIGN_LEFT, WIFALIGN_TOP)
		D.font.SetColor(0xFFADA3FF)
		D.font.SetShadowColor(Vector(0,0,0,1))
		D.path.AddFont(D.font)

		-- With the new scripting system, use SCRIPT_DIR() string variable to load files relative to the script directory.
		-- Also running a script can now return its PID, which you can use to kill the script you just launched in this script
		-- Down below is a small demo to open a file on another script and open it relative to that script's path
		D.subscript_PID = dofile(SCRIPT_DIR() .. "subscript_demo/load_dojo.lua", true)
		backlog_post("subscript PID: "..D.subscript_PID)
	end

	while true do
		D.counter = D.counter + 0.00001
		-- Here's a fun thing: edit the text below and reload the script (press R)!
		-- e.g "Hello There!"" to "Yeehaw"
		D.font.SetText("File source: " .. SCRIPT_DIR() .. "\nHello there! I'm Dr. Okabe, THE mad scientist. \nEl. Psy. Kongroo. \nAlternate universe index: " .. D.counter)
		update()
		-- Here's an example on how to reload exactly this script
		if(input.Press(string.byte('R'))) then
			-- This is an example to restart a script using killProcessPID, to keep script PID add true to the argument
			killProcessPID(SCRIPT_PID(), true)
			backlog_post("RESTART")
			dofile(SCRIPT_FILE(), SCRIPT_PID())
			return
		end
		if(input.Press(KEYBOARD_BUTTON_ESCAPE)) then
			-- restore previous component
			-- so if you loaded this script from the editor, you can go back to the editor with ESC
			-- This is an example to exit a script using killProcessPID
			killProcessPID(SCRIPT_PID())
			backlog_post("EXIT")
			application.SetActivePath(D.prevPath)
			return
		end
	end
end)