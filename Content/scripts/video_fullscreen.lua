killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: video_fullscreen.lua")

runProcess(function() 

    local renderpath = application.GetActivePath()

    local video = Video(script_dir() .. "/video.mp4")
    local videoinstance = VideoInstance(video)
    videoinstance.Play()

    local sprite = Sprite()

    renderpath.AddVideoSprite(videoinstance, sprite)

    while videoinstance.IsEnded() == false do
        update()

        sprite.SetParams(ImageParams(0,0,GetScreenWidth(), GetScreenHeight()))

    end

    -- clean up after video ended:
    renderpath.RemoveSprite(sprite) -- also removes video because sprite and video were added together with AddVideoSprite

    backlog_post("video ended and removed from RenderPath2D")

end)

backlog_post("---> END SCRIPT: video_fullscreen.lua")
