-- Integration tests for physics

local passedTestCount = 0
local failedTestCount = 0

runProcess(function()
	dofile(script_dir() .. "gravity01.lua")
	waitSignal("integration_test_complete")

	dofile(script_dir() .. "gravity01.lua")
	waitSignal("integration_test_complete")

	dofile(script_dir() .. "gravity01.lua")
	waitSignal("integration_test_complete")

	dofile(script_dir() .. "gravity02.lua")
	waitSignal("integration_test_complete")
	
	killProcesses()
	backlog_post("Tests passed: " .. passedTestCount .. ", failed: " .. failedTestCount)

end)



runProcess(function()
	while true do
		waitSignal("integration_test_pass")
		passedTestCount = passedTestCount + 1
	end
end)

runProcess(function()
	while true do
		waitSignal("integration_test_fail")
		failedTestCount = failedTestCount + 1
	end
end)




