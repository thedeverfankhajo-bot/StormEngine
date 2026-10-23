#!/usr/bin/env python3
"""Validate the static GitHub Pages site using only Python's standard library."""
from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "site"
REQUIRED = (
    "index.html", "404.html", "robots.txt", "sitemap.xml",
    "assets/css/site.css", "assets/js/site.js", "assets/images/storm-mark.svg",
)
REQUIRED_IDS = {
    "main", "architecture", "roadmap", "docs", "engine-canvas", "projection-lab",
    "projection-canvas", "projection-fov", "projection-aspect", "projection-near",
    "projection-far", "projection-depth", "projection-matrix",
    "copy-command", "language-toggle", "theme-toggle",
}


class Inspector(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.ids: set[str] = set()
        self.duplicates: set[str] = set()
        self.references: list[tuple[str, str]] = []
        self.active_external: list[str] = []
        self.inline_handlers: list[str] = []
        self.inline_scripts = 0
        self.inline_styles = 0
        self.csp = ""

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        identifier = values.get("id")
        if identifier:
            if identifier in self.ids:
                self.duplicates.add(identifier)
            self.ids.add(identifier)

        if tag == "meta" and (values.get("http-equiv") or "").lower() == "content-security-policy":
            self.csp = values.get("content") or ""

        rels = set((values.get("rel") or "").lower().split())
        for name, value in attrs:
            low_name = name.lower()
            if low_name.startswith("on"):
                self.inline_handlers.append(tag + "[" + name + "]")
            if value is None or low_name not in {"href", "src", "poster", "srcset"}:
                continue
            candidates = [item.strip().split()[0] for item in value.split(",") if item.strip()] if low_name == "srcset" else [value.strip()]
            for candidate in candidates:
                if not candidate:
                    continue
                parsed = urlsplit(candidate)
                if parsed.scheme.lower() == "javascript":
                    self.active_external.append(candidate)
                    continue
                if parsed.scheme or parsed.netloc:
                    if tag == "script" and low_name == "src" and parsed.scheme in {"http", "https"}:
                        self.active_external.append(candidate)
                    if tag == "link" and low_name == "href" and rels.intersection({"stylesheet", "preload", "modulepreload"}) and parsed.scheme in {"http", "https"}:
                        self.active_external.append(candidate)
                    continue
                target_path = unquote(parsed.path)
                if target_path or (low_name == "href" and parsed.fragment):
                    self.references.append((target_path, unquote(parsed.fragment)))

        if tag == "script" and not values.get("src"):
            self.inline_scripts += 1
        if tag == "style":
            self.inline_styles += 1


def issue(message: str) -> None:
    print("FAIL: " + message)


def inspect_html(path: Path, relative: str, errors: list[str]) -> Inspector | None:
    try:
        source = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        errors.append(f"cannot read {relative}: {exc}")
        return None
    parser = Inspector()
    parser.feed(source)
    parser.close()
    if parser.duplicates:
        errors.append(f"{relative}: duplicate ID(s): " + ", ".join(sorted(parser.duplicates)))
    if parser.inline_handlers:
        errors.append(f"{relative}: inline event handler(s): " + ", ".join(parser.inline_handlers))
    if parser.inline_scripts:
        errors.append(f"{relative}: inline JavaScript is not permitted; use assets/js/site.js")
    if parser.inline_styles:
        errors.append(f"{relative}: inline style blocks are not permitted; use assets/css/site.css")
    if parser.active_external:
        errors.append(f"{relative}: external active assets or javascript: URLs: " + ", ".join(sorted(set(parser.active_external))))
    return parser


def main() -> int:
    errors: list[str] = []
    for item in REQUIRED:
        if not (SITE / item).is_file():
            errors.append("required file missing: site/" + item)
    if errors:
        for error in errors:
            issue(error)
        return 1

    pages: dict[str, Inspector] = {}
    page_sources: dict[str, Path] = {}
    for name in ("index.html", "404.html"):
        rel = name
        parser = inspect_html(SITE / name, rel, errors)
        if parser:
            pages[rel] = parser
            page_sources[rel] = SITE / name

    missing_ids = REQUIRED_IDS - pages.get("index.html", Inspector()).ids
    if missing_ids:
        errors.append("index.html is missing required controls/sections: " + ", ".join(sorted(missing_ids)))

    homepage = pages.get("index.html")
    if homepage:
        if not homepage.csp:
            errors.append("index.html has no Content-Security-Policy meta tag")
        else:
            directives = {part.strip() for part in homepage.csp.split(";") if part.strip()}
            for expected in ("default-src 'self'", "script-src 'self'", "style-src 'self'", "connect-src 'none'", "object-src 'none'"):
                if expected not in directives:
                    errors.append("index.html CSP is missing " + expected)

        html = (SITE / "index.html").read_text(encoding="utf-8")
        for marker in ('data-t="hero-title-a"', 'data-t="roadmap-title-a"', 'data-t="docs-title"', 'data-t="reality-note"', 'data-t="projection-title"', 'data-t="projection-depth-label"'):
            if marker not in html:
                errors.append("index.html missing required content marker: " + marker)

    refs_checked = 0
    for page_name, parser in pages.items():
        page_path = page_sources[page_name]
        for raw_path, fragment in parser.references:
            target = (page_path.parent / raw_path).resolve() if raw_path else page_path.resolve()
            try:
                target.relative_to(SITE.resolve())
            except ValueError:
                errors.append(f"{page_name}: local link escapes site/: {raw_path}")
                continue
            if not target.exists():
                errors.append(f"{page_name}: broken local path {raw_path!r}")
                continue
            if fragment and target.is_file() and target.suffix.lower() in {".html", ".htm"}:
                destination_name = target.relative_to(SITE.resolve()).as_posix()
                destination = pages.get(destination_name)
                if destination is None:
                    destination = inspect_html(target, destination_name, errors)
                    if destination:
                        pages[destination_name] = destination
                if destination and fragment not in destination.ids:
                    errors.append(f"{page_name}: broken fragment #{fragment} on {destination_name}")
            refs_checked += 1

    css_path = SITE / "assets/css/site.css"
    css = css_path.read_text(encoding="utf-8")
    for raw in re.findall(r"url\(\s*['\"]?([^)'\"\s]+)", css, re.IGNORECASE):
        url = raw.strip()
        if url.startswith(("data:", "#")):
            continue
        parsed = urlsplit(url)
        if parsed.scheme or parsed.netloc:
            errors.append("external CSS asset is not permitted: " + url)
            continue
        if parsed.path and not (css_path.parent / unquote(parsed.path)).resolve().exists():
            errors.append("CSS references missing asset: " + url)

    js_path = SITE / "assets/js/site.js"
    javascript = js_path.read_text(encoding="utf-8")
    if "document.write(" in javascript:
        errors.append("site JavaScript uses document.write()")
    for marker in ("function updateProjection()", "Math.fround", "projection-near-result", "projection-far-result", 'const depthNumerator = range === "zero-to-one"'):
        if marker not in javascript:
            errors.append("site JavaScript is missing projection lab behavior: " + marker)
    if re.search(r"\b(?:fetch|XMLHttpRequest|WebSocket)\s*\(", javascript):
        errors.append("site JavaScript contains network API usage; the published page is intended to be self-contained")

    try:
        tree = ET.parse(SITE / "sitemap.xml")
        if not tree.getroot().tag.endswith("urlset"):
            errors.append("sitemap.xml root element must be urlset")
    except ET.ParseError as exc:
        errors.append("sitemap.xml is invalid XML: " + str(exc))

    if errors:
        for error in errors:
            issue(error)
        print(f"Website checks failed: {len(errors)} problem(s).")
        return 1

    print(f"Website checks passed: {len(REQUIRED)} required files, {len(pages['index.html'].ids)} unique homepage IDs, {refs_checked} local references, secure CSP and valid sitemap.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
