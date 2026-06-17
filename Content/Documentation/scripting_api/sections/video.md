# Video

[← Back to Scripting API index](../index.md)

The video interface consists of two types of objects: Video and VideoInstance.
Note: these are the underlying objects that VideoComponents use in the scene, to
easily set videos to materials or lights, look at the
[VideoComponent](#videocomponent) object that you can use with the scene's
entity component system.

## Video

```lua
    --- Loads an MP4 video file (currently only the H264 internal compression
    --- format is supported).
    ---
    ---@param filename string
    ---
    ---@return Video
    function Video(filename) end

    --- A video resource loaded from file.
    ---
    ---@class Video
    local Video = {}

    --- Returns true if the video was successfully created.
    ---
    ---@return boolean
    function Video.IsValid() end

    --- Returns the duration seconds.
    ---
    ---@return number
    function Video.GetDurationSeconds() end
```

## VideoInstance

The VideoInstance object is responsible to decode the video frames and output
them to textures. One Video can be decoded with multiple VideoInstances to
display frames of the video at different timings. Normally you would use one
VideoInstance for a video, unless you want to show the video multiple times at
once at different locations.

```lua
    --- Creates a decoder instance for the video data.
    ---
    ---@param video Video
    ---
    ---@return VideoInstance
    function VideoInstance(video) end

    --- A playing instance of a Video, with playback controls.
    ---
    ---@class VideoInstance
    local VideoInstance = {}


    --- Returns true if the video instance was successfully created.
    ---
    ---@return boolean
    function VideoInstance.IsValid() end

    --- Play.
    function VideoInstance.Play() end

    --- Pause.
    function VideoInstance.Pause() end

    --- Stop.
    function VideoInstance.Stop() end

    --- Sets whether playback loops when the video reaches the end.
    ---
    ---@param looped boolean
    function VideoInstance.SetLooped(looped) end

    --- Seek.
    ---
    ---@param timerSeconds number
    function VideoInstance.Seek(timerSeconds) end

    --- Returns current video playback timer for this decoder instance in
    --- seconds.
    ---
    ---@return number
    function VideoInstance.GetCurrentTimer() end

    --- Returns true if the video playback has ended.
    ---
    ---@return boolean
    function VideoInstance.IsEnded() end
```
