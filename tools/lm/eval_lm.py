#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Held-out perplexity gate for a trained (unquantized) charlm."""
import argparse
import sys

from lm_common import BOS_ID, PROB_FLOOR, UNK_ID, load_model


def score_rec(probs, backoffs, ctx, w) -> float:
    if not ctx:
        p = probs.get((w,))
        if p is None and w != UNK_ID:
            p = probs.get((UNK_ID,))
        return p if p is not None else PROB_FLOOR
    p = probs.get(ctx + (w,))
    if p is not None:
        return p
    return backoffs.get(ctx, 0.0) + score_rec(probs, backoffs, ctx[1:], w)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("model_raw")
    ap.add_argument("heldout")
    ap.add_argument("--max-ppl", type=float, default=0.0,
                    help="exit 1 when perplexity exceeds this (0 = report)")
    args = ap.parse_args()

    model = load_model(args.model_raw)
    order = model["order"]
    probs = model["probs"]
    backoffs = model["backoffs"]
    char2id = model["char2id"]
    window = order - 1

    total = 0.0
    n = 0
    with open(args.heldout, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            ids = [BOS_ID] * window
            ids.extend(char2id.get(c, UNK_ID) for c in line)
            for i in range(window, len(ids)):
                ctx = tuple(ids[i - window:i])
                total += score_rec(probs, backoffs, ctx, ids[i])
                n += 1
    if n == 0:
        print("empty held-out set", file=sys.stderr)
        return 1
    ppl = 10.0 ** (-total / n)
    print(f"chars={n} avg_log10p={total / n:.4f} perplexity={ppl:.2f}")
    if args.max_ppl > 0 and ppl > args.max_ppl:
        print(f"FAIL: perplexity {ppl:.2f} > {args.max_ppl:.2f}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
