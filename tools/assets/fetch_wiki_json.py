#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Stream a HuggingFace wikipedia-cn JSON array into one CJK sentence per
line.

Reads stdin (default) or a file, extracts every record's "completion"
field (plain article text), splits it into CJK-only sentences and stops as
soon as --max-lines is reached -- which closes the pipe and aborts a
network download long before the full multi-hundred-MB file transfers.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "lm"))
from prepare_corpus import segments_from_text  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", nargs="?", default="-",
                    help="JSON array input ('-' = stdin)")
    ap.add_argument("output", help="normalized corpus path")
    ap.add_argument("--max-lines", type=int, default=0,
                    help="stop after this many sentences (0 = no limit)")
    args = ap.parse_args()

    fin = sys.stdin if args.input == "-" else open(
        args.input, "r", encoding="utf-8", errors="replace")
    written = 0
    with open(args.output, "w", encoding="utf-8") as fout:
        for line in fin:
            stripped = line.strip()
            if not stripped.startswith('"completion"'):
                continue
            try:
                value = json.loads(
                    "{" + stripped.rstrip(",") + "}")["completion"]
            except (ValueError, KeyError):
                continue
            for seg in segments_from_text(value):
                if len(seg) >= 2:
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
