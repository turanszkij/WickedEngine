-- This script will play a camera shake effect

local camera = GetCamera()
local shake_offset = Vector()
local shake_x = -1
local shake_y = -1
local shake_elapsed = 0

-- Outside settable params:
scriptableCameraShakeAmount = 0.2
scriptableCameraShakeFrequency = 0.05
scriptableCameraShakeAggressiveness = 0.1

runProcess(function()
    while true do
        if input.Press(KEYBOARD_BUTTON_F6) then
            if scriptableCameraShakeAmount > 0 then
                scriptableCameraShakeAmount = 0
            else
                scriptableCameraShakeAmount = 0.1
            end
        end
        if shake_elapsed > scriptableCameraShakeFrequency then
            shake_elapsed = 0
            if scriptableCameraShakeAmount > 0 then
                local up = camera.GetUpDirection()
                local forward = camera.GetLookDirection()
                local side = vector.Cross(up, forward)
                shake_x = shake_x * -1
                shake_y = shake_y * -1
                shake_offset = vector.Add(
                    vector.Multiply(side, math.random() * shake_x * scriptableCameraShakeAmount),
                    vector.Multiply(up, math.random() * shake_y * scriptableCameraShakeAmount)
                )
            else
                shake_offset = Vector()
            end
        end
        shake_elapsed = shake_elapsed + getDeltaTime()

        local pos = camera.GetPosition()
        pos = vector.Lerp(pos, vector.Add(pos, shake_offset), scriptableCameraShakeAggressiveness)
        camera.SetPosition(pos)
        camera.UpdateCamera()
        update()
    end
end)
