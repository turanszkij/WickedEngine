#!/usr/bin/env python3
"""generate_stubs.py

Generate LuaLS (lua-language-server / EmmyLua) definition stubs for the Wicked
Engine scripting API from this documentation.

The reference under `sections/` is written as annotated Lua inside ```lua fenced
code blocks, so generating the stubs is just a matter of extracting those blocks.

Usage
-----
    python3 generate_stubs.py
        Combine every section into one file: ./wicked_engine_bindings.lua

    python3 generate_stubs.py <output.lua>
        Write the combined file to <output.lua>.

    python3 generate_stubs.py --split
        Write one stub per section into ./library/wicked_engine/

    python3 generate_stubs.py --split <outdir>
        Write one stub per section into <outdir>/

Point your `.luarc.json` `workspace.library` at the generated file (or folder)
to get autocomplete and type checking. Requires only Python 3 (no dependencies).

Notes
-----
* Fenced blocks that contain a `---@diagnostic disable` line are treated as
  illustrative usage examples and are skipped (they are not API definitions).
* The generated files are derived artifacts; the Markdown under `sections/` is
  the source of truth. They are intentionally not committed (see .gitignore).
"""
import argparse
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


def section_files():
    """Return the sorted list of section Markdown files (or exit if none)."""
    files = sorted(glob.glob(os.path.join(SECTIONS_DIR, "*.md")))
    if not files:
        print(f"error: no section files found in {SECTIONS_DIR}", file=sys.stderr)
        sys.exit(1)
    return files


def generate_single(out_path):
    """Combine every section's blocks into one `---@meta` definition file."""
    parts = []
    for md in section_files():
        name = os.path.splitext(os.path.basename(md))[0]
        blocks = extract_blocks(md)
        if not blocks:
            continue
        # A searchable banner so Ctrl+F can jump between topics in the one file.
        parts.append(f"-- ===== sections/{name}.md " + "=" * max(0, 50 - len(name)))
        parts.append("\n\n".join(blocks))
    content = "---@meta\n\n" + "\n\n".join(parts) + "\n"
    out_dir = os.path.dirname(os.path.abspath(out_path))
    os.makedirs(out_dir, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Done. Generated {out_path}")


def generate_split(out_dir):
    """Write one `<section>.lua` definition file per section into out_dir."""
    os.makedirs(out_dir, exist_ok=True)
    count = 0
    for md in section_files():
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


def main():
    parser = argparse.ArgumentParser(
        description="Generate LuaLS stubs from the scripting API documentation.")
    parser.add_argument(
        "output", nargs="?",
        help="output file (default mode) or output directory (with --split)")
    parser.add_argument(
        "--split", action="store_true",
        help="generate one stub file per section into a directory instead of "
             "a single combined file")
    args = parser.parse_args()

    if args.split:
        out_dir = args.output or os.path.join(HERE, "library", "wicked_engine")
        generate_split(out_dir)
    else:
        out_path = args.output or os.path.join(HERE, "wicked_engine_bindings.lua")
        generate_single(out_path)


if __name__ == "__main__":
    main()
