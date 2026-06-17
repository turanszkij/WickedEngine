# Utility Tools

[← Back to Scripting API index](../index.md)

This section describes the common tools for scripting which are not necessarily engine features.

These are some helpers to aid in debugging:

Helpers to determine the current platform, if need to implement specific
functionality:

```lua
    --- Send a signal globally. This can wake up processes if there are any
    --- waiting on the same signal name.
    ---
    ---@param name string
    function signal(name) end

    --- Wait until a specified signal arrives.
    ---
    ---@param name string
    function waitSignal(name) end

    --- Start a new process.
    ---
    ---@param func fun()
    ---
    ---@return boolean success
    ---@return thread co
    function runProcess(func) end

    --- Stop and remove all processes.
    function killProcesses() end

    --- Stop and remove a single process, identified by its coroutine.
    ---
    ---@param co thread
    function killProcess(co) end

    --- Stop and remove all processes started from the given script PID.
    ---
    ---@param pid integer
    function killProcessPID(pid) end

    --- Stop and remove all processes started from the given script file.
    ---
    ---@param file string
    function killProcessFile(file) end

    --- Wait until some time has passed (to be used from inside a process).
    ---
    ---@param seconds number
    function waitSeconds(seconds) end

    --- Get reflection data from object.
    ---
    ---@param object table
    function getprops(object) end

    --- Get the length of a table.
    ---
    ---@param object table
    ---
    ---@return integer
    function len(object) end

    --- Post table contents to the backlog.
    ---
    ---@param list table
    function backlog_post_list(list) end

    --- Modifies the logging level.
    ---
    ---@param level LogLevel
    function backlog_setlevel(level) end

    --- Severity levels for backlog/console log messages.
    ---
    ---@enum LogLevel
    LogLevel = {
        None = 0,
        Default = 1,
        Warning = 2,
        Error = 3,
        Success = 4,
    }

    --- Wait for a fixed update step to be called (to be used from inside a
    --- process).
    function fixedupdate() end

    --- Wait for variable update step to be called (to be used from inside a
    --- process).
    function update() end

    --- Wait for a render step to be called (to be used from inside a process).
    function render() end

    --- Returns the delta time in seconds (time passed since previous update()).
    ---
    ---@return number
    function getDeltaTime() end

    --- Linear interpolation.
    ---
    ---@param a number
    ---@param b number
    ---@param t number
    ---
    ---@return number
    function math.lerp(a, b, t) end

    --- Inverse linear interpolation.
    ---
    ---@param a number
    ---@param b number
    ---@param t number
    ---
    ---@return number
    function math.inverse_lerp(a, b, t) end

    --- Inverse linear interpolation.
    ---
    ---@param a number
    ---@param b number
    ---@param t number
    ---
    ---@return number
    function math.inverselerp(a, b, t) end

    --- Clamp x between min and max.
    ---
    ---@param x number
    ---@param min number
    ---@param max number
    ---
    ---@return number
    function math.clamp(x, min, max) end

    --- Clamp x between 0 and 1.
    ---
    ---@param x number
    ---
    ---@return number
    function math.saturate(x) end

    --- Round x to nearest integer.
    ---
    ---@param x number
    ---
    ---@return number
    function math.round(x) end

    --- Executes a binary LUA file.
    ---
    ---@param filename string
    function dobinaryfile(filename) end

    --- Compiles a text LUA file (filename_src) into a binary LUA file
    --- (filename_dst).
    ---
    ---@param filename_src string
    ---@param dilename_dst string
    function compilebinaryfile(filename_src, dilename_dst) end

    --- Returns true if current application is the editor, false otherwise.
    ---
    ---@return boolean
    function IsThisEditor() end

    --- Returns control to the editor and kills running scripts.
    function ReturnToEditor() end

    --- Returns true if this is a debug build, false otherwise.
    ---
    ---@return boolean
    function IsThisDebugBuild() end

    --- Returns whether platform windows.
    ---
    ---@return boolean
    function IsPlatformWindows() end

    --- Returns whether platform linux.
    ---
    ---@return boolean
    function IsPlatformLinux() end

    --- Returns whether platform macos.
    ---
    ---@return boolean
    function IsPlatformMACOS() end

    --- Returns whether platform ios.
    ---
    ---@return boolean
    function IsPlatformIOS() end

    --- Returns whether platform ps5.
    ---
    ---@return boolean
    function IsPlatformPS5() end

    --- Returns whether platform xbox.
    ---
    ---@return boolean
    function IsPlatformXBOX() end

    --- Returns the engine major version number.
    ---
    ---@return integer
    function GetVersionMajor() end

    --- Returns the engine minor version number.
    ---
    ---@return integer
    function GetVersionMinor() end

    --- Returns the engine revision version number.
    ---
    ---@return integer
    function GetVersionRevision() end

    --- Returns the full engine version string.
    ---
    ---@return string
    function GetVersionString() end

    --- Returns the engine credits text.
    ---
    ---@return string
    function GetCreditsString() end

    --- Returns the engine supporters text.
    ---
    ---@return string
    function GetSupportersString() end

    --- Returns the file path of the currently running script
    --- (empty string if not available).
    ---
    ---@return string
    function script_file() end

    --- Returns the directory of the currently running script
    --- (empty string if not available).
    ---
    ---@return string
    function script_dir() end

    --- Returns the process id (PID) of the currently running script
    --- (0 if not available).
    ---
    ---@return integer
    function script_pid() end
```
