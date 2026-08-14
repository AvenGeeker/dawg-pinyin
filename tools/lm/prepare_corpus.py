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
    if path == "-":
        return bz2.open(sys.stdin.buffer, "rt", encoding="utf-8",
                        errors="replace")
    if path.endswith(".bz2"):
        return bz2.open(path, "rt", encoding="utf-8", errors="replace")
    return open(path, "r", encoding="utf-8", errors="replace")


def iter_wiki_texts(fin, chunk_size: int = 1 << 20):
    """Yield <text> element bodies from a streamed MediaWiki XML dump.

    Reads in chunks so multi-GB dumps never sit in memory; a carry-over
    tail keeps matches that straddle chunk boundaries intact.
    """
    buf = ""
    carry = ""
    while True:
        data = fin.read(chunk_size)
        eof = not data
        buf = carry + data
        pos = 0
        while True:
            m = TEXT_RE.search(buf, pos)
            if m is None:
                break
            yield m.group(1)
            pos = m.end()
        if eof:
            return
        # Keep everything from the last unprocessed match start (a partial
        # <text ...>...</text> may span into the next chunk); drop the rest.
        lt = buf.rfind("<text", pos)
        carry = buf[lt:] if lt != -1 else ""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input",
                    help="raw text or wiki XML dump (.bz2 ok; '-' = stdin, "
                         "which bz2-decodes streamed input)")
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
            for body in iter_wiki_texts(fin):
                text = WIKI_STRIP_RE.sub(" ", body)
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
