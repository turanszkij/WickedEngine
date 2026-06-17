# Texture

[← Back to Scripting API index](../index.md)

```lua
    --- Creates a texture. With a filename, loads the image from file; with no
    --- argument, returns an empty (invalid) texture handle.
    ---
    ---@param filename? string
    ---
    ---@return Texture
    function Texture(filename) end

    --- A 2D texture image, loaded from file or created procedurally.
    ---
    ---@class Texture
    local Texture = {}

    --- Returns whether the texture contains valid data (was created or loaded
    --- successfully).
    ---
    ---@return boolean
    function Texture.IsValid() end

    --- Returns the width of the texture in pixels.
    ---
    ---@return integer
    function Texture.GetWidth() end

    --- Returns the height of the texture in pixels.
    ---
    ---@return integer
    function Texture.GetHeight() end

    --- Returns the depth of the texture (for 3D/volume textures).
    ---
    ---@return integer
    function Texture.GetDepth() end

    --- Returns the number of slices in the texture array.
    ---
    ---@return integer
    function Texture.GetArraySize() end

    --- Saves the texture to a file. The extension in the filename selects the
    --- format and must be one of: .JPG, .PNG, .TGA, .BMP, .DDS.
    ---
    ---@param filename string
    function Texture.Save(filename) end

    --- Sets the highest allowed texture asset resolution (only affects DDS
    --- textures that contain mipmaps).
    ---
    ---@param resolution integer
    function SetTextureResolutionLimit(resolution) end

    --- Returns the current texture resolution limit.
    ---
    ---@return integer
    function GetTextureResolutionLimit() end
```

> The engine also defines streaming-memory-threshold variants of these two
> functions, but they are not currently exposed to Lua, so only the
> resolution-limit form above is callable.

## texturehelper

```lua
    --- A global utility object for creating textures programmatically. It
    --- shares the `Texture` type, so the handles it returns expose the query
    --- methods above.
    ---
    ---@class TextureHelper
    local TextureHelper = {}

    --- Global helper for creating textures programmatically.
    ---
    ---@type TextureHelper
    texturehelper = nil

    --- Returns the built-in Wicked Engine logo texture.
    ---
    ---@return Texture
    function TextureHelper.GetLogo() end

    --- Creates a gradient texture from the given parameters.
    ---
    ---@param type GradientType kind of gradient to generate
    ---@param width? integer texture width in pixels (default 256)
    ---@param height? integer texture height in pixels (default 256)
    ---@param uv_start? Vector gradient start UV (default Vector(0, 0))
    ---@param uv_end? Vector gradient end UV (default Vector(0, 0))
    ---@param flags? GradientFlags modifier flags (default GradientFlags.None)
    ---@param swizzle? string per-channel swizzle (default "rgba")
    ---@param perlin_scale? number perlin noise scale (default 1)
    ---@param perlin_seed? integer perlin noise seed (default 1234)
    ---@param perlin_octaves? integer perlin noise octaves (default 8)
    ---@param perlin_persistence? number perlin noise persistence (default 0.5)
    ---
    ---@return Texture
    function TextureHelper.CreateGradientTexture(
        type,
        width,
        height,
        uv_start,
        uv_end,
        flags,
        swizzle,
        perlin_scale,
        perlin_seed,
        perlin_octaves,
        perlin_persistence
    ) end

    --- Creates a lens distortion normal map (16-bit precision).
    ---
    ---@param width? integer texture width in pixels (default 256)
    ---@param height? integer texture height in pixels (default 256)
    ---@param uv_start? Vector distortion center UV (default Vector(0.5, 0.5))
    ---@param radius? number distortion radius (default 0.5)
    ---@param squish? number vertical squish factor (default 1)
    ---@param blend? number blend amount (default 1)
    ---@param edge_smoothness? number edge smoothness (default 0.04)
    ---
    ---@return Texture
    function TextureHelper.CreateLensDistortionNormalMap(
        width,
        height,
        uv_start,
        radius,
        squish,
        blend,
        edge_smoothness
    ) end
```

The gradient functions above use these enums:

```lua
    --- Gradient shapes for texturehelper.CreateGradientTexture.
    ---
    ---@enum GradientType
    GradientType = {
        Linear = 0,
        Circular = 1,
        Angular = 2,
    }

    --- Modifier flags for texturehelper.CreateGradientTexture (combine with
    --- bitwise or).
    ---
    ---@enum GradientFlags
    GradientFlags = {
        None = 0,
        Inverse = 1 << 0,
        Smoothstep = 1 << 1,
        PerlinNoise = 1 << 2,
        R16Unorm = 1 << 3,
    }
```

Example texture creation:

```lua
    -- Example usage (sprite/material are placeholders for your own objects).
    ---@diagnostic disable: undefined-global, param-type-mismatch
    texture = texturehelper.CreateGradientTexture(
        GradientType.Circular, -- gradient type
        256, 256, -- resolution of the texture
        -- start and end UV coordinates set the gradient direction and extent:
        Vector(0.5, 0.5), Vector(0.5, 0),
        -- modifier flags (bitwise combination):
        GradientFlags.Inverse | GradientFlags.Smoothstep
            | GradientFlags.PerlinNoise,
        -- per channel, one of: 0, 1, r, g, b, a, x, y, z, w (lower or upper
        -- case):
        "rrr1",
        2, -- perlin noise scale
        123, -- perlin noise seed
        6, -- perlin noise octaves
        0.8 -- perlin noise persistence
    )
    texture.Save("gradient.png") -- save it to a file
    sprite.SetTexture(texture) -- assign it to a sprite
    material.SetTexture(TextureSlot.BASECOLORMAP, texture) -- to a material
```
