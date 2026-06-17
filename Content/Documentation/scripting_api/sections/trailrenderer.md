# TrailRenderer

[← Back to Scripting API index](../index.md)

```lua
    --- Trail Renderer.
    ---
    ---@return TrailRenderer
    function TrailRenderer() end

    --- Renders a smooth ribbon trail through a series of points, e.g. for
    --- projectiles or weapon swipes.
    ---
    ---@class TrailRenderer
    local TrailRenderer = {}

    --- Adds a new point to the trail. Note: if rotation is not specified, then
    --- point will be camera facing, otherwise UP direction will be rotated.
    ---
    ---@param pos Vector
    ---@param width? number
    ---@param color? Vector
    ---@param rotationQuaternion? Vector
    function TrailRenderer.AddPoint(pos, width, color, rotationQuaternion) end

    --- Cuts the trail at last point and starts a new trail. You can specify
    --- that this cut will create a loop of the previously added points.
    ---
    ---@param loop? boolean
    function TrailRenderer.Cut(loop) end

    --- Applies fade for the whole trail continuously, and removes segments that
    --- can be removed due to faded.
    ---
    ---@param amount number
    function TrailRenderer.Fade(amount) end

    --- Removes all points and cuts from the trail.
    ---
    function TrailRenderer.Clear() end

    --- Returns the number of points in the trail.
    ---
    ---@return integer
    function TrailRenderer.GetPointCount() end

    --- Returns the position, width and color of the trail point at the given
    --- index.
    ---
    ---@param index integer
    ---
    ---@return Vector position
    ---@return number width
    ---@return Vector color
    function TrailRenderer.GetPoint(index) end

    --- Sets the point parameters on the specified index.
    ---
    ---@param pos Vector
    ---@param width? number
    ---@param color? Vector
    function TrailRenderer.SetPoint(pos, width, color) end

    --- Set blend mode of the whole trail.
    ---
    ---@param blendmode integer
    function TrailRenderer.SetBlendMode(blendmode) end

    --- Returns the blend mode.
    ---
    ---@return integer
    function TrailRenderer.GetBlendMode() end

    --- Set the subdivision amount of the whole trail.
    ---
    ---@param subdiv integer
    function TrailRenderer.SetSubdivision(subdiv) end

    --- Returns the subdivision.
    ---
    ---@return integer
    function TrailRenderer.GetSubdivision() end

    --- Set the width of the whole trail.
    ---
    ---@param width number
    function TrailRenderer.SetWidth(width) end

    --- Returns the width.
    ---
    ---@return number
    function TrailRenderer.GetWidth() end

    --- Set the color of the whole trail.
    ---
    ---@param color Vector
    function TrailRenderer.SetColor(color) end

    --- Returns the color.
    ---
    ---@return Vector
    function TrailRenderer.GetColor() end

    --- Set the texture of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexture(tex) end

    --- Returns the texture.
    ---
    ---@return Texture
    function TrailRenderer.GetTexture() end

    --- Set the texture2 of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexture2(tex) end

    --- Returns the texture2.
    ---
    ---@return Texture
    function TrailRenderer.GetTexture2() end

    --- Set the texture UV tiling multiply-add value of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexMulAdd(tex) end

    --- Returns the tex mul add.
    ---
    ---@return Texture
    function TrailRenderer.GetTexMulAdd() end

    --- Set the texture2 UV tiling multiply-add value of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexMulAdd2(tex) end

    --- Returns the tex mul add2.
    ---
    ---@return Texture
    function TrailRenderer.GetTexMulAdd2() end

    --- Sets the depth soften amount (default = 10).
    ---
    ---@param value number
    function TrailRenderer.SetDepthSoften(value) end
```
