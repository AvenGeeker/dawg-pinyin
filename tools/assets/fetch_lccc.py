#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Stream the LCCC (Large-scale Chinese Conversation Corpus) gzipped jsonl
into one CJK sentence per line.

Each input line is a JSON array of dialog turns; turns are word-segmented
("你 去 那儿") and are rejoined by stripping whitespace, which is lossless
for the CJK-only sentence splitter. Reading stops at --max-lines so a
network download can be aborted early.
"""
import argparse
import gzip
import io
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "lm"))
from prepare_corpus import segments_from_text  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", nargs="?", default="-",
                    help="gzipped jsonl input ('-' = stdin)")
    ap.add_argument("output", help="normalized corpus path")
    ap.add_argument("--max-lines", type=int, default=0,
                    help="stop after this many sentences (0 = no limit)")
    args = ap.parse_args()

    if args.input == "-":
        fin = gzip.open(sys.stdin.buffer, "rt", encoding="utf-8",
                        errors="replace")
    else:
        fin = gzip.open(args.input, "rt", encoding="utf-8",
                        errors="replace")
    written = 0
    with open(args.output, "w", encoding="utf-8") as fout:
        for line in fin:
            line = line.strip()
            if not line:
                continue
            try:
                turns = json.loads(line)
            except ValueError:
                continue
            if isinstance(turns, str):
                turns = [turns]
            for turn in turns:
                text = "".join(turn.split())
                for seg in segments_from_text(text):
                    if len(seg) >= 2:
                        fout.write(seg + "\n")
                        written += 1
                        if args.max_lines and written >= args.max_lines:
                            break
                if args.max_lines and written >= args.max_lines:
                    break
            if args.max_lines and written >= args.max_lines:
                break
    print(f"sentences={written} -> {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
