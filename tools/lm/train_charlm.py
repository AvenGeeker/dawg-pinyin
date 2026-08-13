#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Train a character-level interpolated modified Kneser-Ney n-gram model.

Input : normalized corpus (one CJK sentence per line, see prepare_corpus.py)
Output: intermediate .raw pickle consumed by quantize_lm.py / eval_lm.py

The pipeline keeps counts in memory and targets demo/test-scale corpora;
for full-corpus training use a machine with enough RAM (documented in
tools/lm/README.md). Sentence starts are padded with (order-1) <s> symbols
so runtime contexts match training-time conditioning.
"""
import argparse
import math
import sys
from collections import defaultdict

from lm_common import BOS_ID, UNK_ID, save_model


def count_ngrams(lines, char2id, order):
    raw = [defaultdict(int) for _ in range(order + 1)]  # index by n
    for line in lines:
        ids = [BOS_ID] * (order - 1)
        ids.extend(char2id.get(c, UNK_ID) for c in line)
        for n in range(1, order + 1):
            for i in range(n - 1, len(ids)):
                raw[n][tuple(ids[i - n + 1:i + 1])] += 1
    return raw


def continuation_counts(raw_next):
    """N1+(., w_1..w_n) from raw (n+1)-gram counts."""
    out = defaultdict(int)
    for key in raw_next:
        out[key[1:]] += 1
    return out


def discounts(counts):
    """Modified KN discounts (D1, D2, D3+) from a count distribution."""
    n1 = n2 = n3 = n4 = 0
    for c in counts.values():
        if c == 1:
            n1 += 1
        elif c == 2:
            n2 += 1
        elif c == 3:
            n3 += 1
        elif c == 4:
            n4 += 1
    if n1 == 0 or n2 == 0:
        return 0.1, 0.2, 0.3
    y = n1 / (n1 + 2.0 * n2)
    d1 = 1.0 - 2.0 * y * (n2 / n1)
    d2 = 2.0 - 3.0 * y * (n3 / n2) if n3 > 0 else 2.0
    d3 = 3.0 - 4.0 * y * (n4 / n3) if n4 > 0 else 3.0
    return max(d1, 0.01), max(d2, 0.01), max(d3, 0.01)


def discount_for(c, d1, d2, d3):
    if c == 1:
        return d1
    if c == 2:
        return d2
    return d3


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("corpus")
    ap.add_argument("output")
    ap.add_argument("--order", type=int, default=6)
    ap.add_argument("--vocab", type=int, default=8190,
                    help="vocabulary size including <unk> and <s>")
    ap.add_argument("--min-count", type=int, default=2,
                    help="drop n-grams (order>=3) below this count")
    ap.add_argument("--top-k", type=int, default=128,
                    help="keep at most this many children per context "
                         "(order>=3); 0 disables")
    args = ap.parse_args()
    if not 2 <= args.order <= 8:
        print("order must be within [2, 8]", file=sys.stderr)
        return 2

    with open(args.corpus, "r", encoding="utf-8") as f:
        lines = [line.strip() for line in f if line.strip()]
    if not lines:
        print("empty corpus", file=sys.stderr)
        return 1

    # Pass 1: vocabulary.
    unigrams = defaultdict(int)
    for line in lines:
        for ch in line:
            unigrams[ch] += 1
    top = sorted(unigrams.items(), key=lambda kv: (-kv[1], kv[0]))
    vocab_chars = ["", ""]  # ids 0/1 reserved for <unk>/<s>
    vocab_chars.extend(ch for ch, _ in top[: max(args.vocab - 2, 1)])
    char2id = {ch: i for i, ch in enumerate(vocab_chars) if i >= 2}

    # Pass 2: raw counts.
    raw = count_ngrams(lines, char2id, args.order)

    # Counts actually used per order: raw at the top, continuation below.
    used = [None] * (args.order + 1)
    used[args.order] = dict(raw[args.order])
    for n in range(args.order - 1, 0, -1):
        used[n] = dict(continuation_counts(raw[n + 1]))

    d = [None] + [discounts(used[n]) for n in range(1, args.order + 1)]

    probs = {}      # tuple -> log10 prob
    backoffs = {}   # context tuple -> log10 backoff weight
    p_lower = {}    # previous order probs for interpolation

    for n in range(1, args.order + 1):
        counts = used[n]
        d1, d2, d3 = d[n]
        p_cur = {}
        if n == 1:
            total = sum(counts.values()) or 1
            disc = {}
            for w, c in counts.items():
                disc[w] = max(c - discount_for(c, d1, d2, d3), 0.0)
            s = sum(disc.values())
            remainder = max(total - s, 0.0)
            v = len(vocab_chars)
            denom = s + remainder
            for w in range(v):
                mass = disc.get(w, 0.0) + remainder / v
                p_cur[(w,)] = mass / denom
        else:
            by_ctx = defaultdict(list)
            for key, c in counts.items():
                by_ctx[key[:-1]].append((key[-1], c))
            for h, children in by_ctx.items():
                total = sum(c for _, c in children)
                # Optional pruning for higher orders.
                kept = children
                if n >= 3:
                    if args.min_count > 1:
                        kept = [(w, c) for w, c in kept if c >= args.min_count]
                        if not kept:
                            kept = [max(children, key=lambda wc: wc[1])]
                def interp(w):
                    key = h[1:] + (w,)
                    return p_lower.get(key, p_lower.get((UNK_ID,), 1e-10))
                if n >= 3 and args.top_k > 0 and len(kept) > args.top_k:
                    # Keep the children that contribute most to the KL
                    # divergence against the backed-off distribution.
                    scored = []
                    for w, c in kept:
                        base = max(c - discount_for(c, d1, d2, d3), 0.0) / total
                        scored.append((c * max(base, 1e-12), w, c))
                    scored.sort(reverse=True)
                    kept = [(w, c) for _, w, c in scored[: args.top_k]]
                mass_sum = 0.0
                for w, c in kept:
                    base = max(c - discount_for(c, d1, d2, d3), 0.0) / total
                    p_cur[h + (w,)] = base
                    mass_sum += base
                # Backoff absorbs all unkept probability mass.
                gamma = max(1.0 - mass_sum, 1e-12)
                backoff_log = math.log10(gamma)
                if h:
                    backoffs[h] = backoff_log
                # Interpolation: add gamma * p_lower into kept entries.
                for w, _c in kept:
                    key = h + (w,)
                    p_cur[key] += gamma * interp(w)
        probs.update(p_cur)
        p_lower = p_cur
        print(f"order {n}: kept {len(p_cur)} n-grams", file=sys.stderr)

    # Publish everything in log10 domain (quantizer/eval/reader all assume it).
    probs_log = {k: math.log10(v) for k, v in probs.items() if v > 0.0}
    backoffs_log = {k: math.log10(v) for k, v in backoffs.items() if v > 0.0}
    model = {
        "order": args.order,
        "vocab": vocab_chars,
        "probs": probs_log,
        "backoffs": backoffs_log,
        "char2id": char2id,
    }
    save_model(args.output, model)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
