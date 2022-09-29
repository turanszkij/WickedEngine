-- This script shows how content can be accessed relative to the current script directory
--	This will work when calling scripts from scripts

backlog_post("---> START SCRIPT: load_model.lua")

LoadModel(script_dir() .. "../../models/teapot.wiscene")

backlog_post("---> END SCRIPT: load_model.lua")
