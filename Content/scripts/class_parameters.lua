-- This is a script to demonstrate new parameter access to lua c classes in Wicked Engine
-- Now you can access class parameters like you access table values just like usual, which now retrieves
-- and change class data without using separate getter-setter functions

backlog_post("---> START SCRIPT: class_parameters.lua")

ClearWorld()

local scene = GetScene()

local sun_entity = scene.Entity_Create()
local sun_name = scene.Component_CreateName(sun_entity)
sun_name.SetName("THE SUN")
scene.Component_CreateLayer(sun_entity)
scene.Component_CreateTransform(sun_entity)
local sun = scene.Component_CreateLight(sun_entity)
-- sun.SetType() can now be done like this
sun.Type = 0
sun.Intensity = 10.0

local weather_entity = scene.Entity_Create()
local weather_name = scene.Component_CreateName(weather_entity)
weather_name.SetName("My Animated Weather")
local weather = scene.Component_CreateWeather(weather_entity)
weather.SetRealisticSky(true)
weather.SetVolumetricClouds(true)
weather.skyExposure = 1.2

weather.VolumetricCloudParams.CloudStartHeight = -100.0
weather.VolumetricCloudParams.WeatherScale = 0.04
weather.VolumetricCloudParams.WeatherDensityAmount = 0.5
weather.VolumetricCloudParams.SkewAlongWindDirection = 5.0

runProcess(function()
    while true do
        update()
        if(input.Press(string.byte('R'))) then
            killProcessPID(script_pid(), true)
            backlog_post("RESTART")
            dofile(script_file(), script_pid())
            return
        end
    end
end)

backlog_post("---> END SCRIPT: class_parameters.lua")