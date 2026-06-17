# Sprites and Fonts

[← Back to Scripting API index](../index.md)

## Sprite

```lua
    --- Constructs a sprite, optionally from a texture and a mask texture file.
    ---
    ---@param texture? string
    ---@param maskTexture? string
    ---
    ---@return Sprite
    function Sprite(texture, maskTexture) end

    --- Render images on the screen.
    ---
    ---@class Sprite
    ---
    ---@field Params ImageParams
    ---@field Anim SpriteAnim
    local Sprite = {}

    --- Sets the rendering parameters (position, size, color, …) for the sprite.
    ---
    ---@param effects ImageParams
    function Sprite.SetParams(effects) end

    --- Returns the sprite's rendering parameters.
    ---
    ---@return ImageParams
    function Sprite.GetParams() end

    --- Sets the animation helper that drives this sprite.
    ---
    ---@param anim SpriteAnim
    function Sprite.SetAnim(anim) end

    --- Returns the sprite's animation helper.
    ---
    ---@return SpriteAnim
    function Sprite.GetAnim() end

    --- Sets the base color texture.
    ---
    ---@param texture Texture
    function Sprite.SetTexture(texture) end

    --- Returns the base color texture.
    ---
    ---@return Texture
    function Sprite.GetTexture() end

    --- Sets the mask texture (multiplied with the base color).
    ---
    ---@param texture Texture
    function Sprite.SetMaskTexture(texture) end

    --- Returns the mask texture.
    ---
    ---@return Texture
    function Sprite.GetMaskTexture() end

    --- Sets the background texture (used by the background effect).
    ---
    ---@param texture Texture
    function Sprite.SetBackgroundTexture(texture) end

    --- Returns the background texture.
    ---
    ---@return Texture
    function Sprite.GetBackgroundTexture() end

    --- Shows or hides the sprite.
    ---
    ---@param value boolean
    function Sprite.SetHidden(value) end

    --- Returns whether the sprite is hidden.
    ---
    ---@return boolean
    function Sprite.IsHidden() end
```

### ImageParams

Specify Sprite properties, like position, size, color and effects.

```lua
    --- Constructs image parameters. Called with two numbers they are the width
    --- and height (at position 0, 0); called with three or four they are posX,
    --- posY, width and height. Defaults: pos (0, 0), size (100, 100).
    ---
    ---@param posX number
    ---@param posY number
    ---@param width number
    ---@param height? number
    ---
    ---@return ImageParams
    ---
    ---@overload fun(width: number, height?: number): ImageParams
    function ImageParams(posX, posY, width, height) end

    ---@class ImageParams
    ---@field Pos Vector
    ---@field Size Vector
    ---@field Pivot Vector
    ---@field Color Vector
    ---@field Opacity number
    ---@field Fade number
    ---@field Rotation number
    ---@field TexOffset Vector
    ---@field TexOffset2 Vector
    local ImageParams = {}

    --- Returns the position.
    ---
    ---@return Vector
    function ImageParams.GetPos() end

    --- Returns the size.
    ---
    ---@return Vector
    function ImageParams.GetSize() end

    --- Returns the pivot point (rotation/scaling origin, in [0, 1] of the
    --- size).
    ---
    ---@return Vector
    function ImageParams.GetPivot() end

    --- Returns the color/tint (RGBA).
    ---
    ---@return Vector
    function ImageParams.GetColor() end

    --- Returns the opacity (alpha multiplier).
    ---
    ---@return number
    function ImageParams.GetOpacity() end

    --- Returns the saturation.
    ---
    ---@return number
    function ImageParams.GetSaturation() end

    --- Returns the fade amount (0: visible, 1: faded out).
    ---
    ---@return number
    function ImageParams.GetFade() end

    --- Returns the rotation in radians.
    ---
    ---@return number
    function ImageParams.GetRotation() end

    --- Returns the texture UV offset.
    ---
    ---@return Vector
    function ImageParams.GetTexOffset() end

    --- Returns the secondary texture UV offset.
    ---
    ---@return Vector
    function ImageParams.GetTexOffset2() end

    --- Returns the border soften amount.
    ---
    ---@return number
    function ImageParams.GetBorderSoften() end

    --- Returns the draw rectangle (x, y, width, height) in texture pixels.
    ---
    ---@return Vector
    function ImageParams.GetDrawRect() end

    --- Returns the secondary draw rectangle (x, y, width, height).
    ---
    ---@return Vector
    function ImageParams.GetDrawRect2() end

    --- Returns whether the draw rectangle is enabled.
    ---
    ---@return boolean
    function ImageParams.IsDrawRectEnabled() end

    --- Returns whether the secondary draw rectangle is enabled.
    ---
    ---@return boolean
    function ImageParams.IsDrawRect2Enabled() end

    --- Returns whether horizontal mirroring is enabled.
    ---
    ---@return boolean
    function ImageParams.IsMirrorEnabled() end

    --- Returns whether the background blur is enabled.
    ---
    ---@return boolean
    function ImageParams.IsBackgroundBlurEnabled() end

    --- Returns whether the background is enabled.
    ---
    ---@return boolean
    function ImageParams.IsBackgroundEnabled() end

    --- Returns whether the distortion mask is enabled.
    ---
    ---@return boolean
    function ImageParams.IsDistortionMaskEnabled() end

    --- Sets the position.
    ---
    ---@param pos Vector
    function ImageParams.SetPos(pos) end

    --- Sets the size.
    ---
    ---@param size Vector
    function ImageParams.SetSize(size) end

    --- Sets the pivot point (rotation/scaling origin, in [0, 1] of the size).
    ---
    ---@param value Vector
    function ImageParams.SetPivot(value) end

    --- Sets the color/tint (RGBA).
    ---
    ---@param value Vector
    function ImageParams.SetColor(value) end

    --- Sets the opacity (alpha multiplier).
    ---
    ---@param opacity number
    function ImageParams.SetOpacity(opacity) end

    --- Sets the saturation.
    ---
    ---@param saturation number
    function ImageParams.SetSaturation(saturation) end

    --- Sets the fade amount (0: visible, 1: faded out).
    ---
    ---@param fade number
    function ImageParams.SetFade(fade) end

    --- Sets the stencil test mode and reference value.
    ---
    ---@param stencilMode integer one of the STENCILMODE_* constants
    ---@param stencilRef integer
    function ImageParams.SetStencil(stencilMode, stencilRef) end

    --- Sets how the stencil reference value is interpreted.
    ---
    ---@param stencilRefMode integer one of the STENCILREFMODE_* constants
    function ImageParams.SetStencilRefMode(stencilRefMode) end

    --- Sets the blend mode (one of the BLENDMODE_* constants).
    ---
    ---@param blendMode integer
    function ImageParams.SetBlendMode(blendMode) end

    --- Sets the sampling quality (one of the QUALITY_* constants).
    ---
    ---@param quality integer
    function ImageParams.SetQuality(quality) end

    --- Sets the texture addressing mode (one of the SAMPLEMODE_* constants).
    ---
    ---@param sampleMode integer
    function ImageParams.SetSampleMode(sampleMode) end

    --- Sets the rotation in radians.
    ---
    ---@param rotation number
    function ImageParams.SetRotation(rotation) end

    --- Sets the texture UV offset.
    ---
    ---@param value Vector
    function ImageParams.SetTexOffset(value) end

    --- Sets the secondary texture UV offset.
    ---
    ---@param value Vector
    function ImageParams.SetTexOffset2(value) end

    --- Sets the border soften amount.
    ---
    ---@param alpha number
    function ImageParams.SetBorderSoften(alpha) end

    --- Enables the draw rectangle (x, y, width, height) in texture pixels.
    ---
    ---@param value Vector
    function ImageParams.EnableDrawRect(value) end

    --- Enables the secondary draw rectangle (x, y, width, height).
    ---
    ---@param value Vector
    function ImageParams.EnableDrawRect2(value) end

    --- Disables the draw rectangle.
    function ImageParams.DisableDrawRect() end

    --- Disables the secondary draw rectangle.
    function ImageParams.DisableDrawRect2() end

    --- Enables horizontal mirroring.
    function ImageParams.EnableMirror() end

    --- Disables horizontal mirroring.
    function ImageParams.DisableMirror() end

    --- Enables the background blur effect.
    function ImageParams.EnableBackgroundBlur() end

    --- Disables the background blur effect.
    function ImageParams.DisableBackgroundBlur() end

    --- Enables drawing the background.
    function ImageParams.EnableBackground() end

    --- Disables drawing the background.
    function ImageParams.DisableBackground() end

    --- Enables using the mask texture as a distortion mask.
    function ImageParams.EnableDistortionMask() end

    --- Disables the distortion mask.
    function ImageParams.DisableDistortionMask() end

    --- Sets the alpha range used when masking (start and end thresholds).
    ---
    ---@param start number
    ---@param end_ number
    function ImageParams.SetMaskAlphaRange(start, end_) end

    --- Returns the mask alpha range as start, end.
    ---
    ---@return number range_start
    ---@return number range_end
    function ImageParams.GetMaskAlphaRange() end

    --- Sets the direction of the angular softness effect.
    ---
    ---@param value Vector
    function ImageParams.SetAngularSoftnessDirection(value) end

    --- Sets the inner angle of the angular softness effect.
    ---
    ---@param value number
    function ImageParams.SetAngularSoftnessInnerAngle(value) end

    --- Sets the outer angle of the angular softness effect.
    ---
    ---@param value number
    function ImageParams.SetAngularSoftnessOuterAngle(value) end

    --- Enables double-sided angular softness.
    function ImageParams.EnableAngularSoftnessDoubleSided() end

    --- Inverts the angular softness effect.
    function ImageParams.EnableAngularSoftnessInverse() end

    --- Disables double-sided angular softness.
    function ImageParams.DisableAngularSoftnessDoubleSided() end

    --- Disables inverted angular softness.
    function ImageParams.DisableAngularSoftnessInverse() end

    --- Enables rounded corners.
    function ImageParams.EnableCornerRounding() end

    --- Disables rounded corners.
    function ImageParams.DisableCornerRounding() end

    --- Sets the rounding of one corner (0-3) with a radius and segment count.
    ---
    ---@param corner integer corner index in [0, 3]
    ---@param rounding number corner radius
    ---@param segments? integer subdivision count (default 18)
    function ImageParams.SetCornerRounding(corner, rounding, segments) end

    --- Fills the image with a gradient between two UV coordinates.
    ---
    ---@param type ImageGradientType
    ---@param uv_start Vector
    ---@param uv_end Vector
    ---@param color Vector
    function ImageParams.SetGradient(type, uv_start, uv_end, color) end
```

The following constants and enum are used by the ImageParams setters:

```lua
    -- Stencil comparison modes (use with SetStencil).
    STENCILMODE_DISABLED = 0
    STENCILMODE_EQUAL = 1
    STENCILMODE_LESS = 2
    STENCILMODE_LESSEQUAL = 3
    STENCILMODE_GREATER = 4
    STENCILMODE_GREATEREQUAL = 5
    STENCILMODE_NOT = 6
    STENCILMODE_ALWAYS = 7

    -- How the stencil reference value is interpreted (use with
    -- SetStencilRefMode).
    STENCILREFMODE_ENGINE = 0
    STENCILREFMODE_USER = 1
    STENCILREFMODE_ALL = 2

    -- Texture addressing modes (use with SetSampleMode).
    SAMPLEMODE_CLAMP = 0
    SAMPLEMODE_WRAP = 1
    SAMPLEMODE_MIRROR = 2

    -- Sampling quality (use with SetQuality).
    QUALITY_NEAREST = 0
    QUALITY_LINEAR = 1
    QUALITY_ANISOTROPIC = 2
    QUALITY_BICUBIC = 3

    -- Blend modes (use with SetBlendMode).
    BLENDMODE_OPAQUE = 0
    BLENDMODE_ALPHA = 1
    BLENDMODE_PREMULTIPLIED = 2
    BLENDMODE_ADDITIVE = 3

    ---@enum ImageGradientType
    ImageGradientType = {
        None = 0,
        Linear = 1,
        LinearReflected = 2,
        Circular = 3,
    }
```

### SpriteAnim

```lua
    --- Constructs a sprite animation helper.
    ---
    ---@return SpriteAnim
    function SpriteAnim() end

    --- Animate Sprites easily with this helper.
    ---
    ---@class SpriteAnim
    ---
    ---@field Rot number
    ---@field Rotation number
    ---@field Opacity number
    ---@field Fade number
    ---@field Repeatable boolean
    ---@field Velocity Vector
    ---@field ScaleX number
    ---@field ScaleY number
    ---@field MovingTexAnim MovingTexAnim
    ---@field DrawRecAnim DrawRectAnim
    local SpriteAnim = {}

    --- Sets the per-second rotation offset used by the wobble effect.
    ---
    ---@param val number
    function SpriteAnim.SetRot(val) end

    --- Sets the per-second rotation speed.
    ---
    ---@param val number
    function SpriteAnim.SetRotation(val) end

    --- Sets the per-second opacity change.
    ---
    ---@param val number
    function SpriteAnim.SetOpacity(val) end

    --- Sets the per-second fade change.
    ---
    ---@param val number
    function SpriteAnim.SetFade(val) end

    --- Sets the wobble animation amount.
    ---
    ---@param val number
    function SpriteAnim.SetWobbleAnimAmount(val) end

    --- Sets the wobble animation speed.
    ---
    ---@param val number
    function SpriteAnim.SetWobbleAnimSpeed(val) end

    --- Sets whether the animation repeats.
    ---
    ---@param val boolean
    function SpriteAnim.SetRepeatable(val) end

    --- Sets the per-second position velocity.
    ---
    ---@param val Vector
    function SpriteAnim.SetVelocity(val) end

    --- Sets the per-second horizontal scale change.
    ---
    ---@param val number
    function SpriteAnim.SetScaleX(val) end

    --- Sets the per-second vertical scale change.
    ---
    ---@param val number
    function SpriteAnim.SetScaleY(val) end

    --- Sets the moving-texture sub-animation.
    ---
    ---@param val MovingTexAnim
    function SpriteAnim.SetMovingTexAnim(val) end

    --- Sets the frame-by-frame draw-rectangle sub-animation.
    ---
    ---@param val DrawRectAnim
    function SpriteAnim.SetDrawRecAnim(val) end

    --- Returns the per-second rotation offset.
    ---
    ---@return number
    function SpriteAnim.GetRot() end

    --- Returns the per-second rotation speed.
    ---
    ---@return number
    function SpriteAnim.GetRotation() end

    --- Returns the per-second opacity change.
    ---
    ---@return number
    function SpriteAnim.GetOpacity() end

    --- Returns the per-second fade change.
    ---
    ---@return number
    function SpriteAnim.GetFade() end

    --- Returns whether the animation repeats.
    ---
    ---@return boolean
    function SpriteAnim.GetRepeatable() end

    --- Returns the per-second position velocity.
    ---
    ---@return Vector
    function SpriteAnim.GetVelocity() end

    --- Returns the per-second horizontal scale change.
    ---
    ---@return number
    function SpriteAnim.GetScaleX() end

    --- Returns the per-second vertical scale change.
    ---
    ---@return number
    function SpriteAnim.GetScaleY() end

    --- Returns the moving-texture sub-animation.
    ---
    ---@return MovingTexAnim
    function SpriteAnim.GetMovingTexAnim() end

    --- Returns the frame-by-frame draw-rectangle sub-animation.
    ---
    ---@return DrawRectAnim
    function SpriteAnim.GetDrawRecAnim() end
```

### MovingTexAnim

```lua
    --- Constructs a moving-texture animation, optionally with X and Y speeds.
    ---
    ---@param speedX? number
    ---@param speedY? number
    ---
    ---@return MovingTexAnim
    function MovingTexAnim(speedX, speedY) end

    --- Scroll a sprite's texture continuously.
    ---
    ---@class MovingTexAnim
    ---
    ---@field SpeedX number
    ---@field SpeedY number
    local MovingTexAnim = {}

    --- Sets the horizontal scroll speed.
    ---
    ---@param val number
    function MovingTexAnim.SetSpeedX(val) end

    --- Sets the vertical scroll speed.
    ---
    ---@param val number
    function MovingTexAnim.SetSpeedY(val) end

    --- Returns the horizontal scroll speed.
    ---
    ---@return number
    function MovingTexAnim.GetSpeedX() end

    --- Returns the vertical scroll speed.
    ---
    ---@return number
    function MovingTexAnim.GetSpeedY() end
```

### DrawRectAnim

```lua
    --- Constructs a frame-by-frame draw-rectangle animation.
    ---
    ---@return DrawRectAnim
    function DrawRectAnim() end

    --- Animate a sprite frame by frame from a sprite sheet.
    ---
    ---@class DrawRectAnim
    ---
    ---@field FrameRate number
    ---@field FrameCount integer
    ---@field HorizontalFrameCount integer
    local DrawRectAnim = {}

    --- Sets the playback frame rate (frames per second).
    ---
    ---@param val number
    function DrawRectAnim.SetFrameRate(val) end

    --- Sets the total number of frames.
    ---
    ---@param val integer
    function DrawRectAnim.SetFrameCount(val) end

    --- Sets the number of frames per row in the sprite sheet.
    ---
    ---@param val integer
    function DrawRectAnim.SetHorizontalFrameCount(val) end

    --- Returns the playback frame rate.
    ---
    ---@return number
    function DrawRectAnim.GetFrameRate() end

    --- Returns the total number of frames.
    ---
    ---@return integer
    function DrawRectAnim.GetFrameCount() end

    --- Returns the number of frames per row.
    ---
    ---@return integer
    function DrawRectAnim.GetHorizontalFrameCount() end
```

## SpriteFont

```lua
    --- Constructs a sprite font, optionally with initial text.
    ---
    ---@param text? string
    ---
    ---@return SpriteFont
    function SpriteFont(text) end

    --- Render text with a custom font.
    ---
    ---@class SpriteFont
    ---
    ---@field Text string
    ---@field Size integer
    ---@field Pos Vector
    ---@field Spacing Vector
    ---@field Align integer
    ---@field Color Vector
    ---@field ShadowColor Vector
    ---@field Bolden number
    ---@field Softness number
    ---@field ShadowBolden number
    ---@field ShadowSoftness number
    ---@field ShadowOffset Vector
    local SpriteFont = {}

    --- Sets the font style (typeface) by name, with an optional pixel size.
    ---
    ---@param fontstyle string
    ---@param size? integer font size in pixels (default 16)
    function SpriteFont.SetStyle(fontstyle, size) end

    --- Sets the displayed text (empty string if omitted).
    ---
    ---@param text? string
    function SpriteFont.SetText(text) end

    --- Sets the font size in pixels.
    ---
    ---@param size integer
    function SpriteFont.SetSize(size) end

    --- Sets an additional uniform scale applied on top of the size.
    ---
    ---@param scale number
    function SpriteFont.SetScale(scale) end

    --- Sets the screen position.
    ---
    ---@param pos Vector
    function SpriteFont.SetPos(pos) end

    --- Sets the horizontal and vertical spacing between characters and lines.
    ---
    ---@param spacing Vector
    function SpriteFont.SetSpacing(spacing) end

    --- Sets the horizontal and (optional) vertical alignment.
    ---
    ---@param halign integer one of the WIFALIGN_* constants
    ---@param valign? integer one of the WIFALIGN_* constants
    function SpriteFont.SetAlign(halign, valign) end

    --- Sets the text color, as a Vector (RGBA) or a packed hex color code.
    ---
    ---@param color Vector
    ---
    ---@overload fun(colorHexCode: integer)
    function SpriteFont.SetColor(color) end

    --- Sets the shadow color, as a Vector (RGBA) or a packed hex color code.
    ---
    ---@param shadowcolor Vector
    ---
    ---@overload fun(colorHexCode: integer)
    function SpriteFont.SetShadowColor(shadowcolor) end

    --- Sets how much the glyphs are bold.
    ---
    ---@param value number
    function SpriteFont.SetBolden(value) end

    --- Sets the glyph edge softness.
    ---
    ---@param value number
    function SpriteFont.SetSoftness(value) end

    --- Sets how much the shadow glyphs are bold.
    ---
    ---@param value number
    function SpriteFont.SetShadowBolden(value) end

    --- Sets the shadow edge softness.
    ---
    ---@param value number
    function SpriteFont.SetShadowSoftness(value) end

    --- Sets the shadow offset.
    ---
    ---@param value Vector
    function SpriteFont.SetShadowOffset(value) end

    --- Sets the width at which text wraps (<= 0 disables wrapping).
    ---
    ---@param value number
    function SpriteFont.SetHorizontalWrapping(value) end

    --- Shows or hides the text.
    ---
    ---@param value boolean
    function SpriteFont.SetHidden(value) end

    --- Enables flipping the letters horizontally.
    ---
    ---@param value boolean
    function SpriteFont.SetFlippedHorizontally(value) end

    --- Enables flipping the letters vertically.
    ---
    ---@param value boolean
    function SpriteFont.SetFlippedVertically(value) end

    --- Returns the displayed text.
    ---
    ---@return string
    function SpriteFont.GetText() end

    --- Returns the font size in pixels.
    ---
    ---@return integer
    function SpriteFont.GetSize() end

    --- Returns the additional uniform scale.
    ---
    ---@return number
    function SpriteFont.GetScale() end

    --- Returns the screen position.
    ---
    ---@return Vector
    function SpriteFont.GetPos() end

    --- Returns the character and line spacing.
    ---
    ---@return Vector
    function SpriteFont.GetSpacing() end

    --- Returns the horizontal and vertical alignment.
    ---
    ---@return integer halign
    ---@return integer valign
    function SpriteFont.GetAlign() end

    --- Returns the text color.
    ---
    ---@return Vector
    function SpriteFont.GetColor() end

    --- Returns the shadow color.
    ---
    ---@return Vector
    function SpriteFont.GetShadowColor() end

    --- Returns how much the glyphs are bold.
    ---
    ---@return number
    function SpriteFont.GetBolden() end

    --- Returns the glyph edge softness.
    ---
    ---@return number
    function SpriteFont.GetSoftness() end

    --- Returns how much the shadow glyphs are bold.
    ---
    ---@return number
    function SpriteFont.GetShadowBolden() end

    --- Returns the shadow edge softness.
    ---
    ---@return number
    function SpriteFont.GetShadowSoftness() end

    --- Returns the shadow offset.
    ---
    ---@return Vector
    function SpriteFont.GetShadowOffset() end

    --- Returns the wrapping width.
    ---
    ---@return number
    function SpriteFont.GetHorizontalWrapping() end

    --- Returns whether the text is hidden.
    ---
    ---@return boolean
    function SpriteFont.IsHidden() end

    --- Returns whether the letters are flipped horizontally.
    ---
    ---@return boolean
    function SpriteFont.IsFlippedHorizontally() end

    --- Returns whether the letters are flipped vertically.
    ---
    ---@return boolean
    function SpriteFont.IsFlippedVertically() end

    --- Returns the rendered text width and height in a Vector's X and Y.
    ---
    ---@return Vector
    function SpriteFont.TextSize() end

    --- Sets the time to fully type the text, in seconds (0 disables
    --- typewriting).
    ---
    ---@param value number
    function SpriteFont.SetTypewriterTime(value) end

    --- Sets whether the typewriter animation restarts when finished.
    ---
    ---@param value boolean
    function SpriteFont.SetTypewriterLooped(value) end

    --- Sets the character index the typewriter animation starts from.
    ---
    ---@param value integer
    function SpriteFont.SetTypewriterCharacterStart(value) end

    --- Sets a sound effect played as each new letter appears.
    ---
    ---@param sound Sound
    ---@param soundinstance SoundInstance
    function SpriteFont.SetTypewriterSound(sound, soundinstance) end

    --- Resets the typewriter animation to the first character.
    ---
    function SpriteFont.ResetTypewriter() end

    --- Finishes the typewriter animation immediately.
    ---
    function SpriteFont.TypewriterFinish() end

    --- Returns whether the typewriter animation has finished.
    ---
    ---@return boolean
    function SpriteFont.IsTypewriterFinished() end
```

The text alignment constants used by SetAlign and GetAlign:

```lua
    WIFALIGN_LEFT = 0
    WIFALIGN_CENTER = 1
    WIFALIGN_RIGHT = 2
    WIFALIGN_TOP = 3
    WIFALIGN_BOTTOM = 4
```
