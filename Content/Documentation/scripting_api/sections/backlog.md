# BackLog

[← Back to Scripting API index](../index.md)

The BackLog is the engine's scripting console: type text with the keyboard and
run it with the RETURN key; script errors are also displayed here. These
functions are in the global scope and manipulate the BackLog.

```lua
    --- Removes all entries from the backlog.
    function backlog_clear() end

    --- Posts a string to the backlog.
    ---
    ---@param params string
    function backlog_post(params) end

    --- Sets the font size of the backlog.
    ---
    ---@param size integer
    function backlog_fontsize(size) end

    --- Returns whether the backlog is currently active (visible).
    ---
    ---@return boolean
    function backlog_isactive() end

    --- Sets the row spacing of the backlog text.
    ---
    ---@param spacing integer
    function backlog_fontrowspacing(spacing) end

    --- Opens (shows) the backlog console.
    function backlog_open() end

    --- Disables and locks the backlog so the HOME key won't bring it up.
    function backlog_lock() end

    --- Unlocks the backlog so it can be toggled with the HOME key.
    function backlog_unlock() end

    --- Disables Lua code execution from the backlog.
    function backlog_blocklua() end

    --- Re-enables Lua code execution from the backlog.
    function backlog_unblocklua() end

    --- Flushes pending backlog messages immediately.
    function backlog_flush() end

    --- Sets the interval (in milliseconds) at which the backlog auto-flushes
    --- pending messages.
    ---
    ---@param milliseconds integer
    function backlog_setautoflushinterval(milliseconds) end
```
