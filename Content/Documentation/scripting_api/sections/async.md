# Async

[← Back to Scripting API index](../index.md)

```lua
    --- Constructs a new Async tracker object.
    ---
    ---@return Async
    function Async() end

    --- The Async object can be used for tracking or waiting for completion of
    --- functions running on background threads.
    ---
    ---@class Async
    local Async = {}

    --- Wait for completion of async tasks on this tracker.
    function Async.Wait() end

    --- Checks if all async tasks on this tracker have been completed.
    ---
    ---@return boolean
    function Async.IsCompleted() end
```
