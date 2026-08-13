#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Shared helpers for the charlm training pipeline.

Binary layout constants MUST match core/src/lm/char_lm_format.h.
"""
import pickle

UNK_ID = 0
BOS_ID = 1
BOS_CHAR = 0x10FFFD

PROB_FLOOR = -10.0
PROB_CEIL = 0.0
PROB_SCALE = 25.5
BACKOFF_MIN = -2.0
BACKOFF_MAX = 2.0
BACKOFF_SCALE = 63.75


def quantize_prob(log10_p: float) -> int:
    if log10_p <= PROB_FLOOR:
        return 0
    if log10_p >= PROB_CEIL:
        return 255
    return int(round((log10_p - PROB_FLOOR) * PROB_SCALE))


def quantize_backoff(log10_bo: float) -> int:
    if log10_bo <= BACKOFF_MIN:
        return 0
    if log10_bo >= BACKOFF_MAX:
        return 255
    return int(round((log10_bo - BACKOFF_MIN) * BACKOFF_SCALE))


def save_model(path: str, model: dict) -> None:
    with open(path, "wb") as f:
        pickle.dump(model, f, protocol=4)


def load_model(path: str) -> dict:
    with open(path, "rb") as f:
        return pickle.load(f)


def iter_sentences(path: str):
    """Yield lists of char ids? No: yield raw CJK strings, one per line."""
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                yield line


def score_with_backoff(model: dict, word_ids, context_ids) -> float:
    """Python mirror of dawg::CharLm::ScoreWord (unquantized probabilities).

    probs: dict tuple(ids) -> log10 p, where the tuple is (history..., w)
    backoffs: dict tuple(history) -> log10 gamma; empty tuple = root
    """
    order = model["order"]
    window = order - 1
    probs = model["probs"]
    backoffs = model["backoffs"]

    def p_unigram(w):
        p = probs.get((w,))
        if p is not None:
            return p
        if w != UNK_ID:
            p = probs.get((UNK_ID,))
            if p is not None:
                return p
        return PROB_FLOOR

    def score_rec(ctx, w):
        # ctx: tuple of ids, oldest first, len <= window
        if not ctx:
            return p_unigram(w)
        p = probs.get(ctx + (w,))
        if p is not None:
            return p
        bo = backoffs.get(ctx, 0.0)
        return bo + score_rec(ctx[1:], w)

    if context_ids:
        ctx = tuple(context_ids[-window:])
    else:
        ctx = tuple([BOS_ID] * window)
    total = 0.0
    for w in word_ids:
        total += score_rec(ctx, w)
        ctx = (ctx + (w,))[-window:]
    return total
