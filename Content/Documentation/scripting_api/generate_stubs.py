#!/usr/bin/env python3
"""generate_stubs.py

Generate LuaLS (lua-language-server / EmmyLua) definition stubs for the Wicked
Engine scripting API from this documentation.

The documentation under `sections/` is written as annotated Lua inside ```lua
fenced code blocks, so generating the stubs is just a matter of extracting those
blocks: one `sections/<name>.md` file produces one `<name>.lua` stub file.

Usage
-----
    python3 generate_stubs.py            # writes to ./library/wicked_engine/
    python3 generate_stubs.py <outdir>   # writes to <outdir>/

Copy the resulting `wicked_engine/` folder into your Lua project and point your
`.luarc.json` at it (see README.md). Requires only Python 3 (no dependencies).

Notes
-----
* Fenced blocks that contain a `---@diagnostic disable` line are treated as
  illustrative usage examples and are skipped (they are not API definitions).
* The generated files are derived artifacts; the Markdown under `sections/` is
  the source of truth. They are intentionally not committed (see .gitignore).
"""
import glob
import os
import sys
import textwrap

HERE = os.path.dirname(os.path.abspath(__file__))
SECTIONS_DIR = os.path.join(HERE, "sections")


def extract_blocks(md_path):
    """Return the list of API ```lua blocks in a Markdown file (dedented).

    Blocks marked with `---@diagnostic disable` are usage examples and skipped.
    """
    lines = open(md_path, encoding="utf-8").read().split("\n")
    blocks = []
    i = 0
    while i < len(lines):
        if lines[i].strip() == "```lua":
            j = i + 1
            buf = []
            while j < len(lines) and lines[j].strip() != "```":
                buf.append(lines[j])
                j += 1
            block = textwrap.dedent("\n".join(buf)).rstrip()
            if "---@diagnostic disable" not in block:
                blocks.append(block)
            i = j
        i += 1
    return blocks


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        HERE, "library", "wicked_engine")
    os.makedirs(out_dir, exist_ok=True)

    section_files = sorted(glob.glob(os.path.join(SECTIONS_DIR, "*.md")))
    if not section_files:
        print(f"error: no section files found in {SECTIONS_DIR}", file=sys.stderr)
        sys.exit(1)

    count = 0
    for md in section_files:
        name = os.path.splitext(os.path.basename(md))[0]
        blocks = extract_blocks(md)
        if not blocks:
            continue
        content = "---@meta\n\n" + "\n\n".join(blocks) + "\n"
        with open(os.path.join(out_dir, name + ".lua"), "w", encoding="utf-8") as f:
            f.write(content)
        count += 1
        print(f"  wrote {name}.lua")

    print(f"Done. Generated {count} stub files in {out_dir}")


if __name__ == "__main__":
    main()
