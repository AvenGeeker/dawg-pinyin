#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Normalize raw text into one CJK sentence per line.

Accepts plain text or a zhwiki-style XML dump (detected via --wiki or a
.xml suffix). Non-CJK code points break sentences; punctuation and markup
are dropped. Output is written as UTF-8 text and is never committed: the
corpus stays out of the repository on purpose.
"""
import argparse
import bz2
import re
import sys

TEXT_RE = re.compile(r"<text\b[^>]*>(.*?)</text>", re.S)
WIKI_STRIP_RE = re.compile(
    r"\{\{[^{}]*\}\}|\[\[[^\]]*\]\]|<[^>]+>|&[a-zA-Z]+;|'{2,}|={2,}"
)


def is_cjk(cp: int) -> bool:
    return 0x4E00 <= cp <= 0x9FFF


def segments_from_text(text: str):
    current = []
    for ch in text:
        if is_cjk(ord(ch)):
            current.append(ch)
        else:
            if current:
                yield "".join(current)
                current = []
    if current:
        yield "".join(current)


def open_input(path: str):
    if path.endswith(".bz2"):
        return bz2.open(path, "rt", encoding="utf-8", errors="replace")
    return open(path, "r", encoding="utf-8", errors="replace")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", help="raw text or wiki XML dump (.bz2 ok)")
    ap.add_argument("output", help="normalized corpus path")
    ap.add_argument("--wiki", action="store_true",
                    help="treat input as a MediaWiki XML dump")
    ap.add_argument("--max-lines", type=int, default=0,
                    help="stop after this many sentences (0 = no limit)")
    args = ap.parse_args()

    wiki = args.wiki or args.input.endswith(".xml") or args.input.endswith(
        ".xml.bz2")
    written = 0
    with open_input(args.input) as fin, open(args.output, "w",
                                              encoding="utf-8") as fout:
        if wiki:
            content = fin.read()
            for match in TEXT_RE.finditer(content):
                text = WIKI_STRIP_RE.sub(" ", match.group(1))
                for seg in segments_from_text(text):
                    if len(seg) >= 2:
                        fout.write(seg + "\n")
                        written += 1
                        if args.max_lines and written >= args.max_lines:
                            break
                if args.max_lines and written >= args.max_lines:
                    break
        else:
            for line in fin:
                for seg in segments_from_text(line):
                    if len(seg) >= 1:
                        fout.write(seg + "\n")
                        written += 1
                        if args.max_lines and written >= args.max_lines:
                            break
                if args.max_lines and written >= args.max_lines:
                    break
    print(f"sentences={written} -> {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
