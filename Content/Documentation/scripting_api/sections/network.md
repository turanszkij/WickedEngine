# Network

[← Back to Scripting API index](../index.md)

Scripting handle for the engine's networking system. It is registered as the
`Network` class and exposed through the global `network` object.

> **Note:** the underlying `wi::network` C++ API (sockets, connections, send /
> receive) is **not** surfaced to Lua yet, so this object currently exposes no
> methods or properties. It is documented here for completeness; the type and
> global exist and can be referenced, but have no callable members until the
> engine binds them.

```lua
    --- Creates a Network object. Use the global `network` instead of
    --- constructing one.
    ---
    ---@return Network
    function Network() end

    --- Networking system handle. Currently a placeholder: the engine registers
    --- the type and the global `network` object, but exposes no scriptable
    --- methods or properties yet.
    ---
    ---@class Network
    local Network = {}

    --- Use this global object to access networking (no methods are bound yet).
    ---
    ---@type Network
    network = nil
```
