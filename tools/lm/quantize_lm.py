#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Quantize a trained charlm (.raw pickle) into the .clm binary format.

Layout and quantization constants must match core/src/lm/char_lm_format.h.
"""
import argparse
import struct
import sys

from lm_common import (BOS_CHAR, BOS_ID, quantize_backoff, quantize_prob,
                       load_model)

MAGIC = b"CLM1"
VERSION = 1


class Node:
    __slots__ = ("char_id", "prob", "bo", "children")

    def __init__(self, char_id):
        self.char_id = char_id
        self.prob = None
        self.bo = 0.0
        self.children = {}


def build_trie(model):
    nodes = [Node(BOS_ID)]
    probs = model["probs"]
    backoffs = model["backoffs"]
    for key, log10_p in probs.items():
        node = nodes[0]
        for char_id in key[:-1]:
            nxt = node.children.get(char_id)
            if nxt is None:
                nxt = len(nodes)
                nodes.append(Node(char_id))
                node.children[char_id] = nxt
            node = nodes[nxt]
        last = key[-1]
        nxt = node.children.get(last)
        if nxt is None:
            nxt = len(nodes)
            nodes.append(Node(last))
            node.children[last] = nxt
        nodes[nxt].prob = log10_p
    # Backoff weights: attach stored values by walking trie paths.
    def walk(idx, path):
        node = nodes[idx]
        if path:
            node.bo = backoffs.get(path, 0.0)
        for char_id, child in node.children.items():
            walk(child, path + (char_id,))
    walk(0, ())
    return nodes


def emit(nodes):
    """BFS-number nodes so every node's children are contiguous in the node
    array (the reader indexes children straight into the node array)."""
    order = [0]
    first_child = {}
    cursor = 0
    while cursor < len(order):
        idx = order[cursor]
        kids = [nodes[idx].children[c] for c in sorted(nodes[idx].children)]
        if kids:
            first_child[idx] = len(order)
            order.extend(kids)
        else:
            first_child[idx] = 0
        cursor += 1

    node_blob = bytearray()
    for old_idx in order:
        node = nodes[old_idx]
        num_children = len(node.children)
        fc = first_child[old_idx] if num_children else 0
        prob_q = quantize_prob(node.prob if node.prob is not None else -10.0)
        node_blob += struct.pack("<IBBHII", node.char_id, prob_q,
                                 quantize_backoff(node.bo), 0, fc,
                                 num_children)
    return bytes(node_blob), len(order)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("model_raw")
    ap.add_argument("output_clm")
    args = ap.parse_args()

    model = load_model(args.model_raw)
    vocab_chars = model["vocab"]
    nodes = build_trie(model)
    node_blob, node_count = emit(nodes)

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
