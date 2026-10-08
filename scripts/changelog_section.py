#!/usr/bin/env python3
"""Print one version's section of CHANGELOG.md (Keep a Changelog format).

usage: changelog_section.py CHANGELOG.md VERSION

Prints the text under "## [VERSION]" up to the next "## [" heading. A leading "v" on
VERSION is ignored. When that version has no section it falls back to "## [Unreleased]";
when neither exists it prints nothing. Warnings go to stderr, so stdout is only the notes.
"""
import re
import sys

HEADING = re.compile(r"^##\s+\[([^\]]+)\]")
LINK_DEFINITION = re.compile(r"^\[[^\]]+\]:\s")


def section(lines, name):
    """Lines under the heading `name`, or None when there is no such heading."""
    found = False
    inside = False
    body = []
    for line in lines:
        match = HEADING.match(line)
        if match:
            if inside:
                break
            inside = match.group(1).strip().lower() == name.lower()
            found = found or inside
            continue
        if inside:
            body.append(line)
    if not found:
        return None
    while body and (not body[-1].strip() or LINK_DEFINITION.match(body[-1])):
        body.pop()
    while body and not body[0].strip():
        body.pop(0)
    return body


def main(argv):
    if len(argv) != 3:
        sys.exit(__doc__)
    path, version = argv[1], argv[2].lstrip("vV")
    with open(path, encoding="utf-8") as handle:
        lines = handle.read().splitlines()

    for name in (version, "Unreleased"):
        body = section(lines, name)
        if body is not None:
            if name != version:
                print(f"note: no [{version}] section in {path}; using [{name}]",
                      file=sys.stderr)
            print("\n".join(body))
            return
    print(f"warning: no [{version}] or [Unreleased] section in {path}", file=sys.stderr)


if __name__ == "__main__":
    main(sys.argv)
