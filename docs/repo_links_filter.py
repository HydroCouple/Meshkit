# SPDX-License-Identifier: Apache-2.0
"""Doxygen input filter for MeshKit's Markdown pages.

The Markdown files are written for GitHub, where ``[setup.py](setup.py)`` opens the
file in the repository. The documentation site hosts only the pages, so this filter
rewrites relative links to repository files that are not Markdown pages into links
to the file on GitHub. Links to ``.md`` files, anchors, images and absolute URLs are
left for Doxygen. It also escapes a GitHub handle that opens link text, as in
``[@cbuahin](...)``: Doxygen 1.13 reads ``@cbuahin`` as a command there. Lines are
rewritten in place, so warning line numbers still match.

Doxygen runs it as ``python3 repo_links_filter.py <file>`` (FILTER_PATTERNS in the
Doxyfile) and reads the filtered text from standard output.
"""
import os
import re
import sys

REPOSITORY = "https://github.com/HydroCouple/Meshkit"
BRANCH = "main"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LINK = re.compile(r"(?<!!)(\]\()([^)\s]+)(\))")
LEFT_TO_DOXYGEN = re.compile(r"^(?:[a-z][a-z0-9+.-]*:|#)|\.md(?:#|$)|\.(?:png|svg|jpe?g|gif)$",
                             re.IGNORECASE)
HANDLE = re.compile(r"\[@(?=[A-Za-z0-9-]+\])")


def rewrite(target: str, folder: str) -> str:
    if LEFT_TO_DOXYGEN.search(target):
        return target
    path, _, anchor = target.partition("#")
    absolute = os.path.normpath(os.path.join(folder, path))
    relative = os.path.relpath(absolute, ROOT)
    if relative.startswith(".."):
        return target
    kind = "tree" if os.path.isdir(absolute) else "blob"
    url = f"{REPOSITORY}/{kind}/{BRANCH}/{relative.replace(os.sep, '/')}"
    return url + ("#" + anchor if anchor else "")


def main() -> None:
    source = os.path.abspath(sys.argv[1])
    folder = os.path.dirname(source)
    fenced = False
    with open(source, encoding="utf-8") as stream:
        for line in stream:
            if line.lstrip().startswith(("```", "~~~")):
                fenced = not fenced
            if not fenced:
                line = LINK.sub(lambda m: m.group(1) + rewrite(m.group(2), folder) + m.group(3),
                                line)
                line = HANDLE.sub(r"[\\@", line)
            sys.stdout.write(line)


if __name__ == "__main__":
    main()
