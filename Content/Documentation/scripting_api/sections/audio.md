# Audio

[← Back to Scripting API index](../index.md)

```lua
    --- Creates an Audio object. Use the global `audio` instead of constructing
    --- one.
    ---
    ---@return Audio
    function Audio() end

    --- Loads and plays an audio files.
    ---
    ---@class Audio
    local Audio = {}

    --- The audio device.
    ---
    ---@type Audio
    audio = nil

    --- Creates a sound file, returns true if successful, false otherwise.
    ---
    ---@param filename string
    ---@param sound Sound
    ---
    ---@return boolean
    function Audio.CreateSound(filename, sound) end

    --- Creates a sound instance that can be replayed, returns true if
    --- successful, false otherwise.
    ---
    ---@param sound Sound
    ---@param soundinstance SoundInstance
    ---
    ---@return boolean
    function Audio.CreateSoundInstance(sound, soundinstance) end

    --- Plays the audio.
    ---
    ---@param soundinstance SoundInstance
    function Audio.Play(soundinstance) end

    --- Pauses the audio.
    ---
    ---@param soundinstance SoundInstance
    function Audio.Pause(soundinstance) end

    --- Stops the audio.
    ---
    ---@param soundinstance SoundInstance
    function Audio.Stop(soundinstance) end

    --- Returns the volume of a `soundinstance`. If `soundinstance` is not
    --- provided, returns the master volume.
    ---
    ---@param soundinstance? SoundInstance
    ---
    ---@return number
    function Audio.GetVolume(soundinstance) end

    --- Sets the volume of a `soundinstance`. If `soundinstance` is not
    --- provided, sets the master volume.
    ---
    ---@param volume number
    ---@param soundinstance? SoundInstance
    function Audio.SetVolume(volume, soundinstance) end

    --- Disable looping. By default, sound instances are looped when created.
    ---
    ---@param soundinstance SoundInstance
    function Audio.ExitLoop(soundinstance) end

    --- Returns the volume of the submix group.
    ---
    ---@param submixtype integer
    ---
    ---@return number
    function Audio.GetSubmixVolume(submixtype) end

    --- Sets the volume for a submix group.
    ---
    ---@param submixtype integer
    ---@param volume number
    function Audio.SetSubmixVolume(submixtype, volume) end

    --- Adds 3D effect to the sound instance.
    ---
    ---@param soundinstance SoundInstance
    ---@param instance3D SoundInstance3D
    function Audio.Update3D(soundinstance, instance3D) end

    --- Sets an environment effect for reverb globally. Refer to Reverb Types
    --- section for acceptable input values.
    ---
    ---@param reverbtype integer
    function Audio.SetReverb(reverbtype) end
```

## Sound

```lua
    --- Creates a sound. With a filename it loads the sound from file; with no
    --- argument it creates an empty sound.
    ---
    ---@param name? string
    ---
    ---@return Sound
    function Sound(name) end

    --- An audio file. Can be instanced several times via SoundInstance.
    ---
    ---@class Sound
    local Sound = {}

    --- Returns whether the sound was created successfully.
    ---
    ---@return boolean
    function Sound.IsValid() end
```

## SoundInstance

```lua
    --- Creates a sound instance. With a sound it is created from that sound
    --- (with optional begin offset and length in seconds); with no argument it
    --- creates an empty instance.
    ---
    ---@param sound? Sound
    ---@param begin? number
    ---@param length? number
    ---
    ---@return SoundInstance
    function SoundInstance(sound, begin, length) end

    --- An audio file instance that can be played. Note: after modifying
    --- parameters of the SoundInstance, the SoundInstance will need to be
    --- recreated from a specified sound.
    ---
    ---@class SoundInstance
    local SoundInstance = {}

    --- Set a submix type group (default is SUBMIX_TYPE_SOUNDEFFECT).
    ---
    ---@param submixtype integer
    function SoundInstance.SetSubmixType(submixtype) end

    --- Beginning of the playback in seconds, relative to the Sound it will be
    --- created from (0 = from beginning).
    ---
    ---@param seconds number
    function SoundInstance.SetBegin(seconds) end

    --- Length in seconds (0 = until end).
    ---
    ---@param seconds number
    function SoundInstance.SetLength(seconds) end

    --- Loop region begin in seconds, relative to the instance begin time (0 =
    --- from beginning).
    ---
    ---@param seconds number
    function SoundInstance.SetLoopBegin(seconds) end

    --- Loop region length in seconds (0 = until the end).
    ---
    ---@param seconds number
    function SoundInstance.SetLoopLength(seconds) end

    --- Enable/disable reverb for the sound instance.
    ---
    ---@param value boolean
    function SoundInstance.SetEnableReverb(value) end

    --- Enable/disable looping for the sound instance.
    ---
    ---@param value boolean
    function SoundInstance.SetLooped(value) end

    --- Returns the submix type.
    ---
    ---@return integer
    function SoundInstance.GetSubmixType() end

    --- Returns the begin.
    ---
    ---@return number
    function SoundInstance.GetBegin() end

    --- Returns the length.
    ---
    ---@return number
    function SoundInstance.GetLength() end

    --- Returns the loop begin.
    ---
    ---@return number
    function SoundInstance.GetLoopBegin() end

    --- Returns the loop length.
    ---
    ---@return number
    function SoundInstance.GetLoopLength() end

    --- Returns whether reverb is enabled.
    ---
    ---@return boolean
    function SoundInstance.IsEnableReverb() end

    --- Returns whether looped is enabled.
    ---
    ---@return boolean
    function SoundInstance.IsLooped() end

    --- Returns whether the sound instance was created successfully.
    ---
    ---@return boolean
    function SoundInstance.IsValid() end
```

## SoundInstance3D

```lua
    --- Creates the 3D relation object. By default, the listener and emitter are
    --- on the same position, and that disables the 3D effect.
    ---
    ---@return SoundInstance3D
    function SoundInstance3D() end

    --- Describes the relation between a sound instance and a listener in a 3D
    --- world.
    ---
    ---@class SoundInstance3D
    local SoundInstance3D = {}

    --- Sets the listener position.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerPos(value) end

    --- Sets the listener up.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerUp(value) end

    --- Sets the listener front.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerFront(value) end

    --- Sets the listener velocity.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerVelocity(value) end

    --- Sets the emitter position.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterPos(value) end

    --- Sets the emitter up.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterUp(value) end

    --- Sets the emitter front.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterFront(value) end

    --- Sets the emitter velocity.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterVelocity(value) end

    --- Sets the emitter radius.
    ---
    ---@param radius number
    function SoundInstance3D.SetEmitterRadius(radius) end
```

## Submix Types

The submix types group sound instances together to be controlled together.

```lua
    --- Sound effect group.
    ---
    ---@type integer
    SUBMIX_TYPE_SOUNDEFFECT = 0

    --- Music group.
    ---
    ---@type integer
    SUBMIX_TYPE_MUSIC = 1

    --- User submix group.
    ---
    ---@type integer
    SUBMIX_TYPE_USER0 = 2

    --- uUer submix group.
    ---
    ---@type integer
    SUBMIX_TYPE_USER1 = 3
```

## Reverb Types

The reverb types are built in presets that can mimic a specific kind of
environment.

```lua
    --- REVERB PRESET DEFAULT.
    ---
    ---@type integer
    REVERB_PRESET_DEFAULT = 0

    --- REVERB PRESET GENERIC.
    ---
    ---@type integer
    REVERB_PRESET_GENERIC = 1

    --- REVERB PRESET FOREST.
    ---
    ---@type integer
    REVERB_PRESET_FOREST = 2

    --- REVERB PRESET PADDEDCELL.
    ---
    ---@type integer
    REVERB_PRESET_PADDEDCELL = 3

    --- REVERB PRESET ROOM.
    ---
    ---@type integer
    REVERB_PRESET_ROOM = 4

    --- REVERB PRESET BATHROOM.
    ---
    ---@type integer
    REVERB_PRESET_BATHROOM = 5

    --- REVERB PRESET LIVINGROOM.
    ---
    ---@type integer
    REVERB_PRESET_LIVINGROOM = 6

    --- REVERB PRESET STONEROOM.
    ---
    ---@type integer
    REVERB_PRESET_STONEROOM = 7

    --- REVERB PRESET AUDITORIUM.
    ---
    ---@type integer
    REVERB_PRESET_AUDITORIUM = 8

    --- REVERB PRESET CONCERTHALL.
    ---
    ---@type integer
    REVERB_PRESET_CONCERTHALL = 9

    --- REVERB PRESET CAVE.
    ---
    ---@type integer
    REVERB_PRESET_CAVE = 10

    --- REVERB PRESET ARENA.
    ---
    ---@type integer
    REVERB_PRESET_ARENA = 11

    --- REVERB PRESET HANGAR.
    ---
    ---@type integer
    REVERB_PRESET_HANGAR = 12

    --- REVERB PRESET CARPETEDHALLWAY.
    ---
    ---@type integer
    REVERB_PRESET_CARPETEDHALLWAY = 13

    --- REVERB PRESET HALLWAY.
    ---
    ---@type integer
    REVERB_PRESET_HALLWAY = 14

    --- REVERB PRESET STONECORRIDOR.
    ---
    ---@type integer
    REVERB_PRESET_STONECORRIDOR = 15

    --- REVERB PRESET ALLEY.
    ---
    ---@type integer
    REVERB_PRESET_ALLEY = 16

    --- REVERB PRESET CITY.
    ---
    ---@type integer
    REVERB_PRESET_CITY = 17

    --- REVERB PRESET MOUNTAINS.
    ---
    ---@type integer
    REVERB_PRESET_MOUNTAINS = 18

    --- REVERB PRESET QUARRY.
    ---
    ---@type integer
    REVERB_PRESET_QUARRY = 19

    --- REVERB PRESET PLAIN.
    ---
    ---@type integer
    REVERB_PRESET_PLAIN = 20

    --- REVERB PRESET PARKINGLOT.
    ---
    ---@type integer
    REVERB_PRESET_PARKINGLOT = 21

    --- REVERB PRESET SEWERPIPE.
    ---
    ---@type integer
    REVERB_PRESET_SEWERPIPE = 22

    --- REVERB PRESET UNDERWATER.
    ---
    ---@type integer
    REVERB_PRESET_UNDERWATER = 23

    --- REVERB PRESET SMALLROOM.
    ---
    ---@type integer
    REVERB_PRESET_SMALLROOM = 24

    --- REVERB PRESET MEDIUMROOM.
    ---
    ---@type integer
    REVERB_PRESET_MEDIUMROOM = 25

    --- REVERB PRESET LARGEROOM.
    ---
    ---@type integer
    REVERB_PRESET_LARGEROOM = 26

    --- REVERB PRESET MEDIUMHALL.
    ---
    ---@type integer
    REVERB_PRESET_MEDIUMHALL = 27

    --- REVERB PRESET LARGEHALL.
    ---
    ---@type integer
    REVERB_PRESET_LARGEHALL = 28

    --- REVERB PRESET PLATE.
    ---
    ---@type integer
    REVERB_PRESET_PLATE = 29
```
