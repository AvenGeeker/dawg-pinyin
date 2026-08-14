#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Quantize a trained charlm (.raw pickle) into the .clm binary format.

Layout and quantization constants must match core/src/lm/char_lm_format.h.
"""
import argparse
import struct
import sys

from lm_common import (BOS_CHAR, BOS_ID, PROB_FLOOR, quantize_backoff,
                       quantize_prob, load_model)

MAGIC = b"CLM1"
VERSION = 1


def build_node_blob(model):
    """Memory-efficient BFS node emission.

    Trie nodes are every prefix of every stored n-gram key (pruning may
    remove an n-gram while keeping longer extensions, so prefixes are not
    necessarily keys themselves). Tuple lexicographic order equals BFS
    child order (parent order x sorted char id), so sorting keys level by
    level and threading child ranges with two pointers replaces the full
    in-memory Node trie.
    """
    probs = model["probs"]
    backoffs = model["backoffs"]
    all_keys = list(probs.keys())
    max_len = max(len(k) for k in all_keys)

    # levels[d] = sorted unique d-prefixes of all keys (trie depth d).
    levels = {}
    for depth in range(1, max_len + 1):
        levels[depth] = sorted({k[:depth] for k in all_keys
                                if len(k) >= depth})

    blob = bytearray()

    def pack(char_id, prob, bo, first_child, num_children):
        blob.extend(struct.pack(
            "<IBBHII", char_id,
            quantize_prob(prob if prob is not None else PROB_FLOOR),
            quantize_backoff(bo), 0, first_child, num_children))

    # Root node.
    level1 = levels.get(1, [])
    pack(BOS_ID, None, 0.0, 1 if level1 else 0, len(level1))
    node_count = 1
    next_level_start = 1  # BFS index where level 1 begins

    for depth in range(1, max_len + 1):
        current = levels[depth]
        children = levels.get(depth + 1, [])
        level_start = next_level_start
        child_base = level_start + len(current)
        child_cursor = 0
        for key in current:
            start = child_cursor
            while child_cursor < len(children) and \
                    children[child_cursor][:depth] == key:
                child_cursor += 1
            num = child_cursor - start
            first = child_base + start if num else 0
            pack(key[-1], probs.get(key), backoffs.get(key, 0.0), first, num)
            node_count += 1
        next_level_start = child_base

    return bytes(blob), node_count


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("model_raw")
    ap.add_argument("output_clm")
    args = ap.parse_args()

    model = load_model(args.model_raw)
    vocab_chars = model["vocab"]
    node_blob, node_count = build_node_blob(model)

    vocab_blob = bytearray()
    for i, ch in enumerate(vocab_chars):
        if i == 0:
            cp = 0
        elif i == 1:
            cp = BOS_CHAR
        else:
            cp = ord(ch)
        vocab_blob += struct.pack("<I", cp)

    with open(args.output_clm, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<IIII", VERSION, model["order"],
                            len(vocab_chars), node_count))
        f.write(bytes(vocab_blob))
        f.write(node_blob)
    total = 20 + len(vocab_blob) + len(node_blob)
    print(f"nodes={node_count} vocab={len(vocab_chars)} bytes={total} "
          f"-> {args.output_clm}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
