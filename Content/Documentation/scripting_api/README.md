# Wicked Engine Scripting API

Reference documentation for the Lua scripting API, plus a small generator that
turns it into editor IntelliSense.

- **[Read the API reference →](index.md)** — intro, conventions and the full
  table of contents.

## Folder structure

```text
scripting_api/
  index.md            Entry point: introduction + table of contents
  sections/*.md       The API reference, one file per topic (the source of truth)
  generate_stubs.py   Generates LuaLS definition stubs from sections/*.md
  library/            Generated stubs (created by the script; git-ignored)
```

The API in `sections/*.md` is written as annotated Lua inside fenced `lua` code
blocks, using [LuaLS](https://luals.github.io/) (EmmyLua) annotations. Because
the documentation *is* valid annotated Lua, it doubles as the source for editor
autocompletion and type checking.

## Editor support (autocomplete & type checking)

With the [Lua Language Server](https://luals.github.io/) (e.g. the *sumneko.lua*
VS Code extension) you can get autocomplete, signatures and type checking for
the whole scripting API in your own Lua project.

### 1. Generate the stubs

Requires only Python 3 (no dependencies):

```sh
cd Content/Documentation/scripting_api
python3 generate_stubs.py
```

This extracts every fenced `lua` block from `sections/*.md` into one `.lua` file
per section under `library/wicked_engine/` (one section → one stub file).

### 2. Install them into your project

Copy the generated `wicked_engine/` folder into your project, for example into a
`library/` directory:

```text
your_project/
  .luarc.json
  library/
    wicked_engine/      <- the generated folder
  your_scripts.lua
```

Re-run the generator and copy again whenever the documentation changes.

### 3. Configure `.luarc.json`

Create a `.luarc.json` at the root of your project pointing the language server
at the stub folder:

```json
{
  "$schema": "https://raw.githubusercontent.com/LuaLS/vscode-lua/master/setting/schema.json",
  "workspace": {
    "library": ["library/wicked_engine"],
    "checkThirdParty": false
  },
  "runtime": {
    "version": "Lua 5.4"
  }
}
```

`workspace.library` is the only required entry — it makes the language server
load the stubs. With it, the whole engine scripting API is recognized: the
stubs declare the engine global objects (`vector`, `matrix`, `application`,
`main`, `audio`, `input`, `physics`, `texturehelper`) and every engine global
function, constant and enum, so you don't need to list any of those.

If your own project uses globals that aren't defined in any of your `.lua`
files, add them under `diagnostics.globals` so they aren't reported as
undefined, for example:

```json
  "diagnostics": {
    "globals": [ "my_custom_global", "MY_CONSTANT" ]
  }
```

(A global your code *assigns* in a workspace file — e.g. `scene = GetScene()` —
is already known and does not need to be listed.)

## Editing the documentation

`sections/*.md` is the source of truth; the stubs are derived from it.

- Document each binding as annotated Lua in a fenced `lua` block (see the
  conventions in [index.md](index.md#reading-this-documentation)).
- A new `sections/<name>.md` file becomes a `<name>.lua` stub automatically.
- Mark illustrative *usage* snippets (not API definitions) with a
  `---@diagnostic disable: ...` line as the first line of the block; the
  generator skips those so they don't end up in the stubs.
- After editing, re-run `generate_stubs.py` and, ideally, validate with
  `lua-language-server --check library/wicked_engine`.
