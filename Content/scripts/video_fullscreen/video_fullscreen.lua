killProcesses()  -- stops all running lua coroutine processes

backlog_post("---> START SCRIPT: video_fullscreen.lua")

runProcess(function() 

    local prevrenderpath = application.GetActivePath()
    local renderpath = RenderPath2D()
    application.SetActivePath(renderpath)

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

    if prevrenderpath ~= nil then
        backlog_post("switrched back to previous RenderPath2D with cross fade")
        application.SetActivePath(prevrenderpath, 0.5, 0, 0, 0, FadeType.CrossFade)
    end

end)

backlog_post("---> END SCRIPT: video_fullscreen.lua")
