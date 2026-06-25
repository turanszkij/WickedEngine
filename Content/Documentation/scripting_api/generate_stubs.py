#!/usr/bin/env python3
"""generate_stubs.py

Generate LuaLS (lua-language-server / EmmyLua) definition stubs for the Wicked
Engine scripting API from this documentation.

The reference in `../ScriptingAPI-Documentation.md` is written as annotated Lua
inside ```lua fenced code blocks, so generating the stubs is just a matter of
extracting those blocks.

Usage
-----
    python3 generate_stubs.py
        Combine every section into one file: ./wicked_engine_bindings.lua

    python3 generate_stubs.py <output.lua>
        Write the combined file to <output.lua>.

    python3 generate_stubs.py --split
        Write one stub per topic into ./library/wicked_engine/

    python3 generate_stubs.py --split <outdir>
        Write one stub per topic into <outdir>/

Point your `.luarc.json` `workspace.library` at the generated file (or folder)
to get autocomplete and type checking. Requires only Python 3 (no dependencies).

Notes
-----
* Only the API reference body is scanned: extraction starts at the first
  "## Utility Tools" heading, so illustrative snippets in the introductory
  sections are never mistaken for bindings.
* Fenced blocks that contain a `---@diagnostic disable` line are treated as
  illustrative usage examples and are skipped (they are not API definitions).
* The generated files are derived artifacts; the Markdown documentation is the
  source of truth. They are intentionally not committed (see .gitignore).
"""
import argparse
import os
import re
import sys
import textwrap

HERE = os.path.dirname(os.path.abspath(__file__))
DOC_FILE = os.path.join(os.path.dirname(HERE), "ScriptingAPI-Documentation.md")

# Extraction begins at this heading; everything before it is introductory prose.
API_START_HEADING = "## Utility Tools"


def _topic_filename(heading_text):
    """A filesystem-safe stub filename derived from a heading."""
    name = heading_text.strip().lower()
    name = re.sub(r"[^a-z0-9]+", "_", name).strip("_")
    return name + ".lua"


def extract_sections():
    """Return [(topic, [blocks])] from the API body, in document order.

    A "topic" is the nearest enclosing `##` or `###` heading. Blocks marked with
    `---@diagnostic disable` are usage examples and are skipped. Headings and
    fenced blocks before `API_START_HEADING` are ignored.
    """
    if not os.path.exists(DOC_FILE):
        print(f"error: documentation not found: {DOC_FILE}", file=sys.stderr)
        sys.exit(1)

    lines = open(DOC_FILE, encoding="utf-8").read().split("\n")
    sections = []          # ordered [(topic, [blocks])]
    index = {}             # topic -> position in `sections`
    topic = None
    started = False
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if not started:
            if stripped == API_START_HEADING:
                started = True
                topic = "Utility Tools"
            else:
                i += 1
                continue

        m = re.match(r"^(#{2,3})\s+(.*)$", line)
        if m:
            topic = m.group(2).strip()

        if stripped == "```lua":
            j = i + 1
            buf = []
            while j < len(lines) and lines[j].strip() != "```":
                buf.append(lines[j])
                j += 1
            block = textwrap.dedent("\n".join(buf)).rstrip()
            if "---@diagnostic disable" not in block:
                if topic not in index:
                    index[topic] = len(sections)
                    sections.append((topic, []))
                sections[index[topic]][1].append(block)
            i = j
        i += 1

    if not sections:
        print(f"error: no API blocks found after '{API_START_HEADING}'",
              file=sys.stderr)
        sys.exit(1)
    return sections


def generate_single(out_path):
    """Combine every topic's blocks into one `---@meta` definition file."""
    parts = []
    for topic, blocks in extract_sections():
        # A searchable banner so Ctrl+F can jump between topics in the one file.
        banner = f"-- ===== {topic} "
        parts.append(banner + "=" * max(0, 78 - len(banner)))
        parts.append("\n\n".join(blocks))
    content = "---@meta\n\n" + "\n\n".join(parts) + "\n"
    out_dir = os.path.dirname(os.path.abspath(out_path))
    os.makedirs(out_dir, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Done. Generated {out_path}")


def generate_split(out_dir):
    """Write one `<topic>.lua` definition file per topic into out_dir."""
    os.makedirs(out_dir, exist_ok=True)
    count = 0
    for topic, blocks in extract_sections():
        fname = _topic_filename(topic)
        content = "---@meta\n\n" + "\n\n".join(blocks) + "\n"
        with open(os.path.join(out_dir, fname), "w", encoding="utf-8") as f:
            f.write(content)
        count += 1
        print(f"  wrote {fname}")
    print(f"Done. Generated {count} stub files in {out_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate LuaLS stubs from the scripting API documentation.")
    parser.add_argument(
        "output", nargs="?",
        help="output file (default mode) or output directory (with --split)")
    parser.add_argument(
        "--split", action="store_true",
        help="generate one stub file per topic into a directory instead of "
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
