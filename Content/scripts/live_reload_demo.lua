local Text = ""

runProcess(function() 
    -- Use this code to preserve data when the script reloads
    syncData("Text", text)

    ClearWorld()
	local prevPath = application.GetActivePath()
	local path = RenderPath3D()
	path.SetLightShaftsEnabled(true)
	application.SetActivePath(path)

	local font = SpriteFont("Hello there! I'm Dr. Okabe, THE mad scientist. \nEl. Psy. Kongroo. \n(Edit & save this text, then go back to the main window to see the result)");
	font.SetSize(30)
	font.SetPos(Vector(10, 10))
	font.SetAlign(WIFALIGN_LEFT, WIFALIGN_TOP)
	font.SetColor(0xFFADA3FF)
	font.SetShadowColor(Vector(0,0,0,1))
	path.AddFont(font)

	LoadModel(SCRIPT_DIR .. "../models/dojo.wiscene")
end)