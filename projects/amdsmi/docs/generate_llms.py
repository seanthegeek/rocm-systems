#!/usr/bin/env python3
"""Generate ``llms.txt`` and ``llms-full.txt`` from the documentation sources.

These files follow the llms.txt standard (https://llmstxt.org/) and let large
language models and AI assistants navigate the AMD SMI documentation.

They are produced automatically during the Sphinx build (wired up in
``conf.py``) so they can never drift from the docs:

* The section structure and links come from ``sphinx/_toc.yml.in``.
* Each link's description comes from that page's MyST ``html_meta`` description
  (falling back to its top-level heading), so descriptions live next to the
  docs they describe.
* ``llms-full.txt`` inlines the prose pages verbatim.

Run standalone to preview the output without a full docs build::

    python generate_llms.py
"""

from __future__ import annotations

import re
from pathlib import Path

import yaml

DOCS_DIR = Path(__file__).parent.resolve()
TOC_FILE = DOCS_DIR / "sphinx" / "_toc.yml.in"
AMDSMI_H = DOCS_DIR.parent / "include" / "amd_smi" / "amdsmi.h"

BASE_URL = "https://rocm.docs.amd.com/projects/amdsmi/en/latest/"
REPO_URL = "https://github.com/ROCm/rocm-systems/tree/develop/projects/amdsmi"

# TOC captions whose entries are non-essential. Per the llms.txt standard these
# are merged into an "Optional" section that consumers may skip.
OPTIONAL_CAPTIONS = {"Tutorials", "About"}

# Stable, project-level facts surfaced near the top of llms.txt.
KEY_FACTS = [
    "AMD SMI supports Linux bare metal and Linux virtual machine guest "
    "environments. For SR-IOV virtualization hosts, see the AMD SMI for "
    "Virtualization documentation: "
    "https://instinct.docs.amd.com/projects/amd-smi-virt/en/latest/.",
    "AMD SMI is the successor to ROCm SMI and the esmi_ib_library.",
    "The `amdgpu` driver must be loaded for `amdsmi_init()` to work.",
]

# Prose pages are inlined into llms-full.txt; generated API reference pages
# (doxygen/autodoc directives) and license boilerplate are linked, not inlined.
FULLTEXT_EXCLUDE_PREFIXES = ("reference/",)
FULLTEXT_EXCLUDE_FILES = {"license.md"}


def get_version() -> str:
    """Read ``MAJOR.MINOR.RELEASE`` from ``amdsmi.h``."""
    content = AMDSMI_H.read_text(encoding="utf-8")

    def _field(name: str) -> str:
        match = re.search(rf"^#define\s+{name}\s+(\d+)\s*$", content, re.MULTILINE)
        if not match:
            raise ValueError(f"Could not find {name} in {AMDSMI_H}")
        return match.group(1)

    return ".".join(
        _field(f"AMDSMI_LIB_VERSION_{part}")
        for part in ("MAJOR", "MINOR", "RELEASE")
    )


def _read_frontmatter_and_body(path: Path) -> tuple[dict, str]:
    text = path.read_text(encoding="utf-8")
    match = re.match(r"^---\n(.*?)\n---\n?(.*)$", text, re.DOTALL)
    if match:
        return yaml.safe_load(match.group(1)) or {}, match.group(2)
    return {}, text


def _description(path: Path) -> str:
    frontmatter, _ = _read_frontmatter_and_body(path)
    try:
        return frontmatter["myst"]["html_meta"]["description lang=en"].strip()
    except (KeyError, TypeError):
        return ""


def _heading(path: Path) -> str:
    _, body = _read_frontmatter_and_body(path)
    for line in body.splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return ""


def _url_for(rel_md: str) -> str:
    return BASE_URL + (rel_md[:-3] + ".html" if rel_md.endswith(".md") else rel_md)


def _load_toc() -> dict:
    raw = TOC_FILE.read_text(encoding="utf-8").replace("${branch}", "develop")
    return yaml.safe_load(raw)


def _summary() -> str:
    """The project description paragraph from index.md, as one line."""
    _, body = _read_frontmatter_and_body(DOCS_DIR / "index.md")
    lines = body.splitlines()
    index = 0
    while index < len(lines) and not lines[index].startswith("# "):
        index += 1
    index += 1
    while index < len(lines) and not lines[index].strip():
        index += 1
    paragraph = []
    while index < len(lines) and lines[index].strip():
        paragraph.append(lines[index].strip())
        index += 1
    return " ".join(paragraph)


def _render_entry(entry: dict, indent: int = 0) -> list[str]:
    pad = "  " * indent
    if "url" in entry:
        title = entry.get("title") or entry["url"]
        return [f"{pad}- [{title}]({entry['url']})"]

    rel = entry["file"]
    path = DOCS_DIR / rel
    title = entry.get("title") or _heading(path) or rel
    bullet = f"{pad}- [{title}]({_url_for(rel)})"
    description = _description(path)
    if description:
        bullet += f": {description}"
    lines = [bullet]
    for child in entry.get("entries", []):
        lines += _render_entry(child, indent + 1)
    return lines


def build_index() -> str:
    toc = _load_toc()
    out = [
        "# AMD SMI",
        "",
        f"> {_summary()}",
        "",
        "This file follows the [llms.txt standard](https://llmstxt.org/) to help "
        "large language models and AI assistants navigate the AMD SMI "
        "documentation. Links point to the canonical published documentation at "
        f"<{BASE_URL}>. For the full documentation inlined into a single file, "
        f"see [llms-full.txt]({BASE_URL}llms-full.txt). The source lives at "
        f"<{REPO_URL}>.",
        "",
    ]
    out += [f"- {fact}" for fact in KEY_FACTS]

    optional: list[str] = []
    for subtree in toc.get("subtrees", []):
        caption = subtree.get("caption", "")
        rendered: list[str] = []
        for entry in subtree.get("entries", []):
            rendered += _render_entry(entry)
        if caption in OPTIONAL_CAPTIONS:
            optional += rendered
        else:
            out += ["", f"## {caption}", ""] + rendered

    out += ["", "## Optional", ""] + optional
    out += [
        f"- [Source repository]({REPO_URL}): AMD SMI source within the "
        "rocm-systems monorepo.",
        "- [Contributing to ROCm Systems]"
        "(https://github.com/ROCm/rocm-systems/blob/develop/CONTRIBUTING.md): "
        "Contribution workflow and guidelines.",
    ]
    return "\n".join(out) + "\n"


def _collect_files(entries: list[dict], acc: list[str]) -> None:
    for entry in entries:
        if "file" in entry:
            acc.append(entry["file"])
            _collect_files(entry.get("entries", []), acc)


def build_fulltext(version: str) -> str:
    toc = _load_toc()
    ordered = ["index.md"]
    collected: list[str] = []
    for subtree in toc.get("subtrees", []):
        _collect_files(subtree.get("entries", []), collected)
    for rel in collected:
        if rel not in ordered:
            ordered.append(rel)

    parts = [
        "# AMD SMI — full documentation (llms-full.txt)\n\n"
        "> This file follows the llms.txt standard (https://llmstxt.org/) and\n"
        "> inlines the full AMD SMI prose documentation into a single file for\n"
        "> consumption by large language models and AI assistants. It is generated\n"
        "> from the documentation sources at build time. For the curated link\n"
        f"> index, see llms.txt. For the rendered docs, see {BASE_URL}.\n"
        f"> Source: {REPO_URL}\n>\n"
        "> The API reference (C/C++, Python, Go) is generated from source and is\n"
        f"> not inlined here; see {BASE_URL} for the full API."
    ]

    for rel in ordered:
        if rel in FULLTEXT_EXCLUDE_FILES or rel.startswith(FULLTEXT_EXCLUDE_PREFIXES):
            continue
        _, body = _read_frontmatter_and_body(DOCS_DIR / rel)
        body = body.replace("{{ AMDSMI_VERSION }}", version).strip()
        separator = (
            "<!-- " + "=" * 60 + " -->\n"
            f"<!-- Source: docs/{rel} -->\n"
            "<!-- " + "=" * 60 + " -->"
        )
        parts.append(f"{separator}\n\n{body}")

    return "\n\n".join(parts) + "\n"


def write_all(version: str | None = None, out_dir: Path | None = None) -> list[Path]:
    """Write llms.txt and llms-full.txt; return the paths written."""
    version = version or get_version()
    out_dir = Path(out_dir) if out_dir else DOCS_DIR
    written = []
    for name, content in (
        ("llms.txt", build_index()),
        ("llms-full.txt", build_fulltext(version)),
    ):
        path = out_dir / name
        path.write_text(content, encoding="utf-8")
        written.append(path)
    return written


if __name__ == "__main__":
    for path in write_all():
        print(f"wrote {path}")
