# Canvas

[← Back to Scripting API index](../index.md)

```lua
    --- Creates a Canvas object. In practice you obtain the active canvas from
    --- the application (e.g. `application:GetCanvas()`) instead of constructing
    --- one.
    ---
    ---@return Canvas
    function Canvas() end

    --- Describes a drawable area.
    ---
    ---@class Canvas
    local Canvas = {}

    --- Get the canvas pixels per inch (DPI).
    ---
    ---@return number
    function Canvas.GetDPI() end

    --- Scaling factor between physical and logical size.
    ---
    ---@return number
    function Canvas.GetDPIScaling() end

    --- A custom scaling factor on top of the DPI scaling.
    ---
    ---@return number
    function Canvas.GetCustomScaling() end

    --- Sets a custom scaling factor that will be applied on top of DPI scaling.
    ---
    ---@param value number
    function Canvas.SetCustomScaling(value) end

    --- Get the canvas width in pixels.
    ---
    ---@return integer
    function Canvas.GetPhysicalWidth() end

    --- Get the canvas height in pixels.
    ---
    ---@return integer
    function Canvas.GetPhysicalHeight() end

    --- Get the canvas width in dpi scaled units.
    ---
    ---@return number
    function Canvas.GetLogicalWidth() end

    --- Get the canvas height in dpi scaled units.
    ---
    ---@return number
    function Canvas.GetLogicalHeight() end
```
